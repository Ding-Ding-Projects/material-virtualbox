/*
 * The shell: frameless title bar with tabs, navigation rail, router, global
 * shortcuts and the connection banner.
 */

import { h, icon, ICONS, mount, persist } from './core.js';
import { store, api, applyPrefs, setPrefs, t } from './services.js';
import { openPalette, openNotificationCentre, renderToasts, registerPaletteSource, teleport } from './features/overlays.js';
import { reconcile, closeTab, openTab, openEvery, togglePin } from './logic/tabs-model.js';
import { renderMachines, refreshMachines } from './pages/machines.js';
import { renderSettings, renderNewVm } from './pages/settings.js';
import { renderMedia, renderNetwork, renderExtensions, renderLogs, renderActivity } from './pages/tools.js';
import { renderScreen, closeScreen } from './pages/screen.js';
import { renderHistory, renderPrefs, renderTabManager, renderOnboarding } from './pages/app-pages.js';

const CATALOG = [
    { id: 'machines', label: 'Machines', icon: ICONS.machine, render: renderMachines },
    { id: 'media', label: 'Media', icon: ICONS.media, render: renderMedia },
    { id: 'network', label: 'Network', icon: ICONS.network, render: renderNetwork },
    { id: 'extensions', label: 'Extensions', icon: ICONS.extensions, render: renderExtensions },
    { id: 'activity', label: 'Activity', icon: ICONS.activity, render: renderActivity },
    { id: 'logs', label: 'Logs', icon: ICONS.logs, render: renderLogs },
    { id: 'history', label: 'History', icon: ICONS.history, render: renderHistory },
    { id: 'prefs', label: 'Preferences', icon: ICONS.prefs, render: renderPrefs }
];
const EXTRA = {
    settings: { label: 'Settings', render: renderSettings },
    newvm: { label: 'New machine', render: renderNewVm },
    tabs: { label: 'Tab manager', render: (root) => renderTabManager(root, tabsApi) },
    onboarding: { label: 'First-run setup', render: renderOnboarding },
    screen: { label: 'Machine screen', render: renderScreen }
};

let tabs = reconcile(CATALOG, persist.load('tabs', null));
const saveTabs = (next) => { tabs = next; persist.save('tabs', tabs); drawTabs(); };

const tabsApi = {
    describe: () => tabs.open.map((id) => { const c = CATALOG.find((x) => x.id === id); return { id, title: c.label, group: 'Manager', state: '', pinned: tabs.pinned.includes(id) }; }),
    pinned: () => tabs.pinned,
    open: (id) => { saveTabs(openTab(tabs, id)); store.set({ route: id }); },
    closeMany: (ids) => { let n = tabs; for (const id of ids) n = closeTab(n, id); saveTabs(n); },
    openEvery: () => saveTabs(openEvery(tabs, CATALOG))
};

const els = {};

function drawTabs() {
    const route = store.get().route;
    mount(els.tabs, tabs.open.map((id) => {
        const c = CATALOG.find((x) => x.id === id);
        const pinned = tabs.pinned.includes(id);
        return h('div', { class: 'tab st', role: 'tab', tabindex: '0', 'aria-selected': String(route === id), onclick: () => store.set({ route: id }), onkeydown: (e) => { if (e.key === 'Enter' || e.key === ' ') store.set({ route: id }); },
            oncontextmenu: (e) => { e.preventDefault(); saveTabs(togglePin(tabs, id)); } },
            pinned ? icon(ICONS.pin, 13, { fill: 'currentColor', class: 'pin' }) : null, t(c.label),
            pinned ? null : h('button', { class: 'x st', 'aria-label': 'Close ' + c.label, onclick: (e) => { e.stopPropagation(); const next = closeTab(tabs, id); saveTabs(next); if (route === id) store.set({ route: next.open[0] || 'machines' }); } }, icon(ICONS.close, 12)));
    }), tabs.closed.length ? h('button', { class: 'tab st', title: 'Open every tab', onclick: () => saveTabs(openEvery(tabs, CATALOG)) }, icon(ICONS.plus, 14), 'Open every tab') : null);
}

function drawRail() {
    const route = store.get().route;
    mount(els.rail,
        h('button', { class: 'fab st pop', 'aria-label': 'New virtual machine', title: 'New virtual machine', onclick: () => store.set({ route: 'newvm' }) }, icon(ICONS.plus, 24)),
        CATALOG.map((c, i) => h('button', { class: 'rail-item st slide', 'aria-current': route === c.id ? 'page' : 'false', style: { animationDelay: 60 + i * 50 + 'ms' }, onclick: () => { saveTabs(openTab(tabs, c.id)); store.set({ route: c.id }); } },
            h('span', { class: 'pill' }, icon(c.icon, 22)), t(c.label))));
}

function drawBanner() {
    const s = store.get().status;
    if (s.mode === 'demo') mount(els.banner, h('div', { class: 'banner warn', role: 'status', style: { borderRadius: 0, padding: '8px 18px' } }, icon(ICONS.info, 18), h('strong', null, 'Demo data.'), 'Started with --demo: nothing here is a real machine, and no change reaches VirtualBox.'));
    else if (s.mode === 'missing') mount(els.banner, h('div', { class: 'banner error', role: 'alert', style: { borderRadius: 0, padding: '8px 18px' } }, icon(ICONS.info, 18), h('strong', null, 'Not connected to VirtualBox.'), s.reason, ' Install VBoxMaterialBridge next to VirtualBox, or start with --demo to look around.'));
    else mount(els.banner);
}

function drawBell() {
    const unread = store.get().notifications.filter((n) => !n.read).length;
    mount(els.bell, icon(ICONS.bell), unread ? h('span', { class: 'badge pop' }, String(Math.min(unread, 99))) : null);
    els.bell.setAttribute('aria-label', 'Notifications, ' + unread + ' unread');
}

function route() {
    const r = store.get().route;
    const page = CATALOG.find((c) => c.id === r) || EXTRA[r] || CATALOG[0];
    if (r !== 'screen') closeScreen();
    els.main.scrollTop = 0;
    page.render(els.main);
}

registerPaletteSource(() => [
    ...CATALOG.map((c) => ({ scope: 'Pages', title: c.label, path: 'Go to', icon: c.icon, run: () => { saveTabs(openTab(tabs, c.id)); store.set({ route: c.id }); } })),
    { scope: 'Pages', title: 'New virtual machine', path: 'Wizard', icon: ICONS.plus, run: () => store.set({ route: 'newvm' }) },
    { scope: 'Pages', title: 'Tab manager', path: 'Tabs', icon: ICONS.tabs, keys: 'Ctrl+Shift+T', run: () => store.set({ route: 'tabs' }) },
    { scope: 'Pages', title: 'First-run setup', path: 'Setup', icon: ICONS.info, run: () => store.set({ route: 'onboarding' }) },
    { scope: 'Actions', title: 'Open every tab', path: 'Tabs', icon: ICONS.tabs, run: () => saveTabs(openEvery(tabs, CATALOG)) },
    { scope: 'Settings', title: 'Theme seed', path: 'Preferences · Appearance', icon: ICONS.prefs, run: () => { store.set({ route: 'prefs' }); teleport('#pref-seed'); } },
    { scope: 'Settings', title: 'Animation level', path: 'Preferences · Motion', icon: ICONS.prefs, run: () => { store.set({ route: 'prefs' }); teleport('#pref-motion'); } },
    { scope: 'Settings', title: 'Low stimulation', path: 'Preferences', icon: ICONS.prefs, toggle: { get: () => store.get().prefs.lowStim, set: (v) => setPrefs({ lowStim: v }) }, run: () => setPrefs({ lowStim: !store.get().prefs.lowStim }) },
    { scope: 'Settings', title: 'School mode', path: 'Preferences', icon: ICONS.prefs, toggle: { get: () => store.get().prefs.school, set: (v) => setPrefs({ school: v }) }, run: () => setPrefs({ school: !store.get().prefs.school }) }
]);

function build() {
    const app = document.getElementById('app');
    els.tabs = h('div', { class: 'tabs', role: 'tablist', 'aria-label': 'Open tabs' });
    els.bell = h('button', { class: 'icon-btn st', onclick: openNotificationCentre });
    els.rail = h('nav', { class: 'rail', 'aria-label': 'Destinations' });
    els.main = h('div', { class: 'main', id: 'main' });
    els.banner = h('div');
    mount(app, h('div', { class: 'app' },
        h('header', { class: 'titlebar' },
            h('div', { class: 'brand', 'aria-hidden': 'true' }, icon(ICONS.machine, 18)),
            h('span', { class: 'brand-name' }, 'Material Virtual Machine'),
            els.tabs,
            h('button', { class: 'search-pill st', onclick: openPalette }, icon(ICONS.search, 18), h('span', { class: 'grow', style: { textAlign: 'left' } }, 'Search everything'), h('span', { class: 'kbd' }, 'Ctrl+Shift+F')),
            els.bell,
            h('button', { class: 'icon-btn st', 'aria-label': 'History', title: 'History (Ctrl+H)', onclick: () => store.set({ route: 'history' }) }, icon(ICONS.history)),
            h('div', { class: 'row', style: { gap: 0, flexWrap: 'nowrap' } },
                h('button', { class: 'win-btn st', 'aria-label': 'Minimize', onclick: () => window.mvb.window.minimize() }, icon(ICONS.min, 14)),
                h('button', { class: 'win-btn st', 'aria-label': 'Maximize', onclick: () => window.mvb.window.toggleMaximize() }, icon(ICONS.max, 13)),
                h('button', { class: 'win-btn close st', 'aria-label': 'Close', onclick: () => window.mvb.window.close() }, icon(ICONS.close, 14)))),
        els.banner,
        h('div', { class: 'body' }, els.rail, els.main)));
    renderToasts(document.getElementById('toasts'));
}

document.addEventListener('keydown', (e) => {
    const k = e.key.toLowerCase();
    if (e.ctrlKey && e.shiftKey && k === 'f') { e.preventDefault(); openPalette(); }
    else if (e.ctrlKey && e.shiftKey && k === 't') { e.preventDefault(); store.set({ route: 'tabs' }); }
    else if (e.ctrlKey && !e.shiftKey && k === 'h') { e.preventDefault(); store.set({ route: 'history' }); }
});

async function start() {
    applyPrefs(store.get().prefs);
    build();
    const status = await window.mvb.status();
    store.set({ status });
    if (status.mode !== 'missing') {
        const host = await api('host.info', {}, { quiet: true }).catch(() => null);
        store.set({ host });
        await refreshMachines();
    }
    if (!store.get().prefs.onboarded) store.set({ route: 'onboarding' });

    let last = {};
    store.subscribe((s) => {
        if (s.route !== last.route) { persist.save('route', s.route); drawTabs(); drawRail(); route(); }
        else if (s.machines !== last.machines || s.selectedId !== last.selectedId || s.prefs !== last.prefs) {
            persist.save('selectedId', s.selectedId);
            if (['machines', 'settings', 'logs', 'prefs'].includes(s.route)) route();
            else if (s.route === 'screen') renderScreen(els.main);
            if (s.prefs !== last.prefs) { drawTabs(); drawRail(); }
        }
        if (s.notifications !== last.notifications) drawBell();
        if (s.status !== last.status) drawBanner();
        last = s;
    });
    drawTabs(); drawRail(); drawBanner(); drawBell(); route();
    /* Keep machine states current; VBoxSVC is the source of truth. */
    setInterval(() => { if (store.get().status.mode !== 'missing') refreshMachines(); }, 4000);
}

start();
