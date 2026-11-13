/* Exercises the demo provider's answers for every management method, so the
 * protocol the screens rely on stays stable. */
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const demo = require('../main/demo-provider.cjs');
const OFF = '4c5d6e7f-8a9b-4c0d-9e2f-3a4b5c6d7e8f';
const RUNNING = '7c1d5a0e-1b2c-4d3e-8f90-0a1b2c3d4e5f';

test('settings round-trip through settingsUpdate', async () => {
    await demo.call('machine.settingsUpdate', { id: OFF, firmware: 'BIOS', tpm: '2.0', nestedVirt: true, bootOrder: ['disk', 'network'], audio: { controller: 'AC97' } });
    const s = await demo.call('machine.settings', { id: OFF });
    assert.equal(s.system.firmware, 'BIOS');
    assert.equal(s.system.tpm, '2.0');
    assert.deepEqual(s.system.bootOrder, ['disk', 'network', 'none', 'none']);
    assert.equal(s.audio.controller, 'AC97');
});

test('a running machine refuses hardware changes but accepts runtime modes', async () => {
    await assert.rejects(demo.call('machine.settingsUpdate', { id: RUNNING, tpm: 'none' }), /power it off/);
    await demo.call('machine.runtime', { id: RUNNING, clipboardMode: 'hostToGuest' });
    assert.equal((await demo.call('machine.settings', { id: RUNNING })).clipboardMode, 'hostToGuest');
});

test('USB filters are added and removed by index', async () => {
    await demo.call('machine.settingsUpdate', { id: OFF, usb: { filtersAdd: [{ name: 'A' }, { name: 'B' }] } });
    await demo.call('machine.settingsUpdate', { id: OFF, usb: { filtersRemove: [0] } });
    assert.deepEqual((await demo.call('machine.settings', { id: OFF })).usb.filters.map((f) => f.name), ['B']);
});

test('port rules validate their ports', async () => {
    await assert.rejects(demo.call('networks.action', { action: 'natUpdate', name: 'LabNet', rulesAdd: [{ name: 'x', hostPort: 0, guestIp: '10.0.2.15', guestPort: 22 }] }), /between 1 and 65535/);
});

test('extension packs need the license accepted', async () => {
    await assert.rejects(demo.call('extpacks.action', { action: 'install', path: 'a.vbox-extpack' }), /license/);
});

test('an attached medium cannot be removed', async () => {
    await assert.rejects(demo.call('media.action', { action: 'remove', mediumId: 'm1' }), /Release/);
});
