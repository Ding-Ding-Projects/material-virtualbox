/*
 * Browser-style tab model: every destination opens by default, closed tabs
 * stay closed, pinned tabs never close in bulk, and bulk close is always
 * previewed before it is authorised.
 */

import { makeMatcher } from './regex-model.js';

/**
 * Reconciles the saved strip with the current catalog.
 *  - fresh install (no saved state): every catalog tab, in catalog order
 *  - update: tabs the strip has never offered are appended
 *  - a tab the person closed stays closed
 */
export function reconcile(catalog, saved) {
    if (!saved) {
        return { open: catalog.map((t) => t.id), closed: [], offered: catalog.map((t) => t.id), pinned: [] };
    }
    const offered = new Set(saved.offered || []);
    const open = (saved.open || []).filter((id) => catalog.some((t) => t.id === id));
    for (const t of catalog) {
        if (!offered.has(t.id)) {
            open.push(t.id);
            offered.add(t.id);
        }
    }
    return { open, closed: (saved.closed || []).filter((id) => !open.includes(id)), offered: [...offered], pinned: (saved.pinned || []).filter((id) => open.includes(id)) };
}

export function closeTab(state, id) {
    if (!state.open.includes(id)) return state;
    return { ...state, open: state.open.filter((x) => x !== id), closed: [...state.closed.filter((x) => x !== id), id], pinned: state.pinned.filter((x) => x !== id) };
}

export function openTab(state, id) {
    if (state.open.includes(id)) return state;
    return { ...state, open: [...state.open, id], closed: state.closed.filter((x) => x !== id), offered: state.offered.includes(id) ? state.offered : [...state.offered, id] };
}

export function openEvery(state, catalog) {
    return catalog.reduce((s, t) => openTab(s, t.id), state);
}

export function togglePin(state, id) {
    const pinned = state.pinned.includes(id) ? state.pinned.filter((x) => x !== id) : [...state.pinned, id];
    const open = [...pinned.filter((x) => state.open.includes(x)), ...state.open.filter((x) => !pinned.includes(x))];
    return { ...state, pinned, open };
}

/**
 * The four independent tab searches compose with AND.  Returns the tabs
 * that match, given the tab descriptors { id, title, group, state }.
 */
export function filterTabs(tabs, q) {
    const title = makeMatcher(q.title);
    const notTitle = q.notTitle ? makeMatcher(q.notTitle) : () => false;
    const group = makeMatcher(q.group);
    const state = makeMatcher(q.state);
    return tabs.filter((t) => title(t.title) && !notTitle(t.title) && group(t.group || '') && state(t.state || ''));
}

/**
 * Resolves which tabs a bulk close would close.  mode is "containing" or
 * "notContaining"; pinned tabs and the active tab are never included.
 */
export function resolveCloseSet(tabs, query, mode, { pinned = [], activeId = null } = {}) {
    /* An empty query never selects anything: bulk close needs an explicit pattern. */
    if (!String(query || '').trim()) return { closing: [], keeping: tabs.map((t) => t.id) };
    const match = makeMatcher(query);
    const candidates = tabs.filter((t) => !pinned.includes(t.id) && t.id !== activeId);
    const closing = candidates.filter((t) => (mode === 'notContaining' ? !match(t.title) : match(t.title)));
    return { closing: closing.map((t) => t.id), keeping: tabs.filter((t) => !closing.includes(t)).map((t) => t.id) };
}
