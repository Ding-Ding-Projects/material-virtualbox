/*
 * Machine screen: the only machine window.  The machine runs headless; its
 * display is streamed through a dedicated bridge process, and keyboard and
 * mouse go back through IKeyboard and IMouse.  The host key (Right Ctrl)
 * releases the keyboard.
 */

import { h, icon, ICONS, mount } from '../core.js';
import { store, api, notify, recordHistory } from '../services.js';
import { makeCode, breakCode, ctrlAltDel, buttonMask, toGuest } from '../logic/scancodes.js';
import { stateLabel } from '../logic/format.js';
import { refreshMachines } from './machines.js';

const MIN_FRAME_MS = 33; /* about 30 frames a second at most */
let session = null;

function screenCall(method, params) {
    return window.mvb.screen(method, params);
}

export function openScreen(id) {
    store.set({ route: 'screen', screenId: id });
}

export async function closeScreen() {
    if (!session) return;
    const s = session;
    session = null;
    s.alive = false;
    s.cleanup();
    await screenCall('screen.detach', { id: s.id }).catch(() => null);
}

export function renderScreen(root) {
    const id = store.get().screenId;
    const m = store.get().machines.find((x) => x.id === id);
    if (session && session.id !== id) closeScreen();
    if (!m) return mount(root, h('div', { class: 'page' }, h('div', { class: 'empty' }, 'This machine is no longer registered.')));
    if (session && session.root === root && document.contains(session.view)) { session.updateChrome(m); return; }

    const st = { id, root, alive: true, guest: { width: 0, height: 0 }, captured: false, scale: 'fit', frames: 0, fps: 0, lastFpsAt: performance.now(), error: null, island: true, pendingMove: null };
    session = st;

    const img = h('img', { alt: 'Screen of ' + m.name, draggable: 'false', style: { maxWidth: '100%', maxHeight: '100%', objectFit: 'contain', imageRendering: 'auto', userSelect: 'none', cursor: 'none', borderRadius: '6px', boxShadow: '0 30px 80px rgba(0,0,0,.6)' } });
    const status = h('span', { class: 'muted mono', style: { fontSize: '12px' } });
    const capture = h('span', { class: 'chip', style: { height: '28px' } });
    const overlay = h('div', { style: { position: 'absolute', inset: 0, display: 'none', placeItems: 'center', background: 'rgba(5,4,7,.55)', backdropFilter: 'blur(14px)' } });
    const view = h('div', { tabindex: '0', role: 'application', 'aria-label': 'Screen of ' + m.name + '. Click to capture keyboard and mouse; Right Ctrl releases.',
        style: { position: 'relative', flex: 1, minHeight: 0, display: 'grid', placeItems: 'center', background: 'radial-gradient(1200px 700px at 50% 120%, #13243B, #050407)', overflow: 'auto', outline: 'none' } }, img, overlay);
    const islandBody = h('div');
    const island = h('div', { class: 'glass', style: { position: 'absolute', left: '50%', top: '14px', transform: 'translateX(-50%)', borderRadius: '28px', background: 'rgba(20,18,24,.86)', boxShadow: '0 20px 50px rgba(0,0,0,.5)', zIndex: 2, transition: 'width 520ms var(--ease-spring)', animation: 'mvOpen 700ms var(--ease-spring) both' } }, islandBody);
    const frameWrap = h('div', { style: { position: 'relative', display: 'flex', flexDirection: 'column', height: '100%' } }, island, view,
        h('footer', { class: 'row', style: { padding: '8px 14px', background: 'var(--surface)', flexWrap: 'nowrap' } }, capture, status, h('span', { class: 'grow' }), h('span', { class: 'kbd' }, 'Host key: Right Ctrl')));
    st.view = view;
    mount(root, frameWrap);

    const send = (codes) => codes && codes.length && screenCall('screen.keys', { id, scancodes: codes }).catch(() => null);
    const setCaptured = (on) => {
        st.captured = on;
        capture.textContent = on ? 'Keyboard and mouse captured' : 'Click the screen to capture';
        capture.setAttribute('aria-pressed', String(on));
        if (on) view.focus();
    };
    setCaptured(false);

    const control = async (action) => {
        try {
            await api('machine.control', { id, action });
            recordHistory({ title: action + ': ' + m.name, scope: m.name, category: 'Machines', diff: [['+', 'Action: ' + action]] });
            await refreshMachines();
        } catch { /* notified */ }
    };
    const scales = [['fit', 'Fit', 'M4 4h16v16H4zM9 9h6v6H9z'], ['actual', '1:1', 'M4 5h16v14H4z'], ['guest', 'Resize guest', 'M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5']];
    const applyScale = () => {
        if (st.scale === 'actual') { img.style.maxWidth = 'none'; img.style.maxHeight = 'none'; } else { img.style.maxWidth = '100%'; img.style.maxHeight = '100%'; }
        if (st.scale === 'guest') {
            const r = view.getBoundingClientRect();
            screenCall('screen.resize', { id, width: Math.max(640, Math.floor(r.width / 8) * 8), height: Math.max(480, Math.floor(r.height / 8) * 8) }).catch((e) => notify({ kind: 'warning', title: 'The guest did not resize', body: e.message }));
        }
    };

    st.updateChrome = (mm) => {
        const paused = mm.state === 'paused';
        overlay.style.display = paused ? 'grid' : 'none';
        mount(overlay, paused ? h('div', { class: 'rise', style: { display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '14px' } },
            h('span', { style: { fontSize: '88px', fontWeight: 900, letterSpacing: '-3px', fontVariationSettings: "'wdth' 150" } }, 'Paused'),
            h('button', { class: 'btn filled big st', onclick: () => control('resume') }, 'Resume')) : null);
        mount(islandBody,
            h('div', { class: 'row', style: { height: '56px', padding: '0 8px 0 18px', flexWrap: 'nowrap', gap: '6px' } },
                h('button', { class: 'st row', 'aria-expanded': String(st.island), style: { whiteSpace: 'nowrap', height: '40px', padding: '0 12px 0 0', borderRadius: '20px', fontWeight: 700, flexWrap: 'nowrap' }, onclick: () => { st.island = !st.island; st.updateChrome(mm); } },
                    h('span', { class: 'dot ' + (paused ? 'bg-warning' : 'bg-success pulse') }), mm.name + ' · ' + stateLabel(mm.state)),
                h('span', { class: 'grow', style: { minWidth: '20px' } }),
                h('button', { class: 'icon-btn st', 'aria-label': paused ? 'Resume' : 'Pause', title: paused ? 'Resume' : 'Pause', onclick: () => control(paused ? 'resume' : 'pause') }, icon(paused ? ICONS.play : ICONS.pause, 18)),
                h('button', { class: 'icon-btn st', 'aria-label': 'Take snapshot', title: 'Take snapshot', onclick: async () => { try { await api('snapshots.take', { id, name: 'Snapshot ' + new Date().toLocaleString() }); notify({ kind: 'success', title: 'Snapshot taken', body: mm.name }); } catch { /* notified */ } } }, icon(ICONS.camera, 18)),
                h('button', { class: 'btn st mono', style: { height: '40px', padding: '0 12px', fontSize: '12px' }, 'aria-label': 'Send Ctrl+Alt+Del', onclick: () => send(ctrlAltDel()) }, 'C·A·Del'),
                h('button', { class: 'icon-btn st', 'aria-label': 'Full screen', title: 'Full screen', onclick: () => (document.fullscreenElement ? document.exitFullscreen() : frameWrap.requestFullscreen()).catch(() => null) }, icon(scales[2][2], 18)),
                h('button', { class: 'icon-btn st tone-error', 'aria-label': 'Shut down', title: 'Shut down (ACPI)', onclick: () => control('acpiShutdown') }, icon(ICONS.power, 18)),
                h('button', { class: 'icon-btn st', 'aria-label': 'Back to the library', title: 'Back to the library', onclick: () => { closeScreen(); store.set({ route: 'machines' }); } }, icon(ICONS.close, 18))),
            st.island ? h('div', { class: 'fade', style: { display: 'grid', gridTemplateColumns: 'repeat(3, minmax(0, 1fr))', gap: '8px', padding: '4px 14px 14px' } }, scales.map(([k, label, ic]) =>
                h('button', { class: 'st', 'aria-pressed': String(st.scale === k), style: { display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '6px', padding: '12px 6px', borderRadius: '18px', fontSize: '12px', fontWeight: 650, background: st.scale === k ? 'var(--primary-container)' : 'rgba(43,41,48,.8)', color: st.scale === k ? 'var(--on-primary-container)' : 'var(--on-surface-variant)' }, onclick: () => { st.scale = k; applyScale(); st.updateChrome(mm); } }, icon(ic, 22), label))) : null);
        island.style.width = st.island ? '560px' : '';
        if (mm.state !== 'running' && mm.state !== 'paused') {
            st.alive = false;
            mount(view, h('div', { class: 'empty', style: { background: 'transparent' } }, h('div', { class: 'headline' }, mm.name + ' is ' + stateLabel(mm.state).toLowerCase()),
                h('button', { class: 'btn filled big st', onclick: () => { closeScreen(); store.set({ route: 'machines' }); } }, 'Back to the library')));
        }
    };
    st.updateChrome(m);

    /* Input. */
    const onKey = (e) => {
        if (!st.captured || store.get().route !== 'screen') return;
        if (e.code === 'ControlRight') { if (e.type === 'keydown') setCaptured(false); e.preventDefault(); return; }
        const codes = e.type === 'keydown' ? makeCode(e.code) : breakCode(e.code);
        if (codes) { e.preventDefault(); e.stopPropagation(); send(codes); }
    };
    window.addEventListener('keydown', onKey, true);
    window.addEventListener('keyup', onKey, true);
    const point = (e) => toGuest(e.clientX, e.clientY, img.getBoundingClientRect(), st.guest);
    const pushMouse = (e, dz = 0) => {
        if (!st.captured || !st.guest.width) return;
        const p = point(e);
        screenCall('screen.mouse', { id, x: p.x, y: p.y, dz, dw: 0, buttons: buttonMask(e.buttons) }).catch(() => null);
    };
    img.addEventListener('pointerdown', (e) => { if (!st.captured) { setCaptured(true); return; } pushMouse(e); e.preventDefault(); });
    img.addEventListener('pointerup', (e) => pushMouse(e));
    img.addEventListener('pointermove', (e) => { if (st.captured) st.pendingMove = e; });
    img.addEventListener('wheel', (e) => { if (st.captured) { pushMouse(e, Math.sign(e.deltaY)); e.preventDefault(); } }, { passive: false });
    img.addEventListener('contextmenu', (e) => e.preventDefault());
    st.cleanup = () => { window.removeEventListener('keydown', onKey, true); window.removeEventListener('keyup', onKey, true); if (st.blobUrl) URL.revokeObjectURL(st.blobUrl); };

    /* Frames. */
    const loop = async () => {
        try {
            const att = await screenCall('screen.attach', { id });
            st.guest = { width: att.width, height: att.height };
        } catch (e) {
            mount(view, h('div', { class: 'empty', style: { background: 'transparent' } }, h('div', { class: 'headline' }, 'Could not attach to the screen'), e.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, '')));
            return;
        }
        while (st.alive && session === st) {
            const t0 = performance.now();
            if (st.pendingMove) { const e = st.pendingMove; st.pendingMove = null; pushMouse(e); }
            try {
                const f = await screenCall('screen.frame', { id });
                if (f.width) {
                    st.guest = { width: f.width, height: f.height };
                    if (f.png && f.png.byteLength) {
                        /* In-process module: raw PNG bytes, shown through a blob URL. */
                        const url = URL.createObjectURL(new Blob([f.png], { type: 'image/png' }));
                        const prev = st.blobUrl;
                        st.blobUrl = url;
                        img.src = url;
                        if (prev) setTimeout(() => URL.revokeObjectURL(prev), 1000);
                    } else {
                        img.src = 'data:image/' + (f.format === 'svg' ? 'svg+xml' : 'png') + ';base64,' + f.data;
                    }
                    if (st.scale === 'actual') { img.style.width = f.width + 'px'; } else img.style.width = '';
                }
                st.frames++;
            } catch (e) {
                status.textContent = 'Frame failed: ' + String(e.message).slice(0, 80);
                await new Promise((r) => setTimeout(r, 1000));
            }
            const now = performance.now();
            if (now - st.lastFpsAt > 1000) { st.fps = st.frames * 1000 / (now - st.lastFpsAt); st.frames = 0; st.lastFpsAt = now; }
            status.textContent = st.guest.width + ' × ' + st.guest.height + ' · ' + st.fps.toFixed(1) + ' fps';
            const wait = MIN_FRAME_MS - (performance.now() - t0);
            if (wait > 0) await new Promise((r) => setTimeout(r, wait));
        }
    };
    loop();
}

/** Starts a stopped machine headless and opens its screen. */
export async function startAndOpen(m) {
    notify({ kind: 'info', title: 'Starting ' + m.name, body: 'The screen opens here when the machine is up.' });
    await api('machine.start', { id: m.id, type: 'headless' });
    await refreshMachines();
    openScreen(m.id);
}
