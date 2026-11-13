/*
 * Demo provider: answers the bridge protocol from sample data so the shell
 * can be explored, captured and tested on a machine without VirtualBox.
 * It only runs when started with --demo; the renderer shows a banner on
 * every screen while it is active, so sample data is never mistaken for real.
 */
'use strict';

const now = Date.now();
const H = 3600 * 1000;

const machines = [
    { id: '7c1d5a0e-1b2c-4d3e-8f90-0a1b2c3d4e5f', name: 'Windows 11 Dev Box', osTypeId: 'Windows11_64', state: 'running', memoryMB: 16384, cpus: 8, snapshotCount: 3, lastStateChangeMs: now - 1.7 * H, groups: ['/Development'], accessible: true },
    { id: '2a3b4c5d-6e7f-4a8b-9c0d-1e2f3a4b5c6d', name: 'Ubuntu 24.04 Server', osTypeId: 'Ubuntu_64', state: 'running', memoryMB: 8192, cpus: 4, snapshotCount: 1, lastStateChangeMs: now - 5 * H, groups: ['/Lab'], accessible: true },
    { id: '3b4c5d6e-7f8a-4b9c-8d1e-2f3a4b5c6d7e', name: 'Fedora Workstation', osTypeId: 'Fedora_64', state: 'saved', memoryMB: 6144, cpus: 4, snapshotCount: 2, lastStateChangeMs: now - 26 * H, groups: ['/Lab'], accessible: true },
    { id: '4c5d6e7f-8a9b-4c0d-9e2f-3a4b5c6d7e8f', name: 'Debian Build Agent', osTypeId: 'Debian_64', state: 'poweredOff', memoryMB: 4096, cpus: 2, snapshotCount: 0, lastStateChangeMs: now - 72 * H, groups: ['/Build'], accessible: true },
    { id: '5d6e7f8a-9b0c-4d1e-8f3a-4b5c6d7e8f9a', name: 'FreeBSD Router', osTypeId: 'FreeBSD_64', state: 'aborted', memoryMB: 1024, cpus: 1, snapshotCount: 0, lastStateChangeMs: now - 0.9 * H, groups: ['/Network'], accessible: true }
];

const GB = 1024 * 1024 * 1024;
const media = [
    { id: 'm1', name: 'Dev Box.vdi', location: 'D:\\VMs\\Dev Box\\Dev Box.vdi', format: 'VDI', logicalSize: 80 * GB, size: 49.6 * GB, attachedTo: ['Windows 11 Dev Box'] },
    { id: 'm2', name: 'ubuntu-server.vdi', location: 'D:\\VMs\\Ubuntu\\ubuntu-server.vdi', format: 'VDI', logicalSize: 40 * GB, size: 16.4 * GB, attachedTo: ['Ubuntu 24.04 Server'] },
    { id: 'm3', name: 'fedora.vmdk', location: 'E:\\Lab\\Fedora\\fedora.vmdk', format: 'VMDK', logicalSize: 50 * GB, size: 17.5 * GB, attachedTo: ['Fedora Workstation'] },
    { id: 'm4', name: 'debian-agent.vdi', location: 'D:\\VMs\\Debian\\debian-agent.vdi', format: 'VDI', logicalSize: 20 * GB, size: 4.4 * GB, attachedTo: ['Debian Build Agent'] },
    { id: 'm5', name: 'scratch.vhd', location: 'E:\\Scratch\\scratch.vhd', format: 'VHD', logicalSize: 16 * GB, size: 1.3 * GB, attachedTo: [] }
];

const LOG = [
    '00:00:00.512031 VirtualBox VM 7.2.97 r170000 win.amd64',
    '00:00:00.512200 Host: Windows 11 Pro 24H2, 16 threads, 32 GB',
    '00:00:00.910442 NEM: Hyper-V API is available',
    '00:00:01.002113 Firmware: UEFI with Secure Boot',
    '00:00:01.450880 WARNING: CPUM: guest requested AVX-512, masked',
    '00:00:02.004917 Display: VMSVGA 3D enabled',
    '00:00:09.551730 Guest Additions: 7.2.97 reported, facilities ready',
    '00:00:12.090001 Shared folder Projects mounted',
    '00:00:15.733002 USB: proxy failed to claim device, rc=VERR_SHARING_VIOLATION',
    '00:00:16.100000 NAT: DNS proxy using host resolver'
].join('\n');

let tick = 0;
const demoInput = { x: 640, y: 400, buttons: 0, keys: 0 };

/** A sample desktop drawn as SVG; the pointer and key count prove input arrives. */
function demoFrame(m) {
    const t = new Date().toLocaleTimeString();
    return '<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="800" viewBox="0 0 1280 800">' +
        '<defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#13243B"/><stop offset="1" stop-color="#0E1626"/></linearGradient></defs>' +
        '<rect width="1280" height="800" fill="url(#g)"/>' +
        '<rect x="80" y="120" width="560" height="320" rx="14" fill="#F2F3F7"/><rect x="104" y="148" width="220" height="14" rx="7" fill="#C9CCD3"/>' +
        '<rect x="104" y="180" width="480" height="9" rx="4" fill="#E3E5EA"/><rect x="104" y="202" width="420" height="9" rx="4" fill="#E3E5EA"/>' +
        '<text x="104" y="300" font-family="Segoe UI, sans-serif" font-size="28" fill="#1D1B20">' + m.name.replace(/[<&]/g, '') + '</text>' +
        '<text x="104" y="340" font-family="Segoe UI, sans-serif" font-size="18" fill="#49454F">Demo guest · ' + t + ' · keys received: ' + demoInput.keys + '</text>' +
        '<rect x="440" y="740" width="400" height="44" rx="14" fill="rgba(255,255,255,.14)"/>' +
        '<path d="M' + demoInput.x + ' ' + demoInput.y + ' l0 22 l6 -6 l5 11 l4 -2 l-5 -11 l8 0 z" fill="' + (demoInput.buttons ? '#FFB869' : '#FFFFFF') + '" stroke="#000" stroke-width="1"/>' +
        '</svg>';
}

function find(id) {
    const m = machines.find((x) => x.id === id || x.name === id);
    if (!m) throw new Error("No machine with id '" + id + "'");
    return m;
}

async function call(method, p = {}) {
    switch (method) {
        case 'host.info': return { version: '7.2.97 (demo data)', os: 'Windows 11', osVersion: '24H2', cpus: 16, memoryMB: 32768, memoryAvailableMB: 13900 };
        case 'machines.list': return machines.map((m) => ({ ...m }));
        case 'machine.details': {
            const m = find(p.id);
            return {
                summary: { ...m }, vramMB: 128, firmware: 'UEFI', description: '', settingsFile: 'D:\\VMs\\' + m.name + '\\' + m.name + '.vbox',
                storage: [{ controller: 'NVMe', port: 0, device: 0, type: 'disk', medium: m.name + '.vdi', logicalSize: 80 * GB }, { controller: 'SATA', port: 1, device: 0, type: 'optical' }],
                network: [{ slot: 0, attachment: 'nat' }, { slot: 1, attachment: 'hostOnly' }],
                sharedFolders: [{ name: 'Projects', hostPath: 'D:\\Projects' }]
            };
        }
        case 'machine.start': { const m = find(p.id); m.state = 'running'; m.lastStateChangeMs = Date.now(); return { started: true }; }
        case 'machine.control': {
            const m = find(p.id);
            if (p.action === 'show') { if (m.state !== 'running' && m.state !== 'paused') throw new Error('This machine has no window to show'); return { done: true, windowId: 0 }; }
            const next = { pause: 'paused', resume: 'running', reset: 'running', acpiShutdown: 'poweredOff', powerOff: 'poweredOff', saveState: 'saved' }[p.action];
            if (!next) throw new Error("Unknown action '" + p.action + "'");
            if (m.state !== 'running' && m.state !== 'paused') throw new Error('The machine is not running');
            m.state = next; m.lastStateChangeMs = Date.now();
            return { done: true };
        }
        case 'machine.update': {
            const m = find(p.id);
            if (p.memoryMB) m.memoryMB = p.memoryMB;
            if (p.cpus) m.cpus = p.cpus;
            if (p.name) m.name = p.name;
            return { ...m };
        }
        case 'machine.create': {
            if (!p.name) throw new Error("Missing string parameter 'name'");
            if (machines.some((m) => m.name === p.name)) throw new Error('A machine named ' + p.name + ' already exists');
            const m = { id: 'demo-' + Date.now().toString(16), name: p.name, osTypeId: p.osTypeId || 'Other', state: 'poweredOff', memoryMB: p.memoryMB || 2048, cpus: p.cpus || 2,
                snapshotCount: 0, lastStateChangeMs: Date.now(), groups: [p.group ? '/' + String(p.group).replace(/^\//, '') : '/'], accessible: true };
            machines.push(m);
            if (p.diskGB) media.push({ id: 'd' + m.id, name: p.name + '.vdi', location: 'D:\\VMs\\' + p.name + '\\' + p.name + '.vdi', format: 'VDI', logicalSize: p.diskGB * GB, size: 2 * 1024 * 1024, attachedTo: [p.name] });
            return { machine: { ...m }, disk: p.diskGB ? 'D:\\VMs\\' + p.name + '\\' + p.name + '.vdi' : '' };
        }
        case 'machine.delete': {
            const m = find(p.id);
            if (p.confirmName !== m.name) throw new Error('Confirmation name does not match the machine');
            machines.splice(machines.indexOf(m), 1);
            return { deleted: true, media: 1 };
        }
        case 'snapshots.list': {
            const m = find(p.id);
            if (!m.snapshotCount) return { currentId: '', root: null };
            const names = ['Clean install', 'Toolchain ready', 'Before driver update'].slice(0, m.snapshotCount);
            let node = null;
            for (let i = names.length - 1; i >= 0; i--) node = { id: 's' + i, name: names[i], description: '', timeMs: now - (names.length - i) * 48 * H, online: false, children: node ? [node] : [] };
            return { currentId: 's' + (names.length - 1), root: node };
        }
        case 'snapshots.take': { const m = find(p.id); m.snapshotCount++; return { id: 's' + m.snapshotCount }; }
        case 'media.list': return p.kind && p.kind !== 'disk' ? [] : media.map((x) => ({ ...x }));
        case 'networks.list': return {
            natNetworks: [{ name: 'LabNet', cidr: '10.0.2.0/24', enabled: true, dhcp: true }],
            hostInterfaces: [{ name: 'VirtualBox Host-Only Ethernet Adapter', ip: '192.168.56.1', mask: '255.255.255.0', kind: 'hostOnly', dhcp: false },
                { name: 'Wi-Fi', ip: '192.168.1.24', mask: '255.255.255.0', kind: 'bridged', dhcp: true }]
        };
        case 'logs.read': find(p.id); return { file: 'VBox.log', offset: 0, nextOffset: LOG.length, text: p.offset ? '' : LOG };
        case 'extpacks.list': return [{ name: 'Oracle VirtualBox Extension Pack', version: '7.2.97', description: 'USB 3 passthrough, disk encryption, PXE boot, webcam', usable: true, whyUnusable: '' }];
        case 'metrics.sample': {
            tick++;
            const wave = (k) => Math.max(2, Math.min(98, 35 + 25 * Math.sin(tick / 3 + k) + 10 * Math.cos(tick / 1.7 + k * 2)));
            return [{ metric: 'CPU/Load/User', object: 'host', unit: '%', value: wave(0) }, { metric: 'RAM/Usage/Used', object: 'host', unit: 'kB', value: (18 + Math.sin(tick / 9)) * 1024 * 1024 }]
                .concat(machines.filter((m) => m.state === 'running').map((m, i) => ({ metric: 'CPU/Load/User', object: m.id, unit: '%', value: wave(i + 1) })));
        }
        case 'screen.attach': {
            const m = find(p.id);
            if (m.state !== 'running' && m.state !== 'paused') throw new Error('The machine is not running');
            return { width: 1280, height: 800 };
        }
        case 'screen.frame': return { width: 1280, height: 800, format: 'svg', data: Buffer.from(demoFrame(find(p.id))).toString('base64') };
        case 'screen.keys': demoInput.keys += (p.scancodes || []).length; return { stored: (p.scancodes || []).length };
        case 'screen.mouse': demoInput.x = p.x; demoInput.y = p.y; demoInput.buttons = p.buttons; return { done: true };
        case 'screen.resize': return { requested: true };
        case 'screen.detach': return { detached: true };
        default: throw new Error("Unknown method '" + method + "'");
    }
}

module.exports = { call };
