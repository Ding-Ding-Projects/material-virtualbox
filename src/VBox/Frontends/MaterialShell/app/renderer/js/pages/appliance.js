/*
 * Appliances: export machines to an OVF/OVA file and import one back.
 * Export picks powered-off machines, a format and a destination; import
 * reads the file first, shows what it contains and any warnings, then
 * imports.  Both are three-step flows with Back and Next.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, api, notify, recordHistory } from '../services.js';
import { refreshMachines, tileColors } from './machines.js';
import { monogram, stateLabel } from '../logic/format.js';

let flow = null;

const clean = (e) => String(e.message || e).replace(/^Error invoking remote method '[^']+': (Error: )?/, '');

function fresh() {
    const s = store.get();
    return { mode: s.applianceMode || 'export', step: 0, ids: new Set(s.applianceIds || []), format: 'ova', path: '', manifest: true, keepMacs: false, inspect: null, busy: false, error: '', result: null };
}

export function renderAppliance(root) {
    if (!flow || flow.mode !== (store.get().applianceMode || 'export') || store.get().applianceIds !== flow.seed) {
        flow = fresh();
        flow.seed = store.get().applianceIds;
    }
    const f = flow;
    const redraw = () => renderAppliance(root);
    const machines = store.get().machines;
    const exportable = machines.filter((m) => m.state !== 'running' && m.state !== 'paused');
    const steps = f.mode === 'export' ? ['Machines', 'File', 'Done'] : ['File', 'Contents', 'Done'];
    const folder = (store.get().defaultMachineFolder || 'D:\\VMs').replace(/[\\/]+$/, '');
    if (!f.path) f.path = f.mode === 'export' ? folder + '\\' + ((machines.find((m) => f.ids.has(m.id)) || {}).name || 'appliance').replace(/[^\w .-]+/g, '-') + '.ova' : '';

    const run = async () => {
        f.busy = true; f.error = ''; redraw();
        try {
            if (f.mode === 'export') {
                const path = f.format === 'ova' ? f.path.replace(/\.ovf$/i, '.ova') : f.path.replace(/\.ova$/i, '.ovf');
                await api('appliance', { action: 'export', path, format: 'ovf-2.0', manifest: f.manifest, machines: [...f.ids].map((id) => ({ id })) }, { quiet: true });
                const names = machines.filter((m) => f.ids.has(m.id)).map((m) => m.name);
                recordHistory({ title: 'Appliance exported', scope: names.join(', '), category: 'Appliances', diff: names.map((n) => ['+', n]).concat([['+', path]]) });
                notify({ kind: 'success', title: 'Appliance exported', body: path });
                f.result = path;
            } else {
                const r = await api('appliance', { action: 'import', path: f.path.trim(), keepMacs: f.keepMacs }, { quiet: true });
                recordHistory({ title: 'Appliance imported', scope: f.path.trim().split(/[\\/]/).pop(), category: 'Appliances', diff: f.inspect.systems.map((x) => ['+', x.name]) });
                notify({ kind: 'success', title: 'Appliance imported', body: f.inspect.systems.map((x) => x.name).join(', ') });
                await refreshMachines();
                f.result = (r.machines || [])[0] || '';
            }
            f.step = 2;
        } catch (e) { f.error = clean(e); }
        f.busy = false; redraw();
    };
    const inspect = async () => {
        f.busy = true; f.error = ''; redraw();
        try { f.inspect = await api('appliance', { action: 'inspect', path: f.path.trim() }, { quiet: true }); f.step = 1; } catch (e) { f.error = clean(e); }
        f.busy = false; redraw();
    };

    let content, canNext = true, next = () => { f.step++; redraw(); }, nextLabel = 'Next';
    if (f.mode === 'export' && f.step === 0) {
        canNext = f.ids.size > 0;
        content = [h('h1', { class: 'display', style: { fontSize: '56px' } }, 'Choose machines'),
            h('span', { class: 'muted' }, 'Running machines cannot be exported; shut them down first.'),
            h('div', { class: 'grid g3' }, machines.map((m, i) => {
                const [bg, fg] = tileColors(m.name);
                const ok = exportable.includes(m), on = f.ids.has(m.id);
                return h('button', { class: 'card st lift rise', 'aria-pressed': String(on), disabled: ok ? undefined : true, style: { textAlign: 'left', display: 'flex', gap: '14px', alignItems: 'center', borderRadius: '24px', border: '2px solid ' + (on ? 'var(--primary)' : 'transparent'), opacity: ok ? 1 : .5, ...stagger(i, 50) },
                    onclick: () => { on ? f.ids.delete(m.id) : f.ids.add(m.id); f.path = ''; redraw(); } },
                    h('span', { class: 'tile', style: { background: bg, color: fg } }, monogram(m.name)),
                    h('span', { class: 'grow' }, h('strong', null, m.name), h('div', { class: 'muted', style: { fontSize: '12px' } }, stateLabel(m.state))),
                    on ? icon(ICONS.check, 20) : null);
            }))];
    } else if (f.mode === 'export' && f.step === 1) {
        canNext = !!f.path.trim() && !f.busy; next = run; nextLabel = f.busy ? 'Exporting…' : 'Export';
        content = [h('h1', { class: 'display', style: { fontSize: '56px' } }, 'Where to save'),
            h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Format' }, [['ova', 'One .ova file'], ['ovf', '.ovf with separate disks']].map(([k, l]) =>
                h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(f.format === k), onclick: () => { f.format = k; f.path = k === 'ova' ? f.path.replace(/\.ovf$/i, '.ova') : f.path.replace(/\.ova$/i, '.ovf'); redraw(); } }, l))),
            h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, 'File'), h('input', { value: f.path, oninput: (e) => { f.path = e.target.value; const b = root.querySelector('#appliance-next'); if (b) b.disabled = !f.path.trim(); } })),
            h('label', { class: 'row' }, h('input', { type: 'checkbox', checked: f.manifest ? true : undefined, onchange: (e) => { f.manifest = e.target.checked; } }), 'Write a manifest so the files can be checked on import')];
    } else if (f.mode === 'import' && f.step === 0) {
        canNext = /\.(ova|ovf)$/i.test(f.path.trim()) && !f.busy; next = inspect; nextLabel = f.busy ? 'Reading…' : 'Read file';
        content = [h('h1', { class: 'display', style: { fontSize: '56px' } }, 'Import an appliance'),
            h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, 'File'), h('input', { value: f.path, placeholder: 'D:\\Downloads\\appliance.ova', oninput: (e) => { f.path = e.target.value; const b = root.querySelector('#appliance-next'); if (b) b.disabled = !/\.(ova|ovf)$/i.test(f.path.trim()); } }),
                h('span', { class: 'muted', style: { fontSize: '12px' } }, 'An .ova or .ovf file on this computer'))];
    } else if (f.mode === 'import' && f.step === 1) {
        canNext = !f.busy; next = run; nextLabel = f.busy ? 'Importing…' : 'Import';
        content = [h('h1', { class: 'display', style: { fontSize: '56px' } }, 'Contents'),
            h('div', { class: 'grid g3' }, f.inspect.systems.map((x, i) => h('div', { class: 'card rise', style: { borderRadius: '24px', ...stagger(i, 60) } }, h('div', { class: 'kicker' }, 'Machine ' + (i + 1)), h('strong', { style: { fontSize: '18px' } }, x.name)))),
            f.inspect.warnings.length ? h('div', { class: 'banner warn' }, f.inspect.warnings.join(' ')) : null,
            h('label', { class: 'row' }, h('input', { type: 'checkbox', checked: f.keepMacs ? true : undefined, onchange: (e) => { f.keepMacs = e.target.checked; } }), 'Keep the network MAC addresses from the file')];
    } else {
        content = [h('div', { class: 'pop', style: { width: '96px', height: '96px', borderRadius: '32px', display: 'grid', placeItems: 'center', background: 'var(--primary-container)', color: 'var(--on-primary-container)' } }, icon(ICONS.check, 48)),
            h('h1', { class: 'display', style: { fontSize: '56px' } }, f.mode === 'export' ? 'Exported' : 'Imported'),
            h('div', { class: 'mono muted' }, f.mode === 'export' ? f.result : f.inspect.systems.map((x) => x.name).join(', ')),
            h('div', { class: 'row' },
                h('button', { class: 'btn filled st', onclick: () => { if (f.mode === 'import' && f.result) store.set({ selectedId: f.result }); store.set({ route: 'machines' }); } }, 'Back to the library'),
                h('button', { class: 'btn outlined st', onclick: () => { flow = null; redraw(); } }, f.mode === 'export' ? 'Export another' : 'Import another'))];
    }

    mount(root, h('div', { class: 'page', style: { maxWidth: '980px' } },
        h('div', { class: 'spread' },
            h('div', { class: 'segmented', role: 'tablist', 'aria-label': 'Appliance task' }, [['export', 'Export'], ['import', 'Import']].map(([k, l]) =>
                h('button', { role: 'tab', class: 'st', 'aria-selected': String(f.mode === k), disabled: f.busy ? true : undefined, onclick: () => { store.set({ applianceMode: k, applianceIds: [] }); } }, l))),
            h('ol', { class: 'row', 'aria-label': 'Progress', style: { listStyle: 'none', margin: 0, padding: 0 } }, steps.map((label, i) => h('li', { class: 'chip', 'aria-current': i === f.step ? 'step' : undefined, style: { height: '30px', background: i === f.step ? 'var(--primary-container)' : i < f.step ? 'var(--secondary-container)' : undefined } }, (i + 1) + ' · ' + label)))),
        h('div', { class: 'rise', style: { display: 'flex', flexDirection: 'column', gap: '16px' } }, content),
        f.error ? h('div', { class: 'banner error', role: 'alert' }, f.error) : null,
        f.step < 2 ? h('div', { class: 'row', style: { justifyContent: 'flex-end' } },
            h('button', { class: 'btn outlined st', disabled: f.step === 0 || f.busy ? true : undefined, onclick: () => { f.step--; f.error = ''; redraw(); } }, 'Back'),
            h('button', { class: 'btn filled st', id: 'appliance-next', disabled: canNext ? undefined : true, onclick: next }, nextLabel, icon(ICONS.arrow, 18))) : null));
}
