/*
 * Shell-level pages: History, Preferences, Tab manager and First-run setup.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, api, notify, recordHistory, setPrefs, SEEDS } from '../services.js';
import { searchField, openExport } from '../features/overlays.js';
import { relativeTime, megabytes } from '../logic/format.js';
import { filterTabs, resolveCloseSet } from '../logic/tabs-model.js';
import { refreshMachines } from './machines.js';

/* ---------------------------------------------------------------- history */

const hist = { cat: 'All', matcher: () => true, cur: null };

export function renderHistory(root) {
    const all = store.get().history;
    const cats = ['All', ...new Set(all.map((r) => r.category))];
    const shown = all.filter((r) => (hist.cat === 'All' || r.category === hist.cat) && hist.matcher(r.title + ' ' + r.scope));
    const cur = shown.find((r) => r.id === hist.cur) || shown[0];
    const restore = async (r) => {
        if (!r.undo) return;
        try {
            await api(r.undo.method, r.undo.params);
            recordHistory({ title: 'Restored: ' + r.title, scope: r.scope, category: 'Restore', diff: r.diff.map(([s, t]) => [s === '+' ? '-' : s === '-' ? '+' : s, t]) });
            notify({ kind: 'success', title: 'Restored as a new revision', body: r.title });
            await refreshMachines();
            renderHistory(root);
        } catch { /* notified */ }
    };
    mount(root, h('div', { class: 'page' },
        h('div', { class: 'spread' }, h('div', null, h('span', { class: 'kicker' }, 'Ctrl+H · append-only'), h('h1', { class: 'display' }, 'History')),
            h('div', { class: 'row' }, cats.map((c) => h('button', { class: 'chip st', 'aria-pressed': String(hist.cat === c), onclick: () => { hist.cat = c; renderHistory(root); } }, c, ' ', h('span', { class: 'muted' }, String(c === 'All' ? all.length : all.filter((r) => r.category === c).length)))))),
        searchField({ placeholder: 'Search history', onChange: (m) => { hist.matcher = m; renderHistory(root); }, samples: () => all.slice(0, 12).map((r) => r.title) }),
        all.length ? h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: '22px' } },
            h('ol', { 'aria-label': 'Revisions', style: { flex: '999 1 480px', minWidth: 0, position: 'relative', margin: 0, padding: '0 0 0 34px', listStyle: 'none', display: 'flex', flexDirection: 'column', gap: '8px' } },
                h('span', { 'aria-hidden': 'true', style: { position: 'absolute', left: '13px', top: '8px', bottom: '8px', width: '3px', borderRadius: '2px', background: 'linear-gradient(180deg, var(--success), var(--primary) 30%, var(--outline-variant))' } }),
                shown.slice(0, 300).map((r, i) => h('li', { class: 'slide', style: { position: 'relative', ...stagger(Math.min(i, 20), 40) } },
                    h('span', { 'aria-hidden': 'true', style: { position: 'absolute', left: '-28px', top: '18px', width: '16px', height: '16px', borderRadius: '8px', border: '3px solid var(--bg)', background: r === cur ? 'var(--primary)' : r.category === 'Restore' ? 'var(--success)' : '#625B71' } }),
                    h('button', { class: 'st', 'aria-pressed': String(r === cur), style: { width: '100%', display: 'flex', alignItems: 'center', gap: '12px', padding: '14px 16px', borderRadius: '20px', textAlign: 'left', background: r === cur ? '#2B2340' : 'var(--surface-container-low)' }, onclick: () => { hist.cur = r.id; renderHistory(root); } },
                        h('span', { class: 'grow' }, h('strong', null, r.title), h('div', { class: 'muted', style: { fontSize: '12px' } }, relativeTime(r.at) + ' · ' + r.scope)),
                        h('span', { class: 'chip', style: { height: '26px' } }, r.category))))),
            cur ? h('aside', { class: 'pane-in', style: { flex: '1 1 340px', maxWidth: '440px', display: 'flex', flexDirection: 'column', gap: '12px' } },
                h('div', { class: 'card', style: { padding: '22px', borderRadius: '30px', display: 'flex', flexDirection: 'column', gap: '12px' } },
                    h('span', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'What changed'), h('h2', { class: 'title', style: { fontSize: '22px' } }, cur.title),
                    h('div', { class: 'mono', style: { fontSize: '13px', borderRadius: '16px', overflow: 'hidden' } }, cur.diff.map(([sign, text]) => h('div', { style: { display: 'flex', gap: '10px', padding: '6px 12px', background: sign === '+' ? '#17261A' : sign === '-' ? '#2A1416' : 'var(--surface-container)', color: sign === '+' ? '#C4EEBE' : sign === '-' ? '#FFDAD6' : 'var(--on-surface-variant)' } }, h('span', null, sign), h('span', null, text)))),
                    h('div', { class: 'row' },
                        h('button', { class: 'btn filled st grow', disabled: cur.undo ? undefined : true, title: cur.undo ? '' : 'This revision has nothing to restore', onclick: () => restore(cur) }, 'Restore as new revision'),
                        h('button', { class: 'btn outlined st', onclick: () => openExport({ source: 'History', rows: shown.map((r) => ({ section: new Date(r.at).toISOString(), key: r.category, value: r.title })), fileStem: 'history' }) }, 'Export')))) : null)
            : h('div', { class: 'empty' }, h('strong', null, 'Nothing has changed yet.'), 'Starts, settings changes, snapshots and deletions are recorded here, newest first.')));
}

/* ------------------------------------------------------------ preferences */

export function renderPrefs(root) {
    const p = store.get().prefs;
    const toggle = (key, label, sub) => h('section', { class: 'card lift rise', style: { gridColumn: 'span 3', display: 'flex', alignItems: 'center', gap: '14px', borderRadius: '24px' } },
        h('span', { class: 'grow' }, h('strong', null, label), h('div', { class: 'muted', style: { fontSize: '12.5px' } }, sub)),
        h('button', { class: 'switch st', role: 'switch', 'aria-checked': String(!!p[key]), 'aria-label': label, onclick: () => { setPrefs({ [key]: !p[key] }); renderPrefs(root); } }, h('span', { class: 'knob' })));
    mount(root, h('div', { class: 'page' },
        h('h1', { class: 'display' }, 'Preferences'),
        h('div', { class: 'grid g6' },
            h('section', { class: 'card rise', id: 'pref-seed', tabindex: '-1', style: { gridColumn: 'span 6', borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '14px' } },
                h('h2', { class: 'title' }, 'Theme seed'),
                h('div', { class: 'row', style: { gap: '12px' } }, SEEDS.map((s, i) => h('button', { class: 'st', 'aria-label': 'Seed ' + s.hex, 'aria-pressed': String(p.seed === i), style: { width: '56px', height: '56px', borderRadius: p.seed === i ? '18px' : '28px', background: s.hex, boxShadow: p.seed === i ? '0 0 0 4px var(--bg), 0 0 0 7px ' + s.primary : 'none', transition: 'border-radius 400ms var(--ease-spring), box-shadow 300ms', animation: 'mvPop 600ms var(--ease-spring) both', animationDelay: i * 50 + 'ms' }, onclick: () => { setPrefs({ seed: i }); renderPrefs(root); } })))),
            h('section', { class: 'card rise', style: { gridColumn: 'span 3', borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '10px' } },
                h('h2', { class: 'title' }, 'Language'),
                h('div', { role: 'radiogroup', 'aria-label': 'Language', style: { display: 'flex', flexDirection: 'column', gap: '8px' } }, [['en', 'A', 'English'], ['yue', '粵', '廣東話 (playful)'], ['both', 'A粵', 'Bilingual compact']].map(([id, glyph, label]) => h('button', { class: 'st', role: 'radio', 'aria-checked': String(p.language === id), style: { display: 'flex', alignItems: 'center', gap: '12px', height: '52px', padding: '0 16px', borderRadius: '18px', textAlign: 'left', fontWeight: 700, background: p.language === id ? 'var(--primary-container)' : 'var(--surface-container-high)', color: p.language === id ? 'var(--on-primary-container)' : 'var(--on-surface)' }, onclick: () => { setPrefs({ language: id }); renderPrefs(root); } }, h('span', { style: { width: '36px', fontSize: '18px' } }, glyph), label)))),
            h('section', { class: 'card rise', id: 'pref-motion', tabindex: '-1', style: { gridColumn: 'span 3', borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '12px' } },
                h('h2', { class: 'title' }, 'Motion'),
                h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Animation level' }, [['off', 'Off'], ['calm', 'Calm'], ['lively', 'Lively'], ['super', 'Super']].map(([id, label]) => h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(p.motion === id), onclick: () => { setPrefs({ motion: id }); renderPrefs(root); } }, label))),
                h('div', { class: 'row', 'aria-hidden': 'true', style: { height: '50px', alignItems: 'flex-end' } }, [0, 1, 2, 3, 4].map((i) => h('span', { style: { width: '20px', height: '20px', borderRadius: '10px', background: 'var(--primary)', animation: 'mvPop 900ms var(--ease-spring) infinite alternate', animationDelay: i * 120 + 'ms' } }))),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'The system reduced-motion setting always wins. No state is shown by motion alone.')),
            h('section', { class: 'card rise', id: 'pref-folder', tabindex: '-1', style: { gridColumn: 'span 6', borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '10px' } },
                h('h2', { class: 'title' }, 'Default machine folder'),
                h('div', { class: 'row', style: { flexWrap: 'nowrap' } },
                    h('label', { class: 'field grow', style: { height: '44px' } }, h('span', { class: 'sr-only' }, 'Default machine folder'), h('input', { id: 'pref-folder-input', value: store.get().defaultMachineFolder || '', placeholder: 'D:\\VMs' })),
                    h('button', { class: 'btn filled st', onclick: async () => {
                        const v = root.querySelector('#pref-folder-input').value.trim();
                        const before = store.get().defaultMachineFolder || '';
                        if (!v || v === before) return;
                        try {
                            const r = await api('system.properties', { defaultMachineFolder: v });
                            store.set({ defaultMachineFolder: r.defaultMachineFolder });
                            recordHistory({ title: 'Default machine folder changed', scope: 'Preferences', category: 'Preferences', diff: [['-', before], ['+', r.defaultMachineFolder]], undo: before ? { method: 'system.properties', params: { defaultMachineFolder: before } } : null });
                            notify({ kind: 'success', title: 'Saved', body: r.defaultMachineFolder });
                        } catch { /* notified */ }
                    } }, 'Save')),
                h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'New machines, clones and imports go here. Stored by VirtualBox for every front end on this computer.')),
            toggle('lowStim', 'Low stimulation', 'Calmer motion and solid surfaces'),
            toggle('school', 'School mode', 'Plain wording everywhere'),
            toggle('emoji', 'Dialog emojis', 'Playful emoji on confirmations'),
            h('section', { class: 'card rise', style: { gridColumn: 'span 3', display: 'flex', alignItems: 'center', gap: '14px', borderRadius: '24px' } },
                h('span', { class: 'grow' }, h('strong', null, 'First-run setup'), h('div', { class: 'muted', style: { fontSize: '12.5px' } }, 'Walk through the checks again')),
                h('button', { class: 'btn tonal st', onclick: () => store.set({ route: 'onboarding' }) }, 'Open')))));
}

/* ------------------------------------------------------------ tab manager */

const tm = { q: { title: '', notTitle: '', group: '', state: '' }, mode: 'containing' };

export function renderTabManager(root, tabsApi) {
    const tabs = tabsApi.describe();
    const shown = filterTabs(tabs, tm.q);
    const set = resolveCloseSet(shown, tm.q.title, tm.mode, { pinned: tabsApi.pinned(), activeId: store.get().route });
    const fields = [['title', 'Title contains'], ['notTitle', 'Title does not contain'], ['group', 'Group'], ['state', 'Machine state']];
    mount(root, h('div', { class: 'page' },
        h('div', { class: 'spread' }, h('h1', { class: 'display' }, 'Tab manager'), h('span', { class: 'kbd' }, 'Ctrl+Shift+T')),
        h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: '20px' } },
            h('main', { style: { flex: '999 1 520px', minWidth: 0, display: 'flex', flexDirection: 'column', gap: '12px' } },
                h('div', { class: 'grid g2' }, fields.map(([k, label]) => h('div', { 'data-k': k }, h('div', { class: 'kicker', style: { marginBottom: '6px' } }, label),
                    searchField({ placeholder: 'Any', value: tm.q[k], samples: () => tabs.map((t) => t.title), onChange: (_m, q) => {
                        tm.q[k] = q;
                        renderTabManager(root, tabsApi);
                        const el = root.querySelector('[data-k="' + k + '"] input');
                        if (el) { el.focus(); el.setSelectionRange(el.value.length, el.value.length); }
                    } })))),
                h('div', { role: 'list', 'aria-label': 'Matching tabs', style: { display: 'flex', flexDirection: 'column', gap: '6px' } }, shown.map((t, i) => {
                    const closing = set.closing.includes(t.id);
                    return h('div', { role: 'listitem', class: 'rise', style: { display: 'flex', alignItems: 'center', gap: '12px', padding: '10px 14px', borderRadius: '16px', background: closing ? '#2A1A1C' : 'var(--surface-container-low)', ...stagger(i, 35) } },
                        h('span', { class: 'grow', style: { fontWeight: 650, textDecoration: closing ? 'line-through' : 'none' } }, t.title), h('span', { class: 'muted', style: { fontSize: '12px' } }, t.group || ''),
                        h('span', { style: { fontSize: '11px', fontWeight: 800, padding: '4px 8px', borderRadius: '8px', background: t.pinned ? 'var(--primary-container)' : closing ? 'var(--error-container)' : 'var(--surface-container-highest)' } }, t.pinned ? 'Pinned' : closing ? 'Will close' : 'Stays'),
                        h('button', { class: 'btn st', style: { height: '34px' }, onclick: () => tabsApi.open(t.id) }, 'Go'));
                }))),
            h('aside', { style: { flex: '1 1 320px', maxWidth: '400px' } },
                h('div', { style: { padding: '22px', borderRadius: '30px', background: 'linear-gradient(160deg, #3A1C1A, var(--surface-container-low))', display: 'flex', flexDirection: 'column', gap: '14px' } },
                    h('span', { class: 'kicker', style: { color: 'var(--error)' } }, 'Preview, then authorise'),
                    h('div', { class: 'segmented', role: 'radiogroup', 'aria-label': 'Close mode' }, [['containing', 'Close containing'], ['notContaining', 'Close not containing']].map(([id, label]) => h('button', { class: 'st grow', role: 'radio', 'aria-checked': String(tm.mode === id), onclick: () => { tm.mode = id; renderTabManager(root, tabsApi); } }, label))),
                    h('div', { style: { fontSize: '72px', lineHeight: 1, fontWeight: 900, color: '#FFDAD6' } }, String(set.closing.length)),
                    h('span', null, 'tabs will close, using the title search above. Pinned tabs and the current tab stay open.'),
                    h('button', { class: 'btn danger big st', disabled: set.closing.length ? undefined : true, onclick: () => { tabsApi.closeMany(set.closing); renderTabManager(root, tabsApi); } }, 'Close ' + set.closing.length + ' tabs'),
                    h('button', { class: 'btn outlined st', onclick: () => { tabsApi.openEvery(); renderTabManager(root, tabsApi); } }, 'Open every tab'))))));
}

/* ------------------------------------------------------- first-run setup */

const ob = { step: 0, skipped: [] };

export async function renderOnboarding(root) {
    const s = store.get();
    const host = s.host;
    const status = s.status;
    const steps = [
        { title: 'Make it yours', body: 'Pick a language, a seed colour and how lively the motion should be. Change any of it later in Preferences.',
          tasks: [['Language', { en: 'English', yue: '廣東話', both: 'Bilingual' }[s.prefs.language], 1], ['Theme seed', SEEDS[s.prefs.seed].hex, 1], ['Motion', s.prefs.motion, 1]],
          extra: h('button', { class: 'btn tonal st', style: { alignSelf: 'flex-start' }, onclick: () => store.set({ route: 'prefs' }) }, 'Open Preferences') },
        { title: 'Check this computer', body: 'Real checks against the VirtualBox service. Anything amber explains what to do next.',
          tasks: [['VirtualBox service', status.mode === 'bridge' ? (host ? 'Connected, version ' + host.version : 'Not answering yet') : status.mode === 'demo' ? 'Demo mode: no real service' : 'Not connected: ' + status.reason, host ? 1 : 2],
            ['Host memory', host ? megabytes(host.memoryMB) + ' total, ' + megabytes(host.memoryAvailableMB) + ' free' : 'Unknown', host ? 1 : 2],
            ['Hypervisor driver', 'Unsigned in this build: Windows loads it only if you allow unsigned drivers', 2]] },
        { title: 'Machines on this computer', body: 'Machines already registered with VirtualBox show up in the library straight away.',
          tasks: [['Registered machines', s.machines.length + ' found', 1]] },
        { title: 'You are set', body: 'Open the library, or create your first machine.', tasks: [] }];
    const st = steps[ob.step];
    const finish = () => { setPrefsDone(); store.set({ route: 'machines' }); };
    mount(root, h('div', { style: { display: 'flex', flexWrap: 'wrap', minHeight: '100%' } },
        h('div', { style: { flex: '1 1 380px', maxWidth: '520px', minHeight: '640px', position: 'relative', display: 'grid', placeItems: 'center', background: 'linear-gradient(170deg, #2B2340, #120F18)' } },
            h('div', { 'aria-hidden': 'true', style: { width: '300px', height: '300px', background: 'linear-gradient(135deg, var(--primary), var(--tertiary) 60%, var(--warning))', animation: 'mvMorph 9s ease-in-out infinite, mvSpin 40s linear infinite', borderRadius: '42%', opacity: .9 } }),
            h('div', { style: { position: 'absolute', textAlign: 'center', color: '#1D1B20' } }, h('div', { style: { fontSize: '96px', fontWeight: 900, lineHeight: 1 } }, String(ob.step + 1)), h('strong', null, 'of ' + steps.length))),
        h('main', { class: 'page', style: { flex: '999 1 520px', padding: '56px' } },
            h('div', { class: 'pane-in', style: { display: 'flex', flexDirection: 'column', gap: '20px' } },
                h('span', { class: 'kicker' }, 'Step ' + (ob.step + 1)), h('h1', { class: 'display', style: { fontSize: '60px' } }, st.title), h('p', { class: 'muted', style: { fontSize: '16px', maxWidth: '560px', margin: 0 } }, st.body),
                st.tasks.map(([label, result, ok], i) => h('div', { class: 'card rise', style: { display: 'flex', alignItems: 'center', gap: '14px', borderRadius: '20px', ...stagger(i, 160, 150) } },
                    h('span', { style: { width: '32px', height: '32px', borderRadius: '16px', display: 'grid', placeItems: 'center', background: ok === 1 ? 'var(--success)' : 'var(--warning)', color: '#0B0A0F' } }, icon(ok === 1 ? ICONS.check : ICONS.info, 18)),
                    h('span', { class: 'grow' }, h('strong', null, label), h('div', { class: 'muted', style: { fontSize: '12.5px' } }, result)))),
                st.extra || null),
            h('div', { class: 'grow' }),
            h('div', { class: 'row' },
                h('button', { class: 'btn outlined big st', disabled: ob.step === 0 ? true : undefined, onclick: () => { ob.step--; renderOnboarding(root); } }, 'Back'),
                ob.step < steps.length - 1 ? h('button', { class: 'btn big st', onclick: () => { ob.skipped.push(ob.step); ob.step++; renderOnboarding(root); } }, 'Skip this step') : null,
                h('span', { class: 'grow' }), h('span', { class: 'muted', style: { fontSize: '12px' } }, 'Progress saves as you go'),
                ob.step === steps.length - 1 ? h('button', { class: 'btn tonal big st', onclick: () => { setPrefsDone(); store.set({ route: 'newvm' }); } }, 'Create a machine') : null,
                h('button', { class: 'btn filled big st', onclick: () => { if (ob.step === steps.length - 1) finish(); else { ob.step++; renderOnboarding(root); } } }, ob.step === steps.length - 1 ? 'Open the library' : 'Next')))));
}

function setPrefsDone() {
    setPrefs({ onboarded: true });
}

