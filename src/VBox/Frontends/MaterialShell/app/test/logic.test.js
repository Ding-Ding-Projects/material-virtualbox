import { test } from 'node:test';
import assert from 'node:assert/strict';

import { buildSource, buildFlags, compile, preview, describe, makeMatcher, escapeLiteral } from '../renderer/js/logic/regex-model.js';
import { reconcile, closeTab, openTab, openEvery, togglePin, filterTabs, resolveCloseSet } from '../renderer/js/logic/tabs-model.js';
import { FORMATS, exportRows, importRows } from '../renderer/js/logic/export-formats.js';
import { bytes, megabytes, primaryAction, stateTone, parseLog, monogram } from '../renderer/js/logic/format.js';

test('regex builder composes constructs, literals and alternations', () => {
    const src = buildSource(['start', { either: ['Windows', 'Ubuntu'] }, 'optSpace', 'digits']);
    assert.equal(src, '^(?:Windows|Ubuntu)\\s*\\d+');
    assert.equal(buildFlags(['u', 'i']), 'iu');
    const r = preview(src, 'i', ['Windows 11 Dev Box', 'Fedora 41', 'ubuntu 24.04']);
    assert.deepEqual(r.results.map((x) => x.hit), [true, false, true]);
    assert.equal(r.results[0].match, 'Windows 11');
});

test('regex builder reports invalid patterns without throwing', () => {
    const c = compile('(unclosed', '');
    assert.equal(c.ok, false);
    assert.ok(c.error);
    const p = preview('(', '', ['x']);
    assert.equal(p.ok, false);
});

test('regex preview stops at the time budget', () => {
    let t = 0;
    const r = preview('a', '', Array.from({ length: 50 }, () => 'a'), () => (t += 40));
    assert.equal(r.truncated, true);
    assert.ok(r.results.length < 50);
});

test('literals are escaped and descriptions are plain', () => {
    assert.equal(escapeLiteral('a.vdi (copy)'), 'a\\.vdi \\(copy\\)');
    assert.equal(describe([]), 'Add a construct to start.');
    assert.match(describe(['start', { literal: 'Win' }]), /start of text, then "Win"/);
});

test('matcher accepts /regex/flags and plain text', () => {
    assert.equal(makeMatcher('/^ub/i')('Ubuntu'), true);
    assert.equal(makeMatcher('dev')('Windows 11 Dev Box'), true);
    assert.equal(makeMatcher('/(/')('anything'), false);
    assert.equal(makeMatcher('')('x'), true);
});

const CATALOG = [{ id: 'machines' }, { id: 'media' }, { id: 'network' }];

test('every tab opens on a fresh install and closed tabs stay closed', () => {
    let s = reconcile(CATALOG, null);
    assert.deepEqual(s.open, ['machines', 'media', 'network']);
    s = closeTab(s, 'media');
    const again = reconcile(CATALOG, s);
    assert.deepEqual(again.open, ['machines', 'network']);
    assert.deepEqual(again.closed, ['media']);
});

test('an update opens only tabs the strip never offered', () => {
    const s = closeTab(reconcile(CATALOG, null), 'media');
    const next = reconcile([...CATALOG, { id: 'activity' }], s);
    assert.deepEqual(next.open, ['machines', 'network', 'activity']);
    assert.deepEqual(openEvery(next, CATALOG).open, ['machines', 'network', 'activity', 'media']);
    assert.deepEqual(openTab(next, 'media').closed, []);
});

test('pinned tabs move first and survive bulk close', () => {
    let s = reconcile(CATALOG, null);
    s = togglePin(s, 'network');
    assert.deepEqual(s.open, ['network', 'machines', 'media']);
    const tabs = [{ id: 'network', title: 'Network' }, { id: 'machines', title: 'Machines' }, { id: 'media', title: 'Media' }];
    const set = resolveCloseSet(tabs, '/^M/', 'containing', { pinned: s.pinned, activeId: 'machines' });
    assert.deepEqual(set.closing, ['media']);
    const inverse = resolveCloseSet(tabs, 'Media', 'notContaining', { pinned: s.pinned });
    assert.deepEqual(inverse.closing, ['machines']);
    assert.deepEqual(resolveCloseSet(tabs, '  ', 'containing').closing, []);
    assert.deepEqual(resolveCloseSet(tabs, '', 'notContaining').closing, []);
});

test('four tab searches compose', () => {
    const tabs = [{ id: 1, title: 'Ubuntu Server', group: 'Lab', state: 'running' }, { id: 2, title: 'Ubuntu Desktop', group: 'Lab', state: 'poweredOff' }, { id: 3, title: 'Fedora', group: 'Lab', state: 'running' }];
    assert.deepEqual(filterTabs(tabs, { title: 'ubuntu', notTitle: '', group: 'lab', state: 'running' }).map((t) => t.id), [1]);
    assert.deepEqual(filterTabs(tabs, { title: '', notTitle: 'Desktop', group: '', state: '' }).map((t) => t.id), [1, 3]);
});

const ROWS = [
    { section: 'general', key: 'name', value: 'Windows 11 "Dev", Box' },
    { section: 'system', key: 'memoryMB', value: '16384' },
    { section: 'system', key: 'note', value: 'line one\nline two' }
];

test('every export format produces text', () => {
    for (const f of FORMATS) {
        const out = exportRows(f.ext, ROWS, { source: 'test' });
        assert.ok(out.length > 10, f.ext);
    }
});

test('json, ndjson, csv and tsv round trip', () => {
    for (const ext of ['json', 'ndjson', 'csv', 'tsv']) {
        assert.deepEqual(importRows(ext, exportRows(ext, ROWS, {})), ROWS, ext);
    }
});

test('formatting helpers', () => {
    assert.equal(bytes(1536), '1.5 KB');
    assert.equal(megabytes(16384), '16 GB');
    assert.equal(megabytes(512), '512 MB');
    assert.equal(primaryAction('running').id, 'show');
    assert.equal(primaryAction('saved').label, 'Resume');
    assert.equal(stateTone('aborted'), 'error');
    assert.equal(monogram('  ubuntu'), 'U');
    const rows = parseLog('00:00:01.123456 VMM: starting\n00:00:02.000000 USB: proxy failed VERR_SHARING_VIOLATION');
    assert.equal(rows[1].severity, 'error');
    assert.equal(rows[0].time, '00:00:01.123');
});

import { makeCode, breakCode, ctrlAltDel, buttonMask, toGuest } from '../renderer/js/logic/scancodes.js';

test('scan codes follow set 1 with extended prefixes', () => {
    assert.deepEqual(makeCode('KeyA'), [0x1e]);
    assert.deepEqual(breakCode('KeyA'), [0x9e]);
    assert.deepEqual(makeCode('ArrowUp'), [0xe0, 0x48]);
    assert.deepEqual(breakCode('ArrowUp'), [0xe0, 0xc8]);
    assert.equal(makeCode('Fn'), null);
    assert.deepEqual(ctrlAltDel(), [0x1d, 0x38, 0xe0, 0x53, 0xe0, 0xd3, 0xb8, 0x9d]);
});

test('pointer maps to guest pixels and buttons to the Main API mask', () => {
    assert.deepEqual(toGuest(150, 100, { left: 100, top: 50, width: 640, height: 400 }, { width: 1280, height: 800 }), { x: 100, y: 100 });
    assert.deepEqual(toGuest(-5, 9999, { left: 0, top: 0, width: 640, height: 400 }, { width: 1280, height: 800 }), { x: 0, y: 799 });
    assert.equal(buttonMask(1 | 2 | 4), 7);
    assert.equal(buttonMask(0), 0);
});

import { createRequire } from 'node:module';
const require = createRequire(import.meta.url);
const { NativeProvider } = require('../main/bridge-client.cjs');

test('in-process provider routes channels, frames and errors', async () => {
    const seen = [];
    const fake = {
        call: async (channel, req) => { const r = JSON.parse(req); seen.push([channel, r.method]); return r.method === 'bad' ? JSON.stringify({ id: r.id, error: { message: 'nope', hrc: -1 } }) : JSON.stringify({ id: r.id, result: { ok: r.params.x } }); },
        frame: async (channel, req) => { seen.push([channel, JSON.parse(req).method]); return { json: JSON.stringify({ id: 1, result: { width: 2, height: 1, format: 'png', data: '' } }), png: Buffer.from([137, 80]) }; },
        close: (channel) => seen.push([channel, 'close'])
    };
    const p = new NativeProvider(fake);
    assert.deepEqual(await p.call('host.info', { x: 3 }), { ok: 3 });
    await assert.rejects(p.call('bad'), /nope/);
    const f = await p.screen('vm1', 'screen.frame', { id: 'vm1' });
    assert.equal(f.png.length, 2);
    await p.screen('vm1', 'screen.detach', { id: 'vm1' });
    assert.deepEqual(seen, [['main', 'host.info'], ['main', 'bad'], ['screen:vm1', 'screen.frame'], ['screen:vm1', 'screen.detach'], ['screen:vm1', 'close']]);
});
