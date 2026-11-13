/*
 * Talks to VBoxMaterialBridge over stdio: one JSON request per line in, one
 * JSON response per line out.  Every request has a timeout so a stuck
 * VBoxSVC never freezes the window, and a crashed bridge is restarted on the
 * next call with the reason surfaced to the renderer.
 */
'use strict';

const { spawn } = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
const readline = require('node:readline');

const DEFAULT_TIMEOUT_MS = 30000;
const LONG_METHODS = new Set(['machine.start', 'machine.control', 'machine.delete', 'snapshots.take']);

/** Where to look for the bridge: next to the shell, then the VirtualBox install folder. */
function findBridge(appDir) {
    const exe = process.platform === 'win32' ? 'VBoxMaterialBridge.exe' : 'VBoxMaterialBridge';
    const candidates = [
        process.env.MVB_BRIDGE_PATH,
        path.join(appDir, '..', exe),
        path.join(appDir, 'bridge', exe),
        process.env.VBOX_MSI_INSTALL_PATH && path.join(process.env.VBOX_MSI_INSTALL_PATH, exe),
        process.env.VBOX_INSTALL_PATH && path.join(process.env.VBOX_INSTALL_PATH, exe),
        process.platform === 'win32' ? path.join(process.env.ProgramFiles || 'C:\\Program Files', 'Oracle', 'VirtualBox', exe) : null
    ].filter(Boolean);
    return candidates.find((p) => { try { return fs.statSync(p).isFile(); } catch { return false; } }) || null;
}

class BridgeClient {
    constructor(exePath) {
        this.exePath = exePath;
        this.proc = null;
        this.nextId = 1;
        this.pending = new Map();
        this.ready = null;
        this.lastError = null;
    }

    start() {
        if (this.ready) return this.ready;
        this.ready = new Promise((resolve, reject) => {
            const proc = spawn(this.exePath, [], { stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true });
            this.proc = proc;
            let stderr = '';
            proc.stderr.on('data', (d) => { stderr = (stderr + d.toString()).slice(-4000); });
            proc.on('error', (e) => { this.lastError = e.message; this.reset(e.message); reject(e); });
            proc.on('exit', (code) => {
                const msg = 'The bridge stopped (exit ' + code + ')' + (stderr ? ': ' + stderr.trim().split('\n').pop() : '');
                this.lastError = msg;
                this.reset(msg);
            });
            readline.createInterface({ input: proc.stdout }).on('line', (line) => {
                let msg;
                try { msg = JSON.parse(line); } catch { return; }
                if (msg.id === 0 && msg.result && msg.result.ready) { resolve(msg.result); return; }
                if (msg.id === null && msg.error) { reject(new Error(msg.error.message)); return; }
                const p = this.pending.get(msg.id);
                if (!p) return;
                this.pending.delete(msg.id);
                clearTimeout(p.timer);
                if (msg.error) {
                    const err = new Error(msg.error.message);
                    err.hrc = msg.error.hrc;
                    p.reject(err);
                } else p.resolve(msg.result);
            });
        });
        return this.ready;
    }

    reset(reason) {
        for (const p of this.pending.values()) { clearTimeout(p.timer); p.reject(new Error(reason)); }
        this.pending.clear();
        this.proc = null;
        this.ready = null;
    }

    async call(method, params = {}) {
        await this.start();
        const id = this.nextId++;
        return new Promise((resolve, reject) => {
            const timeout = LONG_METHODS.has(method) ? 10 * 60000 : DEFAULT_TIMEOUT_MS;
            const timer = setTimeout(() => { this.pending.delete(id); reject(new Error(method + ' timed out')); }, timeout);
            this.pending.set(id, { resolve, reject, timer });
            this.proc.stdin.write(JSON.stringify({ id, method, params }) + '\n');
        });
    }

    stop() {
        if (this.proc) { try { this.proc.stdin.end(); } catch { /* already gone */ } }
    }
}

module.exports = { BridgeClient, findBridge };
