# Dim sum startup surprise

`UIMd3DimSumSurprise` is a process-wide singleton, created/destroyed in `main.cpp`
alongside `UIMd3History`/`UIMd3NotificationCentre`/`UIMd3Changelog`, that gives roughly
one launch in ten a small, un-opt-out-able, bilingual delight.

## What it does today

- **A flat 10% chance per eligible launch.** `QRandomGenerator::global()->bounded(10) == 0`
  is checked once, in the constructor, and the drawn dish index (or "none drawn")
  is fixed for the rest of the process's life — a fresh draw happens only on the
  next launch.
- **One dish, named bilingually.** `UIMd3DimSumSurprise::dishes()` compiles in twelve
  dish names as English/Cantonese pairs (e.g. `"Shrimp dumpling - Har Gow"`); one is
  chosen at random from that fixed list.
- **Non-blocking and auto-dismissing.** The toast is scheduled with a plain
  `QTimer::singleShot(4000, ...)` called *before* `QApplication::exec()` even
  starts — so it can only ever fire once the real event loop is already running
  everything else, never delaying, gating, or racing startup. It is a
  `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus`
  top-level widget shown with `Qt::WA_ShowWithoutActivating`, so it never steals
  focus, and it closes itself (fading out first, unless reduced motion is in
  effect) six seconds after appearing.
- **Never twice in one launch.** The drawn index is consumed (reset to "none")
  the instant the toast is shown; the singleton only ever schedules one timer.
- **No opt-out.** There is no setting, no extra-data key, no menu item that
  disables it. This is deliberate, per the house contract.
- **Never on a first run, and never on the launch right after an update.**
  `shouldSkipThisLaunch()` records the running build's `RTBldCfgVersion()` string
  in a small local marker file (`md3-dimsum-surprise-lastseen.txt`, next to the
  other MD3 local state under the Qt application-data location) every launch. If
  no marker could be read (first run ever) or the recorded version differs from
  this launch's version (this is the first launch of a build that just changed),
  the draw is skipped for *this* launch only — the marker is still updated, so
  the very next ordinary launch on the same build is eligible again.
- **Never on the error paths `main.cpp` already has.** The singleton is created
  after the CPU-architecture and Qt-version fatal checks (which `qFatal()`/exit
  before any MD3 singleton exists) and its timer is armed before
  `QApplication::exec()` runs. If `!uiCommon().isValid()` makes `main()` break out
  before ever calling `a.exec()`, the event loop that the timer needs never runs,
  so the toast structurally cannot appear on that path.

## Meaningful accessible text

The toast sets `setAccessibleName()`/`setAccessibleDescription()` on both the
top-level widget and its heading label to the full bilingual sentence (e.g.
*"Dim sum of the moment: Shrimp dumpling - Har Gow. Photo not included in this
build."*), and posts a `QAccessible::Alert` event via
`QAccessible::updateAccessibility()` so assistive technology has a chance to
announce it proactively — the toast never takes keyboard focus, so a screen
reader that only reacts to focus changes would otherwise never see it.

## Reduced motion

`prefersReducedMotion()` asks `QStyleHints::reduceMotion()` when the Qt this is
built against is new enough to expose it (Qt ≥ 6.6.0, checked at compile time
with `QT_VERSION_CHECK`); on any older Qt there is no reliable cross-platform
signal available, so motion is assumed to be fine. When reduced motion is
requested, the toast is dismissed with a plain `close()` instead of an opacity
fade.

## No dim-sum photography is bundled, on purpose

This repository's own house rule for this feature is explicit: dim-sum
photographs belong to the separate public-catalog project and must never be
vendored, generated, or fetched into this repository — and this build
environment has no network access regardless. `UIMd3DimSumSurprise` therefore
never tries to load, draw, or reference any picture. Every toast shows an
explicit, honest **"Photo not included in this build"** placeholder exactly
where a picture would otherwise go. This is the correct, complete
implementation of the "picture where available" half of the contract for this
repository, not a stand-in for one.

## Deliberately not yet done (honest scope)

- **No automated test.** No `tstUIMd3DimSumSurprise` exists; the 10% draw, the
  first-run/update skip, and the bilingual formatting were verified by reading
  the code, not by running it.
- **No compile verification in this lane.** This environment has no Windows/Qt
  toolchain; see `LocalGates.md`. Compile verification runs on the Windows
  packaging workflow, same as every other MD3 source added this way.
- **No screen capture** of the toast appearing, fading, or its accessible
  announcement — none was taken.
- **No positioning awareness of multiple monitors beyond "the primary screen's
  available geometry"** — on an unusual multi-monitor layout the toast could, in
  principle, land somewhere less convenient than intended; this is a cosmetic
  risk only.

## Suggested articles

- [`README.md`](README.md) — index of the MD3 retrofit effort this file belongs to.
- [`Changelog.md`](Changelog.md) / [`NotificationCentre.md`](NotificationCentre.md) —
  the two closest sibling singletons this class's lifecycle pattern was copied from.
