/*
 * Main process: one frameless window, the bridge (or the labelled demo
 * provider), and a few narrow IPC handlers.  All VirtualBox work happens in
 * VBoxSVC through the bridge; the running machine window stays the native
 * VirtualBoxVM process launched by machine.start.
 */
'use strict';

/* Bridge calls such as starting a machine wait in Node's worker pool; give it room. */
process.env.UV_THREADPOOL_SIZE = process.env.UV_THREADPOOL_SIZE || '16';

const { app, BrowserWindow, ipcMain, dialog, shell } = require('electron');
const fs = require('node:fs/promises');
const fsSync = require('node:fs');
const path = require('node:path');
const { BridgeClient, NativeProvider, findBridge, findNativeModule } = require('./bridge-client.cjs');

const DEMO = process.argv.includes('--demo') || process.env.MVB_DEMO === '1';
let provider = null;
let providerInfo = { mode: 'none', reason: '' };

/*
 * Crash guard for the in-process module.  A native crash takes the whole
 * process down, so the count of unfinished native sessions lives on disk:
 * it goes up before loading and back to zero on a clean quit.  After two
 * sessions that never finished, the shell uses the separate bridge instead.
 */
const NATIVE_CRASH_LIMIT = 2;
function guardPath() { return path.join(app.getPath('userData'), 'native-bridge-guard.json'); }
function readGuard() { try { return JSON.parse(fsSync.readFileSync(guardPath(), 'utf8')); } catch { return { unfinished: 0 }; } }
function writeGuard(g) { try { fsSync.mkdirSync(path.dirname(guardPath()), { recursive: true }); fsSync.writeFileSync(guardPath(), JSON.stringify(g)); } catch { /* best effort */ } }

function initProvider() {
    if (DEMO) {
        provider = require('./demo-provider.cjs');
        providerInfo = { mode: 'demo', reason: 'Started with --demo: every screen shows sample data.' };
        return;
    }
    const wantSeparate = process.argv.includes('--separate-bridge') || process.env.MVB_SEPARATE_BRIDGE === '1';
    const guard = readGuard();
    let fallbackReason = '';
    const nativeFile = findNativeModule(app.getAppPath());
    if (!wantSeparate && nativeFile && guard.unfinished < NATIVE_CRASH_LIMIT) {
        writeGuard({ unfinished: guard.unfinished + 1 });
        try {
            provider = NativeProvider.load(nativeFile);
            providerInfo = { mode: 'native', reason: nativeFile };
            app.on('will-quit', () => { provider.stop(); writeGuard({ unfinished: 0 }); });
            return;
        } catch (e) {
            writeGuard({ unfinished: 0 });
            fallbackReason = 'The in-process module did not load (' + e.message + ').';
        }
    } else if (nativeFile && guard.unfinished >= NATIVE_CRASH_LIMIT) {
        fallbackReason = 'The in-process module stopped unexpectedly twice, so the separate bridge is in use. Start once with --reset-bridge to try it again.';
    }
    const exe = findBridge(app.getAppPath());
    if (!exe) {
        providerInfo = { mode: 'missing', reason: 'Neither VBoxMaterialBridgeNode nor VBoxMaterialBridge was found next to the shell or in the VirtualBox folder.' };
        return;
    }
    const client = new BridgeClient(exe);
    provider = client;
    providerInfo = { mode: 'bridge', reason: exe, fallback: fallbackReason };
    app.on('before-quit', () => client.stop());
}

if (process.argv.includes('--reset-bridge')) app.whenReady().then(() => writeGuard({ unfinished: 0 }));

function createWindow() {
    const win = new BrowserWindow({
        width: 1440,
        height: 900,
        minWidth: 720,
        minHeight: 520,
        frame: false,
        backgroundColor: '#0F0D13',
        title: 'Material Virtual Machine',
        show: false,
        webPreferences: {
            preload: path.join(__dirname, 'preload.cjs'),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
            spellcheck: false
        }
    });
    win.once('ready-to-show', () => win.show());
    win.webContents.setWindowOpenHandler(({ url }) => {
        if (/^https:\/\//.test(url)) shell.openExternal(url);
        return { action: 'deny' };
    });
    win.webContents.on('will-navigate', (e) => e.preventDefault());
    win.loadFile(path.join(__dirname, '..', 'renderer', 'index.html'));
    return win;
}

ipcMain.handle('bridge:status', () => providerInfo);

ipcMain.handle('bridge:call', async (_e, method, params) => {
    if (!provider) throw new Error(providerInfo.reason || 'No VirtualBox connection');
    return provider.call(method, params);
});

/*
 * Machine screens: one bridge process per open screen, so megabyte frames
 * never queue behind library calls.  Closing the screen ends the process.
 */
const screens = new Map();
const SCREEN_METHODS = new Set(['screen.attach', 'screen.frame', 'screen.keys', 'screen.mouse', 'screen.resize', 'screen.detach']);

ipcMain.handle('screen:call', async (_e, method, params) => {
    if (!SCREEN_METHODS.has(method)) throw new Error('Method not allowed: ' + method);
    const id = String(params && params.id || '');
    if (!id) throw new Error('A machine id is required');
    if (providerInfo.mode === 'demo') return provider.call(method, params);
    if (providerInfo.mode === 'native') return provider.screen(id, method, params);
    if (providerInfo.mode !== 'bridge') throw new Error(providerInfo.reason || 'No VirtualBox connection');
    let client = screens.get(id);
    if (!client) {
        client = new BridgeClient(provider.exePath);
        screens.set(id, client);
    }
    try {
        return await client.call(method, params);
    } finally {
        if (method === 'screen.detach') { client.stop(); screens.delete(id); }
    }
});

app.on('before-quit', () => { for (const c of screens.values()) c.stop(); });

ipcMain.on('window:minimize', (e) => BrowserWindow.fromWebContents(e.sender)?.minimize());
ipcMain.on('window:toggleMaximize', (e) => {
    const w = BrowserWindow.fromWebContents(e.sender);
    if (w) (w.isMaximized() ? w.unmaximize() : w.maximize());
});
ipcMain.on('window:close', (e) => BrowserWindow.fromWebContents(e.sender)?.close());

ipcMain.handle('file:save', async (e, suggestedName, text) => {
    const w = BrowserWindow.fromWebContents(e.sender);
    const res = await dialog.showSaveDialog(w, { defaultPath: path.basename(suggestedName) });
    if (res.canceled || !res.filePath) return { saved: false };
    await fs.writeFile(res.filePath, text, 'utf8');
    return { saved: true, path: res.filePath };
});

ipcMain.handle('editor:open', async (_e, filePath) => {
    /* Only files the bridge reported (absolute paths ending in .vbox or .log) may be handed off. */
    if (!path.isAbsolute(filePath) || !/\.(vbox|log)$/i.test(filePath)) throw new Error('Only machine settings and log files can be opened');
    const err = await shell.openPath(filePath);
    if (err) throw new Error(err);
    return { opened: true };
});

app.whenReady().then(() => {
    initProvider();
    createWindow();
    app.on('activate', () => { if (BrowserWindow.getAllWindows().length === 0) createWindow(); });
});

app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
