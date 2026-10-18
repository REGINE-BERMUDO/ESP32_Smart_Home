(() => {
    'use strict';

    const root = document.documentElement;
    const themeButtons = document.querySelectorAll('[data-theme-choice]');
    const commandButtons = document.querySelectorAll('[data-command-button]');
    const panel = document.getElementById('relay-panel');
    const feedback = document.getElementById('feedback');
    const feedbackTitle = document.getElementById('feedback-title');
    const feedbackDetail = document.getElementById('feedback-detail');
    let queuedCommand = null;
    let commandRevision = 0;
    let commandDrain = null;
    let commandController = null;
    let pendingVisualTarget = null;
    let polling = false;
    let reportGeneration = 0;
    let statusController = null;
    let stateStream = null;
    let streamHealthy = false;
    let streamMessageAt = 0;
    let displayedRelayState = null;
    let lastKnownReportAt = null;
    const pageStartedAt = performance.now();
    let commandCheckTimer = null;
    let commandCheckGeneration = 0;
    let commandTarget = null;

    function renderControls() {
        const known = displayedRelayState === 'on' || displayedRelayState === 'off';
        const visual = pendingVisualTarget || (known ? displayedRelayState : 'unknown');
        panel.dataset.visualState = visual;
        panel.dataset.commandPending = String(pendingVisualTarget !== null);
        panel.setAttribute('aria-busy', String(pendingVisualTarget !== null));
        commandButtons.forEach(button => {
            const value = button.dataset.commandButton.toLowerCase();
            const selected = known && value === displayedRelayState;
            button.dataset.selected = String(selected);
            button.dataset.active = String(value === visual);
            const label = value === 'on' ? 'Turn on' : 'Turn off';
            button.setAttribute('aria-label', label + (pendingVisualTarget === value
                ? ', request pending; waiting for relay report'
                : selected ? ', relay reports ' + value.toUpperCase() : ''));
        });
    }

    function stopCommandChecks(clearPreview = true) {
        ++commandCheckGeneration;
        if (commandCheckTimer !== null) clearTimeout(commandCheckTimer);
        commandCheckTimer = null;
        commandTarget = null;
        if (clearPreview) {
            pendingVisualTarget = null;
            renderControls();
        }
    }

    function followCommand(command) {
        stopCommandChecks(false);
        commandTarget = command.toLowerCase();
        const generation = commandCheckGeneration;
        const deadline = performance.now() + 2500;
        async function check() {
            commandCheckTimer = null;
            if (generation !== commandCheckGeneration) return;
            if (performance.now() >= deadline) {
                stopCommandChecks();
                return;
            }
            // A briefly stalled stream can still look healthy. Read actual state independently.
            await refreshRelayState();
            if (generation !== commandCheckGeneration) return;
            if (performance.now() >= deadline) {
                stopCommandChecks();
                return;
            }
            commandCheckTimer = setTimeout(check, 200);
        }
        commandCheckTimer = setTimeout(check, 200);
    }

    function showRelayState(state, checking = false) {
        const known = state === 'on' || state === 'off';
        lastKnownReportAt = known ? performance.now() : null;
        if (known && commandTarget === state) stopCommandChecks();
        const nextState = known ? state : checking ? 'checking' : 'unknown';
        if (displayedRelayState === nextState) { renderControls(); return; }
        displayedRelayState = nextState;
        panel.dataset.relayState = known ? state : 'unknown';
        document.getElementById('relay-state').textContent = known
            ? 'Relay is ' + state.toUpperCase() : checking ? 'Checking relay…' : 'State unavailable';
        document.getElementById('relay-state-detail').textContent = known
            ? 'Reported by RelayNode. Updates automatically.'
            : checking ? 'Waiting for the first relay report.'
            : 'Waiting for a fresh RelayNode response. Check its power and Mesh connection.';
        renderControls();
    }

    function clearExpiredState() {
        if (displayedRelayState === 'checking' && performance.now() - pageStartedAt < 3500) return;
        if (lastKnownReportAt === null || performance.now() - lastKnownReportAt >= 5000) {
            showRelayState('unknown');
        }
    }

    async function refreshRelayState() {
        if (polling) return;
        polling = true;
        const controller = new AbortController();
        statusController = controller;
        const generation = reportGeneration;
        const timeout = setTimeout(() => controller.abort(), 3500);
        try {
            const response = await fetch('/status', {cache: 'no-store', signal: controller.signal});
            if (!response.ok) throw Error('Status unavailable');
            const report = await response.json();
            if (generation === reportGeneration) showRelayState(report && report.state);
        } catch (_) {
            if (generation === reportGeneration) clearExpiredState();
        } finally {
            clearTimeout(timeout);
            polling = false;
            statusController = null;
        }
    }

    function connectStateStream() {
        if (typeof EventSource !== 'function' || stateStream) return;
        let source;
        try { source = new EventSource('/events'); } catch (_) { return; }
        stateStream = source;
        streamMessageAt = performance.now();
        source.onmessage = event => {
            if (stateStream !== source) return;
            ++reportGeneration;
            if (statusController) statusController.abort();
            try {
                const report = JSON.parse(event.data);
                if (!report || !['on', 'off', 'unknown'].includes(report.state)) throw Error('Invalid relay report');
                streamHealthy = true;
                streamMessageAt = performance.now();
                showRelayState(report.state);
            } catch (_) {
                streamHealthy = false;
                showRelayState('unknown');
            }
        };
        source.onerror = () => {
            if (stateStream !== source) return;
            streamHealthy = false;
            clearExpiredState();
            refreshRelayState(); // EventSource reconnects automatically; HTTP covers the gap.
        };
    }

    function checkStateConnection() {
        if (stateStream && performance.now() - streamMessageAt >= 5000) {
            stateStream.close();
            stateStream = null;
            streamHealthy = false;
            ++reportGeneration;
            clearExpiredState();
        }
        clearExpiredState();
        connectStateStream();
        if (!streamHealthy) return refreshRelayState();
    }

    window.addEventListener('pagehide', () => {
        queuedCommand = null;
        ++commandRevision;
        if (commandController) commandController.abort();
        stopCommandChecks();
        if (stateStream) stateStream.close();
        stateStream = null;
        streamHealthy = false;
        ++reportGeneration;
        if (statusController) statusController.abort();
        showRelayState('unknown');
    });

    function setTheme(theme) {
        root.dataset.theme = theme;
        themeButtons.forEach(button => {
            button.setAttribute('aria-pressed', String(button.dataset.themeChoice === theme));
        });
        try { localStorage.setItem('smart-home-theme', theme); } catch (_) {}
    }

    let theme = root.dataset.theme || (matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
    try {
        const saved = localStorage.getItem('smart-home-theme');
        if (saved === 'light' || saved === 'dark') theme = saved;
    } catch (_) {}
    setTheme(theme);
    themeButtons.forEach(button => button.addEventListener('click', () => setTheme(button.dataset.themeChoice)));
    showRelayState('unknown', true);
    refreshRelayState();
    connectStateStream();
    setInterval(checkStateConnection, 1500);

    async function drainCommands() {
        while (queuedCommand) {
            const request = queuedCommand;
            queuedCommand = null;
            const controller = new AbortController();
            commandController = controller;
            const timeout = setTimeout(() => controller.abort(), 10000);
            try {
                const response = await fetch(request.action, {
                    method: 'POST',
                    headers: {'X-Requested-With': 'fetch', 'Accept': 'text/plain'},
                    cache: 'no-store',
                    signal: controller.signal
                });
                const message = await response.text();
                // An older reply must never overwrite the user's newest intent.
                if (request.revision !== commandRevision) continue;
                if (!response.ok) {
                    stopCommandChecks();
                    feedback.dataset.state = 'error';
                    feedbackTitle.textContent = 'Request not sent';
                    feedbackDetail.textContent = message || 'Check the gateway connection and try again.';
                    continue;
                }
                document.getElementById('last-command').textContent = request.command + ' requested';
                document.getElementById('command-time').textContent = 'Sent at ' + new Date().toLocaleTimeString([], {
                    hour: '2-digit', minute: '2-digit', second: '2-digit'
                });
                feedback.dataset.state = 'success';
                feedbackTitle.textContent = 'Request sent';
                feedbackDetail.textContent = 'Sending is accepted; switching is not confirmed here. See the reported state above.';
                followCommand(request.command);
            } catch (_) {
                if (request.revision !== commandRevision) continue;
                stopCommandChecks();
                feedback.dataset.state = 'error';
                feedbackTitle.textContent = 'No response from the gateway';
                feedbackDetail.textContent = 'The command may have been sent. Check the relay and your Wi-Fi before trying again.';
            } finally {
                clearTimeout(timeout);
                commandController = null;
            }
        }
    }

    function sendQueuedCommands() {
        if (!commandDrain) {
            commandDrain = Promise.resolve().then(drainCommands).finally(() => {
                commandDrain = null;
                if (queuedCommand) sendQueuedCommands();
            });
        }
        return commandDrain;
    }

    document.querySelectorAll('.command-form').forEach(form => {
        form.addEventListener('submit', event => {
            event.preventDefault();
            const command = form.dataset.command;
            queuedCommand = {command, action: form.getAttribute('action'), revision: ++commandRevision};
            stopCommandChecks(false);
            pendingVisualTarget = command.toLowerCase();
            renderControls();
            feedback.dataset.state = 'pending';
            feedbackTitle.textContent = command === 'ON' ? 'Turning on...' : 'Turning off...';
            feedbackDetail.textContent = 'Waiting for relay confirmation. You can change your request.';
            return sendQueuedCommands();
        });
    });
})();
