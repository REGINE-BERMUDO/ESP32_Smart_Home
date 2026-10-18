(() => {
    'use strict';

    const root = document.documentElement;
    const themeButtons = document.querySelectorAll('[data-theme-choice]');
    const commandButtons = document.querySelectorAll('[data-command-button]');
    const panel = document.getElementById('relay-panel');
    const feedback = document.getElementById('feedback');
    const feedbackTitle = document.getElementById('feedback-title');
    const feedbackDetail = document.getElementById('feedback-detail');
    let busy = false;

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

    document.querySelectorAll('.command-form').forEach(form => {
        form.addEventListener('submit', async event => {
            event.preventDefault();
            if (busy) return;
            busy = true;
            commandButtons.forEach(button => { button.disabled = true; });
            panel.setAttribute('aria-busy', 'true');
            const command = form.dataset.command;
            feedback.dataset.state = 'pending';
            feedbackTitle.textContent = 'Sending ' + command + ' request…';
            feedbackDetail.textContent = 'Waiting for the gateway.';

            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 10000);
            try {
                const response = await fetch(form.getAttribute('action'), {
                    method: 'POST',
                    headers: {'X-Requested-With': 'fetch', 'Accept': 'text/plain'},
                    cache: 'no-store',
                    signal: controller.signal
                });
                const message = await response.text();
                if (!response.ok) {
                    feedback.dataset.state = 'error';
                    feedbackTitle.textContent = 'Request not sent';
                    feedbackDetail.textContent = message || 'Check the gateway connection and try again.';
                    return;
                }

                // This marks the accepted web request, not measured relay state.
                panel.dataset.lastCommand = command;
                commandButtons.forEach(button => {
                    const selected = button.dataset.commandButton === command;
                    button.dataset.selected = String(selected);
                    const label = button.dataset.commandButton === 'ON' ? 'Turn on' : 'Turn off';
                    button.setAttribute('aria-label', label + (selected ? ', last accepted web request' : ''));
                });
                document.getElementById('last-command').textContent = command + ' requested';
                document.getElementById('command-time').textContent = 'Sent at ' + new Date().toLocaleTimeString([], {
                    hour: '2-digit', minute: '2-digit', second: '2-digit'
                });
                feedback.dataset.state = 'success';
                feedbackTitle.textContent = 'Request sent';
                feedbackDetail.textContent = 'Relay state is not confirmed.';
            } catch (_) {
                // A lost HTTP response does not prove whether the command was sent.
                feedback.dataset.state = 'error';
                feedbackTitle.textContent = 'No response from the gateway';
                feedbackDetail.textContent = 'The command may have been sent. Check the relay and your Wi-Fi before trying again.';
            } finally {
                clearTimeout(timeout);
                busy = false;
                commandButtons.forEach(button => { button.disabled = false; });
                panel.setAttribute('aria-busy', 'false');
            }
        });
    });
})();
