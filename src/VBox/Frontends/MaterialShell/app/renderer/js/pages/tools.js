/*
 * Manager tools: Media, Network, Extensions, Logs and Activity.
 */

import { h, icon, ICONS, mount, stagger } from '../core.js';
import { store, api, notify, recordHistory } from '../services.js';
import { searchField, openExport, formDialog, confirmDestructive } from '../features/overlays.js';
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
            header(null, 'Media', h('button', { class: 'btn tonal st', onclick: () => formDialog({ title: 'Add an existing file', kicker: 'Media',
                fields: [{ key: 'location', label: 'File on this computer', placeholder: 'E:\\ISOs\\installer.iso' }, { key: 'kind', label: 'Type', value: media.kind, options: [['disk', 'Hard disk'], ['optical', 'Optical image'], ['floppy', 'Floppy image']] }],
                validate: (v) => v.location.trim() ? '' : 'Enter the full path.', actionLabel: 'Add',
                onSubmit: async (v) => { await api('media.action', { action: 'open', location: v.location.trim(), kind: v.kind }, { quiet: true }); recordHistory({ title: 'Medium added: ' + v.location.trim().split(/[\\/]/).pop(), scope: 'Media', category: 'Media', diff: [['+', v.location.trim()]] }); media.kind = v.kind; load(); } }) }, icon(ICONS.plus, 18), 'Add'), h('div', { class: 'segmented', role: 'tablist', 'aria-label': 'Media type' }, [['disk', 'Hard disks'], ['optical', 'Optical'], ['floppy', 'Floppy']].map(([k, l]) => h('button', { role: 'tab', class: 'st', 'aria-selected': String(media.kind === k), onclick: () => { media.kind = k; media.sel.clear(); load(); } }, l)))),
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
                    h('div', { class: 'card' }, h('div', { class: 'muted', style: { fontSize: '12px' } }, 'Attached to'), h('div', { style: { fontWeight: 700 } }, cur.attachedTo.join(', ') || 'Nothing')),
                    mediaActions(cur, load)) : null),
            media.sel.size ? h('div', { role: 'toolbar', 'aria-label': 'Bulk actions', class: 'pop', style: { position: 'sticky', bottom: '16px', alignSelf: 'center', display: 'flex', alignItems: 'center', gap: '8px', padding: '8px 8px 8px 20px', borderRadius: '32px', background: 'var(--on-surface)', color: 'var(--surface)', boxShadow: '0 20px 50px rgba(0,0,0,.55)' } },
                h('strong', null, media.sel.size + ' selected'),
                h('button', { class: 'btn st', onclick: () => openExport({ source: 'Selected media', rows: media.items.filter((m) => media.sel.has(m.id)).flatMap((m) => [['location', m.location], ['format', m.format], ['logicalSize', m.logicalSize], ['size', m.size], ['attachedTo', m.attachedTo.join('; ')]].map(([k, v]) => ({ section: m.name, key: k, value: String(v) }))), fileStem: 'media' }) }, 'Export list'),
                h('button', { class: 'btn st', onclick: () => { media.items.forEach((m) => media.sel.add(m.id)); draw(); } }, 'Select all ' + media.items.length),
                h('button', { class: 'icon-btn st', 'aria-label': 'Clear selection', style: { color: 'inherit' }, onclick: () => { media.sel.clear(); draw(); } }, icon(ICONS.close, 16))) : null);
    };
    draw();
    load();
}

function mediaActions(cur, reload) {
    const run = async (action, params, title, diff) => {
        await api('media.action', { action, mediumId: cur.id, ...params }, { quiet: true });
        recordHistory({ title: title + ': ' + cur.name, scope: 'Media', category: 'Media', diff });
        notify({ kind: 'success', title, body: cur.name });
        reload();
    };
    const wrap = (fn) => async () => { try { await fn(); } catch (e) { notify({ kind: 'error', title: 'That did not work', body: String(e.message).replace(/^Error invoking remote method '[^']+': (Error: )?/, '') }); } };
    const isDisk = media.kind === 'disk';
    const GB = 1024 * 1024 * 1024;
    return h('div', { class: 'grid g2' },
        isDisk ? h('button', { class: 'btn tonal st', onclick: () => formDialog({ title: 'Resize ' + cur.name, kicker: 'Currently ' + bytes(cur.logicalSize), fields: [{ key: 'sizeGB', label: 'New size (GB)', type: 'number', value: Math.ceil(cur.logicalSize / GB) + 10, hint: 'Disks can only grow. Extend the partition inside the guest afterwards.' }],
            validate: (v) => v.sizeGB * GB <= cur.logicalSize ? 'Choose a size larger than now.' : '', actionLabel: 'Resize', onSubmit: (v) => run('resize', { sizeGB: v.sizeGB }, 'Disk resized', [['-', bytes(cur.logicalSize)], ['+', v.sizeGB + ' GB']]) }) }, 'Resize') : null,
        isDisk ? h('button', { class: 'btn tonal st', onclick: wrap(() => run('compact', {}, 'Disk compacted', [['+', 'Compacted']])) }, 'Compact') : null,
        isDisk ? h('button', { class: 'btn tonal st', onclick: () => formDialog({ title: 'Copy ' + cur.name, fields: [{ key: 'target', label: 'New file', value: cur.location.replace(/(\.[^.\\/]+)$/, ' copy$1') }, { key: 'format', label: 'Format', value: 'VDI', options: [['VDI', 'VDI'], ['VMDK', 'VMDK'], ['VHD', 'VHD']] }],
            validate: (v) => !v.target.trim() ? 'Enter where the copy goes.' : v.target.trim() === cur.location ? 'Choose a different file.' : '', actionLabel: 'Copy', onSubmit: (v) => run('copy', { target: v.target.trim(), format: v.format }, 'Disk copied', [['+', v.target.trim()]]) }) }, 'Copy') : null,
        cur.attachedTo.length ? h('button', { class: 'btn outlined st', onclick: wrap(() => run('release', {}, 'Released', cur.attachedTo.map((n) => ['-', n]))) }, 'Release') : null,
        h('button', { class: 'btn st tone-error', onclick: () => confirmDestructive({ title: 'Remove ' + cur.name + '?', detail: cur.attachedTo.length ? 'Release it from ' + cur.attachedTo.join(', ') + ' first.' : isDisk ? 'The file is deleted from this computer.' : 'The image is removed from the list; the file stays.',
            actionLabel: isDisk ? 'Delete file' : 'Remove', onConfirm: () => run('remove', { deleteFile: isDisk }, isDisk ? 'Disk deleted' : 'Image removed', [['-', cur.location]]) }) }, isDisk ? 'Delete' : 'Remove'));
}

/* ---------------------------------------------------------------- network */

export async function renderNetwork(root) {
    const page = h('div', { class: 'page' });
    mount(root, page, h('div', { class: 'muted', style: { padding: '30px' } }, 'Loading networks…'));
    const n = await api('networks.list', {}, { quiet: true }).catch((e) => ({ error: e.message, natNetworks: [], hostInterfaces: [] }));
    const reload = () => renderNetwork(root);
    const clean = (e) => String(e.message || e).replace(/^Error invoking remote method '[^']+': (Error: )?/, '');
    const act = async (params, title, diff) => {
        const r = await api('networks.action', params, { quiet: true });
        recordHistory({ title, scope: 'Networks', category: 'Network', diff });
        notify({ kind: 'success', title, body: params.name || (r && r.name) || '' });
        reload();
        return r;
    };
    const safe = (fn) => async () => { try { await fn(); } catch (e) { notify({ kind: 'error', title: 'That did not work', body: clean(e) }); } };
    const ipv4 = /^(25[0-5]|2[0-4]\d|1?\d?\d)(\.(25[0-5]|2[0-4]\d|1?\d?\d)){3}$/;
    const nodes = [...n.natNetworks.map((x) => ({ kind: 'NAT network', name: x.name, cidr: x.cidr, color: 'var(--primary)', extra: (x.enabled === false ? 'Off · ' : '') + (x.dhcp ? 'DHCP on' : 'DHCP off') })),
        ...n.hostInterfaces.map((x) => ({ kind: x.kind === 'hostOnly' ? 'Host-only' : 'Bridged', name: x.name, cidr: x.ip + ' / ' + x.mask, color: x.kind === 'hostOnly' ? 'var(--tertiary)' : 'var(--success)', extra: x.dhcp ? 'DHCP' : 'Static' }))];
    const sw = (on, label, onClick) => h('button', { class: 'switch st', role: 'switch', 'aria-checked': String(!!on), 'aria-label': label, onclick: onClick }, h('span', { class: 'knob' }));
    const ruleRow = (net, r, i) => {
        const [name, proto, hostIp, hostPort, guestIp, guestPort] = r.split(':');
        return h('div', { class: 'row slide', style: { flexWrap: 'nowrap', padding: '6px 6px 6px 14px', borderRadius: '14px', background: 'var(--surface-container-high)', ...stagger(i, 40) } },
            h('strong', { class: 'grow' }, name), h('span', { class: 'mono muted', style: { fontSize: '12px' } }, proto.toUpperCase() + ' ' + (hostIp.replace(/[[\]]/g, '') || '*') + ':' + hostPort + ' → ' + guestIp.replace(/[[\]]/g, '') + ':' + guestPort),
            h('button', { class: 'icon-btn st tone-error', 'aria-label': 'Remove rule ' + name, onclick: safe(() => act({ action: 'natUpdate', name: net.name, rulesRemove: [{ name }] }, 'Port rule removed: ' + name, [['-', r]])) }, icon(ICONS.close, 16)));
    };
    mount(root, h('div', { class: 'page' },
        header(null, 'Networks',
            h('button', { class: 'btn tonal st', onclick: () => formDialog({ title: 'New NAT network', fields: [{ key: 'name', label: 'Name', placeholder: 'LabNet2' }, { key: 'cidr', label: 'Range', value: '10.0.' + (n.natNetworks.length + 3) + '.0/24' }, { key: 'dhcp', label: 'Run a DHCP server', type: 'checkbox', value: true }],
                validate: (v) => !v.name.trim() ? 'A name is required.' : n.natNetworks.some((x) => x.name === v.name.trim()) ? 'That name is taken.' : !/^\d+\.\d+\.\d+\.\d+\/\d{1,2}$/.test(v.cidr) ? 'Use a range like 10.0.3.0/24.' : '',
                actionLabel: 'Create', onSubmit: (v) => act({ action: 'natCreate', name: v.name.trim(), cidr: v.cidr, dhcp: v.dhcp }, 'NAT network created: ' + v.name.trim(), [['+', v.name.trim() + ' ' + v.cidr]]) }) }, icon(ICONS.plus, 18), 'NAT network'),
            h('button', { class: 'btn tonal st', onclick: safe(() => act({ action: 'hostOnlyCreate' }, 'Host-only adapter created', [['+', 'Host-only adapter']])) }, icon(ICONS.plus, 18), 'Host-only adapter')),
        n.error ? h('div', { class: 'banner error' }, n.error) : null,
        h('div', { role: 'group', 'aria-label': 'Topology', style: { position: 'relative', padding: '24px', borderRadius: '32px', background: 'radial-gradient(circle at 1px 1px, #2B2930 1px, transparent 1.5px) 0 0 / 24px 24px, #121016', border: '1px solid var(--surface-container-high)', display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '0' } },
            h('div', { class: 'pop', style: { padding: '10px 18px', borderRadius: '20px', background: 'var(--on-surface)', color: 'var(--surface)', fontWeight: 800 } }, 'This computer'),
            h('div', { 'aria-hidden': 'true', style: { width: '2px', height: '50px', background: 'repeating-linear-gradient(180deg, var(--primary) 0 8px, transparent 8px 20px)', backgroundSize: '2px 20px', animation: 'mvFlowV 1.2s linear infinite' } }),
            h('div', { class: 'grid', style: { gridTemplateColumns: 'repeat(auto-fit, minmax(220px, 1fr))', width: '100%' } }, nodes.length ? nodes.map((x, i) => h('div', { class: 'card lift pop', style: { borderRadius: '26px', borderTop: '3px solid ' + x.color, ...stagger(i, 120, 150) } },
                h('div', { class: 'kicker', style: { color: x.color } }, x.kind), h('div', { style: { fontSize: '18px', fontWeight: 800, margin: '4px 0' } }, x.name), h('div', { class: 'mono muted', style: { fontSize: '12px' } }, x.cidr), h('div', { class: 'muted', style: { fontSize: '12px', marginTop: '6px' } }, x.extra)))
                : [h('div', { class: 'empty' }, 'No NAT networks or host interfaces are configured.')])),
        h('h2', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'NAT networks'),
        n.natNetworks.length ? h('div', { class: 'grid g2' }, n.natNetworks.map((net, i) => h('section', { class: 'glass card rise', style: { borderRadius: '28px', display: 'flex', flexDirection: 'column', gap: '10px', ...stagger(i, 70) } },
            h('div', { class: 'row', style: { flexWrap: 'nowrap' } }, h('strong', { class: 'grow', style: { fontSize: '18px' } }, net.name), h('span', { class: 'muted' }, 'On'), sw(net.enabled !== false, 'Enable ' + net.name, safe(() => act({ action: 'natUpdate', name: net.name, enabled: net.enabled === false }, net.name + (net.enabled === false ? ' turned on' : ' turned off'), [['+', 'Enabled: ' + (net.enabled === false)]])))),
            h('div', { class: 'row' }, h('span', { class: 'mono' }, net.cidr), h('span', { class: 'grow' }), h('span', { class: 'muted' }, 'DHCP'), sw(net.dhcp, 'DHCP for ' + net.name, safe(() => act({ action: 'natUpdate', name: net.name, dhcp: !net.dhcp }, 'DHCP ' + (net.dhcp ? 'off' : 'on') + ': ' + net.name, [['+', 'DHCP: ' + !net.dhcp]])))),
            h('div', { class: 'kicker' }, 'Port forwarding'),
            (net.rules || []).length ? (net.rules || []).map((r, k) => ruleRow(net, r, k)) : h('span', { class: 'muted', style: { fontSize: '12.5px' } }, 'No rules.'),
            h('div', { class: 'row' },
                h('button', { class: 'btn tonal st', onclick: () => formDialog({ title: 'Forward a port', kicker: net.name,
                    fields: [{ key: 'name', label: 'Rule name', placeholder: 'ssh' }, { key: 'proto', label: 'Protocol', value: 'tcp', options: [['tcp', 'TCP'], ['udp', 'UDP']] }, { key: 'hostPort', label: 'Host port', type: 'number', value: 2222 },
                        { key: 'guestIp', label: 'Guest address', value: net.cidr.replace(/\.0\/\d+$/, '.15') }, { key: 'guestPort', label: 'Guest port', type: 'number', value: 22 }],
                    validate: (v) => !/^[\w-]{1,32}$/.test(v.name) ? 'Use letters, digits, - or _.' : (net.rules || []).some((r) => r.split(':')[0] === v.name) ? 'That rule name is taken.' : !ipv4.test(v.guestIp) ? 'Guest address must be IPv4.' : [v.hostPort, v.guestPort].some((p) => !(p >= 1 && p <= 65535)) ? 'Ports are 1 to 65535.' : '',
                    actionLabel: 'Add rule', onSubmit: (v) => act({ action: 'natUpdate', name: net.name, rulesAdd: [{ name: v.name, proto: v.proto, hostPort: v.hostPort, guestIp: v.guestIp, guestPort: v.guestPort }] }, 'Port rule added: ' + v.name, [['+', v.name + ' ' + v.hostPort + ' → ' + v.guestIp + ':' + v.guestPort]]) }) }, icon(ICONS.plus, 18), 'Rule'),
                h('button', { class: 'btn outlined st', onclick: () => formDialog({ title: 'Change the range', kicker: net.name, fields: [{ key: 'cidr', label: 'Range', value: net.cidr }], validate: (v) => /^\d+\.\d+\.\d+\.\d+\/\d{1,2}$/.test(v.cidr) ? '' : 'Use a range like 10.0.3.0/24.',
                    actionLabel: 'Save', onSubmit: (v) => act({ action: 'natUpdate', name: net.name, cidr: v.cidr }, 'Range changed: ' + net.name, [['-', net.cidr], ['+', v.cidr]]) }) }, 'Range'),
                h('span', { class: 'grow' }),
                h('button', { class: 'btn st tone-error', onclick: () => confirmDestructive({ title: 'Remove ' + net.name + '?', detail: 'Machines attached to it lose this network until you pick another.', actionLabel: 'Remove', onConfirm: () => act({ action: 'natRemove', name: net.name }, 'NAT network removed: ' + net.name, [['-', net.name + ' ' + net.cidr]]) }) }, 'Remove')))))
            : h('div', { class: 'empty' }, 'No NAT networks yet.'),
        h('h2', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'Host-only adapters'),
        n.hostInterfaces.some((x) => x.kind === 'hostOnly') ? h('div', { class: 'grid g2' }, n.hostInterfaces.filter((x) => x.kind === 'hostOnly').map((x, i) => h('section', { class: 'card rise row', style: { borderRadius: '24px', flexWrap: 'nowrap', ...stagger(i, 60) } },
            icon(ICONS.network), h('span', { class: 'grow' }, h('strong', null, x.name), h('div', { class: 'mono muted', style: { fontSize: '12px' } }, x.ip + ' / ' + x.mask)),
            h('button', { class: 'btn outlined st', onclick: () => formDialog({ title: 'Adapter address', kicker: x.name, fields: [{ key: 'ip', label: 'IPv4 address', value: x.ip }, { key: 'mask', label: 'Network mask', value: x.mask }],
                validate: (v) => ipv4.test(v.ip) && ipv4.test(v.mask) ? '' : 'Both must be IPv4 addresses.', actionLabel: 'Save',
                onSubmit: (v) => act({ action: 'hostOnlyConfigure', name: x.name, ip: v.ip, mask: v.mask }, 'Address changed: ' + x.name, [['-', x.ip + '/' + x.mask], ['+', v.ip + '/' + v.mask]]) }) }, 'Address'),
            h('button', { class: 'btn st tone-error', onclick: () => confirmDestructive({ title: 'Remove ' + x.name + '?', detail: 'The adapter is removed from this computer. Machines using it lose this network.', actionLabel: 'Remove', onConfirm: () => act({ action: 'hostOnlyRemove', name: x.name }, 'Host-only adapter removed', [['-', x.name]]) }) }, 'Remove'))))
            : h('div', { class: 'empty' }, 'No host-only adapters yet.')));
}

/* -------------------------------------------------------------- extensions */

export async function renderExtensions(root) {
    mount(root, h('div', { class: 'page' }, h('div', { class: 'muted' }, 'Loading extension packs…')));
    const packs = await api('extpacks.list', {}, { quiet: true }).catch(() => []);
    const reload = () => renderExtensions(root);
    const install = () => formDialog({ title: 'Install an extension pack', fields: [{ key: 'path', label: 'Extension pack file', placeholder: 'D:\\Downloads\\Oracle_VirtualBox_Extension_Pack.vbox-extpack' }],
        validate: (v) => /\.vbox-extpack$/i.test(v.path.trim()) ? '' : 'Choose a .vbox-extpack file.', actionLabel: 'Read the license',
        onSubmit: async (v) => {
            const info = await api('extpacks.action', { action: 'inspect', path: v.path.trim() }, { quiet: true });
            if (!info.usable) throw new Error('This pack cannot be used: ' + info.whyUnusable);
            setTimeout(() => formDialog({ title: 'Install ' + info.name + ' ' + info.version + '?', kicker: 'License',
                body: h('pre', { class: 'logbox', tabindex: '0', style: { margin: 0, padding: '14px 18px', maxHeight: '260px', whiteSpace: 'pre-wrap' } }, info.license || 'No license text was supplied.'),
                fields: [{ key: 'accept', label: 'I have read and accept the license', type: 'checkbox' }],
                validate: (x) => x.accept ? '' : 'Accept the license to install.', actionLabel: 'Install',
                onSubmit: async () => { await api('extpacks.action', { action: 'install', path: v.path.trim(), licenseAccepted: true }, { quiet: true });
                    recordHistory({ title: 'Extension pack installed: ' + info.name, scope: 'Extensions', category: 'Extensions', diff: [['+', info.name + ' ' + info.version]] });
                    notify({ kind: 'success', title: 'Installed', body: info.name + ' ' + info.version }); reload(); } }), 0);
        } });
    mount(root, h('div', { class: 'page' },
        header(null, 'Extensions', h('button', { class: 'btn filled st', onclick: install }, icon(ICONS.plus, 18), 'Install pack')),
        h('h2', { class: 'kicker', style: { color: 'var(--on-surface-variant)' } }, 'Extension packs'),
        packs.length ? packs.map((p, i) => h('section', { class: 'hero lift rise', style: stagger(i, 80) },
            h('span', { class: 'tile', style: { width: '64px', height: '64px', borderRadius: '22px', background: 'linear-gradient(135deg, var(--primary), var(--tertiary))', color: '#21005D' } }, icon('M21 16V8l-9-5-9 5v8l9 5 9-5zM3.3 7L12 12l8.7-5M12 22V12', 30)),
            h('div', { class: 'grow', style: { flex: '1 1 300px' } }, h('div', { style: { fontSize: '20px', fontWeight: 800 } }, p.name), h('div', { class: 'muted' }, 'Version ' + p.version + ' · ' + p.description)),
            h('span', { style: { padding: '8px 14px', borderRadius: '14px', fontWeight: 750, background: p.usable ? 'rgba(166,215,160,.16)' : 'rgba(242,184,181,.16)', color: p.usable ? 'var(--success)' : 'var(--error)', position: 'relative', zIndex: 1 } }, p.usable ? 'Usable' : 'Not usable: ' + p.whyUnusable),
            h('button', { class: 'btn st tone-error', style: { position: 'relative', zIndex: 1 }, onclick: () => confirmDestructive({ title: 'Uninstall ' + p.name + '?', detail: 'Machines that use its features (USB 2/3, disk encryption, PXE) lose them until it is reinstalled.', actionLabel: 'Uninstall',
                onConfirm: async () => { await api('extpacks.action', { action: 'uninstall', name: p.name }, { quiet: true }); recordHistory({ title: 'Extension pack removed: ' + p.name, scope: 'Extensions', category: 'Extensions', diff: [['-', p.name + ' ' + p.version]] }); notify({ kind: 'success', title: 'Uninstalled', body: p.name }); reload(); } }) }, 'Uninstall')))
            : h('div', { class: 'empty' }, 'No extension pack is installed.', h('button', { class: 'btn filled st', onclick: install }, 'Install pack')),
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
