/*
 * Machines: the library, the selected machine's hero and its four panes
 * (Details, Snapshots, Logs, Resource use).  Actions go to VBoxSVC through
 * the bridge; "Show" opens the native VirtualBoxVM window.
 */

import { h, icon, ICONS, mount, stagger, persist } from '../core.js';
import { store, api, notify, recordHistory, t } from '../services.js';
import { searchField, confirmDestructive, openExport, registerPaletteSource } from '../features/overlays.js';
import { megabytes, bytes, stateLabel, stateTone, primaryAction, monogram, relativeTime, parseLog } from '../logic/format.js';

const TILE = [['#4F378B', '#EADDFF'], ['#633B48', '#FFD8E4'], ['#1E3A5F', '#D1E4FF'], ['#4A4458', '#E8DEF8'], ['#5C3F1E', '#FFDDB8'], ['#2E4A2C', '#C4EEBE']];
export const tileColors = (name) => TILE[[...String(name)].reduce((a, c) => a + c.charCodeAt(0), 0) % TILE.length];

let pane = persist.load('machinePane', 'details');
let filter = 'all';
let matcher = () => true;
let paneCache = { id: null, details: null, snaps: null, log: null };

export async function refreshMachines() {
    const machines = await api('machines.list', {}, { quiet: true }).catch(() => store.get().machines);
    const s = store.get();
    const selectedId = machines.some((m) => m.id === s.selectedId) ? s.selectedId : (machines[0] && machines[0].id) || null;
    if (JSON.stringify(machines) === JSON.stringify(s.machines) && selectedId === s.selectedId) return;
    store.set({ machines, selectedId: machines.some((m) => m.id === s.selectedId) ? s.selectedId : (machines[0] && machines[0].id) || null });
}

export async function machineAction(m, action) {
    try {
        if (action === 'start' || action === 'show') {
            notify({ kind: 'info', title: action === 'show' ? 'Opening ' + m.name : 'Starting ' + m.name, body: 'The machine window opens in its own process.' });
            if (action === 'start') await api('machine.start', { id: m.id, type: 'gui' });
            else await api('machine.control', { id: m.id, action: 'show' });
        } else {
            await api('machine.control', { id: m.id, action });
        }
        recordHistory({ title: actionTitle(action) + ': ' + m.name, scope: m.name, category: 'Machines', diff: [['-', 'State: ' + stateLabel(m.state)], ['+', 'Action: ' + actionTitle(action)]] });
        await refreshMachines();
    } catch { /* api() already raised the notification */ }
}

function actionTitle(a) {
    return { start: 'Started', show: 'Opened', pause: 'Paused', resume: 'Resumed', powerOff: 'Powered off', acpiShutdown: 'Sent shutdown', saveState: 'Saved state', reset: 'Reset' }[a] || a;
}

export function deleteMachine(m) {
    confirmDestructive({
        title: 'Delete ' + m.name,
        detail: 'Unregisters the machine and deletes its settings, snapshots and attached hard disks. This cannot be undone.',
        actionLabel: 'Delete',
        onConfirm: async () => {
            await api('machine.delete', { id: m.id, confirmName: m.name });
            recordHistory({ title: 'Machine deleted: ' + m.name, scope: m.name, category: 'Machines', diff: [['-', 'Machine: ' + m.name], ['-', 'Memory: ' + megabytes(m.memoryMB)]] });
            notify({ kind: 'success', title: 'Deleted ' + m.name });
            await refreshMachines();
        }
    });
}

async function takeSnapshot(m) {
    const name = 'Snapshot ' + new Date().toLocaleString();
    try {
        await api('snapshots.take', { id: m.id, name, description: 'Taken from the Material shell' });
        notify({ kind: 'success', title: 'Snapshot taken', body: name + ' · ' + m.name });
        recordHistory({ title: 'Snapshot taken: ' + name, scope: m.name, category: 'Snapshots', diff: [['+', 'Snapshot: ' + name]] });
        paneCache.snaps = null;
        await refreshMachines();
    } catch { /* notified */ }
}

registerPaletteSource(() => store.get().machines.flatMap((m) => [
    { scope: 'Machines', title: m.name, path: 'Machines · ' + stateLabel(m.state), icon: ICONS.machine, run: () => { store.set({ route: 'machines', selectedId: m.id }); } },
    { scope: 'Actions', title: primaryAction(m.state).label + ' ' + m.name, path: 'Action', icon: ICONS.play, run: () => primaryAction(m.state).id && machineAction(m, primaryAction(m.state).id) },
    { scope: 'Actions', title: 'Take snapshot of ' + m.name, path: 'Action · Snapshots', icon: ICONS.camera, keys: 'Ctrl+Shift+S', run: () => takeSnapshot(m) }
]));

export function renderMachines(root) {
    const s = store.get();
    const list = s.machines.filter((m) => (filter === 'all' || (filter === 'running' ? m.state === 'running' || m.state === 'paused' : m.state === 'saved')) && matcher(m.name + ' ' + (m.osTypeId || '') + ' ' + (m.groups || []).join(' ')));
    const sel = s.machines.find((m) => m.id === s.selectedId) || null;
    const running = s.machines.filter((m) => m.state === 'running').length;

    const listCol = h('section', { class: 'list-col', 'aria-label': 'Virtual machines' },
        h('div', { class: 'spread', style: { alignItems: 'baseline' } }, h('h1', { class: 'title', style: { fontSize: '22px', fontWeight: 800 } }, t('Machines')), h('span', { class: 'muted', style: { fontSize: '12px' } }, s.machines.length + ' machines, ' + running + ' running')),
        searchField({ placeholder: 'Filter machines', onChange: (m) => { matcher = m; rerenderList(); }, samples: () => s.machines.map((m) => m.name) }),
        h('div', { class: 'row', role: 'group', 'aria-label': 'Filters' }, [['all', 'All'], ['running', 'Running'], ['saved', 'Saved']].map(([id, label]) => h('button', { class: 'chip st', 'aria-pressed': String(filter === id), onclick: () => { filter = id; renderMachines(root); } }, label))),
        h('div', { class: 'vm-list', id: 'vm-list' }));

    const rerenderList = () => {
        const visible = store.get().machines.filter((m) => (filter === 'all' || (filter === 'running' ? m.state === 'running' || m.state === 'paused' : m.state === 'saved')) && matcher(m.name + ' ' + (m.osTypeId || '')));
        mount(listCol.querySelector('#vm-list'), visible.length ? visible.map((m, i) => {
            const [bg, fg] = tileColors(m.name), tone = stateTone(m.state);
            return h('button', { class: 'vm-item st lift rise', id: 'vm-' + m.id, 'aria-pressed': String(m.id === store.get().selectedId), style: stagger(i, 60, 60), onclick: () => store.set({ selectedId: m.id }), oncontextmenu: (e) => { e.preventDefault(); store.set({ selectedId: m.id }); } },
                h('span', { class: 'tile', style: { background: bg, color: fg } }, monogram(m.name)),
                h('span', { class: 'grow', style: { display: 'flex', flexDirection: 'column', gap: '2px' } }, h('span', { style: { fontWeight: 650, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' } }, m.name), h('span', { class: 'muted', style: { fontSize: '12px' } }, m.osTypeId)),
                h('span', { class: 'row tone-' + tone, style: { gap: '6px', fontSize: '12px', flexWrap: 'nowrap' } }, h('span', { class: 'dot bg-' + tone + (m.state === 'running' ? ' pulse' : '') }), t(stateLabel(m.state))));
        }) : h('div', { class: 'empty' }, s.machines.length ? 'No machine matches this filter.' : 'No machines yet. Create one with the + button.'));
    };

    const detail = h('main', { class: 'detail-col' });
    mount(root, h('div', { class: 'split' }, listCol, detail));
    rerenderList();
    if (!sel) { mount(detail, h('div', { class: 'empty', style: { flex: 1 } }, h('div', { class: 'headline' }, 'Nothing selected'), 'Pick a machine on the left, or create a new one.')); return; }
    renderDetail(detail, sel, list);
}

function renderDetail(detail, m) {
    const [bg, fg] = tileColors(m.name);
    const pa = primaryAction(m.state);
    const live = m.state === 'running' || m.state === 'paused';
    const hero = h('div', { class: 'hero rise', id: 'machine-hero' },
        h('div', { class: 'tile big', style: { background: bg, color: fg } }, monogram(m.name)),
        h('div', { class: 'grow', style: { flex: '1 1 240px', display: 'flex', flexDirection: 'column', gap: '6px' } },
            h('span', { class: 'kicker' }, (m.groups && m.groups[0] && m.groups[0] !== '/' ? m.groups[0].replace(/^\//, '') : 'Ungrouped')),
            h('h2', { class: 'headline', style: { fontSize: '34px' } }, m.name),
            h('span', { class: 'muted' }, m.osTypeId + ' · ' + m.cpus + ' vCPU · ' + megabytes(m.memoryMB) + ' · ' + t(stateLabel(m.state)) + (m.lastStateChangeMs ? ' ' + relativeTime(m.lastStateChangeMs) : ''))),
        h('div', { class: 'row', style: { position: 'relative', zIndex: 1 } },
            pa.id ? h('button', { class: 'btn filled big st', id: 'primary-action', onclick: () => machineAction(m, pa.id) }, icon(ICONS.play, 18), t(pa.label)) : null,
            live ? h('button', { class: 'btn tonal big st', onclick: () => machineAction(m, m.state === 'paused' ? 'resume' : 'pause') }, icon(m.state === 'paused' ? ICONS.play : ICONS.pause, 18), m.state === 'paused' ? 'Resume' : 'Pause') : null,
            live ? h('button', { class: 'btn outlined big st', onclick: () => machineAction(m, 'acpiShutdown') }, icon(ICONS.power, 18), 'Shut down') : null,
            live ? h('button', { class: 'btn big st tone-error', onclick: () => machineAction(m, 'powerOff') }, 'Power off') : null,
            h('button', { class: 'btn tonal big st', onclick: () => store.set({ route: 'settings' }) }, t('Settings')),
            h('button', { class: 'icon-btn st', 'aria-label': 'Take snapshot', title: 'Take snapshot', onclick: () => takeSnapshot(m) }, icon(ICONS.camera)),
            h('button', { class: 'icon-btn st', 'aria-label': 'Export details', title: 'Export details', onclick: () => exportDetails(m) }, icon(ICONS.export)),
            h('button', { class: 'icon-btn st tone-error', 'aria-label': 'Delete machine', title: 'Delete machine', disabled: live ? true : undefined, onclick: () => deleteMachine(m) }, icon(ICONS.trash))));

    const panes = [['details', 'Details'], ['snapshots', 'Snapshots'], ['logs', 'Logs'], ['resources', 'Resource use']];
    const body = h('div', { id: 'machine-pane' });
    const bar = h('div', { class: 'tabbar', role: 'tablist', 'aria-label': 'Machine tools' }, panes.map(([id, label]) => h('button', { role: 'tab', class: 'st', 'aria-selected': String(pane === id), onclick: () => { pane = id; persist.save('machinePane', id); bar.querySelectorAll('button').forEach((b, i) => b.setAttribute('aria-selected', String(panes[i][0] === id))); drawPane(body, m); } }, label)));
    mount(detail, hero, bar, body);
    drawPane(body, m);
}

async function exportDetails(m) {
    const d = await api('machine.details', { id: m.id }).catch(() => null);
    if (!d) return;
    const rows = [['general', 'name', m.name], ['general', 'osTypeId', m.osTypeId], ['general', 'state', m.state], ['system', 'memoryMB', m.memoryMB], ['system', 'cpus', m.cpus], ['system', 'firmware', d.firmware], ['display', 'vramMB', d.vramMB]]
        .concat(d.storage.map((s, i) => ['storage', 'attachment' + i, s.controller + ' ' + s.port + ':' + s.device + ' ' + s.type + (s.medium ? ' ' + s.medium : '')]))
        .concat(d.network.map((n) => ['network', 'adapter' + (n.slot + 1), n.attachment]))
        .concat(d.sharedFolders.map((f) => ['sharedFolders', f.name, f.hostPath]))
        .map(([section, key, value]) => ({ section, key, value: String(value) }));
    openExport({ source: 'Details of ' + m.name, rows, fileStem: m.name.replace(/[^\w.-]+/g, '-') });
}

async function drawPane(body, m) {
    if (paneCache.id !== m.id) paneCache = { id: m.id, details: null, snaps: null, log: null };
    mount(body, h('div', { class: 'muted fade', style: { padding: '20px' } }, 'Loading…'));
    if (pane === 'details') {
        const d = paneCache.details || (paneCache.details = await api('machine.details', { id: m.id }, { quiet: true }).catch((e) => ({ error: e.message })));
        if (d.error) return mount(body, h('div', { class: 'banner error' }, d.error));
        const cards = [
            ['General', m.osTypeId, d.firmware + ' firmware', ICONS.info],
            ['System', m.cpus + ' vCPU, ' + megabytes(m.memoryMB), 'Settings file on disk', ICONS.chip],
            ['Display', d.vramMB + ' MB video memory', 'Graphics adapter', ICONS.display],
            ['Storage', d.storage.filter((s) => s.type === 'disk').map((s) => s.medium + (s.logicalSize ? ' (' + bytes(s.logicalSize) + ')' : '')).join(', ') || 'No disk', d.storage.length + ' attachments', ICONS.disk],
            ['Network', d.network.map((n) => n.attachment).join(', ') || 'Not attached', d.network.length + ' adapters enabled', ICONS.network],
            ['Shared folders', d.sharedFolders.length + ' folders', d.sharedFolders.map((f) => f.name).join(', ') || 'None', ICONS.folder]];
        mount(body, h('div', { class: 'grid g3' }, cards.map(([title, value, sub, ic], i) => h('div', { class: 'card lift rise', style: stagger(i, 60) },
            h('div', { class: 'row tone-primary', style: { fontWeight: 700, fontSize: '13px' } }, icon(ic, 18), title),
            h('div', { style: { fontSize: '15px', fontWeight: 600, margin: '8px 0 4px' } }, value), h('div', { class: 'muted', style: { fontSize: '12px' } }, sub)))),
            h('div', { class: 'row', style: { marginTop: '12px' } }, h('span', { class: 'mono muted', style: { fontSize: '12px' } }, d.settingsFile),
                h('button', { class: 'btn st', style: { height: '34px' }, onclick: () => window.mvb.openExternalEditor(d.settingsFile).catch((e) => notify({ kind: 'error', title: 'Could not open the editor', body: e.message })) }, 'Open in editor')));
    } else if (pane === 'snapshots') {
        const sn = paneCache.snaps || (paneCache.snaps = await api('snapshots.list', { id: m.id }, { quiet: true }).catch((e) => ({ error: e.message })));
        if (sn.error) return mount(body, h('div', { class: 'banner error' }, sn.error));
        const flat = [];
        const walk = (n, depth) => { if (!n) return; flat.push({ ...n, depth }); n.children.forEach((c) => walk(c, depth + 1)); };
        walk(sn.root, 0);
        mount(body, flat.length ? h('ol', { class: 'timeline' }, flat.concat([{ id: 'now', name: 'Current state', timeMs: Date.now(), depth: flat.length ? flat[flat.length - 1].depth : 0, now: true }]).map((s, i) =>
            h('li', { class: 'slide', style: { ...stagger(i, 80), paddingLeft: (s.depth * 18) + 'px' } },
                h('span', { 'aria-hidden': 'true', style: { width: '24px', display: 'flex', flexDirection: 'column', alignItems: 'center' } }, h('span', { style: { width: '16px', height: '16px', borderRadius: '8px', marginTop: '18px', background: s.now ? 'var(--primary)' : (s.id === sn.currentId ? 'var(--tertiary)' : 'var(--outline)') } }), h('span', { style: { flex: 1, width: '2px', background: 'var(--outline-variant)' } })),
                h('span', { class: 'card grow', style: { margin: '4px 0', display: 'flex', justifyContent: 'space-between', gap: '12px' } }, h('span', null, h('strong', null, s.name), h('div', { class: 'muted', style: { fontSize: '12px' } }, s.now ? 'Live' : new Date(s.timeMs).toLocaleString())), s.id === sn.currentId ? h('span', { class: 'tone-primary', style: { fontSize: '12px', fontWeight: 700 } }, 'Current base') : null))))
            : h('div', { class: 'empty' }, 'No snapshots yet.', h('button', { class: 'btn filled st', onclick: () => takeSnapshot(m) }, icon(ICONS.camera, 18), 'Take the first snapshot')));
    } else if (pane === 'logs') {
        const lg = paneCache.log || (paneCache.log = await api('logs.read', { id: m.id, index: 0 }, { quiet: true }).catch((e) => ({ error: e.message })));
        if (lg.error) return mount(body, h('div', { class: 'empty' }, 'No log for this machine yet. It appears after the first start.'));
        const rows = parseLog(lg.text).slice(-200);
        mount(body, h('div', { class: 'logbox fade', style: { maxHeight: '460px' } }, rows.map((r, i) => h('div', { class: 'logline', style: { animation: 'mvFade 300ms both', animationDelay: Math.min(i, 40) * 15 + 'ms' } }, h('span', { class: 'muted' }, r.time), h('span', { class: { error: 'tone-error', warning: 'tone-warning', notice: 'tone-success' }[r.severity] || '' }, r.text)))),
            h('div', { class: 'row', style: { marginTop: '10px' } }, h('button', { class: 'btn tonal st', onclick: () => store.set({ route: 'logs' }) }, 'Open in log viewer')));
    } else {
        mount(body, h('div', { class: 'grid g2', id: 'resource-meters' }, ['CPU', 'Host memory'].map((title, k) => h('div', { class: 'card', style: { borderRadius: '24px' } },
            h('div', { class: 'spread', style: { alignItems: 'baseline' } }, h('strong', null, title), h('span', { class: 'meter-now', style: { fontSize: '28px', fontWeight: 750, color: k ? 'var(--tertiary)' : 'var(--primary)' } }, '…')),
            h('div', { class: 'bars', 'aria-hidden': 'true', 'data-kind': title }, Array.from({ length: 28 }, (_, i) => h('span', { style: { height: '4%', background: k ? 'var(--tertiary)' : 'var(--primary)', opacity: (0.45 + i / 28 * 0.55).toFixed(2), animationDelay: i * 22 + 'ms' } }))),
            h('div', { class: 'muted', style: { fontSize: '12px', marginTop: '8px' } }, k ? 'Host memory in use; this machine has ' + megabytes(m.memoryMB) + ' assigned' : 'Machine CPU, sampled every second while this pane is open')))));
        pumpResources(body, m);
    }
}

const series = { cpu: [], mem: [] };
let pumpGen = 0;
async function pumpResources(body, m) {
    const gen = ++pumpGen;
    while (gen === pumpGen && pane === 'resources' && document.contains(body) && store.get().selectedId === m.id) {
        const sample = await api('metrics.sample', {}, { quiet: true }).catch(() => []);
        if (gen !== pumpGen) return;
        const cpu = (sample.find((x) => x.object === m.id && x.metric.startsWith('CPU')) || {}).value;
        series.cpu = [...series.cpu, cpu || 0].slice(-28);
        const host = sample.find((x) => x.object === 'host' && x.metric.startsWith('RAM'));
        const hostMB = (store.get().host || {}).memoryMB || 0;
        series.mem = [...series.mem, host && hostMB ? Math.min(100, host.value / 1024 / hostMB * 100) : 0].slice(-28);
        body.querySelectorAll('.bars').forEach((bars, k) => {
            const arr = k ? series.mem : series.cpu;
            [...bars.children].forEach((el, i) => { const v = arr[arr.length - 28 + i]; el.style.height = Math.max(4, v || 0) + '%'; });
            bars.parentElement.querySelector('.meter-now').textContent = cpu === undefined && !k ? 'Not running' : Math.round(arr[arr.length - 1] || 0) + '%';
        });
        await new Promise((r) => setTimeout(r, 1000));
    }
}
