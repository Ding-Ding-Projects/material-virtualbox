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
const configs = new Map();

function demoConfig(m) {
    if (!configs.has(m.id)) {
        configs.set(m.id, { vramMB: 128, controller: 'VMSVGA', monitors: 1, accel3d: true, iso: '',
            adapters: [0, 1, 2, 3].map((slot) => ({ slot, enabled: slot < 2, attachment: ['nat', 'hostOnly', 'none', 'none'][slot], cableConnected: true,
                mac: '080027' + (0xA1B2C0 + slot).toString(16).toUpperCase(), bridgedInterface: 'Wi-Fi', hostOnlyInterface: slot === 1 ? 'VirtualBox Host-Only Ethernet Adapter' : '', internalNetwork: 'intnet', natNetwork: 'LabNet' })),
            folders: [{ name: 'Projects', hostPath: 'D:\\Projects', writable: true, autoMount: true, autoMountPoint: '' }] });
    }
    return configs.get(m.id);
}
const settings = new Map();
const snapNames = new Map();
const hostUsb = [
    { id: 'u1', product: 'YubiKey 5 NFC', manufacturer: 'Yubico', vendorId: '1050', productId: '0407', state: 'available', heldBy: '' },
    { id: 'u2', product: 'USB Flash Drive', manufacturer: 'SanDisk', vendorId: '0781', productId: '5581', state: 'available', heldBy: '' },
    { id: 'u3', product: 'Logitech Webcam C920', manufacturer: 'Logitech', vendorId: '046d', productId: '082d', state: 'busy', heldBy: '' }
];
const natNets = [{ name: 'LabNet', cidr: '10.0.2.0/24', enabled: true, dhcp: true, rules: ['ssh:tcp:[]:2222:[10.0.2.15]:22'] }];
const hostOnly = [{ name: 'VirtualBox Host-Only Ethernet Adapter', ip: '192.168.56.1', mask: '255.255.255.0', kind: 'hostOnly', dhcp: false }];
const extPacks = [{ name: 'Oracle VirtualBox Extension Pack', version: '7.2.97', description: 'USB 3 passthrough, disk encryption, PXE boot, webcam', usable: true, whyUnusable: '' }];
const sysProps = { defaultMachineFolder: 'D:\\VMs' };

function demoSettings(m) {
    if (!settings.has(m.id)) {
        settings.set(m.id, { description: '', osTypeId: m.osTypeId,
            system: { firmware: 'EFI', tpm: m.osTypeId.startsWith('Windows') ? '2.0' : 'none', nestedVirt: false, bootOrder: ['optical', 'disk', 'none', 'none'] },
            audio: { enabled: true, input: false, output: true, controller: 'HDA' },
            serial: [0, 1, 2, 3].map((slot) => ({ slot, enabled: false, mode: 'disconnected', path: '', server: true, ioBase: [0x3f8, 0x2f8, 0x3e8, 0x2e8][slot], irq: [4, 3, 4, 3][slot] })),
            usb: { controllers: [{ name: 'xHCI', type: 'XHCI' }], filters: [] },
            storageControllers: [{ name: 'NVMe', bus: 'NVMe', ports: 1 }, { name: 'SATA', bus: 'SATA', ports: 2 }],
            clipboardMode: 'bidirectional', dndMode: 'hostToGuest' });
    }
    return settings.get(m.id);
}
const locked = (m) => { if (m.state === 'running' || m.state === 'paused' || m.state === 'saved') throw new Error('Locking the machine for changes (power it off first) failed'); };
const progress = () => new Promise((r) => setTimeout(r, 450));

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
            const c = demoConfig(m);
            return {
                summary: { ...m }, vramMB: c.vramMB, graphicsController: c.controller, monitors: c.monitors, accel3d: c.accel3d, firmware: 'UEFI', description: '', settingsFile: 'D:\\VMs\\' + m.name + '\\' + m.name + '.vbox',
                storage: [{ controller: 'NVMe', port: 0, device: 0, type: 'disk', medium: m.name + '.vdi', location: 'D:\\VMs\\' + m.name + '\\' + m.name + '.vdi', logicalSize: 80 * GB },
                    { controller: 'SATA', port: 1, device: 0, type: 'optical', medium: c.iso ? c.iso.split('\\').pop() : undefined, location: c.iso || undefined }].concat((c.extra || []).map((x) => ({ ...x }))),
                network: c.adapters.filter((a) => a.enabled).map((a) => ({ slot: a.slot, attachment: a.attachment })),
                adapters: c.adapters.map((a) => ({ ...a })),
                sharedFolders: c.folders.map((f) => ({ ...f }))
            };
        }
        case 'machine.configure': {
            const m = find(p.id);
            if (m.state === 'running' || m.state === 'paused') throw new Error('Locking the machine for changes (power it off first) failed');
            const c = demoConfig(m);
            let changes = 0;
            if (p.display) { for (const k of ['vramMB', 'controller', 'accel3d', 'monitors']) if (p.display[k] !== undefined) { c[k] = p.display[k]; changes++; } }
            for (const a of p.adapters || []) { Object.assign(c.adapters[a.slot], a); changes++; }
            for (const f of p.sharedFoldersRemove || []) { c.folders = c.folders.filter((x) => x.name !== f.name); changes++; }
            for (const f of p.sharedFoldersAdd || []) {
                if (c.folders.some((x) => x.name === f.name)) throw new Error('A shared folder named ' + f.name + ' already exists');
                c.folders.push({ name: f.name, hostPath: f.hostPath, writable: f.writable !== false, autoMount: f.autoMount !== false, autoMountPoint: f.autoMountPoint || '' }); changes++;
            }
            return { changes };
        }
        case 'storage.mount': { const c = demoConfig(find(p.id)); c.iso = p.location || ''; return { mounted: !!p.location }; }
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
            for (let i = names.length - 1; i >= 0; i--) node = { id: 's' + i, name: snapNames.get(m.id + 's' + i) || names[i], description: '', timeMs: now - (names.length - i) * 48 * H, online: false, children: node ? [node] : [] };
            return { currentId: 's' + (names.length - 1), root: node };
        }
        case 'snapshots.take': { const m = find(p.id); m.snapshotCount++; return { id: 's' + m.snapshotCount }; }
        case 'media.list': return p.kind && p.kind !== 'disk' ? [] : media.map((x) => ({ ...x }));
        case 'networks.list': return {
            natNetworks: natNets.map((x) => ({ name: x.name, cidr: x.cidr, enabled: x.enabled, dhcp: x.dhcp, rules: [...x.rules] })),
            hostInterfaces: hostOnly.map((x) => ({ ...x })).concat([{ name: 'Wi-Fi', ip: '192.168.1.24', mask: '255.255.255.0', kind: 'bridged', dhcp: true }])
        };
        case 'logs.read': find(p.id); return { file: 'VBox.log', offset: 0, nextOffset: LOG.length, text: p.offset ? '' : LOG };
        case 'extpacks.list': return extPacks.map((x) => ({ ...x }));
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
        case 'machine.settings': { const st = demoSettings(find(p.id)); st.usb.filters.forEach((f, i) => { f.index = i; }); return JSON.parse(JSON.stringify(st)); }
        case 'machine.settingsUpdate': {
            const m = find(p.id); locked(m);
            const st = demoSettings(m);
            let changes = 0;
            if (p.description !== undefined) { st.description = p.description; changes++; }
            if (p.osTypeId) { st.osTypeId = p.osTypeId; m.osTypeId = p.osTypeId; changes++; }
            for (const k of ['firmware', 'tpm', 'nestedVirt']) if (p[k] !== undefined) { st.system[k] = p[k]; changes++; }
            if (p.bootOrder) { st.system.bootOrder = [0, 1, 2, 3].map((i) => p.bootOrder[i] || 'none'); changes++; }
            if (p.audio) { Object.assign(st.audio, p.audio); changes++; }
            for (const sp of p.serial || []) { if (!(sp.slot >= 0 && sp.slot <= 3)) throw new Error('Each serial port needs a slot from 0 to 3'); Object.assign(st.serial[sp.slot], sp); changes++; }
            if (p.usb) {
                if (p.usb.controller) { st.usb.controllers = p.usb.controller === 'none' ? [] : p.usb.controller === 'EHCI' ? [{ name: 'OHCI', type: 'OHCI' }, { name: 'EHCI', type: 'EHCI' }] : [{ name: p.usb.controller === 'XHCI' ? 'xHCI' : 'OHCI', type: p.usb.controller }]; changes++; }
                for (const i of [...(p.usb.filtersRemove || [])].sort((a, b) => b - a)) { st.usb.filters.splice(i, 1); changes++; }
                for (const f of p.usb.filtersAdd || []) { st.usb.filters.push({ name: f.name, active: true, vendorId: f.vendorId || '', productId: f.productId || '' }); changes++; }
            }
            const c = demoConfig(m);
            for (const d of p.detach || []) { if (!(c.extra || []).some((x) => x.controller === d.controller && x.port === d.port)) throw new Error('No device at ' + d.controller + ' ' + d.port + ':' + (d.device || 0)); c.extra = c.extra.filter((x) => !(x.controller === d.controller && x.port === d.port)); changes++; }
            for (const r of p.controllersRemove || []) { st.storageControllers = st.storageControllers.filter((x) => x.name !== r.name); c.extra = (c.extra || []).filter((x) => x.controller !== r.name); changes++; }
            for (const a of p.controllersAdd || []) { if (st.storageControllers.some((x) => x.name === a.name)) throw new Error('A controller named ' + a.name + ' already exists'); st.storageControllers.push({ name: a.name, bus: a.bus, ports: 2 }); changes++; }
            for (const a of p.attach || []) {
                if (!st.storageControllers.some((x) => x.name === a.controller)) throw new Error('No controller named ' + a.controller);
                const med = media.find((x) => x.id === a.mediumId);
                c.extra = (c.extra || []).concat([{ controller: a.controller, port: a.port || 0, device: a.device || 0, type: a.type, medium: med ? med.name : '', location: med ? med.location : '', logicalSize: med ? med.logicalSize : 0 }]);
                if (med && !med.attachedTo.includes(m.name)) med.attachedTo.push(m.name);
                changes++;
            }
            return { changes };
        }
        case 'machine.runtime': { const st = demoSettings(find(p.id)); if (p.clipboardMode) st.clipboardMode = p.clipboardMode; if (p.dndMode) st.dndMode = p.dndMode; return { done: true }; }
        case 'machine.clone': {
            const src = find(p.id);
            if (!p.name) throw new Error("Missing string parameter 'name'");
            if (machines.some((m) => m.name === p.name)) throw new Error('A machine named ' + p.name + ' already exists');
            await progress();
            const m = { ...src, id: 'clone-' + Date.now().toString(16), name: p.name, state: 'poweredOff', snapshotCount: p.mode === 'all' ? src.snapshotCount : 0, lastStateChangeMs: Date.now() };
            machines.push(m);
            return { ...m };
        }
        case 'snapshots.action': {
            const m = find(p.id);
            if (!p.snapshotId) throw new Error("Missing string parameter 'snapshotId'");
            if (p.action === 'rename') { if (!p.name) throw new Error("Missing string parameter 'name'"); snapNames.set(m.id + p.snapshotId, p.name); return { done: true }; }
            if (p.action === 'restore') { locked(m); await progress(); return { done: true }; }
            if (p.action === 'delete') { await progress(); m.snapshotCount = Math.max(0, m.snapshotCount - 1); return { done: true }; }
            throw new Error("Unknown snapshot action '" + p.action + "'");
        }
        case 'media.action': {
            if (p.action === 'open') {
                if (!p.location) throw new Error("Missing string parameter 'location'");
                const id = 'o' + Date.now().toString(16);
                media.push({ id, name: p.location.split(/[\\/]/).pop(), location: p.location, format: (p.location.split('.').pop() || 'VDI').toUpperCase(), logicalSize: 8 * GB, size: 1 * GB, attachedTo: [] });
                return { id };
            }
            const med = media.find((x) => x.id === p.mediumId);
            if (!med) throw new Error("No medium with id '" + p.mediumId + "'");
            await progress();
            if (p.action === 'resize') { if (!(p.sizeGB >= 1)) throw new Error('A size of at least 1 GB is required'); if (p.sizeGB * GB < med.logicalSize) throw new Error('Resizing (only growing is supported) failed'); med.logicalSize = p.sizeGB * GB; }
            else if (p.action === 'compact') med.size = Math.round(med.size * 0.82);
            else if (p.action === 'copy') { if (!p.target) throw new Error("Missing string parameter 'target'"); media.push({ ...med, id: 'c' + Date.now().toString(16), name: p.target.split(/[\\/]/).pop(), location: p.target, format: p.format || 'VDI', attachedTo: [] }); }
            else if (p.action === 'release') med.attachedTo = [];
            else if (p.action === 'remove') { if (med.attachedTo.length) throw new Error('Release the medium from ' + med.attachedTo.join(', ') + ' first'); media.splice(media.indexOf(med), 1); }
            else throw new Error("Unknown media action '" + p.action + "'");
            return { done: true };
        }
        case 'networks.action': {
            await progress();
            if (p.action === 'hostOnlyCreate') { const name = 'VirtualBox Host-Only Ethernet Adapter #' + (hostOnly.length + 1); hostOnly.push({ name, ip: '192.168.' + (56 + hostOnly.length) + '.1', mask: '255.255.255.0', kind: 'hostOnly', dhcp: false }); return { name }; }
            if (p.action === 'hostOnlyRemove' || p.action === 'hostOnlyConfigure') {
                const x = hostOnly.find((y) => y.name === p.name);
                if (!x) throw new Error('Finding the adapter failed');
                if (p.action === 'hostOnlyRemove') hostOnly.splice(hostOnly.indexOf(x), 1);
                else { if (!/^\d+\.\d+\.\d+\.\d+$/.test(p.ip || '') || !/^\d+\.\d+\.\d+\.\d+$/.test(p.mask || '')) throw new Error('Setting the address failed'); x.ip = p.ip; x.mask = p.mask; }
                return { done: true };
            }
            let net = natNets.find((y) => y.name === p.name);
            if (p.action === 'natCreate') { if (!p.name) throw new Error("Missing string parameter 'name'"); if (net) throw new Error('A NAT network named ' + p.name + ' already exists'); net = { name: p.name, cidr: '10.0.' + (natNets.length + 3) + '.0/24', enabled: true, dhcp: true, rules: [] }; natNets.push(net); }
            if (!net) throw new Error('Finding the NAT network failed');
            if (p.action === 'natRemove') { natNets.splice(natNets.indexOf(net), 1); return { done: true }; }
            if (p.cidr) net.cidr = p.cidr;
            if (p.dhcp !== undefined) net.dhcp = p.dhcp;
            if (p.enabled !== undefined) net.enabled = p.enabled;
            for (const r of p.rulesRemove || []) net.rules = net.rules.filter((x) => x.split(':')[0] !== r.name);
            for (const r of p.rulesAdd || []) { if (!(r.hostPort >= 1 && r.hostPort <= 65535 && r.guestPort >= 1 && r.guestPort <= 65535)) throw new Error('Ports must be between 1 and 65535'); net.rules.push([r.name, r.proto || 'tcp', '[' + (r.hostIp || '') + ']', r.hostPort, '[' + r.guestIp + ']', r.guestPort].join(':')); }
            return { name: net.name, rules: [...net.rules] };
        }
        case 'extpacks.action': {
            if (p.action === 'uninstall') { await progress(); const i = extPacks.findIndex((x) => x.name === p.name); if (i < 0) throw new Error('Uninstalling failed'); extPacks.splice(i, 1); return { done: true }; }
            if (!p.path || !/\.vbox-extpack$/i.test(p.path)) throw new Error('Opening the extension pack failed');
            const pack = { name: 'Oracle VirtualBox Extension Pack', version: '7.2.97', description: 'USB 3 passthrough, disk encryption, PXE boot, webcam', usable: true, whyUnusable: '',
                license: 'VirtualBox Extension Pack Personal Use and Evaluation License (sample text for the demo).\n\nYou may use this extension pack for personal, educational or evaluation purposes.' };
            if (p.action === 'inspect') return pack;
            if (p.action !== 'install') throw new Error("Unknown extension pack action '" + p.action + "'");
            if (!p.licenseAccepted) throw new Error('The license must be accepted first');
            await progress();
            const i = extPacks.findIndex((x) => x.name === pack.name);
            const entry = { name: pack.name, version: pack.version, description: pack.description, usable: true, whyUnusable: '' };
            if (i >= 0) extPacks[i] = entry; else extPacks.push(entry);
            return { done: true };
        }
        case 'appliance': {
            if (!p.path) throw new Error("Missing string parameter 'path'");
            await progress();
            if (p.action === 'export') { if (!(p.machines || []).length) throw new Error('Choose at least one machine'); (p.machines || []).forEach((x) => find(x.id)); return { path: p.path }; }
            if (!/\.(ova|ovf)$/i.test(p.path)) throw new Error('Reading the appliance failed');
            const base = p.path.split(/[\\/]/).pop().replace(/\.(ova|ovf)$/i, '');
            if (p.action === 'inspect') return { systems: [{ name: base }], warnings: [] };
            if (p.action !== 'import') throw new Error("Unknown appliance action '" + p.action + "'");
            let name = base, n = 1;
            while (machines.some((m) => m.name === name)) name = base + ' ' + (++n);
            const m = { id: 'imp-' + Date.now().toString(16), name, osTypeId: 'Other_64', state: 'poweredOff', memoryMB: 2048, cpus: 2, snapshotCount: 0, lastStateChangeMs: Date.now(), groups: ['/'], accessible: true };
            machines.push(m);
            return { machines: [m.id] };
        }
        case 'usb': {
            if (p.action === 'list') return hostUsb.map((u) => ({ ...u }));
            const m = find(p.id);
            if (m.state !== 'running' && m.state !== 'paused') throw new Error('The machine is not running');
            const u = hostUsb.find((x) => x.id === p.deviceId);
            if (!u) throw new Error('Attaching the USB device failed');
            if (p.action === 'attach') { if (u.state !== 'available') throw new Error('Attaching the USB device failed: the device is ' + u.state); u.state = 'captured'; u.heldBy = m.id; }
            else if (p.action === 'detach') { u.state = 'available'; u.heldBy = ''; }
            else throw new Error("Unknown USB action '" + p.action + "'");
            return { done: true };
        }
        case 'system.properties': if (p.defaultMachineFolder) sysProps.defaultMachineFolder = p.defaultMachineFolder; return { ...sysProps };
        default: throw new Error("Unknown method '" + method + "'");
    }
}

module.exports = { call };
