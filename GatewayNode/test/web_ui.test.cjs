const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const webRoot = process.env.GATEWAY_WEB_ROOT || path.join(__dirname, '../web');
const scriptPath = path.join(webRoot, 'app.js');
const script = fs.existsSync(scriptPath) ? fs.readFileSync(scriptPath, 'utf8') : '';

function element(dataset = {}) {
    return {dataset, textContent: '', disabled: false, attributes: {}, listeners: {},
        setAttribute(name, value) { this.attributes[name] = value; },
        getAttribute(name) { return this.attributes[name]; },
        addEventListener(event, callback) { this.listeners[event] = callback; }};
}

function fixture(fetch, savedTheme, storageBlocked = false,
    statusFetch = async () => ({ok: true, json: async () => ({state: 'unknown'})}), liveEvents = false) {
    const ids = Object.fromEntries(['last-command', 'command-time', 'feedback',
        'feedback-title', 'feedback-detail', 'relay-panel', 'relay-state', 'relay-state-detail'].map(id => [id, element()]));
    ids['last-command'].textContent = 'No command yet';
    const forms = ['ON', 'OFF'].map(command => {
        const form = element({command});
        form.attributes.action = '/' + command.toLowerCase();
        return form;
    });
    const buttons = forms.map(form => element({commandButton: form.dataset.command}));
    const themes = ['light', 'dark'].map(themeChoice => element({themeChoice}));
    const root = element();
    let stored = savedTheme;
    let timeout;
    let statusTimeout;
    let poll;
    let clock = 1000;
    const streams = [];
    const windowListeners = {};
    let commandCheck;
    const context = {document: {documentElement: root, getElementById: id => ids[id],
        querySelectorAll: selector => ({'.command-form': forms, '[data-command-button]': buttons,
            '[data-theme-choice]': themes}[selector] || [])},
        localStorage: {getItem() { if (storageBlocked) throw Error('Blocked'); return stored; },
            setItem(key, value) { if (storageBlocked) throw Error('Blocked'); stored = value; }},
        matchMedia: () => ({matches: false}),
        fetch: (url, options) => url === '/status' ? statusFetch(url, options) : fetch(url, options),
        AbortController, Date: class extends Date { static now() { return clock; } },
        performance: {now: () => clock},
        window: {addEventListener(name, callback) { windowListeners[name] = callback; }},
        setInterval(callback) { poll = callback; return 2; },
        setTimeout(callback, delay) {
            if (delay === 10000) timeout = callback;
            if (delay === 3500) statusTimeout = callback;
            if (delay === 200) commandCheck = callback;
            return delay;
        },
        clearTimeout(id) {
            if (id === 10000) timeout = undefined;
            if (id === 3500) statusTimeout = undefined;
            if (id === 200) commandCheck = undefined;
        }};
    if (liveEvents) context.EventSource = class {
        constructor(url) { assert.equal(url, '/events'); streams.push(this); }
        close() { this.closed = true; }
    };
    vm.runInNewContext(script, context);
    return {ids, buttons, themes, root, stored: () => stored,
        streams, windowListeners, advance(ms) { clock += ms; },
        expire: () => timeout(),
        expireStatus: () => { clock += 3500; statusTimeout(); },
        async checkCommand() {
            assert.equal(typeof commandCheck, 'function', 'Accepted commands need a prompt independent state check');
            const callback = commandCheck;
            commandCheck = undefined;
            await callback();
        },
        hasCommandCheck: () => typeof commandCheck === 'function',
        async poll() { await new Promise(setImmediate); return poll(); },
        submit(index) {
            assert.equal(typeof forms[index].listeners.submit, 'function',
                'The dashboard must intercept form submission instead of navigating away');
            let prevented = false;
            const promise = forms[index].listeners.submit({preventDefault() { prevented = true; }});
            assert.equal(prevented, true);
            return promise;
        }};
}

test('ON and OFF stay on the page and update only the last accepted request', async () => {
    const requests = [];
    const ui = fixture(async (url, options) => {
        requests.push([url, options.method]);
        return {ok: true, text: async () => 'Request sent.'};
    });
    await ui.submit(0);
    assert.equal(ui.ids['last-command'].textContent, 'ON requested');
    await ui.submit(1);
    assert.equal(ui.ids['last-command'].textContent, 'OFF requested');
    assert.deepEqual(requests, [['/on', 'POST'], ['/off', 'POST']]);
    assert.equal(ui.ids['feedback'].dataset.state, 'success');
    assert.match(ui.ids['feedback-detail'].textContent, /not confirmed/);
});

test('reported relay state drives highlights even when it disagrees with a web request', async () => {
    let state = 'off';
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        async () => ({ok: true, json: async () => ({state})}));
    await ui.poll();
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'off');
    assert.equal(ui.buttons[1].dataset.selected, 'true');
    await ui.submit(0);
    assert.equal(ui.buttons[1].dataset.selected, 'true', 'Sending ON cannot invent an ON report');
    assert.equal(ui.ids['last-command'].textContent, 'ON requested');
    state = 'on';
    await ui.poll();
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    assert.match(ui.buttons[0].attributes['aria-label'], /relay reports ON/);
    state = 'off';
    await ui.poll();
    assert.equal(ui.buttons[0].dataset.selected, 'false');
    assert.equal(ui.buttons[1].dataset.selected, 'true');
    assert.equal(ui.ids['relay-state'].textContent, 'Relay is OFF');
});

test('unknown, failed HTTP, invalid JSON data, and lost status clear the highlight', async () => {
    let mode = 'on';
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false, async () => {
        if (mode === 'lost') throw Error('Offline');
        return {ok: mode !== 'http-error', json: async () => ({state: mode})};
    });
    for (const unavailable of ['unknown', 'http-error', 'invalid', 'lost']) {
        mode = 'on';
        await ui.poll();
        assert.equal(ui.buttons[0].dataset.selected, 'true');
        mode = unavailable;
        if (unavailable === 'http-error' || unavailable === 'lost') ui.advance(6000);
        await ui.poll();
        assert.equal(ui.ids['relay-panel'].dataset.relayState, 'unknown');
        assert.equal(ui.ids['relay-state'].textContent, 'State unavailable');
        assert.ok(ui.buttons.every(button => button.dataset.selected === 'false'));
    }
});

test('status polling does not overlap and recovers after a timed-out request', async () => {
    let requests = 0;
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        (url, options) => {
            requests++;
            if (requests > 1) return Promise.resolve({ok: true, json: async () => ({state: 'off'})});
            return new Promise((resolve, reject) => {
                options.signal.addEventListener('abort', () => reject(Error('Timed out')));
            });
        });
    await ui.poll();
    assert.equal(requests, 1);
    ui.expireStatus();
    await new Promise(setImmediate);
    assert.equal(ui.ids['relay-state'].textContent, 'State unavailable');
    await ui.poll();
    assert.equal(requests, 2);
    assert.equal(ui.buttons[1].dataset.selected, 'true');
});

test('rapid taps keep controls responsive and send only the latest queued intent', async () => {
    let finish;
    const requests = [];
    const ui = fixture(url => {
        requests.push(url);
        if (requests.length === 1) return new Promise(resolve => { finish = resolve; });
        return Promise.resolve({ok: true, text: async () => ''});
    });
    const pending = ui.submit(0);
    await new Promise(setImmediate);
    assert.ok(ui.buttons.every(button => !button.disabled));
    const second = ui.submit(1);
    const third = ui.submit(0);
    const last = ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'off');
    assert.equal(ui.buttons[1].dataset.active, 'true');
    assert.deepEqual(requests, ['/on']);
    finish({ok: true, text: async () => ''});
    await Promise.all([pending, second, third, last]);
    assert.deepEqual(requests, ['/on', '/off']);
    assert.equal(ui.ids['last-command'].textContent, 'OFF requested');
    assert.ok(ui.buttons.every(button => !button.disabled));
});

test('click feedback is immediate and labelled pending while reported state remains truthful', async () => {
    let finish;
    const ui = fixture(() => new Promise(resolve => { finish = resolve; }), undefined, false,
        async () => ({ok: true, json: async () => ({state: 'on'})}), true);
    await new Promise(setImmediate);
    const pending = ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'off');
    assert.equal(ui.ids['relay-panel'].dataset.commandPending, 'true');
    assert.match(ui.ids['feedback-title'].textContent, /Turning off/);
    assert.equal(ui.ids['relay-state'].textContent, 'Relay is ON');
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'off', 'Old reports must not play old click animations');
    await new Promise(setImmediate);
    finish({ok: true, text: async () => ''});
    await pending;
    ui.streams[0].onmessage({data: '{"state":"off"}'});
    assert.equal(ui.ids['relay-panel'].dataset.commandPending, 'false');
    assert.equal(ui.ids['relay-state'].textContent, 'Relay is OFF');
});

test('a rejected command removes the pending preview and restores the latest relay report', async () => {
    const ui = fixture(async () => ({ok: false, text: async () => 'Mesh unavailable'}), undefined, false,
        async () => ({ok: true, json: async () => ({state: 'on'})}));
    await new Promise(setImmediate);
    await ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.commandPending, 'false');
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'on');
    assert.equal(ui.ids['feedback-title'].textContent, 'Request not sent');
});

test('live relay updates change the UI without waiting for polling or a web request', async () => {
    let polls = 0;
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        async () => { polls++; return {ok: true, json: async () => ({state: 'unknown'})}; }, true);
    await new Promise(setImmediate);
    assert.equal(ui.streams.length, 1);
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    await ui.poll();
    assert.equal(polls, 1, 'A healthy stream replaces repeated browser polling');
    ui.streams[0].onmessage({data: '{"state":"off"}'});
    assert.equal(ui.buttons[1].dataset.selected, 'true');
});

test('a late polling response cannot overwrite a newer live relay event', async () => {
    let finish;
    const ui = fixture(() => {}, undefined, false,
        () => new Promise(resolve => { finish = resolve; }), true);
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    finish({ok: true, json: async () => ({state: 'off'})});
    await new Promise(setImmediate);
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'on');
});

test('an accepted OFF checks reported state even when a healthy-looking stream stalls', async () => {
    let state = 'on';
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        async () => ({ok: true, json: async () => ({state})}), true);
    await new Promise(setImmediate);
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    await ui.submit(1);
    assert.equal(ui.buttons[0].dataset.selected, 'true', 'POST acceptance cannot invent OFF');
    await ui.checkCommand();
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    assert.equal(ui.hasCommandCheck(), true);
    state = 'off';
    await ui.checkCommand();
    assert.equal(ui.buttons[1].dataset.selected, 'true');
    assert.equal(ui.hasCommandCheck(), false, 'Stop after the relay reports the requested state');
});

test('post-command checks stop at a deadline or a live matching report', async () => {
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        async () => ({ok: true, json: async () => ({state: 'on'})}), true);
    await new Promise(setImmediate);
    await ui.submit(1);
    ui.advance(3000);
    await ui.checkCommand();
    assert.equal(ui.hasCommandCheck(), false);
    assert.equal(ui.buttons[0].dataset.selected, 'true', 'Deadline never invents the requested OFF state');
    await ui.submit(1);
    ui.streams[0].onmessage({data: '{"state":"off"}'});
    assert.equal(ui.hasCommandCheck(), false);
    await ui.submit(0);
    ui.windowListeners.pagehide();
    assert.equal(ui.hasCommandCheck(), false, 'Leaving the page cancels follow-up requests');
});

test('stream failures fall back to polling, clear stale state, and recover on reconnect', async () => {
    const ui = fixture(() => {}, undefined, false,
        async () => ({ok: true, json: async () => ({state: 'off'})}), true);
    await new Promise(setImmediate);
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    ui.streams[0].onerror();
    await new Promise(setImmediate);
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'off');
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    ui.streams[0].onmessage({data: '{"state":"unknown"}'});
    assert.ok(ui.buttons.every(button => button.dataset.selected === 'false'));
});

test('a stream reset preserves a fresh OFF report while fallback is pending', async () => {
    let finish;
    let reads = 0;
    const ui = fixture(() => {}, undefined, false, () => {
        if (++reads === 1) return Promise.resolve({ok: true, json: async () => ({state: 'off'})});
        return new Promise(resolve => { finish = resolve; });
    }, true);
    await new Promise(setImmediate);
    ui.streams[0].onmessage({data: '{"state":"off"}'});
    ui.streams[0].onerror();
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'off', 'A stream reset does not mean RelayNode is unavailable');
    finish({ok: true, json: async () => ({state: 'off'})});
    await new Promise(setImmediate);
    assert.equal(ui.buttons[1].dataset.selected, 'true');
});

test('initial loading is checking, then a real relay report replaces it', async () => {
    let finish;
    const ui = fixture(() => {}, undefined, false,
        () => new Promise(resolve => { finish = resolve; }), true);
    assert.match(ui.ids['relay-state'].textContent, /Checking relay/);
    ui.streams[0].onerror();
    assert.match(ui.ids['relay-state'].textContent, /Checking relay/);
    finish({ok: true, json: async () => ({state: 'off'})});
    await new Promise(setImmediate);
    assert.equal(ui.ids['relay-state'].textContent, 'Relay is OFF');
});

test('one failed HTTP read preserves fresh live state but genuine stale state expires', async () => {
    let fail = false;
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false, async () => {
        if (fail) throw Error('One reset connection');
        return {ok: true, json: async () => ({state: 'on'})};
    }, true);
    await new Promise(setImmediate);
    ui.streams[0].onmessage({data: '{"state":"on"}'});
    fail = true;
    await ui.submit(1);
    await ui.checkCommand();
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    ui.advance(6000);
    await ui.poll();
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'unknown');
    assert.ok(ui.buttons.every(button => button.dataset.selected === 'false'));
});

test('failed follow-up reads preserve recent reports, never invent OFF, and stop at the deadline', async () => {
    let reads = 0;
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false, async () => {
        if (++reads > 1) throw Error('Offline');
        return {ok: true, json: async () => ({state: 'on'})};
    }, true);
    await new Promise(setImmediate);
    await ui.submit(1);
    await ui.checkCommand();
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'on');
    assert.equal(ui.buttons[1].dataset.selected, 'false');
    ui.advance(3000);
    await ui.checkCommand();
    assert.equal(ui.hasCommandCheck(), false);
    assert.equal(reads, 2, 'Do not start another network read past the deadline');
});

test('silent or malformed streams do not leave a stale glowing ON icon', async () => {
    const ui = fixture(() => {}, undefined, false,
        async () => ({ok: true, json: async () => ({state: 'unknown'})}), true);
    await new Promise(setImmediate);
    const original = ui.streams[0];
    original.onmessage({data: '{"state":"on"}'});
    ui.advance(6000);
    await ui.poll();
    assert.equal(original.closed, true);
    assert.equal(ui.streams.length, 2);
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'unknown');
    ui.streams[1].onmessage({data: '{"state":"on"}'});
    ui.streams[1].onmessage({data: 'bad JSON'});
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'unknown');
    original.onmessage({data: '{"state":"on"}'});
    assert.equal(ui.ids['relay-panel'].dataset.relayState, 'unknown', 'Closed streams cannot restore old state');
    ui.windowListeners.pagehide();
    assert.equal(ui.streams[1].closed, true);
});

test('HTTP rejection preserves the previous request and explains recovery', async () => {
    const ui = fixture(async () => ({ok: false, text: async () => 'Check Mesh provisioning and key binding.'}));
    await ui.submit(0);
    assert.equal(ui.ids['last-command'].textContent, 'No command yet');
    assert.equal(ui.ids['feedback-title'].textContent, 'Request not sent');
    assert.equal(ui.ids['feedback'].dataset.state, 'error');
    assert.ok(ui.buttons.every(button => !button.disabled));
});

test('lost response does not claim that a relay command failed or succeeded', async () => {
    const ui = fixture(async () => { throw Error('Network lost'); });
    await ui.submit(0);
    assert.equal(ui.ids['last-command'].textContent, 'No command yet');
    assert.equal(ui.ids['feedback-title'].textContent, 'No response from the gateway');
    assert.match(ui.ids['feedback-detail'].textContent, /may have been sent/);
    assert.ok(ui.buttons.every(button => !button.disabled));
});

test('a timed-out request releases the controls without silently resending', async () => {
    let requests = 0;
    const ui = fixture((url, options) => { requests++; return new Promise((resolve, reject) => {
        options.signal.addEventListener('abort', () => reject(Error('Timed out')));
    }); });
    const pending = ui.submit(0);
    await new Promise(setImmediate);
    ui.expire();
    await pending;
    assert.equal(requests, 1);
    assert.equal(ui.ids['feedback-title'].textContent, 'No response from the gateway');
    assert.ok(ui.buttons.every(button => !button.disabled));
});

test('theme selection restores and saves the preference with accessible selected state', () => {
    const ui = fixture(() => {}, 'dark');
    assert.equal(ui.root.dataset.theme, 'dark');
    assert.equal(ui.themes[1].attributes['aria-pressed'], 'true');
    ui.themes[0].listeners.click();
    assert.equal(ui.root.dataset.theme, 'light');
    assert.equal(ui.stored(), 'light');
    assert.equal(ui.themes[0].attributes['aria-pressed'], 'true');
});

test('blocked browser storage does not break theme switching or relay controls', async () => {
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, true);
    ui.themes[1].listeners.click();
    assert.equal(ui.root.dataset.theme, 'dark');
    await ui.submit(0);
    assert.equal(ui.ids['last-command'].textContent, 'ON requested');
});


test('a superseded HTTP failure cannot erase the latest pending click', async () => {
    const replies = [];
    const requests = [];
    const ui = fixture(url => {
        requests.push(url);
        return new Promise(resolve => replies.push(resolve));
    });
    const first = ui.submit(0);
    await new Promise(setImmediate);
    const latest = ui.submit(1);
    replies[0]({ok: false, text: async () => 'Older failure'});
    await new Promise(setImmediate);
    assert.deepEqual(requests, ['/on', '/off']);
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'off');
    assert.equal(ui.ids['feedback-title'].textContent, 'Turning off...');
    replies[1]({ok: true, text: async () => ''});
    await Promise.all([first, latest]);
    assert.equal(ui.ids['last-command'].textContent, 'OFF requested');
});

test('an unconfirmed preview expires back to the latest real report', async () => {
    const ui = fixture(async () => ({ok: true, text: async () => ''}), undefined, false,
        async () => ({ok: true, json: async () => ({state: 'on'})}));
    await new Promise(setImmediate);
    await ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'off');
    ui.advance(3000);
    await ui.checkCommand();
    assert.equal(ui.ids['relay-panel'].dataset.commandPending, 'false');
    assert.equal(ui.ids['relay-panel'].dataset.visualState, 'on');
    assert.equal(ui.ids['relay-state'].textContent, 'Relay is ON');
});
