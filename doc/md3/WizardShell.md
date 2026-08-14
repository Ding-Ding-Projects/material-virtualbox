# Material 3 wizard shell

The native wizard shell is an embedded presentation layer around the existing
`UINativeWizard` page stack. It applies Material 3 surface, typography, step
progress, and accessible state without replacing the real wizard pages,
validation, button API, or notification center.

## Behavior

- Every `UINativeWizard` instance creates one `UIMd3Wizard` shell in the
  shared `UICommon` target.
- At desktop widths the shell presents a 176–220 logical-pixel step rail beside
  a rounded page card. The card owns the translated page title, an explicit
  “Step *n* of *m*” summary, the real page stack, and the existing wizard action
  row. Below 720 logical pixels the decorative rail collapses while the page
  title and progress summary remain available.
- The step labels come from real `UINativeWizardPage::title()` values and are
  elided visually without changing their full accessible names. Reapplying the
  same titles no longer destroys and rebuilds every step widget.
- The existing page `QStackedWidget` remains the content authority. Basic and
  Expert mode visibility still belongs to `UINativeWizard`; hidden pages are
  omitted from the step rail and the visible page index is mapped back to the
  real stack rather than treating a hidden page as a user-selectable step.
- On Windows, the page stack is hosted in an internal, frame-free scroll area
  and the legacy fixed watermark column is omitted. The dialog is bounded to
  the available working area, so compact and high-scale desktops scroll page
  content instead of forcing the whole wizard off-screen.
- The current step is marked complete as soon as its real page reports
  `isComplete()`, so the step row follows validation rather than navigation
  alone. `sltCurrentIndexChanged()` refreshes the shell after the page is
  initialized, and `sltCompleteChanged()` refreshes it when validity changes;
  both are needed, because a page returned to with Back emits no
  `completeChanged`.
- `setStepTitles()` returns early when the submitted titles are unchanged.
  Page changes and retranslation submit the same list repeatedly, and the
  labels must keep their identity so assistive technology does not see the
  whole step row destroyed and rebuilt on every navigation.
- Back, Next/Finish, Cancel, Help, `validatePage()`, `isComplete()`, page
  initialization, `wizardWindow<T>()`, and notification progress retain their
  existing ownership and `QPushButton*` API. Their existing row is integrated
  into the Material shell with 48-logical-pixel minimum targets rather than
  duplicated by presentation-only controls.
- A completed prior step is announced with a check mark, while the current
  step uses the active Material 3 container role. The shell itself has an
  accessible name and progress description. Each step also exposes a
  localized state description (current, completed, or upcoming) without
  pretending that the decorative step labels are clickable.
- The native page stack has the stable `wizardPageStack` object name, an
  accessible `Wizard pages` name, and a live description naming the current
  page. Back, Next/Finish, Cancel, and Help retain their existing handlers but
  now expose localized accessible names and action descriptions; an incomplete
  page explains why Next is unavailable, and the enabled path restores the
  correct Next or Finish name and description when validation becomes true.

## Configuration and localization

The shell uses the active `UIMd3Theme` surface, typography, spacing, density,
and color roles. Stable `md3.wizard.*` keys provide English, Cantonese, and
bilingual shell and action copy through `UIMd3Language`; the existing Qt
translation remains the safe early-lifecycle fallback. Page titles still come
from the real pages and refresh through the existing translation listener. The
shell does not persist wizard data; the wizard's existing page serializers
remain the authority.

## Failure and security behavior

The shell is presentation-only and does not parse guest data, invoke Git, or
change wizard validation. If the theme singleton is unavailable during an
early construction path, it falls back to the Qt palette and standard spacing.
No page content is copied into a second model, which avoids stale validation
or duplicate notification ownership. Provider- and guest-authored text stays
in the existing page widgets and renderer paths.

## Verification

`UIMd3Wizard.{h,cpp}` is listed in `UICommon_QT_MOCHDRS` and
`UICommon_SOURCES`. `UINativeWizard` composes the shell while retaining every
existing page and button contract. Static source contracts cover the two-card
composition, compact breakpoint, idempotent step catalog, visible-page mapping,
page scroll area, integrated action row, 48-pixel targets, stable language keys,
page-stack accessibility, and native action names. Local Windows x64 kBuild
runs for `UICommon`, `VirtualBox`, and `VirtualBoxVM` all exited 0 on the exact
source tree for this lane. Commit-attributed artifact hashes and GitHub Actions
evidence are recorded after the source commit lands. Native wizard captures
remain blocked until the COM/SDS development runtime can launch the manager;
design previews are not substituted for application evidence.

## Open work

The shell does not yet provide cross-wizard browser tabs, a full validation
summary panel, drag-reordered steps, runtime accessibility-tree captures, or a
genuine native application capture. Those belong to later lanes and must not be inferred
from the visual stepper.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`NotificationCentre.md`](NotificationCentre.md), and
[`RuntimeCapture.md`](RuntimeCapture.md).
