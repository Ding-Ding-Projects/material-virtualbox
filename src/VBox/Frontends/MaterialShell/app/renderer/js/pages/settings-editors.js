/*
 * Editors for the Display, Network, Shared folders and Storage settings
 * pages.  Each keeps a draft of the machine's current values, shows Apply
 * only when the draft differs, and sends one machine.configure (or
 * storage.mount) call; history records the reverse change for restore.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { api, notify, recordHistory } from '../services.js';
import { detachDevice } from './settings-more.js';

let cache = { id: null, details: null, draft: null, nets: null, newFolder: { name: '', hostPath: '', writable: true, autoMount: true }, iso: {} };

const clone = (x) => JSON.parse(JSON.stringify(x));

async function load(m) {
    if (cache.id === m.id && cache.details) return;
    const [details, nets] = await Promise.all([api('machine.details', { id: m.id }), api('networks.list', {}, { quiet: true }).catch(() => ({ natNetworks: [], hostInterfaces: [] }))]);
    cache = { ...cache, id: m.id, details, nets, draft: { display: { vramMB: details.vramMB, controller: details.graphicsController, monitors: details.monitors, accel3d: details.accel3d }, adapters: clone(details.adapters || []) } };
}

export function invalidateEditors() {
    cache = { ...cache, id: null, details: null };
}

const sw = (on, label, onClick, disabled) => h('button', { class: 'switch st', role: 'switch', 'aria-checked': String(!!on), 'aria-label': label, disabled: disabled ? true : undefined, onclick: onClick }, h('span', { class: 'knob' }));
const tile = (span, ...kids) => h('section', { class: 'glass card lift rise', style: { gridColumn: 'span ' + span, borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '12px' } }, kids);

function diffDisplay(a, b) {
    const out = {};
    for (const k of ['vramMB', 'controller', 'monitors', 'accel3d']) if (a[k] !== b[k]) out[k] = b[k];
    return out;
}

function diffAdapters(orig, draft) {
    return draft.filter((d, i) => JSON.stringify(d) !== JSON.stringify(orig[i])).map((d) => {
        const o = orig.find((x) => x.slot === d.slot) || {};
        const out = { slot: d.slot };
        for (const k of ['enabled', 'attachment', 'bridgedInterface', 'hostOnlyInterface', 'internalNetwork', 'natNetwork', 'cableConnected']) if (d[k] !== o[k]) out[k] = d[k];
        return out;
    });
}

async function applyConfigure(m, payload, history, rerender) {
    try {
        const r = await api('machine.configure', { id: m.id, ...payload });
        recordHistory({ ...history, scope: m.name, undo: history.undo ? { method: 'machine.configure', params: { id: m.id, ...history.undo } } : null });
        notify({ kind: 'success', title: 'Settings saved', body: m.name + ' · ' + r.changes + ' change' + (r.changes === 1 ? '' : 's') });
        invalidateEditors();
        rerender();
    } catch { /* notified */ }
}

function actions(dirty, live, onDiscard, onApply) {
    return h('div', { class: 'row', style: { gridColumn: 'span 6', justifyContent: 'flex-end' } },
        live ? h('span', { class: 'muted grow', style: { fontSize: '12.5px' } }, 'Power the machine off to change this page.') : null,
        h('button', { class: 'btn outlined st', disabled: dirty ? undefined : true, onclick: onDiscard }, 'Discard'),
        h('button', { class: 'btn filled st', disabled: dirty && !live ? undefined : true, onclick: onApply }, 'Apply'));
}

/** Renders the editor tiles for section into grid; returns false when the section is not one of these. */
export function renderEditor(section, m, grid, rerender) {
    if (!['display', 'network', 'folders', 'storage'].includes(section)) return false;
    if (cache.id !== m.id || !cache.details) {
        mount(grid, h('div', { class: 'muted', style: { gridColumn: 'span 6' } }, 'Reading the current settings…'));
        load(m).then(() => renderEditor(section, m, grid, rerender)).catch((e) => mount(grid, h('div', { class: 'banner error', style: { gridColumn: 'span 6' } }, e.message)));
        return true;
    }
    const d = cache.details, dr = cache.draft;
    const live = m.state === 'running' || m.state === 'paused' || m.state === 'saved';
    const redraw = () => renderEditor(section, m, grid, rerender);
    const discard = () => { invalidateEditors(); redraw(); };

    if (section === 'display') {
        const orig = { vramMB: d.vramMB, controller: d.graphicsController, monitors: d.monitors, accel3d: d.accel3d };
        const change = diffDisplay(orig, dr.display);
        mount(grid,
            tile(4, h('div', { class: 'spread', style: { alignItems: 'baseline' } }, h('h3', { class: 'title' }, 'Video memory'), h('span', { style: { fontSize: '40px', fontWeight: 850, color: 'var(--primary)' } }, dr.display.vramMB + ' MB')),
                h('input', { type: 'range', min: 16, max: 256, step: 16, value: String(dr.display.vramMB), disabled: live ? true : undefined, 'aria-label': 'Video memory in megabytes', style: { width: '100%', accentColor: 'var(--primary)' }, oninput: (e) => { dr.display.vramMB = Number(e.target.value); redraw(); grid.querySelector('input[type=range]').focus(); } })),
            tile(2, h('h3', { class: 'title' }, 'Monitors'), h('span', { style: { fontSize: '64px', lineHeight: 1, fontWeight: 900, color: 'var(--tertiary)' } }, String(dr.display.monitors)),
                h('div', { class: 'row', style: { flexWrap: 'nowrap' } },
                    h('button', { class: 'btn outlined st grow', 'aria-label': 'Fewer monitors', disabled: live || dr.display.monitors <= 1 ? true : undefined, onclick: () => { dr.display.monitors--; redraw(); } }, '−'),
                    h('button', { class: 'btn tonal st grow', 'aria-label': 'More monitors', disabled: live || dr.display.monitors >= 8 ? true : undefined, onclick: () => { dr.display.monitors++; redraw(); } }, '+'))),
            tile(4, h('h3', { class: 'title' }, 'Graphics controller'),
                h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Graphics controller' }, [['VMSVGA', 'VMSVGA · Linux'], ['VBoxSVGA', 'VBoxSVGA · Windows'], ['VBoxVGA', 'VBoxVGA · legacy']].map(([k, label]) =>
                    h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(dr.display.controller === k), disabled: live ? true : undefined, onclick: () => { dr.display.controller = k; redraw(); } }, label)))),
            tile(2, h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('span', { class: 'grow' }, h('strong', null, '3D acceleration'), h('div', { class: 'muted', style: { fontSize: '12.5px' } }, 'Needs Guest Additions')),
                sw(dr.display.accel3d, '3D acceleration', () => { dr.display.accel3d = !dr.display.accel3d; redraw(); }, live))),
            actions(Object.keys(change).length > 0, live, discard, () => applyConfigure(m, { display: change },
                { title: 'Display changed', category: 'Settings', diff: Object.keys(change).flatMap((k) => [['-', k + ': ' + orig[k]], ['+', k + ': ' + change[k]]]), undo: { display: Object.fromEntries(Object.keys(change).map((k) => [k, orig[k]])) } }, rerender)));
        return true;
    }

    if (section === 'network') {
        const bridged = cache.nets.hostInterfaces.filter((x) => x.kind === 'bridged').map((x) => x.name);
        const hostOnly = cache.nets.hostInterfaces.filter((x) => x.kind === 'hostOnly').map((x) => x.name);
        const natNets = cache.nets.natNetworks.map((x) => x.name);
        const change = diffAdapters(d.adapters || [], dr.adapters);
        const nameField = (a) => {
            const pick = (key, options) => h('select', { 'aria-label': 'Attached to', disabled: live ? true : undefined, onchange: (e) => { a[key] = e.target.value; redraw(); }, style: { height: '40px', borderRadius: '12px', background: 'var(--surface-container-highest)', border: 0, padding: '0 10px' } },
                (options.includes(a[key]) || !a[key] ? options : [a[key], ...options]).map((o) => h('option', { value: o, selected: o === a[key] ? true : undefined }, o)));
            if (a.attachment === 'bridged') return pick('bridgedInterface', bridged.length ? bridged : ['(no host adapter found)']);
            if (a.attachment === 'hostOnly') return pick('hostOnlyInterface', hostOnly.length ? hostOnly : ['(no host-only adapter)']);
            if (a.attachment === 'natNetwork') return pick('natNetwork', natNets.length ? natNets : ['(no NAT network)']);
            if (a.attachment === 'internal') return h('label', { class: 'field', style: { height: '40px' } }, h('span', { class: 'sr-only' }, 'Internal network name'), h('input', { value: a.internalNetwork || 'intnet', disabled: live ? true : undefined, onchange: (e) => { a.internalNetwork = e.target.value; redraw(); } }));
            return null;
        };
        mount(grid, dr.adapters.map((a, i) => h('section', { class: 'glass card rise', style: { gridColumn: 'span 3', borderRadius: '26px', display: 'flex', flexDirection: 'column', gap: '12px', opacity: a.enabled ? 1 : .7, ...stagger(i, 60) } },
            h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('strong', { class: 'grow', style: { fontSize: '17px' } }, 'Adapter ' + (a.slot + 1)), h('span', { class: 'mono muted', style: { fontSize: '11.5px' } }, a.mac), sw(a.enabled, 'Enable adapter ' + (a.slot + 1), () => { a.enabled = !a.enabled; if (a.enabled && a.attachment === 'none') a.attachment = 'nat'; redraw(); }, live)),
            a.enabled ? h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Attached to', style: { flexWrap: 'wrap' } }, [['nat', 'NAT'], ['natNetwork', 'NAT network'], ['bridged', 'Bridged'], ['hostOnly', 'Host-only'], ['internal', 'Internal']].map(([k, label]) =>
                h('button', { class: 'st', role: 'radio', 'aria-checked': String(a.attachment === k), disabled: live ? true : undefined, style: { padding: '0 12px', fontSize: '13px' }, onclick: () => { a.attachment = k; redraw(); } }, label))) : h('span', { class: 'muted' }, 'Off'),
            a.enabled ? nameField(a) : null,
            a.enabled ? h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('span', { class: 'grow' }, 'Cable connected'), sw(a.cableConnected, 'Cable connected', () => { a.cableConnected = !a.cableConnected; redraw(); }, live)) : null)),
            actions(change.length > 0, live, discard, () => applyConfigure(m, { adapters: change },
                { title: 'Network changed', category: 'Settings', diff: change.flatMap((c) => Object.entries(c).filter(([k]) => k !== 'slot').map(([k, v]) => ['+', 'Adapter ' + (c.slot + 1) + ' ' + k + ': ' + v])),
                    undo: { adapters: change.map((c) => { const o = (d.adapters || []).find((x) => x.slot === c.slot); return Object.fromEntries(Object.keys(c).map((k) => [k, o[k]])); }) } }, rerender)));
        return true;
    }

    if (section === 'folders') {
        const nf = cache.newFolder;
        const isValid = () => /^[\w.-]{1,64}$/.test(nf.name.trim()) && !!nf.hostPath.trim() && !d.sharedFolders.some((f) => f.name === nf.name.trim());
        const valid = isValid();
        /* Typing only refreshes the Add button, so focus never jumps while filling the form. */
        const refreshAdd = () => { const b = grid.querySelector('#add-shared-folder'); if (b) b.disabled = !(isValid() && !live); };
        mount(grid,
            h('div', { style: { gridColumn: 'span 6', display: 'flex', flexDirection: 'column', gap: '8px' } }, d.sharedFolders.length ? d.sharedFolders.map((f, i) => h('div', { class: 'card rise row', style: { flexWrap: 'nowrap', borderRadius: '20px', ...stagger(i, 50) } },
                icon(ICONS.folder), h('span', { class: 'grow' }, h('strong', null, f.name), h('div', { class: 'mono muted', style: { fontSize: '12px' } }, f.hostPath)),
                h('span', { class: 'chip', style: { height: '28px' } }, f.writable ? 'Read and write' : 'Read only'), f.autoMount ? h('span', { class: 'chip', style: { height: '28px' } }, 'Auto-mount') : null,
                h('button', { class: 'btn st tone-error', disabled: live ? true : undefined, onclick: () => applyConfigure(m, { sharedFoldersRemove: [{ name: f.name }] },
                    { title: 'Shared folder removed: ' + f.name, category: 'Settings', diff: [['-', f.name + ': ' + f.hostPath]], undo: { sharedFoldersAdd: [f] } }, rerender) }, 'Remove')))
                : h('div', { class: 'empty' }, 'No shared folders. Add one below; the files stay on this computer.')),
            tile(6, h('h3', { class: 'title' }, 'Add a shared folder'),
                h('div', { class: 'grid g2' },
                    h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, 'Name in the guest'), h('input', { value: nf.name, placeholder: 'Projects', oninput: (e) => { nf.name = e.target.value; refreshAdd(); } })),
                    h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, 'Folder on this computer'), h('input', { value: nf.hostPath, placeholder: 'D:\\Projects', oninput: (e) => { nf.hostPath = e.target.value; refreshAdd(); } }))),
                h('div', { class: 'row' },
                    h('span', null, 'Writable'), sw(nf.writable, 'Writable', () => { nf.writable = !nf.writable; redraw(); }),
                    h('span', null, 'Auto-mount'), sw(nf.autoMount, 'Auto-mount', () => { nf.autoMount = !nf.autoMount; redraw(); }),
                    h('span', { class: 'grow' }),
                    h('button', { class: 'btn filled st', id: 'add-shared-folder', disabled: valid && !live ? undefined : true, onclick: () => { const add = { ...nf, name: nf.name.trim(), hostPath: nf.hostPath.trim() }; cache.newFolder = { name: '', hostPath: '', writable: true, autoMount: true };
                        applyConfigure(m, { sharedFoldersAdd: [add] }, { title: 'Shared folder added: ' + add.name, category: 'Settings', diff: [['+', add.name + ': ' + add.hostPath]], undo: { sharedFoldersRemove: [{ name: add.name }] } }, rerender); } }, 'Add folder')),
                live ? h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'Power the machine off to change shared folders here.') : null));
        return true;
    }

    /* storage */
    mount(grid, d.storage.map((s, i) => {
        const optical = s.type === 'optical';
        const key = s.controller + ':' + s.port + ':' + s.device;
        return h('section', { class: 'glass card rise', style: { gridColumn: 'span 3', borderRadius: '26px', display: 'flex', flexDirection: 'column', gap: '10px', ...stagger(i, 60) } },
            h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, icon(optical ? ICONS.media : ICONS.disk), h('strong', { class: 'grow' }, (optical ? 'Optical drive' : 'Hard disk') + ' · ' + s.controller + ' ' + s.port + ':' + s.device)),
            h('div', { class: 'mono muted', style: { fontSize: '12px', wordBreak: 'break-all' } }, s.location || (optical ? 'Empty' : 'No medium')),
            optical ? h('div', { class: 'row', style: { flexWrap: 'nowrap' } },
                h('label', { class: 'field grow', style: { height: '42px' } }, h('span', { class: 'sr-only' }, 'Image file'), h('input', { placeholder: 'E:\\ISOs\\installer.iso', value: cache.iso[key] || '', oninput: (e) => { cache.iso[key] = e.target.value; } })),
                h('button', { class: 'btn tonal st', onclick: async () => { const loc = (cache.iso[key] || '').trim(); if (!loc) return; try { await api('storage.mount', { id: m.id, controller: s.controller, port: s.port, device: s.device, location: loc }); recordHistory({ title: 'Image inserted: ' + loc.split(/[\\/]/).pop(), scope: m.name, category: 'Settings', diff: [['-', key + ' ' + (s.location || 'empty')], ['+', key + ' ' + loc]], undo: { method: 'storage.mount', params: { id: m.id, controller: s.controller, port: s.port, device: s.device, location: s.location || '' } } }); notify({ kind: 'success', title: 'Image inserted', body: loc }); invalidateEditors(); rerender(); } catch { /* notified */ } } }, 'Insert'),
                s.location ? h('button', { class: 'btn outlined st', onclick: async () => { try { await api('storage.mount', { id: m.id, controller: s.controller, port: s.port, device: s.device, location: '' }); recordHistory({ title: 'Image ejected', scope: m.name, category: 'Settings', diff: [['-', key + ' ' + s.location]], undo: { method: 'storage.mount', params: { id: m.id, controller: s.controller, port: s.port, device: s.device, location: s.location } } }); notify({ kind: 'success', title: 'Image ejected', body: m.name }); invalidateEditors(); rerender(); } catch { /* notified */ } } }, 'Eject') : null)
                : h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'Resize, compact or copy disks from Media.'),
            h('button', { class: 'btn st tone-error', style: { alignSelf: 'flex-start' }, disabled: live ? true : undefined, onclick: () => detachDevice(m, s, () => { invalidateEditors(); rerender(); }) }, 'Detach'));
    }), h('div', { class: 'muted', style: { gridColumn: 'span 6', fontSize: '12.5px' } }, 'Inserting and ejecting works while the machine runs.'));
    return true;
}
