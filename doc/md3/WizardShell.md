# Material 3 wizard shell

The native wizard shell is an embedded presentation layer around the existing
`UINativeWizard` page stack. It applies Material 3 surface, typography, step
progress, and accessible state without replacing the real wizard pages,
validation, button API, or notification center.

## Behavior

- Every `UINativeWizard` instance creates one `UIMd3Wizard` shell in the
  shared `UICommon` target.
- The shell presents the translated current page title, an explicit “Step *n*
  of *m*” summary, and a bounded stepper whose labels come from real
  `UINativeWizardPage::title()` values.
- The existing page `QStackedWidget` is inserted as the shell content. Basic
  and Expert mode visibility still belongs to `UINativeWizard`; hidden pages
  are not invented or made selectable by the shell.
- The stepper is built from visible pages only, and the real stack index is
  mapped to the visible step index. A wizard that hides pages in Basic mode
  therefore reports “Step *n* of *m*” over the steps the user can actually
  reach, instead of counting pages that will never be shown.
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
  existing ownership and `QPushButton*` API.
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
and color roles. Page titles are refreshed by the existing translation
listener, so language changes do not create a second label source. The shell
does not persist wizard data; the wizard's existing page serializers remain
the authority.

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
existing page and button contract. Static source contracts cover the shell's
step-state descriptions, the page-stack accessible name/description, and all
native wizard action names. The focused Windows `UICommon` kBuild target is
the required compile gate; native wizard captures are deliberately deferred
for the current task because the COM/SDS runtime boundary is not registered.

## Open work

The shell does not yet provide cross-wizard browser tabs, a full validation
summary panel, drag-reordered steps, or runtime accessibility-tree captures.
Those belong to later lanes and must not be inferred from the visual stepper.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`NotificationCentre.md`](NotificationCentre.md), and
[`RuntimeCapture.md`](RuntimeCapture.md).
