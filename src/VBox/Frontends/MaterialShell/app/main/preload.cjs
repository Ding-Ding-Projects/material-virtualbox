/*
 * The renderer's only door to the outside: a fixed list of bridge methods,
 * window controls and file saving.  Nothing else from Node is exposed.
 */
'use strict';

const { contextBridge, ipcRenderer } = require('electron');

const METHODS = ['host.info', 'machines.list', 'machine.details', 'machine.start', 'machine.control', 'machine.update', 'machine.create', 'machine.configure', 'storage.mount', 'machine.delete',
    'snapshots.list', 'snapshots.take', 'media.list', 'networks.list', 'logs.read', 'extpacks.list', 'metrics.sample',
    'machine.settings', 'machine.settingsUpdate', 'machine.runtime', 'machine.clone', 'snapshots.action', 'media.action', 'networks.action',
    'extpacks.action', 'appliance', 'usb', 'system.properties'];

contextBridge.exposeInMainWorld('mvb', {
    call(method, params) {
        if (!METHODS.includes(method)) return Promise.reject(new Error('Method not allowed: ' + method));
        return ipcRenderer.invoke('bridge:call', method, params || {});
    },
    status: () => ipcRenderer.invoke('bridge:status'),
    screen: (method, params) => ipcRenderer.invoke('screen:call', String(method), params || {}),
    window: {
        minimize: () => ipcRenderer.send('window:minimize'),
        toggleMaximize: () => ipcRenderer.send('window:toggleMaximize'),
        close: () => ipcRenderer.send('window:close')
    },
    saveFile: (suggestedName, text) => ipcRenderer.invoke('file:save', String(suggestedName), String(text)),
    openExternalEditor: (filePath) => ipcRenderer.invoke('editor:open', String(filePath))
});
