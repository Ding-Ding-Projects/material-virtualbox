/*
 * App-wide services: the bridge API, the notification record, the local
 * append-only history and preferences.  Pages import these and nothing else
 * global.
 */

import { createStore, persist } from './core.js';

export const store = createStore({
    route: 'machines',
    machines: [],
    selectedId: persist.load('selectedId', null),
    host: null,
    status: { mode: 'none', reason: '' },
    notifications: persist.load('notifications', []),
    toasts: [],
    history: persist.load('history', []),
    prefs: Object.assign({ seed: 0, language: 'en', motion: 'lively', emoji: false, lowStim: false, school: false, onboarded: false }, persist.load('prefs', {})),
    appearance: persist.load('appearance', {})
});

/** Calls a bridge method.  Failures become a persistent error notification. */
export async function api(method, params, { quiet = false } = {}) {
    try {
        return await window.mvb.call(method, params);
    } catch (e) {
        const message = String(e && e.message || e).replace(/^Error invoking remote method '[^']+': (Error: )?/, '');
        if (!quiet) notify({ kind: 'error', title: 'Could not complete ' + method.replace('.', ' '), body: message });
        throw new Error(message);
    }
}

let toastSeq = 0;

/**
 * Shows a toast and records the notification.  Errors and warnings stay
 * until dismissed; info and success drain after six seconds.
 */
export function notify({ kind = 'info', title, body = '', action = null }) {
    const id = ++toastSeq;
    const entry = { id: Date.now() + '-' + id, kind, title, body, at: Date.now(), read: false };
    const notifications = [entry, ...store.get().notifications].slice(0, 500);
    persist.save('notifications', notifications);
    store.set((s) => ({ notifications, toasts: [...s.toasts, { id, kind, title, body, action, auto: kind === 'info' || kind === 'success' }].slice(-4) }));
    if (kind === 'info' || kind === 'success') setTimeout(() => dismissToast(id), 6000);
}

export function dismissToast(id) {
    store.set((s) => ({ toasts: s.toasts.filter((t) => t.id !== id) }));
}

export function markAllRead() {
    const notifications = store.get().notifications.map((n) => ({ ...n, read: true }));
    persist.save('notifications', notifications);
    store.set({ notifications });
}

export function removeNotifications(ids) {
    const notifications = store.get().notifications.filter((n) => !ids.includes(n.id));
    persist.save('notifications', notifications);
    store.set({ notifications });
}

/**
 * Appends a revision to the local history.  Restores append a new revision
 * that reverses the diff; nothing is ever rewritten.
 */
export function recordHistory({ title, scope, category, diff, undo = null }) {
    const rev = { id: Date.now().toString(36) + Math.random().toString(36).slice(2, 6), title, scope, category, diff, undo, at: Date.now() };
    const history = [rev, ...store.get().history].slice(0, 2000);
    persist.save('history', history);
    store.set({ history });
    return rev;
}

export function setPrefs(patch) {
    const prefs = { ...store.get().prefs, ...patch };
    persist.save('prefs', prefs);
    store.set({ prefs });
    applyPrefs(prefs);
}

export const SEEDS = [
    { hex: '#6750A4', primary: '#D0BCFF', on: '#381E72', container: '#4F378B', onContainer: '#EADDFF' },
    { hex: '#006A6A', primary: '#4CDADA', on: '#003737', container: '#004F4F', onContainer: '#6FF7F6' },
    { hex: '#B3261E', primary: '#FFB4AB', on: '#690005', container: '#93000A', onContainer: '#FFDAD6' },
    { hex: '#8B5000', primary: '#FFB869', on: '#4A2800', container: '#693C00', onContainer: '#FFDCBE' },
    { hex: '#006E1C', primary: '#78DC77', on: '#00390A', container: '#005313', onContainer: '#94F990' },
    { hex: '#0061A4', primary: '#9ECAFF', on: '#003258', container: '#00497D', onContainer: '#D1E4FF' },
    { hex: '#984061', primary: '#FFB0C8', on: '#5E1133', container: '#7B2949', onContainer: '#FFD9E2' },
    { hex: '#5D5F5F', primary: '#C6C7C7', on: '#2F3131', container: '#454747', onContainer: '#E2E2E2' }
];

export function applyPrefs(prefs) {
    const root = document.documentElement;
    const seed = SEEDS[prefs.seed] || SEEDS[0];
    root.style.setProperty('--primary', seed.primary);
    root.style.setProperty('--on-primary', seed.on);
    root.style.setProperty('--primary-container', seed.container);
    root.style.setProperty('--on-primary-container', seed.onContainer);
    root.dataset.motion = prefs.lowStim ? 'calm' : prefs.motion;
    root.dataset.lowStim = String(!!prefs.lowStim);
    root.lang = prefs.language === 'yue' ? 'yue-Hant-HK' : 'en';
}

/**
 * Wording for the three language modes.  Safety text (destructive actions,
 * errors) always stays in plain English beside the Cantonese.
 */
const YUE = {
    'Machines': '機器', 'Media': '媒體', 'Network': '網絡', 'Extensions': '擴充', 'Activity': '活動', 'Logs': '記錄', 'History': '歷史', 'Preferences': '設定',
    'Start': '開機', 'Show': '睇吓', 'Resume': '繼續', 'Settings': '設定', 'Running': '跑緊', 'Powered off': '熄咗', 'Saved': '儲存咗', 'Paused': '暫停咗'
};

export function t(text) {
    const lang = store.get().prefs.language;
    if (lang === 'yue' && !store.get().prefs.school && YUE[text]) return YUE[text];
    if (lang === 'both' && YUE[text]) return text + ' ' + YUE[text];
    return text;
}
