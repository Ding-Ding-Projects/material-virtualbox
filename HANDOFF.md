# Material Virtual Machine handoff

The Material Design 3 rewrite is an incremental presentation layer inside the
existing VirtualBox Qt frontend. It does not introduce a parallel application
or replace VirtualBox models, action pools, COM/XPCOM contracts, page
serializers, validation, guest-display ownership, or session lifecycle.

## Current state

- The shared theme, stock-control style, language service, full regex/search
  infrastructure, appearance editor, command palette, local history, and
  notification history are owned by `UICommon`.
- Theme palettes are derived through the native HCT/CAM16 implementation and
  compiled reference-vector testcase; the former HSL approximation is removed.
- The manager has a native frameless header, responsive navigation rail,
  on-demand workspace tabs, compact navigation, and contextual destination
  heading while preserving the existing tool and chooser models.
- Extensions, Media, Network, Cloud, and VM Activity Overview share a bounded
  Material search and appearance card over their existing item views. Original
  hidden rows are restored when a query or destination changes.
- Global Preferences and per-machine Settings use one bounded selected page at
  a time; search intentionally changes to cross-page results.
- Native wizards use a responsive Material step rail and page card, reconcile
  hidden Basic/Expert pages, scroll internally on compact Windows desktops, and
  retain the real page stack and native action handlers.
- The design ledger still marks every incomplete archive row `In progress`.
  Manager-tool completion, runtime chrome, native interaction evidence,
  installer/release work, and final accessibility proof remain open.

## Verification and blockers

The current manager-tools working tree passes `git diff --check` and local
Windows x64 `UICommon`, `VirtualBox`, and `VirtualBoxVM` builds. The new shared
source compiled, the manager bindings compiled, and all three targets linked
with exit 0. The archive/source contract, workflow structure, XML/HTML, exact
commit, and CI/Pages evidence are the remaining publication gates for this
lane.

The development executable cannot currently open the real manager because its
COM/SDS classes are not registered. Design HTML and thumbnails are not used as
runtime evidence. System-wide registration requires a separately authorized
administrator operation; until that boundary changes, real application capture
rows remain open.

## Next owner

Continue from [`doc/md3/CodexHandoff.md`](doc/md3/CodexHandoff.md), reconcile
each lane against [`doc/md3/DesignCoverage.md`](doc/md3/DesignCoverage.md), and
keep target ownership and existing VirtualBox behavior authoritative. The next
broad lane remains manager-tool completion, followed by runtime chrome. Do not
close the design objective from static or prototype evidence.
