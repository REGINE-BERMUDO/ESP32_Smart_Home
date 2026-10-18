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

function fixture(fetch, savedTheme, storageBlocked = false) {
    const ids = Object.fromEntries(['last-command', 'command-time', 'feedback',
        'feedback-title', 'feedback-detail', 'relay-panel'].map(id => [id, element()]));
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
    const context = {document: {documentElement: root, getElementById: id => ids[id],
        querySelectorAll: selector => ({'.command-form': forms, '[data-command-button]': buttons,
            '[data-theme-choice]': themes}[selector] || [])},
        localStorage: {getItem() { if (storageBlocked) throw Error('Blocked'); return stored; },
            setItem(key, value) { if (storageBlocked) throw Error('Blocked'); stored = value; }},
        matchMedia: () => ({matches: false}), fetch, AbortController, Date,
        setTimeout(callback) { timeout = callback; return 1; }, clearTimeout() { timeout = undefined; }};
    vm.runInNewContext(script, context);
    return {ids, buttons, themes, root, stored: () => stored,
        expire: () => timeout(),
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

test('only the last accepted command is highlighted, with an accessible explanation', async () => {
    const ui = fixture(async () => ({ok: true, text: async () => ''}));
    assert.ok(ui.buttons.every(button => button.dataset.selected !== 'true'));
    await ui.submit(0);
    assert.equal(ui.ids['relay-panel'].dataset.lastCommand, 'ON');
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    assert.equal(ui.buttons[1].dataset.selected, 'false');
    assert.match(ui.buttons[0].attributes['aria-label'], /last accepted web request/);
    await ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.lastCommand, 'OFF');
    assert.equal(ui.buttons[0].dataset.selected, 'false');
    assert.equal(ui.buttons[1].dataset.selected, 'true');
    assert.equal(ui.buttons[0].attributes['aria-label'], 'Turn on');
});

test('pending, rejected, and lost responses preserve the previous accepted highlight', async () => {
    let finish;
    let requests = 0;
    const ui = fixture(() => {
        requests++;
        if (requests === 1) return Promise.resolve({ok: true, text: async () => ''});
        if (requests === 2) return new Promise(resolve => { finish = resolve; });
        return Promise.reject(Error('Connection lost'));
    });
    await ui.submit(0);
    const pending = ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.lastCommand, 'ON');
    finish({ok: false, text: async () => 'Mesh unavailable'});
    await pending;
    assert.equal(ui.buttons[0].dataset.selected, 'true');
    assert.equal(ui.buttons[1].dataset.selected, 'false');
    await ui.submit(1);
    assert.equal(ui.ids['relay-panel'].dataset.lastCommand, 'ON');
    assert.equal(ui.buttons[0].dataset.selected, 'true');
});

test('pending requests prevent duplicate commands and re-enable both controls', async () => {
    let finish;
    let requests = 0;
    const ui = fixture(() => { requests++; return new Promise(resolve => { finish = resolve; }); });
    const pending = ui.submit(0);
    assert.ok(ui.buttons.every(button => button.disabled));
    await ui.submit(1);
    assert.equal(requests, 1);
    finish({ok: true, text: async () => ''});
    await pending;
    assert.ok(ui.buttons.every(button => !button.disabled));
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
