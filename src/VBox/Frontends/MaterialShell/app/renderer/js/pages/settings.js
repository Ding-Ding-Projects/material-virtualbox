/*
 * Machine settings (bento layout, live validation) and the New virtual
 * machine wizard.  Both write through the bridge and record history.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, api, notify, recordHistory } from '../services.js';
import { searchField, registerPaletteSource, teleport } from '../features/overlays.js';
import { megabytes, monogram } from '../logic/format.js';
import { refreshMachines } from './machines.js';
import { renderEditor, invalidateEditors } from './settings-editors.js';

const SECTIONS = [['general', 'General', ICONS.info], ['system', 'System', ICONS.chip], ['display', 'Display', ICONS.display], ['storage', 'Storage', ICONS.disk], ['network', 'Network', ICONS.network], ['folders', 'Shared folders', ICONS.folder]];
let section = 'system';
let draft = null;

registerPaletteSource(() => {
    const m = store.get().machines.find((x) => x.id === store.get().selectedId);
    if (!m) return [];
    return [
        { scope: 'Settings', title: 'Memory for ' + m.name, path: 'Settings · System · Base memory', icon: ICONS.chip, run: () => { section = 'system'; store.set({ route: 'settings' }); teleport('#set-memory'); } },
        { scope: 'Settings', title: 'Processors for ' + m.name, path: 'Settings · System · Processors', icon: ICONS.chip, run: () => { section = 'system'; store.set({ route: 'settings' }); teleport('#set-cpus'); } },
        { scope: 'Settings', title: 'Rename ' + m.name, path: 'Settings · General · Name', icon: ICONS.edit, run: () => { section = 'general'; store.set({ route: 'settings' }); teleport('#set-name'); } }
    ];
});

export function renderSettings(root) {
    const s = store.get();
    const m = s.machines.find((x) => x.id === s.selectedId);
    if (!m) return mount(root, h('div', { class: 'page' }, h('div', { class: 'empty' }, 'Select a machine first.')));
    if (!draft || draft.id !== m.id) { draft = { id: m.id, name: m.name, memoryMB: m.memoryMB, cpus: m.cpus }; invalidateEditors(); }
    const host = s.host || { memoryMB: 0, cpus: 0 };
    const live = m.state === 'running' || m.state === 'paused' || m.state === 'saved';
    const dirty = draft.name !== m.name || draft.memoryMB !== m.memoryMB || draft.cpus !== m.cpus;
    const memWarn = host.memoryMB && draft.memoryMB > host.memoryMB / 2;

    const apply = async () => {
        const diff = [];
        if (draft.name !== m.name) diff.push(['-', 'Name: ' + m.name], ['+', 'Name: ' + draft.name]);
        if (draft.memoryMB !== m.memoryMB) diff.push(['-', 'Base memory: ' + m.memoryMB + ' MB'], ['+', 'Base memory: ' + draft.memoryMB + ' MB']);
        if (draft.cpus !== m.cpus) diff.push(['-', 'Processors: ' + m.cpus], ['+', 'Processors: ' + draft.cpus]);
        try {
            await api('machine.update', { id: m.id, name: draft.name !== m.name ? draft.name : undefined, memoryMB: draft.memoryMB, cpus: draft.cpus });
            recordHistory({ title: 'Settings changed: ' + draft.name, scope: draft.name, category: 'Settings', diff, undo: { method: 'machine.update', params: { id: m.id, name: m.name, memoryMB: m.memoryMB, cpus: m.cpus } } });
            notify({ kind: 'success', title: 'Settings saved', body: draft.name });
            draft = null;
            await refreshMachines();
        } catch { /* notified */ }
    };

    const side = h('aside', { class: 'glass', style: { flex: '1 1 260px', maxWidth: '300px', margin: '16px 0 16px 16px', padding: '18px 12px', borderRadius: '32px', display: 'flex', flexDirection: 'column', gap: '6px', alignSelf: 'flex-start' } },
        h('span', { class: 'kicker', style: { padding: '0 10px' } }, m.name),
        h('h1', { class: 'display', style: { fontSize: '36px', margin: '2px 10px 10px' } }, 'Settings'),
        searchField({ placeholder: 'Search every setting', onChange: (match, q) => { if (!q) return; const hit = SECTIONS.find(([, label]) => match(label)); if (hit) { section = hit[0]; renderSettings(root); } }, samples: () => SECTIONS.map((x) => x[1]) }),
        SECTIONS.map(([id, label, ic], i) => h('button', { class: 'st slide', 'aria-current': section === id ? 'page' : 'false', style: { display: 'flex', alignItems: 'center', gap: '12px', height: '46px', padding: '0 14px', borderRadius: '23px', textAlign: 'left', fontWeight: 600, background: section === id ? 'linear-gradient(90deg, var(--primary-container), var(--secondary-container))' : 'transparent', color: section === id ? 'var(--on-primary-container)' : 'var(--on-surface-variant)', ...stagger(i, 45) }, onclick: () => { section = id; renderSettings(root); } }, icon(ic), label)));

    const tiles = [];
    if (section === 'system') {
        tiles.push(
            h('section', { class: 'glass card lift rise', id: 'set-memory', tabindex: '-1', style: { gridColumn: 'span 4', borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '14px' } },
                h('div', { class: 'spread', style: { alignItems: 'baseline' } }, h('h3', { class: 'title' }, 'Base memory'), h('span', { style: { fontSize: '40px', fontWeight: 850, color: 'var(--primary)' } }, megabytes(draft.memoryMB))),
                h('input', { type: 'range', min: 256, max: Math.max(host.memoryMB || 65536, draft.memoryMB), step: 256, value: String(draft.memoryMB), disabled: live ? true : undefined, 'aria-label': 'Base memory in megabytes', style: { width: '100%', accentColor: 'var(--primary)' }, oninput: (e) => { draft.memoryMB = Number(e.target.value); renderSettings(root); root.querySelector('#set-memory input').focus(); } }),
                h('div', { class: 'row' }, [2048, 4096, 8192, 12288, 16384].map((v) => h('button', { class: 'chip st', 'aria-pressed': String(draft.memoryMB === v), disabled: live ? true : undefined, onclick: () => { draft.memoryMB = v; renderSettings(root); } }, megabytes(v))))),
            h('section', { class: 'glass card lift rise', id: 'set-cpus', tabindex: '-1', style: { gridColumn: 'span 2', borderRadius: '28px', display: 'flex', flexDirection: 'column', justifyContent: 'space-between', gap: '10px', animationDelay: '80ms' } },
                h('h3', { class: 'title' }, 'Processors'),
                h('span', { style: { fontSize: '72px', lineHeight: 1, fontWeight: 900, fontVariationSettings: "'wdth' 140", color: 'var(--tertiary)' } }, String(draft.cpus)),
                h('div', { class: 'row', style: { flexWrap: 'nowrap' } },
                    h('button', { class: 'btn outlined st grow', 'aria-label': 'Fewer processors', disabled: live || draft.cpus <= 1 ? true : undefined, onclick: () => { draft.cpus--; renderSettings(root); } }, '−'),
                    h('button', { class: 'btn tonal st grow', 'aria-label': 'More processors', disabled: live || (host.cpus && draft.cpus >= host.cpus) ? true : undefined, onclick: () => { draft.cpus++; renderSettings(root); } }, '+'))));
    } else if (section === 'general') {
        tiles.push(h('label', { class: 'input-card rise', id: 'set-name', style: { gridColumn: 'span 6' } }, h('span', { class: 'kicker' }, 'Name'), h('input', { value: draft.name, oninput: (e) => { draft.name = e.target.value; } , onchange: () => renderSettings(root) }), h('span', { class: 'muted', style: { fontSize: '12px' } }, 'Shown in the library and on the machine window.')));
    }

    const editorSection = ['display', 'network', 'folders', 'storage'].includes(section);
    const editorGrid = (t) => {
        const grid = h('div', { class: 'grid g6' }, t);
        if (editorSection) renderEditor(section, m, grid, () => renderSettings(root));
        return grid;
    };
    mount(root, h('div', { style: { display: 'flex', flexWrap: 'wrap', minHeight: '100%' } }, side,
        h('div', { class: 'page', style: { flex: '999 1 560px', minWidth: 0 } },
            h('div', { class: 'spread' },
                h('div', null, h('span', { class: 'mono muted', style: { fontSize: '12px' } }, 'Settings / ' + SECTIONS.find((x) => x[0] === section)[1]),
                    h('h2', { class: 'display', style: { background: 'linear-gradient(90deg, var(--on-primary-container), var(--tertiary))', WebkitBackgroundClip: 'text', backgroundClip: 'text', color: 'transparent' } }, SECTIONS.find((x) => x[0] === section)[1])),
                editorSection ? null : h('div', { class: 'row' }, h('button', { class: 'btn outlined st', disabled: dirty ? undefined : true, onclick: () => { draft = null; renderSettings(root); } }, 'Discard'), h('button', { class: 'btn filled st', disabled: dirty && !live && draft.name.trim() ? undefined : true, onclick: apply }, 'Apply'))),
            live && section !== 'storage' ? h('div', { class: 'banner info rise' }, icon(ICONS.info), 'Power the machine off to change hardware. Running and saved machines keep their hardware fixed.') : null,
            memWarn ? h('div', { class: 'banner warn rise' }, icon(ICONS.info), 'Memory is above half of the host\'s ' + megabytes(host.memoryMB) + '. The host may page while this machine runs.') : null,
            editorGrid(tiles))));
}

/* ------------------------------------------------------------- the wizard */

const OS = [['Windows11_64', 'Windows 11', 'W', 'linear-gradient(135deg,#9ECAFF,#D0BCFF)', 8192, 4, 80], ['Ubuntu_64', 'Ubuntu', 'U', 'linear-gradient(135deg,#FFB869,#EFB8C8)', 4096, 2, 40],
    ['Fedora_64', 'Fedora', 'F', 'linear-gradient(135deg,#9ECAFF,#A6D7A0)', 4096, 2, 40], ['Debian_64', 'Debian', 'D', 'linear-gradient(135deg,#EFB8C8,#F2B8B5)', 2048, 2, 20],
    ['FreeBSD_64', 'FreeBSD', 'B', 'linear-gradient(135deg,#FFDDB8,#FFB869)', 1024, 1, 16], ['Other_64', 'Other', '?', 'linear-gradient(135deg,#D0BCFF,#A6D7A0)', 2048, 2, 20]];
let wiz = null;

export function renderNewVm(root) {
    if (!wiz) wiz = { step: 0, os: 0, name: '', group: '', memoryMB: OS[0][4], cpus: OS[0][5], diskGB: OS[0][6], busy: false };
    const steps = ['System', 'Hardware', 'Disk', 'Review'];
    const os = OS[wiz.os];
    const nameTaken = store.get().machines.some((m) => m.name === wiz.name.trim());
    const canNext = wiz.step !== 0 || (wiz.name.trim() && !nameTaken);
    const create = async () => {
        wiz.busy = true; renderNewVm(root);
        try {
            const r = await api('machine.create', { name: wiz.name.trim(), osTypeId: os[0], group: wiz.group.trim() || undefined, memoryMB: wiz.memoryMB, cpus: wiz.cpus, diskGB: wiz.diskGB });
            recordHistory({ title: 'Machine created: ' + wiz.name.trim(), scope: wiz.name.trim(), category: 'Machines', diff: [['+', 'Name: ' + wiz.name.trim()], ['+', 'OS type: ' + os[0]], ['+', 'Memory: ' + wiz.memoryMB + ' MB'], ['+', 'Disk: ' + (r.disk || 'none')]] });
            notify({ kind: 'success', title: 'Created ' + wiz.name.trim(), body: r.disk ? 'Disk: ' + r.disk : 'No disk attached' });
            await refreshMachines();
            store.set({ route: 'machines', selectedId: r.machine.id });
            wiz = null;
        } catch { wiz.busy = false; renderNewVm(root); }
    };
    const field = (label, key, hint, type = 'text') => h('label', { class: 'input-card rise' }, h('span', { class: 'kicker' }, label),
        h('input', { type, value: String(wiz[key]), oninput: (e) => { wiz[key] = type === 'number' ? Number(e.target.value) : e.target.value; }, onchange: () => renderNewVm(root) }), h('span', { class: 'muted', style: { fontSize: '12px' } }, hint));

    let content;
    if (wiz.step === 0) {
        content = [h('h1', { class: 'display', style: { fontSize: '60px' } }, 'Pick an operating system'),
            h('div', { class: 'grid g3' }, OS.map((o, i) => h('button', { class: 'card st lift rise', 'aria-pressed': String(i === wiz.os), style: { textAlign: 'left', minHeight: '140px', display: 'flex', flexDirection: 'column', gap: '16px', borderRadius: '28px', border: '2px solid ' + (i === wiz.os ? 'var(--primary)' : 'transparent'), background: i === wiz.os ? 'rgba(79,55,139,.45)' : 'var(--surface-container-low)', ...stagger(i, 60) },
                onclick: () => { wiz.os = i; wiz.memoryMB = o[4]; wiz.cpus = o[5]; wiz.diskGB = o[6]; renderNewVm(root); } },
                h('span', { class: 'tile', style: { width: '52px', height: '52px', borderRadius: '18px', background: o[3], color: '#0B0A0F', fontSize: '22px', fontWeight: 900 } }, o[2]), h('strong', { style: { fontSize: '18px' } }, o[1])))),
            h('div', { class: 'grid g2' }, field('Name', 'name', nameTaken ? 'A machine with this name already exists.' : 'Shown in the library'), field('Group', 'group', 'Optional, for example Lab'))];
    } else if (wiz.step === 1) {
        content = [h('h1', { class: 'display', style: { fontSize: '60px' } }, 'Shape the hardware'), h('div', { class: 'grid g2' }, field('Memory (MB)', 'memoryMB', 'Suggested for ' + os[1] + ': ' + megabytes(os[4]), 'number'), field('Processors', 'cpus', 'Host has ' + ((store.get().host || {}).cpus || '?') + ' threads', 'number'))];
    } else if (wiz.step === 2) {
        content = [h('h1', { class: 'display', style: { fontSize: '60px' } }, 'Give it a disk'), h('p', { class: 'muted', style: { fontSize: '16px' } }, 'A dynamically allocated VDI only takes the space the guest writes. Set 0 for no disk.'), h('div', { class: 'grid g2' }, field('Size (GB)', 'diskGB', 'Created next to the settings file', 'number'))];
    } else {
        content = [h('h1', { class: 'display', style: { fontSize: '60px' } }, 'Review and create'),
            h('div', { class: 'grid g4' }, [['System', os[1]], ['Name', wiz.name], ['Memory', megabytes(wiz.memoryMB)], ['Processors', wiz.cpus], ['Disk', wiz.diskGB ? wiz.diskGB + ' GB VDI' : 'None'], ['Group', wiz.group || 'None']].map(([k, v], i) => h('div', { class: 'card rise', style: stagger(i, 50) }, h('div', { class: 'muted', style: { fontSize: '12px' } }, k), h('div', { style: { fontSize: '17px', fontWeight: 750 } }, String(v)))))];
    }

    mount(root, h('div', { class: 'page', style: { background: 'radial-gradient(1200px 600px at 85% -10%, #2A1F45 0%, transparent 60%)' } },
        h('div', { class: 'spread' }, h('span', { class: 'mono muted' }, 'New virtual machine'),
            h('ol', { class: 'row', 'aria-label': 'Steps', style: { listStyle: 'none', margin: 0, padding: 0 } }, steps.map((label, i) => h('li', null, h('button', { class: 'st row', 'aria-current': i === wiz.step ? 'step' : 'false', disabled: i > wiz.step ? true : undefined, style: { height: '40px', padding: '0 16px 0 6px', borderRadius: '20px', background: i === wiz.step ? 'rgba(79,55,139,.6)' : 'transparent', fontWeight: 650 }, onclick: () => { wiz.step = i; renderNewVm(root); } },
                h('span', { style: { width: '28px', height: '28px', borderRadius: '14px', display: 'grid', placeItems: 'center', fontSize: '12px', fontWeight: 800, background: i < wiz.step ? 'var(--success)' : (i === wiz.step ? 'var(--primary)' : 'var(--surface-container-highest)'), color: i <= wiz.step ? '#0B0A0F' : 'var(--on-surface-variant)' } }, i < wiz.step ? '✓' : String(i + 1)), label))))),
        h('div', { 'aria-hidden': 'true', style: { height: '4px', borderRadius: '2px', background: 'var(--surface-container-high)', overflow: 'hidden' } }, h('div', { style: { height: '100%', width: ((wiz.step + 1) / steps.length * 100) + '%', background: 'linear-gradient(90deg, var(--primary), var(--tertiary))', transition: 'width 600ms var(--ease-emph-in)' } })),
        h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: '28px' } },
            h('main', { class: 'pane-in', style: { flex: '999 1 520px', minWidth: 0, display: 'flex', flexDirection: 'column', gap: '20px' } }, content),
            h('aside', { style: { flex: '1 1 300px', maxWidth: '400px', display: 'flex', flexDirection: 'column', gap: '14px' } },
                h('div', { style: { height: '300px', borderRadius: '36px', background: 'linear-gradient(160deg, #2B2340, #1A1720)', display: 'grid', placeItems: 'center', position: 'relative', overflow: 'hidden' } },
                    h('div', { 'aria-hidden': 'true', style: { position: 'absolute', width: '260px', height: '260px', borderRadius: '50%', border: '1px dashed rgba(208,188,255,.3)', animation: 'mvSpin 40s linear infinite' } }),
                    h('div', { style: { width: '150px', height: '150px', borderRadius: '48px', display: 'grid', placeItems: 'center', background: os[3], color: '#0B0A0F', fontSize: '68px', fontWeight: 900, animation: 'mvPop 700ms var(--ease-spring) both' } }, wiz.name ? monogram(wiz.name) : os[2])),
                h('div', { class: 'row', style: { flexWrap: 'nowrap' } },
                    h('button', { class: 'btn outlined big st grow', disabled: wiz.step === 0 ? true : undefined, onclick: () => { wiz.step--; renderNewVm(root); } }, 'Back'),
                    h('button', { class: 'btn filled big st', style: { flex: 2 }, disabled: !canNext || wiz.busy ? true : undefined, onclick: () => { if (wiz.step === steps.length - 1) create(); else { wiz.step++; renderNewVm(root); } } }, wiz.busy ? 'Creating…' : wiz.step === steps.length - 1 ? 'Create machine' : 'Next')),
                h('button', { class: 'btn st', onclick: () => { wiz = null; store.set({ route: 'machines' }); } }, 'Cancel')))));
}
