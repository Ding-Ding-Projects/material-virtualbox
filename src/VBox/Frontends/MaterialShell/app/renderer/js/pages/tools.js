/*
 * Manager tools: Media, Network, Extensions, Logs and Activity.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, api, notify } from '../services.js';
import { searchField, openExport } from '../features/overlays.js';
import { bytes, parseLog, megabytes } from '../logic/format.js';
import { tileColors } from './machines.js';

const header = (kicker, title, ...right) => h('div', { class: 'spread' }, h('div', null, kicker ? h('span', { class: 'kicker' }, kicker) : null, h('h1', { class: 'display' }, title)), h('div', { class: 'row' }, right));

/* ------------------------------------------------------------------ media */

const media = { kind: 'disk', sel: new Set(), cur: null, matcher: () => true, items: [] };

export async function renderMedia(root) {
    const page = h('div', { class: 'page' });
    mount(root, page);
    const load = async () => { media.items = await api('media.list', { kind: media.kind }, { quiet: true }).catch(() => []); draw(); };
    const draw = () => {
        const items = media.items.filter((m) => media.matcher(m.name + ' ' + m.location + ' ' + m.format));
        const cur = items.find((m) => m.id === media.cur) || items[0];
        const total = media.items.reduce((a, m) => a + (m.logicalSize || 0), 0), used = media.items.reduce((a, m) => a + (m.size || 0), 0);
        const pct = cur && cur.logicalSize ? Math.round(cur.size / cur.logicalSize * 100) : 0;
        mount(page,
            header(null, 'Media', h('div', { class: 'segmented', role: 'tablist', 'aria-label': 'Media type' }, [['disk', 'Hard disks'], ['optical', 'Optical'], ['floppy', 'Floppy']].map(([k, l]) => h('button', { role: 'tab', class: 'st', 'aria-selected': String(media.kind === k), onclick: () => { media.kind = k; media.sel.clear(); load(); } }, l)))),
            h('div', { class: 'grid g4' }, [['Media', String(media.items.length)], ['Virtual size', bytes(total)], ['On host disk', bytes(used)], ['Unattached', String(media.items.filter((m) => !m.attachedTo.length).length)]].map(([k, v], i) => h('div', { class: 'card rise', style: { borderRadius: '24px', background: i === 3 ? 'var(--primary-container)' : undefined, ...stagger(i, 60) } }, h('div', { class: 'kicker', style: { color: i === 3 ? 'var(--on-primary-container)' : 'var(--on-surface-variant)' } }, k), h('div', { style: { fontSize: '34px', fontWeight: 850 } }, v)))),
            searchField({ placeholder: 'Search media by name, path or format', onChange: (m) => { media.matcher = m; draw(); }, samples: () => media.items.map((m) => m.name) }),
            h('div', { style: { display: 'flex', flexWrap: 'wrap', gap: '18px' } },
                h('div', { role: 'table', 'aria-label': 'Media', style: { flex: '999 1 520px', minWidth: 0, display: 'flex', flexDirection: 'column', gap: '6px' } },
                    items.length ? items.map((m, i) => h('div', { role: 'row', class: 'rise', style: { display: 'grid', gridTemplateColumns: '36px minmax(0,2fr) minmax(0,.7fr) minmax(0,1fr) minmax(0,1.3fr)', gap: '12px', alignItems: 'center', padding: '10px 14px', borderRadius: '18px', background: m === cur ? '#2B2340' : media.sel.has(m.id) ? 'var(--surface-container-high)' : 'transparent', ...stagger(i, 45) } },
                        h('input', { type: 'checkbox', 'aria-label': 'Select ' + m.name, checked: media.sel.has(m.id) ? true : undefined, onchange: (e) => { e.target.checked ? media.sel.add(m.id) : media.sel.delete(m.id); draw(); } }),
                        h('button', { class: 'st', role: 'cell', style: { textAlign: 'left', padding: '4px 8px', borderRadius: '10px' }, onclick: () => { media.cur = m.id; draw(); } }, h('div', { style: { fontWeight: 700 } }, m.name), h('div', { class: 'mono muted', style: { fontSize: '11.5px', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' } }, m.location)),
                        h('span', { role: 'cell' }, h('span', { class: 'mono', style: { padding: '4px 10px', borderRadius: '8px', background: 'var(--surface-container-highest)', fontSize: '12px' } }, m.format)),
                        h('span', { role: 'cell' }, h('div', { style: { fontWeight: 650 } }, bytes(m.logicalSize)), h('div', { style: { height: '6px', borderRadius: '3px', background: 'var(--surface-container-highest)', overflow: 'hidden', marginTop: '6px' } }, h('div', { style: { height: '100%', width: (m.logicalSize ? Math.min(100, m.size / m.logicalSize * 100) : 0) + '%', background: 'var(--primary)' } }))),
                        h('span', { role: 'cell', class: m.attachedTo.length ? 'muted' : 'tone-warning' }, m.attachedTo.join(', ') || 'Not attached')))
                        : h('div', { class: 'empty' }, media.items.length ? 'Nothing matches the search.' : 'No media of this type is registered.')),
                cur ? h('aside', { class: 'pane-in', style: { flex: '1 1 300px', maxWidth: '380px', padding: '22px', borderRadius: '32px', background: 'linear-gradient(170deg, #2B2340, #17151C)', display: 'flex', flexDirection: 'column', gap: '14px' } },
                    h('span', { class: 'kicker' }, 'Details'), h('h2', { class: 'headline', style: { fontSize: '26px' } }, cur.name),
                    h('div', { style: { position: 'relative', width: '180px', height: '180px', alignSelf: 'center' } },
                        h('svg', { width: 180, height: 180, viewBox: '0 0 120 120', role: 'img', 'aria-label': pct + '% of the virtual size is allocated' },
                            h('circle', { cx: 60, cy: 60, r: 48, fill: 'none', stroke: '#2B2930', 'stroke-width': 12 }),
                            h('circle', { cx: 60, cy: 60, r: 48, fill: 'none', stroke: 'var(--primary)', 'stroke-width': 12, 'stroke-linecap': 'round', 'stroke-dasharray': 302, 'stroke-dashoffset': Math.round(302 * (1 - pct / 100)), transform: 'rotate(-90 60 60)', style: 'transition: stroke-dashoffset 700ms var(--ease-emph-in)' })),
                        h('div', { style: { position: 'absolute', inset: 0, display: 'grid', placeItems: 'center', textAlign: 'center' } }, h('span', null, h('div', { style: { fontSize: '32px', fontWeight: 900 } }, pct + '%'), h('div', { class: 'muted', style: { fontSize: '12px' } }, 'of ' + bytes(cur.logicalSize))))),
                    h('div', { class: 'card' }, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Location'), h('div', { class: 'mono', style: { fontSize: '12px', wordBreak: 'break-all' } }, cur.location)),
                    h('div', { class: 'card' }, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Attached to'), h('div', { style: { fontWeight: 700 } }, cur.attachedTo.join(', ') || 'Nothing'))) : null),
            media.sel.size ? h('div', { role: 'toolbar', 'aria-label': 'Bulk actions', class: 'pop', style: { position: 'sticky', bottom: '16px', alignSelf: 'center', display: 'flex', alignItems: 'center', gap: '8px', padding: '8px 8px 8px 20px', borderRadius: '32px', background: 'var(--on-surface)', color: 'var(--surface)', boxShadow: '0 20px 50px rgba(0,0,0,.55)' } },
                h('strong', null, media.sel.size + ' selected'),
                h('button', { class: 'btn st', onclick: () => openExport({ source: 'Selected media', rows: media.items.filter((m) => media.sel.has(m.id)).flatMap((m) => [['location', m.location], ['format', m.format], ['logicalSize', m.logicalSize], ['size', m.size], ['attachedTo', m.attachedTo.join('; ')]].map(([k, v]) => ({ section: m.name, key: k, value: String(v) }))), fileStem: 'media' }) }, 'Export list'),
                h('button', { class: 'btn st', onclick: () => { media.items.forEach((m) => media.sel.add(m.id)); draw(); } }, 'Select all ' + media.items.length),
                h('button', { class: 'icon-btn st', 'aria-label': 'Clear selection', style: { color: 'inherit' }, onclick: () => { media.sel.clear(); draw(); } }, icon(ICONS.close, 16))) : null);
    };
    draw();
    load();
}

/* ---------------------------------------------------------------- network */

export async function renderNetwork(root) {
    const page = h('div', { class: 'page' });
    mount(root, page, h('div', { class: 'muted', style: { padding: '30px' } }, 'Loading networks…'));
    const n = await api('networks.list', {}, { quiet: true }).catch((e) => ({ error: e.message, natNetworks: [], hostInterfaces: [] }));
    const nodes = [...n.natNetworks.map((x) => ({ kind: 'NAT network', name: x.name, cidr: x.cidr, color: 'var(--primary)', extra: x.dhcp ? 'DHCP on' : 'DHCP off' })),
        ...n.hostInterfaces.map((x) => ({ kind: x.kind === 'hostOnly' ? 'Host-only' : 'Bridged', name: x.name, cidr: x.ip + ' / ' + x.mask, color: x.kind === 'hostOnly' ? 'var(--tertiary)' : 'var(--success)', extra: x.dhcp ? 'DHCP' : 'Static' }))];
    mount(root, h('div', { class: 'page' },
        header(null, 'Networks'),
        n.error ? h('div', { class: 'banner error' }, n.error) : null,
        h('div', { role: 'group', 'aria-label': 'Topology', style: { position: 'relative', padding: '24px', borderRadius: '32px', background: 'radial-gradient(circle at 1px 1px, #2B2930 1px, transparent 1.5px) 0 0 / 24px 24px, #121016', border: '1px solid var(--surface-container-high)', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '0' } },
            h('div', { class: 'pop', style: { padding: '10px 18px', borderRadius: '20px', background: 'var(--on-surface)', color: 'var(--surface)', fontWeight: 800 } }, 'This computer'),
            h('div', { 'aria-hidden': 'true', style: { width: '2px', height: '50px', background: 'repeating-linear-gradient(180deg, var(--primary) 0 8px, transparent 8px 20px)', backgroundSize: '2px 20px', animation: 'mvFlowV 1.2s linear infinite' } }),
            h('div', { class: 'grid', style: { gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', width: '100%' } }, nodes.length ? nodes.map((x, i) => h('div', { class: 'card lift pop', style: { borderRadius: '26px', borderTop: '3px solid ' + x.color, ...stagger(i, 120, 150) } },
                h('div', { class: 'kicker', style: { color: x.color } }, x.kind), h('div', { style: { fontSize: '18px', fontWeight: 800, margin: '4px 0' } }, x.name), h('div', { class: 'mono muted', style: { fontSize: '12px' } }, x.cidr), h('div', { class: 'muted', style: { fontSize: '12px', marginTop: '6px' } }, x.extra)))
                : [h('div', { class: 'empty' }, 'No NAT networks or host interfaces are configured.')]))));
}

/* -------------------------------------------------------------- extensions */

export async function renderExtensions(root) {
    mount(root, h('div', { class: 'page' }, h('div', { class: 'muted' }, 'Loading extension packs…')));
    const packs = await api('extpacks.list', {}, { quiet: true }).catch(() => []);
    mount(root, h('div', { class: 'page' },
        header(null, 'Extensions'),
        h('h2', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'Extension packs'),
        packs.length ? packs.map((p, i) => h('section', { class: 'hero lift rise', style: stagger(i, 80) },
            h('span', { class: 'tile', style: { width: '64px', height: '64px', borderRadius: '22px', background: 'linear-gradient(135deg, var(--primary), var(--tertiary))', color: '#21005D' } }, icon('M21 16V8l-9-5-9 5v8l9 5 9-5zM3.3 7L12 12l8.7-5M12 22V12', 30)),
            h('div', { class: 'grow', style: { flex: '1 1 300px' } }, h('div', { style: { fontSize: '20px', fontWeight: 800 } }, p.name), h('div', { class: 'muted' }, 'Version ' + p.version + ' · ' + p.description)),
            h('span', { style: { padding: '8px 14px', borderRadius: '14px', fontWeight: 750, background: p.usable ? 'rgba(166,215,160,.16)' : 'rgba(242,184,181,.16)', color: p.usable ? 'var(--success)' : 'var(--error)', position: 'relative', zIndex: 1 } }, p.usable ? 'Usable' : 'Not usable: ' + p.whyUnusable)))
            : h('div', { class: 'empty' }, 'No extension pack is installed.'),
        h('h2', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'App extensions'),
        h('div', { class: 'empty' }, h('strong', null, 'No app extensions are installed.'), 'App extensions run out of process with declared permissions. The manifest format and samples are described in the shell README.')));
}

/* -------------------------------------------------------------------- logs */

const logs = { idx: 0, sev: { info: true, notice: true, warning: true, error: true }, matcher: () => true, query: '' };

export async function renderLogs(root) {
    const s = store.get();
    const m = s.machines.find((x) => x.id === s.selectedId);
    if (!m) return mount(root, h('div', { class: 'page' }, h('div', { class: 'empty' }, 'Select a machine to read its logs.')));
    mount(root, h('div', { class: 'page' }, h('div', { class: 'muted' }, 'Reading the log…')));
    const lg = await api('logs.read', { id: m.id, index: logs.idx }, { quiet: true }).catch((e) => ({ error: e.message, text: '' }));
    const all = parseLog(lg.text);
    const page = h('div', { class: 'page' });
    const box = h('div', { class: 'logbox', role: 'log', 'aria-label': 'Log lines', style: { flex: 1, minHeight: '420px' } });
    const count = h('span', { style: { padding: '6px 12px', borderRadius: '12px', background: 'var(--primary-container)', color: 'var(--on-primary-container)', fontWeight: 750, fontSize: '12px' } });
    const draw = () => {
        let hits = 0;
        const rx = /^\/(.*)\/([imsu]*)$/.exec(logs.query.trim());
        let re = null;
        try { re = rx ? new RegExp(rx[1], rx[2].replace('g', '') + 'g') : (logs.query.trim() ? new RegExp(logs.query.trim().replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'gi') : null); } catch { re = null; }
        const rows = all.filter((r) => logs.sev[r.severity]).slice(-2000);
        mount(box, rows.map((r, i) => {
            let parts = [r.text];
            if (re) {
                parts = []; let last = 0; re.lastIndex = 0; let mm;
                while ((mm = re.exec(r.text)) && mm[0].length) { parts.push(r.text.slice(last, mm.index), h('mark', null, mm[0])); last = mm.index + mm[0].length; hits++; }
                parts.push(r.text.slice(last));
            }
            return h('div', { class: 'logline' }, h('span', { style: { color: '#625B71', width: '44px', textAlign: 'right' } }, String(i + 1)), h('span', { class: 'muted' }, r.time),
                h('span', { style: { width: '6px', borderRadius: '3px', background: { info: 'var(--info)', notice: 'var(--success)', warning: 'var(--warning)', error: 'var(--error)' }[r.severity] } }), h('span', { class: r.severity === 'error' ? 'tone-error' : '' }, parts));
        }));
        count.textContent = re ? hits + ' matches' : rows.length + ' lines';
    };
    mount(root, page);
    mount(page,
        header(m.name + ' · ' + (lg.file || 'VBox.log'), 'Logs', h('div', { class: 'segmented', role: 'tablist', 'aria-label': 'Log files' }, [0, 1, 2, 3].map((i) => h('button', { role: 'tab', class: 'st mono', 'aria-selected': String(logs.idx === i), onclick: () => { logs.idx = i; renderLogs(root); } }, i ? 'VBox.log.' + i : 'VBox.log')))),
        lg.error ? h('div', { class: 'banner warn' }, 'This log file does not exist yet: ' + lg.error) : null,
        h('div', { class: 'row' },
            h('div', { class: 'grow', style: { flex: '1 1 380px' } }, searchField({ placeholder: 'Search the log, or /regex/i', value: logs.query, onChange: (_m, q) => { logs.query = q; draw(); }, samples: () => all.slice(0, 12).map((r) => r.text) })), count,
            h('div', { class: 'row', role: 'group', 'aria-label': 'Severity' }, ['info', 'notice', 'warning', 'error'].map((sv) => h('button', { class: 'chip st', 'aria-pressed': String(logs.sev[sv]), onclick: (e) => { logs.sev[sv] = !logs.sev[sv]; e.currentTarget.setAttribute('aria-pressed', String(logs.sev[sv])); draw(); } }, h('span', { class: 'dot', style: { background: { info: 'var(--info)', notice: 'var(--success)', warning: 'var(--warning)', error: 'var(--error)' }[sv] } }), sv[0].toUpperCase() + sv.slice(1)))),
            h('button', { class: 'btn tonal st', onclick: () => openExport({ source: 'Log of ' + m.name, rows: all.map((r, i) => ({ section: 'line' + (i + 1), key: r.time || 'text', value: r.text })), fileStem: m.name + '-log' }) }, icon(ICONS.export, 18), 'Export')),
        box);
    draw();
}

/* ---------------------------------------------------------------- activity */

const act = { host: [], vms: {}, gen: 0 };

export function renderActivity(root) {
    const page = h('div', { class: 'page' });
    mount(root, page);
    const path = (arr, w, hgt) => arr.map((v, i) => (i ? 'L' : 'M') + (i * w / Math.max(1, arr.length - 1)).toFixed(1) + ' ' + (hgt - v / 100 * hgt).toFixed(1)).join(' ');
    const draw = (sample) => {
        const hostCpu = sample.find((x) => x.object === 'host' && x.metric.startsWith('CPU'));
        const hostRam = sample.find((x) => x.object === 'host' && x.metric.startsWith('RAM'));
        act.host = [...act.host, hostCpu ? hostCpu.value : 0].slice(-60);
        for (const x of sample.filter((x) => x.object !== 'host' && x.metric.startsWith('CPU'))) act.vms[x.object] = [...(act.vms[x.object] || []), x.value].slice(-40);
        const running = store.get().machines.filter((m) => m.state === 'running');
        const hostLine = path(act.host, 1000, 180);
        mount(page,
            h('div', { class: 'spread' }, h('div', null, h('span', { class: 'kicker row', style: { color: 'var(--success)', gap: '8px' } }, h('span', { class: 'dot bg-success pulse' }), 'Live'), h('h1', { class: 'display' }, 'Activity'))),
            h('section', { class: 'card', 'aria-label': 'Host', style: { padding: '24px', borderRadius: '28px', background: 'var(--hero)' } },
                h('div', { class: 'row', style: { gap: '32px', width: '100%' } },
                    h('div', null, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Host CPU'), h('div', { style: { fontSize: '60px', lineHeight: 1, fontWeight: 900, color: 'var(--primary)' } }, Math.round(act.host[act.host.length - 1] || 0) + '%')),
                    h('div', null, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Host memory used'), h('div', { style: { fontSize: '60px', lineHeight: 1, fontWeight: 900, color: 'var(--tertiary)' } }, hostRam ? megabytes(Math.round(hostRam.value / 1024)) : '…')),
                    h('div', null, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Running machines'), h('div', { style: { fontSize: '60px', lineHeight: 1, fontWeight: 900, color: 'var(--success)' } }, String(running.length)))),
                h('svg', { viewBox: '0 0 1000 180', preserveAspectRatio: 'none', role: 'img', 'aria-label': 'Host CPU over the last minute', style: 'width:100%;height:180px;' },
                    h('defs', null, h('linearGradient', { id: 'mva', x1: 0, y1: 0, x2: 0, y2: 1 }, h('stop', { offset: 0, 'stop-color': 'var(--primary)', 'stop-opacity': .45 }), h('stop', { offset: 1, 'stop-color': 'var(--primary)', 'stop-opacity': 0 }))),
                    h('path', { d: hostLine + ' L1000 180 L0 180 Z', fill: 'url(#mva)' }), h('path', { d: hostLine, fill: 'none', stroke: 'var(--primary)', 'stroke-width': 3, 'stroke-linejoin': 'round' }))),
            h('div', { class: 'grid g3' }, running.length ? running.map((m) => {
                const arr = act.vms[m.id] || [0];
                const [bg, fg] = tileColors(m.name);
                return h('section', { class: 'card lift', style: { borderRadius: '26px' } },
                    h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('span', { class: 'tile', style: { width: '36px', height: '36px', background: bg, color: fg } }, m.name[0]), h('strong', { class: 'grow' }, m.name), h('span', { style: { fontSize: '26px', fontWeight: 850, color: 'var(--primary)' } }, Math.round(arr[arr.length - 1]) + '%')),
                    h('svg', { viewBox: '0 0 300 70', preserveAspectRatio: 'none', role: 'img', 'aria-label': m.name + ' CPU trend', style: 'width:100%;height:70px' }, h('path', { d: path(arr, 300, 70), fill: 'none', stroke: 'var(--primary)', 'stroke-width': 2.5 })),
                    h('div', { class: 'muted', style: { fontSize: '12px' } }, m.cpus + ' vCPU · ' + megabytes(m.memoryMB)));
            }) : [h('div', { class: 'empty', style: { gridColumn: '1 / -1' } }, 'No machine is running. Start one to see its live load here.')]));
    };
    const gen = ++act.gen;
    const loop = async () => {
        if (gen !== act.gen || store.get().route !== 'activity' || !document.contains(page)) return;
        const sample = await api('metrics.sample', {}, { quiet: true }).catch(() => []);
        if (gen !== act.gen) return;
        draw(sample);
        setTimeout(loop, 1000);
    };
    draw([]);
    loop();
}
