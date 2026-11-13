/*
 * KeyboardEvent.code to PC/AT scan code set 1, which IKeyboard::putScancodes
 * expects.  Extended keys carry the 0xE0 prefix; a break code is the make code
 * with bit 7 set on its last byte.
 */

const BASE = {
    Escape: 0x01, Digit1: 0x02, Digit2: 0x03, Digit3: 0x04, Digit4: 0x05, Digit5: 0x06, Digit6: 0x07, Digit7: 0x08, Digit8: 0x09, Digit9: 0x0a, Digit0: 0x0b,
    Minus: 0x0c, Equal: 0x0d, Backspace: 0x0e, Tab: 0x0f,
    KeyQ: 0x10, KeyW: 0x11, KeyE: 0x12, KeyR: 0x13, KeyT: 0x14, KeyY: 0x15, KeyU: 0x16, KeyI: 0x17, KeyO: 0x18, KeyP: 0x19, BracketLeft: 0x1a, BracketRight: 0x1b,
    Enter: 0x1c, ControlLeft: 0x1d,
    KeyA: 0x1e, KeyS: 0x1f, KeyD: 0x20, KeyF: 0x21, KeyG: 0x22, KeyH: 0x23, KeyJ: 0x24, KeyK: 0x25, KeyL: 0x26, Semicolon: 0x27, Quote: 0x28, Backquote: 0x29,
    ShiftLeft: 0x2a, Backslash: 0x2b,
    KeyZ: 0x2c, KeyX: 0x2d, KeyC: 0x2e, KeyV: 0x2f, KeyB: 0x30, KeyN: 0x31, KeyM: 0x32, Comma: 0x33, Period: 0x34, Slash: 0x35, ShiftRight: 0x36,
    NumpadMultiply: 0x37, AltLeft: 0x38, Space: 0x39, CapsLock: 0x3a,
    F1: 0x3b, F2: 0x3c, F3: 0x3d, F4: 0x3e, F5: 0x3f, F6: 0x40, F7: 0x41, F8: 0x42, F9: 0x43, F10: 0x44,
    NumLock: 0x45, ScrollLock: 0x46, Numpad7: 0x47, Numpad8: 0x48, Numpad9: 0x49, NumpadSubtract: 0x4a, Numpad4: 0x4b, Numpad5: 0x4c, Numpad6: 0x4d,
    NumpadAdd: 0x4e, Numpad1: 0x4f, Numpad2: 0x50, Numpad3: 0x51, Numpad0: 0x52, NumpadDecimal: 0x53, IntlBackslash: 0x56, F11: 0x57, F12: 0x58
};

const EXTENDED = {
    NumpadEnter: 0x1c, ControlRight: 0x1d, NumpadDivide: 0x35, PrintScreen: 0x37, AltRight: 0x38,
    Home: 0x47, ArrowUp: 0x48, PageUp: 0x49, ArrowLeft: 0x4b, ArrowRight: 0x4d, End: 0x4f, ArrowDown: 0x50, PageDown: 0x51,
    Insert: 0x52, Delete: 0x53, MetaLeft: 0x5b, MetaRight: 0x5c, ContextMenu: 0x5d
};

/** Make (press) bytes for a KeyboardEvent.code, or null when unmapped. */
export function makeCode(code) {
    if (code in BASE) return [BASE[code]];
    if (code in EXTENDED) return [0xe0, EXTENDED[code]];
    return null;
}

/** Break (release) bytes for a KeyboardEvent.code, or null when unmapped. */
export function breakCode(code) {
    const m = makeCode(code);
    if (!m) return null;
    const out = m.slice();
    out[out.length - 1] |= 0x80;
    return out;
}

/** Ctrl+Alt+Del pressed and released, in order. */
export function ctrlAltDel() {
    return [...makeCode('ControlLeft'), ...makeCode('AltLeft'), ...makeCode('Delete'), ...breakCode('Delete'), ...breakCode('AltLeft'), ...breakCode('ControlLeft')];
}

/** Mouse button bitmask in the Main API's layout: left 1, right 2, middle 4. */
export function buttonMask(domButtons) {
    return (domButtons & 1 ? 1 : 0) | (domButtons & 2 ? 2 : 0) | (domButtons & 4 ? 4 : 0);
}

/**
 * Maps a pointer position on the displayed image back to guest pixels.
 * view: { left, top, width, height } of the drawn frame; guest: { width, height }.
 */
export function toGuest(clientX, clientY, view, guest) {
    const x = Math.round((clientX - view.left) / view.width * guest.width);
    const y = Math.round((clientY - view.top) / view.height * guest.height);
    return { x: Math.max(0, Math.min(guest.width - 1, x)), y: Math.max(0, Math.min(guest.height - 1, y)) };
}
