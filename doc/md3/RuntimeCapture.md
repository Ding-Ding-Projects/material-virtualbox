# Native runtime capture gate

This document defines the evidence required before a native GUI image is added to the project gallery. A design prototype, static HTML preview, or image from another build is not a runtime capture.

[`CaptureMatrix.md`](CaptureMatrix.md) is the enumerated tracking table built on top of this contract: every surface and state that must be captured, and its current `Not captured` status with the exact blocker named. Read this document for what counts as evidence; read that one for what still needs taking.

## Capture contract

Capture the exact built `VirtualBox.exe` through the cheap headless Windows desktop route. Resolve the top-level window handle at capture time, capture the client and non-client surface, and record the commit, target, Qt version, display scale, language mode, and profile state beside the image. Drive settings, wizard, manager tools, notifications, and runtime windows from the real application; do not seed fake machines or replace an unavailable surface with a mock.

The required baseline set is:

- manager cold launch and an empty state;
- settings search, regex validation, language and funny-level controls;
- New VM wizard first, validation, summary, and cancellation states;
- manager tools and notification history;
- runtime normal, fullscreen, scaled, narrow, high-DPI, bilingual, and keyboard-focus states.

## Current evidence

The native target linked successfully at commit `97c15bcc188012121935716d856f600d149df589` with the shared MD3 export fix. Static validation run `31303204687` and Pages run `31303204697` both passed.

The capture gate has partially opened. The real installed manager shell was reached and photographed — see [`CaptureMatrix.md`](CaptureMatrix.md) rows 1, 5, 6, 16, 22, 24, 27, and 31, closed by driving the live application (including via background input delivered to its off-screen HWND) once a genuine unsigned NSIS installer was available. The earlier `REGDB_E_CLASSNOTREG` failure, previously seen on every launch in this environment, did **not** reproduce when the manager shell was captured; row 3 in `CaptureMatrix.md` is left open pending re-verification of whether/when that failure state still occurs.

The gate is not fully open, however. Rows that require a registered virtual machine, a running VM session, or the VirtualBox host kernel drivers remain blocked: this lane is prohibited from creating/starting a VM, and even where drivers could be built, Windows refuses to load unsigned kernel drivers, so no runtime-window capture can be taken here regardless. Those rows, plus several simply not yet attempted, stay `Not captured` in `CaptureMatrix.md`, which is the authority for the current count.

## Failure handling

Record a failed capture with the exact dialog and HRESULT, keep it separate from successful gallery images, and leave the surface in the ledger as pending. Never convert a design thumbnail into runtime evidence merely to make a gallery count pass.

Suggested follow-up: [SettingsSearch.md](SettingsSearch.md), [NavigationRail.md](NavigationRail.md), and the [design coverage ledger](DesignCoverage.md).
