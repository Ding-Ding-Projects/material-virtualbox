/*
 * Tiny DOM and state helpers.  No framework: h() builds elements, a store
 * holds app state, and screens re-render only the region they own.
 */

export function h(tag, props, ...children) {
    const el = document.createElementNS(tag === 'svg' || tag === 'path' || tag === 'circle' || tag === 'rect' || tag === 'defs' || tag === 'linearGradient' || tag === 'stop' ? 'http://www.w3.org/2000/svg' : 'http://www.w3.org/1999/xhtml', tag);
    for (const [k, v] of Object.entries(props || {})) {
        if (v === undefined || v === null || v === false) continue;
        if (k === 'class') el.setAttribute('class', v);
        else if (k === 'style' && typeof v === 'object') Object.assign(el.style, v);
        else if (k.startsWith('on') && typeof v === 'function') el.addEventListener(k.slice(2).toLowerCase(), v);
        else if (k === 'value' && 'value' in el) el.value = v;
        else if (k === 'ref' && typeof v === 'function') v(el);
        else el.setAttribute(k, v === true ? '' : String(v));
    }
    append(el, children);
    return el;
}

function append(el, children) {
    for (const c of children) {
        if (c === null || c === undefined || c === false) continue;
        if (Array.isArray(c)) append(el, c);
        else el.appendChild(c instanceof Node ? c : document.createTextNode(String(c)));
    }
}

/** Replaces the children of a container. */
export function mount(container, ...children) {
    container.replaceChildren();
    append(container, children);
}

/** Stroke icon from one SVG path. */
export function icon(d, size = 20, extra = {}) {
    return h('svg', { width: size, height: size, viewBox: '0 0 24 24', fill: 'none', stroke: 'currentColor', 'stroke-width': 2, 'stroke-linecap': 'round', 'stroke-linejoin': 'round', 'aria-hidden': 'true', ...extra }, h('path', { d }));
}

export const ICONS = {
    machine: 'M3 5h18v11H3zM8 20h8M12 16v4',
    media: 'M12 3a9 9 0 1 0 0 18 9 9 0 1 0 0-18zM12 10a2 2 0 1 0 0 4 2 2 0 1 0 0-4',
    network: 'M5 12h14M12 5v14M5 5h4v4H5zM15 15h4v4h-4z',
    extensions: 'M10 3h4v4h-4zM4 10h4v4H4zM16 10h4v4h-4zM10 17h4v4h-4z',
    activity: 'M3 12h4l3-8 4 16 3-8h4',
    logs: 'M5 4h14v16H5zM8 8h8M8 12h8M8 16h5',
    history: 'M3 12a9 9 0 1 0 3-6.7L3 8M3 3v5h5M12 7v5l3 2',
    prefs: 'M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6zM19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-1.8-.3 1.7 1.7 0 0 0-1 1.5V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-1.1-1.5 1.7 1.7 0 0 0-1.8.3l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0 .3-1.8 1.7 1.7 0 0 0-1.5-1H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.5-1.1 1.7 1.7 0 0 0-.3-1.8l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.8.3H9a1.7 1.7 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 1 1.5 1.7 1.7 0 0 0 1.8-.3l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.8V9a1.7 1.7 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1z',
    plus: 'M12 5v14M5 12h14',
    search: 'M11 4a7 7 0 1 0 0 14 7 7 0 1 0 0-14zM20 20l-3.5-3.5',
    bell: 'M6 8a6 6 0 1 1 12 0c0 7 3 9 3 9H3s3-2 3-9M10 21h4',
    close: 'M6 6l12 12M18 6L6 18',
    min: 'M5 12h14',
    max: 'M5 5h14v14H5z',
    play: 'M7 4v16l13-8z',
    pause: 'M6 4h4v16H6zM14 4h4v16h-4z',
    power: 'M12 3v8M6.5 6.5a8 8 0 1 0 11 0',
    camera: 'M4 8h3l2-3h6l2 3h3v11H4zM12 10a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7',
    trash: 'M4 7h16M10 11v6M14 11v6M6 7l1 13h10l1-13M9 7V4h6v3',
    save: 'M5 3h11l3 3v15H5zM8 3v6h8V3M8 21v-7h8v7',
    export: 'M12 3v12M7 10l5 5 5-5M5 21h14',
    key: 'M7.5 8.5a3.5 3.5 0 1 0 0 7 3.5 3.5 0 0 0 0-7M11 12h10M17 12v3M20 12v2',
    check: 'M5 12l5 5 9-10',
    arrow: 'M5 12h14M13 6l6 6-6 6',
    folder: 'M3 7h6l2 2h10v10H3z',
    chip: 'M7 7h10v10H7zM3 10h4M3 14h4M17 10h4M17 14h4M10 3v4M14 3v4M10 17v4M14 17v4',
    display: 'M3 5h18v11H3zM8 20h8',
    disk: 'M4 6h16v5H4zM4 13h16v5H4z',
    info: 'M12 3a9 9 0 1 0 0 18 9 9 0 1 0 0-18zM12 11v6M12 7.5v.5',
    edit: 'M4 20h4L19 9l-4-4L4 16zM14 6l4 4',
    pin: 'M14 3l7 7-3 1-4 4 1 5-2 1-4-5-5 5-1-1 5-5-5-4 1-2 5 1 4-4z',
    tabs: 'M3 7h7l2-3h9v16H3z'
};

/** A simple observable store. */
export function createStore(initial) {
    let state = initial;
    const subs = new Set();
    return {
        get: () => state,
        set(patch) {
            state = { ...state, ...(typeof patch === 'function' ? patch(state) : patch) };
            for (const fn of subs) fn(state);
        },
        subscribe(fn) { subs.add(fn); return () => subs.delete(fn); }
    };
}

/** Local persistence for preferences, tabs and history; never throws. */
export const persist = {
    load(key, fallback) {
        try { const v = localStorage.getItem('mvb.' + key); return v === null ? fallback : JSON.parse(v); } catch { return fallback; }
    },
    save(key, value) {
        try { localStorage.setItem('mvb.' + key, JSON.stringify(value)); } catch { /* storage full or blocked: keep running */ }
    }
};

/** Staggered animation delay for list item i. */
export function stagger(i, step = 55, base = 0) {
    return { animationDelay: (base + i * step) + 'ms' };
}

export function debounce(fn, ms) {
    let t = null;
    return (...a) => { clearTimeout(t); t = setTimeout(() => fn(...a), ms); };
}
