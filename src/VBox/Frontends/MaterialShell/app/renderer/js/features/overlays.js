/*
 * Shared overlays: search field with the regex builder, command palette,
 * two-key super confirmation, export sheet, toasts and notification centre.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, notify, dismissToast, markAllRead, removeNotifications } from '../services.js';
import { CONSTRUCTS, FLAGS, buildSource, buildFlags, preview, describe, makeMatcher } from '../logic/regex-model.js';
import { FORMATS, exportRows } from '../logic/export-formats.js';
import { relativeTime } from '../logic/format.js';

const layer = () => document.getElementById('overlay');
let lastFocus = null;

function openLayer(...nodes) {
    lastFocus = document.activeElement;
    mount(layer(), ...nodes);
    const first = layer().querySelector('input, button:not([disabled])');
    if (first) first.focus();
}

export function closeLayer() {
    mount(layer());
    if (lastFocus && document.contains(lastFocus)) lastFocus.focus();
}

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && layer() && layer().childElementCount) { e.preventDefault(); closeLayer(); }
});

/* ------------------------------------------------------------------ search */

/**
 * A search field with its ".*" regex builder.  onChange receives a matcher
 * function; plain text matches as a substring, /pattern/flags as a regex.
 */
export function searchField({ placeholder, value = '', onChange, samples = () => [] }) {
    let input;
    const emit = () => onChange(makeMatcher(input.value), input.value);
    const rx = h('button', { class: 'rx-btn st', type: 'button', title: 'Regular expression builder', 'aria-label': 'Open the regular expression builder', onclick: () => openRegexBuilder({ samples: samples(), onUse: (src) => { input.value = src; emit(); } }) }, '.*');
    return h('label', { class: 'field' },
        icon(ICONS.search, 18),
        h('span', { class: 'sr-only' }, placeholder),
        h('input', { placeholder, value, ref: (el) => { input = el; }, oninput: emit }),
        rx);
}

export function openRegexBuilder({ samples = [], onUse }) {
    const st = { parts: ['start'], flags: ['i'], literal: '' };
    const body = h('div', { style: { display: 'flex', flexDirection: 'column', gap: '14px', padding: '22px' } });
    const render = () => {
        let src = '';
        try { src = buildSource(st.parts); } catch { src = ''; }
        const fl = buildFlags(st.flags);
        const pv = preview(src, fl, samples.length ? samples : ['Windows 11 Dev Box', 'Ubuntu 24.04 Server', 'Fedora Workstation']);
        mount(body,
            h('div', { class: 'spread' }, h('h2', { class: 'headline' }, 'Regex builder'), h('span', { class: 'kbd' }, 'Esc')),
            h('div', { class: 'row', style: { minHeight: '56px', padding: '8px 12px', borderRadius: '20px', background: 'var(--surface-dim)', border: '2px solid var(--primary-container)' } },
                h('span', { class: 'mono muted' }, '/'),
                st.parts.map((p, i) => h('span', { class: 'row', style: { gap: '6px', padding: '4px 4px 4px 12px', borderRadius: '14px', background: 'var(--primary-container)', color: 'var(--on-primary-container)', animation: 'mvChip 460ms var(--ease-spring) both' } },
                    h('span', { class: 'mono' }, typeof p === 'string' ? CONSTRUCTS.find((c) => c.id === p).src : (p.literal !== undefined ? p.literal : '(?:' + p.either.join('|') + ')')),
                    h('button', { class: 'st', 'aria-label': 'Remove part', style: { width: '26px', height: '26px', borderRadius: '13px' }, onclick: () => { st.parts.splice(i, 1); render(); } }, icon(ICONS.close, 12)))),
                h('span', { class: 'mono muted' }, '/' + fl),
                h('span', { class: 'grow' }),
                h('span', { class: pv.ok ? 'tone-success' : 'tone-error', style: { fontWeight: 700, fontSize: '12px' } }, pv.ok ? 'Valid · ' + pv.elapsedMs.toFixed(2) + ' ms' : pv.error)),
            h('div', { class: 'grid g4' }, CONSTRUCTS.map((c, i) => h('button', { class: 'card st lift', style: { textAlign: 'left', ...stagger(i, 25) }, onclick: () => { st.parts.push(c.id); render(); } },
                h('div', { class: 'mono tone-primary' }, c.src.length > 18 ? c.src.slice(0, 18) + '…' : c.src), h('div', { class: 'muted', style: { fontSize: '12.5px' } }, c.label)))),
            h('div', { class: 'row' },
                h('label', { class: 'field grow' }, h('span', { class: 'sr-only' }, 'Literal text'), h('input', { placeholder: 'Literal text, or words separated by |', value: st.literal, oninput: (e) => { st.literal = e.target.value; } })),
                h('button', { class: 'btn tonal st', onclick: () => { if (!st.literal) return; const w = st.literal.split('|').filter(Boolean); st.parts.push(w.length > 1 ? { either: w } : { literal: w[0] }); st.literal = ''; render(); } }, 'Add text')),
            h('div', { class: 'row' }, FLAGS.map((f) => h('button', { class: 'chip st', 'aria-pressed': String(st.flags.includes(f.id)), onclick: () => { st.flags = st.flags.includes(f.id) ? st.flags.filter((x) => x !== f.id) : [...st.flags, f.id]; render(); } }, h('span', { class: 'mono' }, f.id), f.label))),
            h('div', { class: 'card', style: { display: 'flex', flexDirection: 'column', gap: '4px', fontFamily: 'var(--mono)', fontSize: '13px' } },
                pv.results.map((r) => h('div', { class: r.hit ? '' : 'muted' }, r.hit ? [r.line.slice(0, r.index), h('mark', null, r.match), r.line.slice(r.index + r.match.length)] : r.line)),
                pv.truncated ? h('div', { class: 'tone-warning' }, 'Stopped at the 300 ms budget.') : null),
            h('div', { class: 'muted' }, describe(st.parts)),
            h('div', { class: 'row', style: { justifyContent: 'flex-end' } },
                h('button', { class: 'btn st', onclick: closeLayer }, 'Cancel'),
                h('button', { class: 'btn filled st', disabled: pv.ok ? undefined : true, onclick: () => { onUse('/' + src + '/' + fl); closeLayer(); } }, 'Use pattern')));
    };
    render();
    openLayer(h('div', { class: 'scrim', onclick: closeLayer }), h('div', { class: 'dialog open-anim', role: 'dialog', 'aria-label': 'Regular expression builder', style: { width: 'min(900px, calc(100vw - 32px))', maxHeight: 'calc(100vh - 120px)', overflow: 'auto' } }, body));
}

/* ----------------------------------------------------------------- palette */

const paletteSources = [];

/** Pages and the shell register palette entries: { title, path, icon, run, keys } providers. */
export function registerPaletteSource(fn) {
    paletteSources.push(fn);
}

export function openPalette() {
    let query = '', sel = 0, scope = 'All';
    const scopes = ['All', 'Machines', 'Actions', 'Settings', 'Pages'];
    const list = h('div', { role: 'listbox', 'aria-label': 'Results', style: { maxHeight: '460px', overflow: 'auto', paddingBottom: '8px' } });
    const scopeRow = h('div', { class: 'row', style: { padding: '0 20px 12px' } });
    let results = [];
    const run = (r) => { closeLayer(); r.run(); };
    const render = () => {
        const match = makeMatcher(query);
        const all = paletteSources.flatMap((fn) => fn());
        results = all.filter((r) => (scope === 'All' || r.scope === scope) && (match(r.title) || match(r.path || '')));
        if (sel >= results.length) sel = Math.max(0, results.length - 1);
        mount(scopeRow, scopes.map((s) => h('button', { class: 'chip st', 'aria-selected': String(s === scope), onclick: () => { scope = s; sel = 0; render(); } }, s, ' ', h('span', { class: 'muted' }, String(all.filter((r) => s === 'All' || r.scope === s).filter((r) => match(r.title) || match(r.path || '')).length)))));
        mount(list, results.length ? results.slice(0, 60).map((r, i) => h('div', { class: 'result', role: 'option', 'aria-selected': String(i === sel), style: stagger(i, 25), onclick: () => run(r), onmouseenter: () => { sel = i; list.querySelectorAll('.result').forEach((el, k) => el.setAttribute('aria-selected', String(k === sel))); } },
            h('span', { class: 'tile', style: { width: '38px', height: '38px', borderRadius: '12px', background: 'var(--primary-container)', color: 'var(--on-primary-container)' } }, icon(r.icon || ICONS.arrow, 18)),
            h('span', { class: 'grow', style: { display: 'flex', flexDirection: 'column' } }, h('span', { style: { fontWeight: 700 } }, r.title), h('span', { class: 'muted', style: { fontSize: '12px' } }, r.path || '')),
            r.toggle ? h('button', { class: 'switch st', role: 'switch', 'aria-checked': String(!!r.toggle.get()), 'aria-label': r.title, onclick: (e) => { e.stopPropagation(); r.toggle.set(!r.toggle.get()); render(); } }, h('span', { class: 'knob' })) : null,
            r.keys ? h('span', { class: 'kbd' }, r.keys) : null))
            : h('div', { class: 'empty' }, 'Nothing matches "' + query + '".'));
    };
    const input = h('input', { placeholder: 'Search machines, actions, settings and pages', style: { flex: 1, minWidth: 0, border: 0, outline: 0, background: 'transparent', fontSize: '20px', fontWeight: 600 },
        oninput: (e) => { query = e.target.value; sel = 0; render(); },
        onkeydown: (e) => {
            if (e.key === 'ArrowDown') { sel = Math.min(results.length - 1, sel + 1); render(); e.preventDefault(); }
            if (e.key === 'ArrowUp') { sel = Math.max(0, sel - 1); render(); e.preventDefault(); }
            if (e.key === 'Enter' && results[sel]) run(results[sel]);
        } });
    render();
    openLayer(h('div', { class: 'scrim', onclick: closeLayer }),
        h('div', { class: 'dialog open-anim', role: 'dialog', 'aria-label': 'Command palette' },
            h('label', { class: 'row', style: { padding: '18px 20px 12px 24px', flexWrap: 'nowrap' } }, icon(ICONS.search, 22, { stroke: 'var(--primary)' }), h('span', { class: 'sr-only' }, 'Search everything'), input,
                h('button', { class: 'rx-btn st', title: 'Regular expression builder', onclick: () => openRegexBuilder({ samples: paletteSources.flatMap((fn) => fn()).map((r) => r.title).slice(0, 12), onUse: (src) => { openPalette(); const i = layer().querySelector('input'); i.value = src; i.dispatchEvent(new Event('input')); } }) }, '.*'),
                h('span', { class: 'kbd' }, 'Esc')),
            scopeRow, list,
            h('div', { class: 'row muted', style: { padding: '10px 24px', background: 'rgba(15,13,19,.6)', fontSize: '12px' } }, 'Up and Down to move', ' · ', 'Enter to run', h('span', { class: 'grow' }), 'Ctrl+Shift+F')));
}

/** Scrolls an element into view and flashes it: the palette's teleport. */
export function teleport(selector) {
    requestAnimationFrame(() => {
        const el = document.querySelector(selector);
        if (!el) return;
        el.scrollIntoView({ behavior: 'smooth', block: 'center' });
        el.classList.remove('flash');
        void el.offsetWidth;
        el.classList.add('flash');
        if (el.focus) el.focus({ preventScroll: true });
    });
}

/* ------------------------------------------------------ super confirmation */

/**
 * Two keys, then a full-range slider, then authorise.  Emergency exit is the
 * default button and Escape always leaves.  onConfirm runs only when all
 * three steps are complete.
 */
export function confirmDestructive({ title, detail, actionLabel, onConfirm }) {
    const st = { k1: false, k2: false, slider: 0, busy: false };
    const box = h('section', { class: 'gate pop', role: 'alertdialog', 'aria-label': title });
    const render = () => {
        const both = st.k1 && st.k2, ready = both && st.slider >= 100;
        mount(box,
            h('div', { class: 'row', style: { flexWrap: 'nowrap', gap: '16px' } },
                h('span', { class: 'tile', style: { width: '56px', height: '56px', borderRadius: '18px', background: 'var(--error-container)', color: 'var(--on-error-container)' } }, icon(ICONS.trash, 26)),
                h('div', null, h('h2', { class: 'headline', style: { fontSize: '26px' } }, title), h('div', { class: 'tone-error' }, detail))),
            h('div', { class: 'grid g2' }, [['k1', 'Key one'], ['k2', 'Key two']].map(([k, label]) => h('button', { class: 'key st', 'aria-pressed': String(st[k]), onclick: () => { st[k] = !st[k]; st.slider = 0; render(); } },
                h('span', { class: 'ring' }, icon(ICONS.key, 40)), h('strong', null, label), h('span', { class: 'muted', style: { fontSize: '12px' } }, st[k] ? 'Turned' : 'Select to turn')))),
            h('label', { style: { display: 'flex', flexDirection: 'column', gap: '8px', opacity: both ? 1 : .4 } },
                h('span', { class: 'spread' }, h('strong', null, 'Slide to authorise'), h('span', { class: 'mono tone-error' }, st.slider + '%')),
                h('input', { type: 'range', class: 'authorise', min: 0, max: 100, step: 1, value: String(st.slider), disabled: both ? undefined : true, 'aria-label': 'Slide to authorise', oninput: (e) => { st.slider = Number(e.target.value); render(); box.querySelector('input[type=range]').focus(); } })),
            h('div', { class: 'row' },
                h('button', { class: 'btn big st', style: { flex: 2, background: 'var(--on-surface)', color: 'var(--surface)', fontWeight: 900 }, onclick: closeLayer, autofocus: true }, 'Emergency exit'),
                h('button', { class: 'btn big danger st', style: { flex: 1 }, disabled: ready && !st.busy ? undefined : true, onclick: async () => { st.busy = true; render(); try { await onConfirm(); closeLayer(); } catch { st.busy = false; render(); box.classList.add('shake'); } } }, st.busy ? 'Working…' : actionLabel)));
    };
    render();
    openLayer(h('div', { class: 'scrim' }), h('div', { style: { position: 'fixed', zIndex: 60, left: '50%', top: '70px', width: 'min(720px, calc(100vw - 32px))', transform: 'translateX(-50%)' } }, box));
    box.querySelector('.btn').focus();
}

/* ------------------------------------------------------------------ export */

export function openExport({ source, rows, fileStem = 'export' }) {
    let ext = 'json';
    const sheet = h('div', { class: 'sheet', role: 'dialog', 'aria-label': 'Export' });
    const render = () => {
        const f = FORMATS.find((x) => x.ext === ext);
        const text = exportRows(ext, rows, { source });
        mount(sheet,
            h('div', { class: 'spread' }, h('div', null, h('div', { class: 'kicker' }, source + ' · ' + rows.length + ' rows'), h('h2', { class: 'headline' }, 'Export')), h('span', { class: 'kbd' }, 'Esc')),
            h('div', { class: 'grid g6', role: 'radiogroup', 'aria-label': 'Format' }, FORMATS.map((x, i) => h('button', { class: 'card st lift', role: 'radio', 'aria-checked': String(x.ext === ext), style: { textAlign: 'left', border: '2px solid ' + (x.ext === ext ? 'var(--primary)' : 'transparent'), ...stagger(i, 25) }, onclick: () => { ext = x.ext; render(); } },
                h('div', { class: 'mono tone-primary' }, '.' + x.ext), h('div', { class: 'muted', style: { fontSize: '12px' } }, x.label)))),
            h('div', { class: 'banner ' + (f.loss ? 'warn' : 'info') }, f.loss || 'Lossless. UTF-8, LF line endings, schema 3.'),
            h('pre', { class: 'logbox', style: { margin: 0, padding: '14px 18px', maxHeight: '240px', whiteSpace: 'pre-wrap' } }, text),
            h('div', { class: 'row', style: { justifyContent: 'flex-end' } },
                h('button', { class: 'btn outlined st', onclick: async () => { await navigator.clipboard.writeText(text); notify({ kind: 'success', title: 'Copied', body: rows.length + ' rows as .' + ext }); } }, 'Copy'),
                h('button', { class: 'btn filled st', onclick: async () => { const r = await window.mvb.saveFile(fileStem + '.' + ext, text); if (r.saved) { notify({ kind: 'success', title: 'Export saved', body: r.path }); closeLayer(); } } }, 'Save .' + ext)));
    };
    render();
    openLayer(h('div', { class: 'scrim', onclick: closeLayer }), sheet);
}

/* ------------------------------------------------- toasts and notifications */

export function renderToasts(container) {
    const draw = (s) => mount(container, s.toasts.map((t) => h('div', { class: 'toast ' + t.kind, role: t.kind === 'error' ? 'alert' : 'status' },
        h('span', { class: 'grow', style: { display: 'flex', flexDirection: 'column' } }, h('strong', null, t.title), t.body ? h('span', { style: { fontSize: '12.5px', opacity: .85 } }, t.body) : null),
        t.action ? h('button', { class: 'btn st', style: { height: '36px', padding: '0 12px' }, onclick: () => { dismissToast(t.id); t.action.run(); } }, t.action.label) : null,
        h('button', { class: 'icon-btn st', 'aria-label': 'Dismiss', onclick: () => dismissToast(t.id) }, icon(ICONS.close, 14)),
        t.auto ? h('span', { class: 'drain', 'aria-hidden': 'true' }) : null)));
    let last = null;
    store.subscribe((s) => { if (s.toasts !== last) { last = s.toasts; draw(s); } });
}

export function openNotificationCentre() {
    let matcher = () => true;
    const selected = new Set();
    const listEl = h('div', { style: { display: 'flex', flexDirection: 'column', gap: '6px' } });
    const draw = () => {
        const items = store.get().notifications.filter((n) => matcher(n.title + ' ' + n.body));
        mount(listEl, items.length ? items.map((n, i) => h('div', { class: 'row', style: { flexWrap: 'nowrap', alignItems: 'flex-start', padding: '12px', borderRadius: '18px', background: n.read ? 'var(--surface-container)' : '#2B2340', animation: 'mvRise 400ms var(--ease-emph-in) both', ...stagger(i, 40) } },
            h('input', { type: 'checkbox', 'aria-label': 'Select ' + n.title, checked: selected.has(n.id) ? true : undefined, onchange: (e) => { e.target.checked ? selected.add(n.id) : selected.delete(n.id); } }),
            h('span', { class: 'dot bg-' + ({ error: 'error', warning: 'warning', success: 'success' }[n.kind] || 'primary'), style: { marginTop: '6px' } }),
            h('span', { class: 'grow', style: { display: 'flex', flexDirection: 'column', gap: '2px' } }, h('strong', null, n.title), h('span', { class: 'muted', style: { fontSize: '12.5px' } }, n.body), h('span', { class: 'muted', style: { fontSize: '11.5px' } }, relativeTime(n.at)))))
            : h('div', { class: 'empty' }, 'No notifications yet. Finished jobs, warnings and errors land here.'));
    };
    draw();
    openLayer(h('div', { class: 'scrim', style: { background: 'transparent', backdropFilter: 'none' }, onclick: closeLayer }),
        h('section', { class: 'panel', 'aria-label': 'Notification centre' },
            h('div', { class: 'row' }, h('h2', { class: 'title grow', style: { fontSize: '22px' } }, 'Notifications'),
                h('button', { class: 'btn st', style: { height: '36px' }, onclick: () => { markAllRead(); draw(); } }, 'Mark all read'),
                h('button', { class: 'btn st tone-error', style: { height: '36px' }, onclick: () => { removeNotifications([...selected]); selected.clear(); draw(); } }, 'Clear selected')),
            searchField({ placeholder: 'Search notifications', onChange: (m) => { matcher = m; draw(); }, samples: () => store.get().notifications.map((n) => n.title) }),
            listEl));
    markAllRead();
}

/* ------------------------------------------------------------- form dialog */

/**
 * A small form in a dialog.  fields: [{ key, label, value, placeholder, type,
 * options: [[value, label]], hint }].  validate(values) returns an error
 * message or ''.  onSubmit(values) may throw; the dialog then stays open.
 * body is optional extra content shown above the fields.
 */
export function formDialog({ title, kicker = '', body = null, fields = [], actionLabel = 'Save', validate = () => '', onSubmit }) {
    const values = Object.fromEntries(fields.map((f) => [f.key, f.value !== undefined ? f.value : (f.type === 'checkbox' ? false : '')]));
    const box = h('section', { class: 'dialog open-anim', role: 'dialog', 'aria-label': title, style: { padding: '24px', display: 'flex', flexDirection: 'column', gap: '14px' } });
    let busy = false, error = '';
    const render = () => {
        const problem = validate(values);
        const focusKey = document.activeElement && document.activeElement.dataset ? document.activeElement.dataset.key : null;
        mount(box,
            h('div', { class: 'spread' }, h('div', null, kicker ? h('div', { class: 'kicker' }, kicker) : null, h('h2', { class: 'headline' }, title)), h('span', { class: 'kbd' }, 'Esc')),
            body,
            fields.map((f) => f.type === 'checkbox'
                ? h('label', { class: 'row', style: { flexWrap: 'nowrap' } }, h('input', { type: 'checkbox', 'data-key': f.key, checked: values[f.key] ? true : undefined, onchange: (e) => { values[f.key] = e.target.checked; render(); } }), h('span', null, f.label))
                : f.options
                    ? h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, f.label), h('select', { 'data-key': f.key, onchange: (e) => { values[f.key] = e.target.value; render(); }, style: { height: '36px', background: 'transparent', color: 'inherit', border: 0 } },
                        f.options.map(([v, l]) => h('option', { value: v, selected: String(values[f.key]) === String(v) ? true : undefined }, l))), f.hint ? h('span', { class: 'muted', style: { fontSize: '12px' } }, f.hint) : null)
                    : h('label', { class: 'input-card' }, h('span', { class: 'kicker' }, f.label),
                        h('input', { 'data-key': f.key, type: f.type || 'text', value: String(values[f.key]), placeholder: f.placeholder || '', oninput: (e) => { values[f.key] = f.type === 'number' ? Number(e.target.value) : e.target.value; const b = box.querySelector('#form-submit'); if (b) b.disabled = !!validate(values) || busy; const p = box.querySelector('#form-problem'); if (p) p.textContent = validate(values); } }),
                        f.hint ? h('span', { class: 'muted', style: { fontSize: '12px' } }, f.hint) : null)),
            h('div', { id: 'form-problem', class: 'tone-warning', role: 'status', style: { fontSize: '12.5px', minHeight: '16px' } }, problem),
            error ? h('div', { class: 'banner error' }, error) : null,
            h('div', { class: 'row', style: { justifyContent: 'flex-end' } },
                h('button', { class: 'btn outlined st', onclick: closeLayer }, 'Cancel'),
                h('button', { class: 'btn filled st', id: 'form-submit', disabled: problem || busy ? true : undefined, onclick: async () => {
                    busy = true; error = ''; render();
                    try { await onSubmit({ ...values }); closeLayer(); } catch (e) { busy = false; error = String(e.message || e).replace(/^Error invoking remote method '[^']+': (Error: )?/, ''); render(); box.classList.add('shake'); }
                } }, busy ? 'Working…' : actionLabel)));
        if (focusKey) { const el = box.querySelector('[data-key="' + focusKey + '"]'); if (el) el.focus(); }
    };
    render();
    openLayer(h('div', { class: 'scrim', onclick: () => { if (!busy) closeLayer(); } }), box);
}
