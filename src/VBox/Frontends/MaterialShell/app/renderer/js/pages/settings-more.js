/*
 * The remaining settings pages: General extras (description, OS type),
 * System extras (firmware, TPM, nested virtualization, boot order), Audio,
 * Serial ports, USB, Interface (clipboard and drag and drop) and storage
 * controllers.  Every change goes through machine.settingsUpdate (or
 * machine.runtime for the modes that work while running) and history keeps
 * the reverse change so it can be restored.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { api, notify, recordHistory } from '../services.js';
import { formDialog, confirmDestructive } from '../features/overlays.js';

export const MORE_SECTIONS = ['general', 'system', 'audio', 'serial', 'usb', 'interface', 'storage'];

let cache = { id: null, s: null, draft: null, media: null };
const clone = (x) => JSON.parse(JSON.stringify(x));

export function invalidateMore() {
    cache = { id: null, s: null, draft: null, media: null };
}

async function load(m) {
    const s = await api('machine.settings', { id: m.id });
    cache = { id: m.id, s, draft: clone(s), media: null };
}

const sw = (on, label, onClick, disabled) => h('button', { class: 'switch st', role: 'switch', 'aria-checked': String(!!on), 'aria-label': label, disabled: disabled ? true : undefined, onclick: onClick }, h('span', { class: 'knob' }));
const tile = (span, i, ...kids) => h('section', { class: 'glass card lift rise', style: { gridColumn: 'span ' + span, borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '12px', ...stagger(i, 60) } }, kids);
const seg = (label, value, options, onPick, disabled) => h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': label, style: { flexWrap: 'wrap' } }, options.map(([k, l]) =>
    h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(value === k), disabled: disabled ? true : undefined, style: { padding: '0 12px' }, onclick: () => onPick(k) }, l)));
const toggleRow = (label, sub, on, onClick, disabled) => h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('span', { class: 'grow' }, h('strong', null, label), sub ? h('div', { class: 'muted', style: { fontSize: '12.5px' } }, sub) : null), sw(on, label, onClick, disabled));

async function update(m, payload, history, rerender, method = 'machine.settingsUpdate') {
    try {
        const r = await api(method, { id: m.id, ...payload });
        recordHistory({ ...history, scope: m.name, category: 'Settings', undo: history.undo ? { method, params: { id: m.id, ...history.undo } } : null });
        notify({ kind: 'success', title: 'Settings saved', body: m.name + (r && r.changes !== undefined ? ' · ' + r.changes + ' change' + (r.changes === 1 ? '' : 's') : '') });
        invalidateMore();
        rerender();
    } catch { /* notified */ }
}

function actions(dirty, live, onDiscard, onApply) {
    return h('div', { class: 'row', style: { gridColumn: 'span 6', justifyContent: 'flex-end' } },
        live ? h('span', { class: 'muted grow', style: { fontSize: '12.5px' } }, 'Power the machine off to change this page.') : null,
        h('button', { class: 'btn outlined st', disabled: dirty ? undefined : true, onclick: onDiscard }, 'Discard'),
        h('button', { class: 'btn filled st', disabled: dirty && !live ? undefined : true, onclick: onApply }, 'Apply'));
}

const BOOT = [['disk', 'Hard disk'], ['optical', 'Optical'], ['network', 'Network'], ['floppy', 'Floppy'], ['none', 'Nothing']];
const OS_TYPES = [['Windows11_64', 'Windows 11'], ['Windows10_64', 'Windows 10'], ['Windows2022_64', 'Windows Server 2022'], ['Ubuntu_64', 'Ubuntu'], ['Fedora_64', 'Fedora'], ['Debian_64', 'Debian'],
    ['ArchLinux_64', 'Arch Linux'], ['RedHat_64', 'Red Hat'], ['FreeBSD_64', 'FreeBSD'], ['OpenBSD_64', 'OpenBSD'], ['Solaris11_64', 'Solaris'], ['MacOS_64', 'macOS'], ['Other_64', 'Other (64-bit)'], ['Other', 'Other']];
const MODES = [['disabled', 'Off'], ['hostToGuest', 'Host to guest'], ['guestToHost', 'Guest to host'], ['bidirectional', 'Both ways']];

/**
 * Renders the extra tiles for section into grid.  Returns false when the
 * section has no extra tiles.  part 'top' renders above the inline tiles of
 * settings.js (only used by sections that live entirely here).
 */
export function renderMore(section, m, grid, rerender) {
    if (!MORE_SECTIONS.includes(section)) return false;
    if (cache.id !== m.id || !cache.s) {
        mount(grid, h('div', { class: 'muted', style: { gridColumn: 'span 6' } }, 'Reading the current settings…'));
        load(m).then(() => renderMore(section, m, grid, rerender)).catch((e) => mount(grid, h('div', { class: 'banner error', style: { gridColumn: 'span 6' } }, e.message)));
        return true;
    }
    const s = cache.s, d = cache.draft;
    const live = m.state === 'running' || m.state === 'paused' || m.state === 'saved';
    const redraw = () => renderMore(section, m, grid, rerender);
    const discard = () => { cache.draft = clone(cache.s); redraw(); };
    const after = () => { rerender(); };

    if (section === 'general') {
        const dirty = d.description !== s.description || d.osTypeId !== s.osTypeId;
        const opts = OS_TYPES.some(([k]) => k === d.osTypeId) ? OS_TYPES : [[d.osTypeId, d.osTypeId]].concat(OS_TYPES);
        mount(grid,
            tile(3, 0, h('h3', { class: 'title' }, 'Operating system type'),
                h('select', { 'aria-label': 'Operating system type', id: 'set-ostype', disabled: live ? true : undefined, onchange: (e) => { d.osTypeId = e.target.value; redraw(); }, style: { height: '44px', borderRadius: '14px', background: 'var(--surface-container-highest)', color: 'inherit', border: 0, padding: '0 12px' } },
                    opts.map(([k, l]) => h('option', { value: k, selected: k === d.osTypeId ? true : undefined }, l))),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'Sets defaults and guest hints; it does not change the installed system.')),
            tile(3, 1, h('h3', { class: 'title' }, 'Description'),
                h('textarea', { 'aria-label': 'Description', id: 'set-description', rows: 4, disabled: live ? true : undefined, style: { borderRadius: '14px', background: 'var(--surface-container-highest)', color: 'inherit', border: 0, padding: '10px 12px', resize: 'vertical', font: 'inherit' },
                    oninput: (e) => { d.description = e.target.value; const a = grid.querySelector('#more-apply'); if (a) a.disabled = live || (d.description === s.description && d.osTypeId === s.osTypeId); const x = grid.querySelector('#more-discard'); if (x) x.disabled = d.description === s.description && d.osTypeId === s.osTypeId; } }, d.description)),
            h('div', { class: 'row', style: { gridColumn: 'span 6', justifyContent: 'flex-end' } },
                h('button', { class: 'btn outlined st', id: 'more-discard', disabled: dirty ? undefined : true, onclick: discard }, 'Discard'),
                h('button', { class: 'btn filled st', id: 'more-apply', disabled: dirty && !live ? undefined : true, onclick: () => {
                    const p = {}, u = {}, diff = [];
                    if (d.description !== s.description) { p.description = d.description; u.description = s.description; diff.push(['-', 'Description: ' + (s.description || '(empty)')], ['+', 'Description: ' + (d.description || '(empty)')]); }
                    if (d.osTypeId !== s.osTypeId) { p.osTypeId = d.osTypeId; u.osTypeId = s.osTypeId; diff.push(['-', 'OS type: ' + s.osTypeId], ['+', 'OS type: ' + d.osTypeId]); }
                    update(m, p, { title: 'General settings changed', diff, undo: u }, after);
                } }, 'Apply')));
        return true;
    }

    if (section === 'system') {
        const sy = d.system, o = s.system;
        const keys = ['firmware', 'tpm', 'nestedVirt'];
        const bootDirty = JSON.stringify(sy.bootOrder) !== JSON.stringify(o.bootOrder);
        const dirty = bootDirty || keys.some((k) => sy[k] !== o[k]);
        const move = (i, dir) => { const j = i + dir; if (j < 0 || j > 3) return; [sy.bootOrder[i], sy.bootOrder[j]] = [sy.bootOrder[j], sy.bootOrder[i]]; redraw(); grid.querySelectorAll('[data-boot]')[j]?.focus(); };
        mount(grid,
            tile(3, 0, h('h3', { class: 'title' }, 'Firmware'), seg('Firmware', sy.firmware, [['EFI', 'UEFI'], ['BIOS', 'Legacy BIOS']], (k) => { sy.firmware = k; redraw(); }, live),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'Windows 11 needs UEFI and TPM 2.0.')),
            tile(3, 1, h('h3', { class: 'title' }, 'Trusted Platform Module'), seg('TPM', sy.tpm, [['none', 'None'], ['1.2', 'TPM 1.2'], ['2.0', 'TPM 2.0']], (k) => { sy.tpm = k; redraw(); }, live)),
            tile(3, 2, toggleRow('Nested virtualization', 'Lets the guest run its own hypervisor (VT-x/AMD-V passed through)', sy.nestedVirt, () => { sy.nestedVirt = !sy.nestedVirt; redraw(); }, live)),
            tile(3, 3, h('h3', { class: 'title' }, 'Boot order'),
                h('ol', { style: { listStyle: 'none', margin: 0, padding: 0, display: 'flex', flexDirection: 'column', gap: '6px' } }, sy.bootOrder.map((b, i) => h('li', { class: 'row slide', style: { flexWrap: 'nowrap', padding: '6px 6px 6px 14px', borderRadius: '16px', background: 'var(--surface-container-high)', ...stagger(i, 50) } },
                    h('span', { class: 'mono muted' }, String(i + 1)),
                    h('select', { 'data-boot': String(i), 'aria-label': 'Boot device ' + (i + 1), disabled: live ? true : undefined, class: 'grow', onchange: (e) => { sy.bootOrder[i] = e.target.value; redraw(); }, style: { height: '36px', background: 'transparent', color: 'inherit', border: 0 } },
                        BOOT.map(([k, l]) => h('option', { value: k, selected: k === b ? true : undefined }, l))),
                    h('button', { class: 'icon-btn st', 'aria-label': 'Move boot device ' + (i + 1) + ' up', disabled: live || i === 0 ? true : undefined, onclick: () => move(i, -1) }, '↑'),
                    h('button', { class: 'icon-btn st', 'aria-label': 'Move boot device ' + (i + 1) + ' down', disabled: live || i === 3 ? true : undefined, onclick: () => move(i, 1) }, '↓'))))),
            actions(dirty, live, discard, () => {
                const p = {}, u = {}, diff = [];
                for (const k of keys) if (sy[k] !== o[k]) { p[k] = sy[k]; u[k] = o[k]; diff.push(['-', k + ': ' + o[k]], ['+', k + ': ' + sy[k]]); }
                if (bootDirty) { p.bootOrder = sy.bootOrder; u.bootOrder = o.bootOrder; diff.push(['-', 'Boot order: ' + o.bootOrder.join(', ')], ['+', 'Boot order: ' + sy.bootOrder.join(', ')]); }
                update(m, p, { title: 'System settings changed', diff, undo: u }, after);
            }));
        return true;
    }

    if (section === 'audio') {
        const a = d.audio, o = s.audio;
        const dirty = JSON.stringify(a) !== JSON.stringify(o);
        mount(grid,
            tile(6, 0, toggleRow('Audio', 'Give the guest a sound card', a.enabled, () => { a.enabled = !a.enabled; redraw(); }, live)),
            a.enabled ? tile(4, 1, h('h3', { class: 'title' }, 'Controller'), seg('Audio controller', a.controller, [['HDA', 'Intel HD Audio'], ['AC97', 'ICH AC97'], ['SB16', 'SoundBlaster 16'], ['VirtioSound', 'Virtio sound']], (k) => { a.controller = k; redraw(); }, live)) : null,
            a.enabled ? tile(2, 2, toggleRow('Output', 'Guest to speakers', a.output, () => { a.output = !a.output; redraw(); }, live), toggleRow('Input', 'Microphone to guest', a.input, () => { a.input = !a.input; redraw(); }, live)) : null,
            actions(dirty, live, discard, () => {
                const diff = Object.keys(a).filter((k) => a[k] !== o[k]).flatMap((k) => [['-', 'Audio ' + k + ': ' + o[k]], ['+', 'Audio ' + k + ': ' + a[k]]]);
                update(m, { audio: a }, { title: 'Audio changed', diff, undo: { audio: o } }, after);
            }));
        return true;
    }

    if (section === 'serial') {
        const changed = d.serial.filter((p, i) => JSON.stringify(p) !== JSON.stringify(s.serial[i]));
        const MODES_PORT = [['disconnected', 'Disconnected'], ['hostPipe', 'Host pipe'], ['hostDevice', 'Host device'], ['rawFile', 'Raw file'], ['tcp', 'TCP']];
        const pathHint = { hostPipe: '\\\\.\\pipe\\com1', hostDevice: 'COM1', rawFile: 'D:\\VMs\\serial.log', tcp: '127.0.0.1:2023' };
        mount(grid, d.serial.map((p, i) => h('section', { class: 'glass card rise', style: { gridColumn: 'span 3', borderRadius: '26px', display: 'flex', flexDirection: 'column', gap: '12px', opacity: p.enabled ? 1 : .72, ...stagger(i, 60) } },
            h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('strong', { class: 'grow', style: { fontSize: '17px' } }, 'COM' + (p.slot + 1)), h('span', { class: 'mono muted', style: { fontSize: '11.5px' } }, '0x' + Number(p.ioBase).toString(16).toUpperCase() + ' · IRQ ' + p.irq), sw(p.enabled, 'Enable COM' + (p.slot + 1), () => { p.enabled = !p.enabled; redraw(); }, live)),
            p.enabled ? [
                h('select', { 'aria-label': 'COM' + (p.slot + 1) + ' mode', disabled: live ? true : undefined, onchange: (e) => { p.mode = e.target.value; redraw(); }, style: { height: '40px', borderRadius: '12px', background: 'var(--surface-container-highest)', color: 'inherit', border: 0, padding: '0 10px' } },
                    MODES_PORT.map(([k, l]) => h('option', { value: k, selected: k === p.mode ? true : undefined }, l))),
                p.mode !== 'disconnected' ? h('label', { class: 'field', style: { height: '40px' } }, h('span', { class: 'sr-only' }, 'Path or address'), h('input', { value: p.path, placeholder: pathHint[p.mode], disabled: live ? true : undefined, onchange: (e) => { p.path = e.target.value; redraw(); } })) : null,
                p.mode === 'hostPipe' || p.mode === 'tcp' ? toggleRow('Create the pipe or socket', 'Off connects to an existing one', p.server, () => { p.server = !p.server; redraw(); }, live) : null] : h('span', { class: 'muted' }, 'Off'))),
            actions(changed.length > 0, live, discard, () => update(m, { serial: changed }, { title: 'Serial ports changed', diff: changed.map((p) => ['+', 'COM' + (p.slot + 1) + ': ' + (p.enabled ? p.mode + ' ' + p.path : 'off')]), undo: { serial: changed.map((p) => s.serial[p.slot]) } }, after)));
        return true;
    }

    if (section === 'usb') {
        const type = s.usb.controllers.some((c) => c.type === 'XHCI') ? 'XHCI' : s.usb.controllers.some((c) => c.type === 'EHCI') ? 'EHCI' : s.usb.controllers.length ? 'OHCI' : 'none';
        mount(grid,
            tile(6, 0, h('h3', { class: 'title' }, 'USB controller'),
                seg('USB controller', type, [['none', 'None'], ['OHCI', 'USB 1.1'], ['EHCI', 'USB 2.0'], ['XHCI', 'USB 3.0']], (k) => {
                    if (k !== type) update(m, { usb: { controller: k } }, { title: 'USB controller: ' + k, diff: [['-', 'USB: ' + type], ['+', 'USB: ' + k]], undo: { usb: { controller: type } } }, after);
                }, live),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'USB 2.0 and 3.0 pass-through need the extension pack on some hosts.')),
            h('div', { style: { gridColumn: 'span 6', display: 'flex', flexDirection: 'column', gap: '8px' } },
                h('div', { class: 'spread' }, h('h3', { class: 'title' }, 'Device filters'),
                    h('button', { class: 'btn tonal st', disabled: live || type === 'none' ? true : undefined, onclick: () => formDialog({ title: 'Add a USB filter', kicker: m.name,
                        fields: [{ key: 'name', label: 'Name', placeholder: 'Security key' }, { key: 'vendorId', label: 'Vendor id (hex, optional)', placeholder: '1050' }, { key: 'productId', label: 'Product id (hex, optional)', placeholder: '0407' }],
                        validate: (v) => !v.name.trim() ? 'A name is required.' : [v.vendorId, v.productId].some((x) => x && !/^[0-9a-fA-F]{4}$/.test(x)) ? 'Ids are four hex digits.' : '',
                        actionLabel: 'Add filter',
                        onSubmit: async (v) => { await api('machine.settingsUpdate', { id: m.id, usb: { filtersAdd: [{ name: v.name.trim(), vendorId: v.vendorId, productId: v.productId }] } }, { quiet: true });
                            recordHistory({ title: 'USB filter added: ' + v.name.trim(), scope: m.name, category: 'Settings', diff: [['+', v.name.trim() + ' ' + (v.vendorId || '*') + ':' + (v.productId || '*')]], undo: { method: 'machine.settingsUpdate', params: { id: m.id, usb: { filtersRemove: [s.usb.filters.length] } } } });
                            invalidateMore(); after(); } }) }, icon(ICONS.plus, 18), 'Add filter')),
                s.usb.filters.length ? s.usb.filters.map((f, i) => h('div', { class: 'card rise row', style: { flexWrap: 'nowrap', borderRadius: '20px', ...stagger(i, 50) } },
                    h('span', { class: 'dot ' + (f.active ? 'bg-success' : '') }), h('span', { class: 'grow' }, h('strong', null, f.name), h('div', { class: 'mono muted', style: { fontSize: '12px' } }, (f.vendorId || '*') + ':' + (f.productId || '*'))),
                    h('button', { class: 'btn st tone-error', disabled: live ? true : undefined, onclick: () => update(m, { usb: { filtersRemove: [f.index] } }, { title: 'USB filter removed: ' + f.name, diff: [['-', f.name]], undo: { usb: { filtersAdd: [f] } } }, after) }, 'Remove')))
                    : h('div', { class: 'empty' }, type === 'none' ? 'Turn on a USB controller to add filters.' : 'No filters. Devices matching a filter attach to the guest automatically.')),
            live ? h('div', { class: 'muted', style: { gridColumn: 'span 6', fontSize: '12.5px' } }, 'Attach a host device to a running machine from the Devices menu on its screen.') : null);
        return true;
    }

    if (section === 'interface') {
        const run = (key, value, label) => update(m, { [key]: value }, { title: label + ': ' + value, diff: [['-', label + ': ' + s[key]], ['+', label + ': ' + value]], undo: { [key]: s[key] } }, after, 'machine.runtime');
        mount(grid,
            tile(6, 0, h('h3', { class: 'title' }, 'Shared clipboard'), seg('Shared clipboard', s.clipboardMode, MODES, (k) => run('clipboardMode', k, 'Clipboard'))),
            tile(6, 1, h('h3', { class: 'title' }, 'Drag and drop'), seg('Drag and drop', s.dndMode, MODES, (k) => run('dndMode', k, 'Drag and drop')),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'Both need Guest Additions and change immediately, even while the machine runs.')));
        return true;
    }

    /* storage: controllers and new attachments, below the existing editor. */
    const BUSES = [['SATA', 'SATA (AHCI)'], ['NVMe', 'NVMe'], ['IDE', 'IDE'], ['SCSI', 'SCSI'], ['SAS', 'SAS'], ['VirtioSCSI', 'Virtio SCSI'], ['USB', 'USB'], ['Floppy', 'Floppy']];
    mount(grid,
        h('div', { class: 'spread', style: { gridColumn: 'span 6' } }, h('h3', { class: 'title' }, 'Controllers'),
            h('div', { class: 'row' },
                h('button', { class: 'btn tonal st', disabled: live ? true : undefined, onclick: () => formDialog({ title: 'Add a storage controller', kicker: m.name,
                    fields: [{ key: 'bus', label: 'Bus', value: 'SATA', options: BUSES }, { key: 'name', label: 'Name', value: '', placeholder: 'SATA 2' }],
                    validate: (v) => !v.name.trim() ? 'A name is required.' : s.storageControllers.some((c) => c.name === v.name.trim()) ? 'That name is taken.' : '',
                    actionLabel: 'Add controller',
                    onSubmit: async (v) => { await api('machine.settingsUpdate', { id: m.id, controllersAdd: [{ name: v.name.trim(), bus: v.bus }] }, { quiet: true });
                        recordHistory({ title: 'Controller added: ' + v.name.trim(), scope: m.name, category: 'Settings', diff: [['+', v.name.trim() + ' (' + v.bus + ')']], undo: { method: 'machine.settingsUpdate', params: { id: m.id, controllersRemove: [{ name: v.name.trim() }] } } });
                        invalidateMore(); after(); } }) }, icon(ICONS.plus, 18), 'Controller'),
                h('button', { class: 'btn tonal st', disabled: live || !s.storageControllers.length ? true : undefined, onclick: async () => {
                    const media = await api('media.list', { kind: 'disk' }, { quiet: true }).catch(() => []);
                    const free = media.filter((x) => !x.attachedTo.length);
                    formDialog({ title: 'Attach a disk or drive', kicker: m.name,
                        fields: [{ key: 'controller', label: 'Controller', value: s.storageControllers[0].name, options: s.storageControllers.map((c) => [c.name, c.name + ' (' + c.bus + ')']) },
                            { key: 'type', label: 'Device', value: free.length ? 'disk' : 'optical', options: [['disk', 'Hard disk'], ['optical', 'Empty optical drive'], ['floppy', 'Empty floppy drive']] },
                            { key: 'mediumId', label: 'Disk (for hard disks)', value: free.length ? free[0].id : '', options: free.length ? free.map((x) => [x.id, x.name]) : [['', 'No unattached disk; create one from Media']] },
                            { key: 'port', label: 'Port', value: 1, type: 'number' }],
                        validate: (v) => v.type === 'disk' && !v.mediumId ? 'Choose a disk.' : !(v.port >= 0 && v.port <= 29) ? 'Port is 0 to 29.' : '',
                        actionLabel: 'Attach',
                        onSubmit: async (v) => { const a = { controller: v.controller, port: v.port, device: 0, type: v.type, mediumId: v.type === 'disk' ? v.mediumId : undefined };
                            await api('machine.settingsUpdate', { id: m.id, attach: [a] }, { quiet: true });
                            recordHistory({ title: 'Device attached', scope: m.name, category: 'Settings', diff: [['+', v.controller + ' ' + v.port + ':0 ' + v.type]], undo: { method: 'machine.settingsUpdate', params: { id: m.id, detach: [{ controller: v.controller, port: v.port, device: 0 }] } } });
                            invalidateMore(); after(); } });
                } }, icon(ICONS.disk, 18), 'Attach'))),
        s.storageControllers.map((c, i) => h('section', { class: 'card rise row', style: { gridColumn: 'span 3', flexWrap: 'nowrap', borderRadius: '22px', ...stagger(i, 50) } },
            icon(ICONS.chip), h('span', { class: 'grow' }, h('strong', null, c.name), h('div', { class: 'muted', style: { fontSize: '12px' } }, c.bus + ' · ' + c.ports + ' port' + (c.ports === 1 ? '' : 's'))),
            h('button', { class: 'btn st tone-error', disabled: live ? true : undefined, onclick: () => confirmDestructive({ title: 'Remove ' + c.name + '?', detail: 'Every drive on this controller is detached. The disk files are kept.', actionLabel: 'Remove',
                onConfirm: async () => { await api('machine.settingsUpdate', { id: m.id, controllersRemove: [{ name: c.name }] }); recordHistory({ title: 'Controller removed: ' + c.name, scope: m.name, category: 'Settings', diff: [['-', c.name + ' (' + c.bus + ')']], undo: { method: 'machine.settingsUpdate', params: { id: m.id, controllersAdd: [{ name: c.name, bus: c.bus }] } } }); invalidateMore(); after(); } }) }, 'Remove'))));
    return true;
}

/** Detach button helper for the storage editor's cards. */
export async function detachDevice(m, s, rerender) {
    try {
        await api('machine.settingsUpdate', { id: m.id, detach: [{ controller: s.controller, port: s.port, device: s.device }] });
        recordHistory({ title: 'Device detached', scope: m.name, category: 'Settings', diff: [['-', s.controller + ' ' + s.port + ':' + s.device + ' ' + (s.medium || s.type)]] });
        notify({ kind: 'success', title: 'Detached', body: s.controller + ' ' + s.port + ':' + s.device });
        invalidateMore();
        rerender();
    } catch { /* notified */ }
}
