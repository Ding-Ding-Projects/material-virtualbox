# Native Material 3 implementation handoff

This handoff keeps the rewrite inside the existing Qt frontend. Prototype HTML,
reference C++, and design dimensions are design evidence; production behavior
stays in the native `VirtualBox`, `VirtualBoxVM`, and `UICommon` targets and in
their existing models, action pools, COM/XPCOM boundaries, validation paths,
and lifecycle.

## Authority and target ownership

1. Existing VirtualBox behavior and safety contracts remain authoritative.
2. Native accessibility, Windows behavior, supported display scaling, and
   truthful responsive layout may amend a prototype dimension when the two
   conflict. The amendment must be documented and tested rather than hidden.
3. Shared theme, language, search, history, notification, settings, wizard, and
   reusable component code belongs to `UICommon`.
4. Manager-only shell code belongs to `VirtualBox`; runtime chrome belongs to
   `VirtualBoxVM`.
5. No lane may introduce a parallel demo frontend or duplicate a real model.

## Coverage invariant

The checked-in design archive has exactly 69 entries. Every entry remains in
[`DesignCoverage.md`](DesignCoverage.md) with its source hash, production path,
evidence, status, and remaining gap. The generated ledger and
[`ArchiveManifest.sha256`](ArchiveManifest.sha256) are the accounting boundary;
an `In progress` row is not silently promoted by a static check or design
preview.

## Evidence boundary

Source wiring, focused object compilation, linked targets, native interaction,
runtime accessibility, release artifacts, CI, Pages deployment, and application
captures are distinct evidence levels. A lower level never stands in for a
higher one. Real captures must come from the built application at a named commit;
the design thumbnail and prototype pages are not application evidence. The
current development runtime cannot reach the manager until its COM/SDS classes
are registered, so capture-dependent rows stay open.

## Current implementation order

The manager shell, settings shell, shared regex/search infrastructure,
appearance/theme services, notification/history surfaces, and responsive wizard
composition are native incremental lanes. The next broad production gaps are:

1. complete manager-tool surfaces around their existing models;
2. replace the theme service's HSL tone approximation with a strict reviewed
   HCT-compatible implementation and regression vectors;
3. compose runtime chrome around `UIMachineWindow` and `UIMachineView` without
   touching guest display, capture, session, or multi-monitor ownership;
4. finish native accessibility, narrow/high-scale interaction, and real capture
   evidence for every implemented surface; and
5. close every remaining concrete row before release-grade integration.

Suggested articles: [`ManagerShell.md`](ManagerShell.md),
[`WizardShell.md`](WizardShell.md), [`RuntimeCapture.md`](RuntimeCapture.md), and
[`DesignCoverage.md`](DesignCoverage.md).
