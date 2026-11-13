/* Small formatting helpers shared by pages and tests. */

export function bytes(n) {
    const v = Number(n) || 0;
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let i = 0, x = v;
    while (x >= 1024 && i < units.length - 1) { x /= 1024; i++; }
    return (i === 0 ? String(x) : x.toFixed(x >= 100 ? 0 : 1)) + ' ' + units[i];
}

export function megabytes(mb) {
    return mb >= 1024 ? (mb / 1024).toFixed(mb % 1024 ? 1 : 0) + ' GB' : mb + ' MB';
}

const STATE_LABELS = {
    poweredOff: 'Powered off', saved: 'Saved', abortedSaved: 'Aborted, saved', aborted: 'Aborted', running: 'Running', paused: 'Paused',
    stuck: 'Guru meditation', teleported: 'Teleported', starting: 'Starting', stopping: 'Stopping', saving: 'Saving', restoring: 'Restoring',
    busy: 'Busy', inaccessible: 'Inaccessible'
};

export function stateLabel(s) {
    return STATE_LABELS[s] || s;
}

/** Tone used by state dots: success, primary, muted, error, warning. */
export function stateTone(s) {
    if (s === 'running') return 'success';
    if (s === 'paused' || s === 'saved' || s === 'abortedSaved') return 'primary';
    if (s === 'aborted' || s === 'stuck' || s === 'inaccessible') return 'error';
    if (s === 'starting' || s === 'stopping' || s === 'saving' || s === 'restoring' || s === 'busy') return 'warning';
    return 'muted';
}

/** The primary action a machine offers in its current state. */
export function primaryAction(s) {
    if (s === 'running' || s === 'paused') return { id: 'show', label: 'Show' };
    if (s === 'saved' || s === 'abortedSaved') return { id: 'start', label: 'Resume' };
    if (s === 'poweredOff' || s === 'aborted' || s === 'teleported') return { id: 'start', label: 'Start' };
    return { id: null, label: stateLabel(s) };
}

export function monogram(name) {
    const m = String(name || '?').trim().match(/[A-Za-z0-9]/);
    return m ? m[0].toUpperCase() : '?';
}

export function relativeTime(ms, now = Date.now()) {
    if (!ms) return '';
    const d = Math.round((now - ms) / 1000);
    if (d < 60) return 'just now';
    if (d < 3600) return Math.round(d / 60) + ' min ago';
    if (d < 86400) return Math.round(d / 3600) + ' h ago';
    return new Date(ms).toLocaleDateString(undefined, { day: 'numeric', month: 'short', year: 'numeric' });
}

/** Splits VBox.log text into { time, text, severity } rows. */
export function parseLog(text) {
    return String(text || '').split(/\r?\n/).filter((l) => l.length).map((line) => {
        const m = /^(\d{2}:\d{2}:\d{2}\.\d{6})\s+(.*)$/.exec(line);
        const body = m ? m[2] : line;
        let severity = 'info';
        if (/\b(error|failed|VERR_|guru)/i.test(body)) severity = 'error';
        else if (/\b(warning|WARN|VWRN_)/.test(body)) severity = 'warning';
        else if (/\b(ready|started|mounted|reported)\b/i.test(body)) severity = 'notice';
        return { time: m ? m[1].slice(0, 12) : '', text: body, severity };
    });
}
