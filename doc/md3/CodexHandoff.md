# Implementation contract

This is the standing contract for anyone — human or agent — continuing the Material Design 3
work in this repository. It exists because the single largest risk to this project is not
missing work: it is a well-intentioned pass that reimplements something VirtualBox already
owns, or that "fixes" a reviewed adaptation back to the raw prototype and silently regresses
accessibility. Read this before touching a surface.

## Mission

Finish the native Qt/C++ implementation so the checked-in `design/` package is the UI
contract for this VirtualBox frontend.

This is **not** a greenfield rewrite. A substantial Material 3 implementation already exists
and is wired into the real frontend and its kBuild targets. The task is to reconcile,
complete, verify and harden it against the design archive.

## Ownership that is not up for negotiation

| Concern | Authority |
|---|---|
| Commands | The existing `QAction` pools. A Material control binds to a `QAction`; it does not re-derive whether the action is available. |
| Machine data and selection | The existing manager/tool models. |
| Settings load, save and validation | The existing `UISettingsPage` subclasses. |
| Wizard flow, validation, commit and cleanup | The existing `UINativeWizard` / `UINativeWizardPage`. |
| VM session, capture, display, multi-monitor and runtime lifecycle | `UIMachineWindow`, `UIMachineLogic`, `UIMachineView` and their handlers. |
| Theme | `UIMd3Theme`, and nothing else. |
| Persistence | VirtualBox extra data and the existing settings APIs. |

Do not add a web frontend, a WebEngine dependency, a React or CSS runtime, a second
preferences store, a second command registry for domain actions, or a second VM state model.
The `design/*.dc.html` prototypes are executable design evidence; `design/support.js` is their
browser bootstrap. Neither is a production dependency.

## Precedence when sources disagree

1. **Safety, session and domain semantics.** Never change VM or session correctness to match a
   prototype.
2. **Accessibility and native platform requirements.** These may override a prototype
   measurement — but only as a *documented* deviation, recorded in the table below.
3. **`design/*.dc.html`.** Default authority for hierarchy, spacing intent, content priority,
   component choice, ordering and visible states.
4. **Approved `doc/md3/*.md` contracts.** Deliberate reconciliations between prototype and
   native constraints.
5. **`design/cpp/md3/*`.** API reference, superseded where production has moved on.
6. **Existing implementation.** Evidence, not automatically the contract. A discrepancy gets
   corrected or documented — never silently blessed.

`design/HANDOFF.md` says the prototype wins outright. That instruction predates the reviewed
native adaptations below and must not be applied literally.

## Approved native deviations

A deviation is legitimate only if it appears here with a reason.

| Design element | Prototype | Production | Reason |
|---|---|---|---|
| Top header height | 44 px in all five prototypes | 48 px | Windows title-bar hit targets and drag regions. Enforced by the manager shell contract and CI. |
| Interactive control minimum | 32–38 px | 48 px minimum focusable target | Pointer and touch hit-target accessibility. `UIMd3Style` raises stock controls to `qMax(48, controlHeight())`. |
| Pixel units | CSS pixels | Logical (device-independent) pixels | Qt 6 lays out in device-independent pixels and applies the device pixel ratio itself. 48 means 48 logical pixels. |
| Error colours | Mixed literals across prototypes | The generated error palette at hue 25, chroma 84 | The prototypes are internally inconsistent here; a generated family keeps destructive surfaces coherent under any seed. See [`TonalPalette.md`](TonalPalette.md). |
| Prototype page-to-page links | Browser navigation between prototypes | Not reproduced | They exist to make the archive inspectable; they are not product navigation. |

Navigation rail width (92 px), workspace strip and tab-body sizing follow the prototypes and
are **not** deviations.

## Material rules

- Resolve every colour through `UIMd3ColorRole` and `md3Theme()`. A prototype colour literal
  must never be copied into a production widget.
- Use `UIMd3Shape`, the state-layer opacities and the motion tokens rather than ad hoc values.
- Style stock Qt controls through `UIMd3Style`. Qt already provides accessibility for standard
  widgets; subclass only where the semantics genuinely differ.
- Every Material widget reacts to `UIMd3Theme::sigThemeChanged`.
- Every per-element customization uses a stable appearance key.
- Palettes are generated in HCT — see [`TonalPalette.md`](TonalPalette.md). The former
  HSL approximation is gone and must not come back; the `tonal-palette` CI job enforces this.

## State: find the owner before writing a field

For every piece of prototype state, ask which production object already owns it. If one does,
the Material layer reads it — it does not keep its own copy.

| Prototype concept | Owner |
|---|---|
| `expert` | The existing VirtualBox experience level, not a new MD3 flag |
| Selected global tool | The existing `UIToolType` model |
| Running / paused | VM and session state |
| Runtime menu check states | `UIActionPoolRuntime` |
| Settings validity | The existing settings pages |
| Wizard completion | `UINativeWizard` |
| Search text and regex flags | The owning `UIMd3SearchField` |
| Theme, brand, density | `UIMd3Theme` and `GUI/Md3/*` extra data |
| Tabs, groups, pins | `UIMd3TabStrip` |
| Notification history | `UIMd3NotificationCentre` |
| Prototype toast arrays and mock VMs | Nothing — preview scaffolding |

## Build ownership

| Code | Target |
|---|---|
| Shared Material primitives and services — theme, colour science, style, language, buttons, search, regex, notifications, history, wizard shell, tab infrastructure | `UICommon` |
| Manager-only shell composition | `VirtualBox` |
| Runtime-only Material chrome | `VirtualBoxVM` |
| Icons | The existing `UIMd3Icons.qrc` |

A source belongs to exactly one target unless there is a stated reason.

## Definition of done for a surface

Compiling and resembling a screenshot is not done. A surface is done when:

- every archive row that covers it names real production files in `DesignCoverage.md`, not a
  directory;
- every designed control and state has a production disposition, or is explicitly recorded as
  preview-only;
- real actions and models are wired, with no duplicated state;
- persistence survives a restart, where the surface persists anything;
- keyboard traversal, focus behaviour and the accessible tree are verified, not asserted;
- dark, light and high-contrast schemes pass, as do the required DPI steps;
- no existing VirtualBox behaviour was lost;
- the evidence recorded is the kind that can fail — a compiled test, a real build, a native
  capture — and it names the target, commit, host and Qt version.

Source-pattern CI is a regression guard. It cannot prove a button is visible, a key sequence
works, or an accessibility tree is correct. Prefer adding an executable gate; the
`tonal-palette` job is the pattern to copy.

## Open lanes

Ordered as the work should be taken, not as it is easiest:

1. **Manager completion** — chooser, details and tool painting; native title-bar behaviour;
   remaining rail, tab and group lanes.
2. **Settings and Preferences completion** — all ten machine pages and every global page;
   explicit disposition for every designed preference field.
3. **Managers completion** — Media, Network, Extension Pack, Cloud, Log and Activity, each
   recomposed around its existing model.
4. **Wizards** — remaining prototype gaps, Basic/Expert linkage, native keyboard and AT proof.
5. **Notifications** — the designed transient presentation, without moving blocking questions
   or progress away from their existing owner.
6. **Runtime chrome** — Material chrome as an adapter over the real action pool, status and
   session. Last, and smallest: mistakes here affect running VMs.
7. **Native acceptance** — DPI matrix, accessibility tree, localization, visual regression,
   installer and update smoke checks.

Native Manager and Runtime capture remain blocked on a registered `VirtualBoxClient` COM
runtime; see [`RuntimeCapture.md`](RuntimeCapture.md). Design previews are not accepted as
application proof, and no lane may be closed on them.
