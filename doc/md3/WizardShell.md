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
- Back, Next/Finish, Cancel, Help, `validatePage()`, `isComplete()`, page
  initialization, `wizardWindow<T>()`, and notification progress retain their
  existing ownership and `QPushButton*` API.
- A completed prior step is announced with a check mark, while the current
  step uses the active Material 3 container role. The shell itself has an
  accessible name and progress description.

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
existing page and button contract. The focused Windows `UICommon` kBuild
target is the required compile gate; native wizard HuiShots are deliberately
deferred for the current task because the COM/SDS runtime boundary is not
registered.

## Open work

The shell does not yet provide cross-wizard browser tabs, a full validation
summary panel, drag-reordered steps, or runtime accessibility-tree captures.
Those belong to later lanes and must not be inferred from the visual stepper.

Suggested articles: [`SettingsSearch.md`](SettingsSearch.md),
[`NotificationCentre.md`](NotificationCentre.md), and
[`RuntimeCapture.md`](RuntimeCapture.md).
