# Native runtime capture gate

This document defines the evidence required before a native GUI image is added to the project gallery. A design prototype, static HTML preview, or image from another build is not a runtime capture.

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

The capture gate is still open. The real executable reaches the COM boundary and reports `REGDB_E_CLASSNOTREG` because the host has no registered `VBoxSDS` Windows service for this checkout. Running the locally built `VBoxSDS.exe` as a console process exits with `ERROR_FAILED_SERVICE_CONTROLLER_CONNECT (0x427)`, which is expected for a service-only entry point. No machine-wide service or COM registration has been attempted. Until an approved registration or another sanctioned runtime host is available, the README must continue to show no finished native GUI gallery.

## Failure handling

Record a failed capture with the exact dialog and HRESULT, keep it separate from successful gallery images, and leave the surface in the ledger as pending. Never convert a design thumbnail into runtime evidence merely to make a gallery count pass.

Suggested follow-up: [SettingsSearch.md](SettingsSearch.md), [NavigationRail.md](NavigationRail.md), and the [design coverage ledger](DesignCoverage.md).
