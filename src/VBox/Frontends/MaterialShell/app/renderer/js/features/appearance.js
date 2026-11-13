/*
 * Per-element appearance editor.  Shift + right-click any card, hero, button,
 * chip or list item to change its colour, corner radius, text size and weight,
 * for that one element or for every element of its kind.  Overrides are kept
 * in local preferences and applied through one generated stylesheet, so the
 * theme defaults stay untouched and Reset removes them cleanly.
 */

import { h, mount, stagger } from '../core.js';
import { store, notify, recordHistory } from '../services.js';
import { persist } from '../core.js';

const KINDS = [['hero', 'hero card'], ['vm-item', 'machine row'], ['card', 'card'], ['btn', 'button'], ['chip', 'chip'], ['tile', 'tile'], ['segmented', 'segmented control']];
const COLOURS = ['', '#4F378B', '#633B48', '#1E3A5F', '#2E4A2C', '#5C3F1E', '#7D5260', '#3B3B48'];
const RADII = [['0', '0'], ['S', '8px'], ['L', '16px'], ['XL', '28px'], ['Full', '999px']];
const SIZES = [['S', '0.9'], ['M', '1'], ['L', '1.12'], ['XL', '1.25']];
const WEIGHTS = [['Regular', '400'], ['Medium', '550'], ['Bold', '750'], ['Black', '900']];

function sheet() {
    let el = document.getElementById('appearance-overrides');
    if (!el) { el = document.createElement('style'); el.id = 'appearance-overrides'; document.head.appendChild(el); }
    return el;
}

/** Rebuilds the override stylesheet from the stored overrides. */
export function applyAppearance(map = store.get().appearance || {}) {
    const rules = Object.entries(map).map(([sel, o]) => {
        const d = [];
        if (o.bg) d.push('background:' + o.bg + ' !important', 'color:#FFFFFF !important');
        if (o.radius) d.push('border-radius:' + o.radius + ' !important');
        if (o.size) d.push('font-size:' + o.size + 'em !important');
        if (o.weight) d.push('font-weight:' + o.weight + ' !important');
        return d.length ? sel + '{' + d.join(';') + '}' : '';
    });
    sheet().textContent = rules.join('\n');
}

function save(map) {
    store.set({ appearance: map });
    persist.save('appearance', map);
    applyAppearance(map);
}

function describe(el) {
    const kind = KINDS.find(([c]) => el.classList.contains(c));
    const head = el.querySelector('h1, h2, h3, strong');
    const label = (el.getAttribute('aria-label') || (head && head.textContent) || el.textContent || '').trim().replace(/\s+/g, ' ').slice(0, 48) || 'Element';
    return { kind, label, id: el.id || '' };
}

function open(el) {
    const info = describe(el);
    const scopes = [];
    if (info.id) scopes.push(['#' + CSS.escape(info.id), 'This element']);
    scopes.push(['.' + info.kind[0], 'Every ' + info.kind[1]]);
    const st = { scope: 0 };
    const before = JSON.parse(JSON.stringify(store.get().appearance || {}));
    const panel = h('section', { class: 'glass pop', role: 'dialog', 'aria-label': 'Appearance editor', style: { position: 'fixed', zIndex: 70, right: '20px', top: '70px', width: 'min(400px, calc(100vw - 40px))', padding: '20px', borderRadius: '30px', background: 'rgba(33,31,38,.96)', boxShadow: '0 30px 80px rgba(0,0,0,.6)', display: 'flex', flexDirection: 'column', gap: '14px' } });
    el.classList.add('appearance-target');
    const close = (commit) => {
        el.classList.remove('appearance-target');
        panel.remove();
        document.removeEventListener('keydown', onKey, true);
        if (!commit) return save(before);
        const after = store.get().appearance || {};
        if (JSON.stringify(after) !== JSON.stringify(before)) {
            recordHistory({ title: 'Appearance changed: ' + info.label, scope: 'Appearance', category: 'Appearance', diff: Object.keys({ ...before, ...after }).filter((k) => JSON.stringify(before[k]) !== JSON.stringify(after[k])).map((k) => ['+', k + ' ' + JSON.stringify(after[k] || 'default')]) });
            notify({ kind: 'success', title: 'Appearance saved', body: scopes[st.scope][1] });
        }
    };
    const onKey = (e) => { if (e.key === 'Escape') { e.preventDefault(); e.stopPropagation(); close(false); } };
    document.addEventListener('keydown', onKey, true);
    const set = (k, v) => { const sel = scopes[st.scope][0]; const map = { ...(store.get().appearance || {}) }; map[sel] = { ...(map[sel] || {}), [k]: v }; if (!v) delete map[sel][k]; if (!Object.keys(map[sel]).length) delete map[sel]; save(map); render(); };
    const row = (label, key, options) => {
        const cur = ((store.get().appearance || {})[scopes[st.scope][0]] || {})[key] || '';
        return h('div', { style: { display: 'flex', flexDirection: 'column', gap: '6px' } }, h('span', { class: 'kicker' }, label),
            h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': label }, [['Default', '']].concat(options).map(([l, v]) => h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(cur === v), style: { padding: '0 8px', fontSize: '12.5px' }, onclick: () => set(key, v) }, l))));
    };
    const render = () => {
        const cur = (store.get().appearance || {})[scopes[st.scope][0]] || {};
        mount(panel,
            h('div', null, h('div', { class: 'kicker' }, 'Appearance editor · ' + info.kind[1]), h('h2', { class: 'headline', style: { fontSize: '22px' } }, info.label)),
            h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Applies to' }, scopes.map(([, l], i) => h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(st.scope === i), onclick: () => { st.scope = i; render(); } }, l))),
            h('div', { style: { display: 'flex', flexDirection: 'column', gap: '6px' } }, h('span', { class: 'kicker' }, 'Colour'),
                h('div', { class: 'row', role: 'radiogroup', 'aria-label': 'Colour' }, COLOURS.map((c, i) => h('button', { class: 'st', role: 'radio', 'aria-checked': String((cur.bg || '') === c), 'aria-label': c || 'Theme default', style: { width: '36px', height: '36px', borderRadius: (cur.bg || '') === c ? '12px' : '18px', background: c || 'repeating-linear-gradient(45deg,#49454F 0 4px,#2B2930 4px 8px)', boxShadow: (cur.bg || '') === c ? '0 0 0 3px var(--primary)' : 'none', transition: 'border-radius 300ms var(--ease-spring)', ...stagger(i, 30) }, onclick: () => set('bg', c) })))),
            row('Corner radius', 'radius', RADII), row('Text size', 'size', SIZES), row('Weight', 'weight', WEIGHTS),
            h('div', { class: 'row', style: { justifyContent: 'flex-end' } },
                h('button', { class: 'btn outlined st', onclick: () => { const map = { ...(store.get().appearance || {}) }; delete map[scopes[st.scope][0]]; save(map); render(); } }, 'Reset element'),
                h('button', { class: 'btn outlined st', onclick: () => close(false) }, 'Cancel'),
                h('button', { class: 'btn filled st', onclick: () => close(true) }, 'Done')));
    };
    render();
    document.body.appendChild(panel);
    panel.querySelector('button').focus();
}

/** Every appearance override, removed at once (from Preferences). */
export function resetAllAppearance() {
    const before = store.get().appearance || {};
    if (!Object.keys(before).length) return;
    save({});
    recordHistory({ title: 'Appearance reset', scope: 'Appearance', category: 'Appearance', diff: Object.keys(before).map((k) => ['-', k]) });
}

export function installAppearanceEditor() {
    applyAppearance();
    document.addEventListener('contextmenu', (e) => {
        if (!e.shiftKey) return;
        const sel = KINDS.map(([c]) => '.' + c).join(',');
        const el = e.target.closest && e.target.closest(sel);
        if (!el || el.closest('[role=dialog]')) return;
        e.preventDefault();
        document.querySelectorAll('[aria-label="Appearance editor"]').forEach((x) => x.remove());
        open(el);
    }, true);
}
