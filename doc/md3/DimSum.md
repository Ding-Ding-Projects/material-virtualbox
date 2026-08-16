# Dim sum startup surprise

`UIMd3DimSum` (`src/VBox/Frontends/VirtualBox/src/md3/UIMd3DimSum.{h,cpp}`) is a
process-wide service, created alongside the other Material 3 services in
`main.cpp`, that gives the Selector UI a 10%-per-launch chance of showing a
small, non-blocking toast naming a randomly chosen dim sum dish, with a
bilingual name and a small original illustration.

## Behavior

- **One fresh draw per process.** `UIMd3DimSum::create()` constructs the
  singleton, which immediately calls `QRandomGenerator::global()->bounded(100) < 10`
  exactly once and stores the result. Because the draw happens at
  construction rather than at display time, "a fresh random draw per launch"
  and "never fire twice in one launch" both hold structurally, not by
  discipline at each call site.
- **Never during a first run.** This codebase has no existing "has the app
  ever been launched before" signal to hook into (confirmed by search before
  this feature was added — see `doc/md3/CompletenessInventory.md` section 7's
  prior "zero matches" state), so this service tracks it itself: a tiny JSON
  file at `<AppDataLocation>/md3-dimsum.json` records `hasLaunchedBefore`. On
  a genuinely fresh profile that file does not exist yet, so launch 1 always
  skips the surprise (and writes the flag for next time); the surprise only
  ever becomes possible from launch 2 onward.
- **Never during an error path or a blocking flow.** `UIVirtualBoxManager::polishEvent()`
  invites the service to show its surprise via `maybeShowAtStartup(this)`,
  which arms a short (1.2 s) deferred timer rather than acting immediately —
  this lets genuine startup work, such as the medium-enumeration warning
  queued right before it in the same function, get first refusal of the
  screen. When the deferred attempt actually runs, it checks
  `QApplication::activeModalWidget()`; if a modal dialog currently owns the
  screen it waits a further second and retries, up to 3 attempts, then gives
  up quietly rather than fighting for attention. A hard startup abort (a
  `QMessageBox::critical` shown before any window is created, an invalid
  `UICommon`) never reaches `polishEvent()` at all, so this path is excluded
  by construction as well.
- **Never gates startup, never steals focus.** `maybeShowAtStartup()` only
  arms a timer and returns immediately. The toast window is created with
  `Qt::WA_ShowWithoutActivating` and is shown with a plain `show()` — never
  `activateWindow()`, never `setFocus()`.
- **Auto-dismissing, with a manual path too.** The toast closes itself after
  9 seconds (`QTimer::singleShot(9000, pToast, &QWidget::close)`), carries a
  visible **Dismiss** button, and installs an application-wide event filter
  for the duration it is visible so pressing <kbd>Escape</kbd> from anywhere
  closes it early — without ever taking keyboard focus itself. The event
  filter is removed in the toast's own destructor, which
  `Qt::WA_DeleteOnClose` guarantees runs once `close()` fires by any of the
  three routes (timeout, button, Escape).
- **No opt-out.** There is no setting, extra-data key, or code path that
  disables this feature. The only persisted state is the one
  `hasLaunchedBefore` flag described above; deleting
  `<AppDataLocation>/md3-dimsum.json` resets that flag (so the next launch is
  treated as a fresh first run again), matching the "delete the local
  application-data record to reset" recovery pattern already used elsewhere
  in this house contract — it does not and cannot disable the feature itself.
- **Bilingual, funny-level aware, and honest about which is which.** The
  toast's chrome — its title ("Dim sum surprise" / "飲茶驚喜"), its
  description sentence, and its Dismiss button — is registered with
  `UIMd3Language::registerText()` under the `md3.dimsum.*` keys and rendered
  through `md3Text()`, so it follows the active language mode and is styled
  by the active English/Cantonese funny-level sliders exactly like every
  other registered surface in this codebase. The dish's own bilingual name is
  **not** run through that styling: it is a fact (which dish this actually
  is), displayed literally in English, Cantonese, or both (joined with
  `" · "`, matching `UIMd3Language`'s own bilingual composition) depending on
  the active language mode. Voice is playful; the dish's real name never is.
- **Accessible.** The toast as a whole carries `setAccessibleName()` (the
  title) and `setAccessibleDescription()` (the full sentence naming the
  dish), and fires a `QAccessible::Alert` event on show so a screen reader
  can notice it despite it never taking focus. The illustration widget itself
  carries `setAccessibleName()` set to the dish's display name — this is this
  feature's alt text, satisfying "give each a meaningful alt text naming the
  dish" for a widget that has no other way to expose one. The **Dismiss**
  button is a real `UIMd3Button` (`UIMd3ButtonVariant_Text`), so it inherits
  that component's existing keyboard activation (<kbd>Space</kbd>/<kbd>Return</kbd>/<kbd>Enter</kbd>),
  visible focus ring, accessible `Button` role, and ≥48×48 minimum touch
  target.
- **No clipping, any width, any scale.** The toast card caps itself to 320
  logical pixels wide, wraps its dish-name and description labels, and reads
  its typography through `UIMd3Widget::effectiveFont()` (which already
  applies the process-wide font-scale and per-element overrides), so it
  scales the same way every other Material 3 surface in this codebase does.
  It is positioned from the *actual* available geometry of the screen the
  Manager window is currently on
  (`pParent->screen()->availableGeometry()`), clamped so it can never be
  placed off-screen even on a small or narrow display.

## Bundled catalogue, and why it is not the public one

The house contract this project otherwise follows for dim sum content points
every such surface at the public
[`Ding-Ding-Projects/dim-sum-photos`](https://github.com/Ding-Ding-Projects/dim-sum-photos)
catalogue, fetched over HTTPS and never vendored into a consumer repository.
This specific feature's own contract is different and more specific: it
requires **bundled local assets only, with no network of any kind** — the
surprise must work offline, on a machine that may have no connectivity at
all, without ever making the shipped application perform a network request
just to decide whether to show a startup toast.

Those two requirements cannot both be satisfied by fetching the public
catalogue at runtime. Given the explicit instruction for this feature ("if
the catalogue is not reachable offline, ship a small bundled subset and say
so"), this implementation ships a small offline subset — 8 dishes — compiled
directly into the binary as a static C++ table
(`g_aDishes` in `UIMd3DimSum.cpp`), in the same bilingual shape as the public
catalogue's own `name.en` / `name.zhHant` fields:

| id | English | Cantonese |
| --- | --- | --- |
| `har-gow` | Har Gow (Shrimp Dumpling) | 蝦餃 |
| `siu-mai` | Siu Mai (Pork and Shrimp Dumpling) | 燒賣 |
| `char-siu-bao` | Char Siu Bao (Barbecue Pork Bun) | 叉燒包 |
| `egg-tart` | Egg Tart | 蛋撻 |
| `cheung-fun` | Cheung Fun (Rice Noodle Roll) | 腸粉 |
| `lo-mai-gai` | Lo Mai Gai (Sticky Rice in Lotus Leaf) | 糯米雞 |
| `turnip-cake` | Turnip Cake | 蘿蔔糕 |
| `ma-lai-go` | Ma Lai Go (Steamed Sponge Cake) | 馬拉糕 |

These are real, well-known dim sum dishes — not invented ones — chosen to
satisfy "resolve dish metadata from the public catalogue pattern... rather
than inventing dishes" as closely as an offline subset honestly can.

**The "picture" for each dish is not a photo.** The house contract that
governs dim sum photos elsewhere in this project explicitly and repeatedly
forbids generating, downloading, scraping, or vendoring dim sum photographs
into a consumer repository under any circumstance — that prohibition is not
relaxed by this feature's own "ship a bundled subset" instruction, which
only speaks to metadata/network scope, not to fabricating photographic
content. So instead of a photo, each dish renders as a small, original,
code-drawn illustration: `md3PaintDishArt()` in `UIMd3DimSum.cpp` paints a
distinct `QPainterPath`/primitive shape per dish (a pleated triangular
dumpling, an open-top cylinder with a pea for siu mai, a cross-scored bun, a
scalloped tart shell, a rolled bar, a leaf-wrapped bundle, a sliced block, a
wedge) using the same `UIMd3Theme` color roles as the rest of the app. This
is original artwork authored directly as code — the same category of asset
as this project's own hand-authored SVG app-icon master documented in
`AppIcon.md` — not a generated, downloaded, stock, or scraped picture of any
kind.

## Known gap: Runtime UI (`VirtualBoxVM.exe`)

`UIMd3DimSum::create()`/`destroy()` in `main.cpp` are guarded by
`#ifndef VBOX_RUNTIME_UI`, so the service is created only in the Selector UI
process. Nothing in `src/VBox/Frontends/VirtualBox/src/runtime/` invites it
to show anything — matching this codebase's existing state for every other
Material 3 surface, per `CompletenessInventory.md`'s standing note that a
repository-wide search for `UIMd3` under `src/runtime/` returns zero matches.
Wiring a startup surprise into the per-VM Runtime window is a reasonable
future extension, but was left out of this pass rather than guessed at,
because "startup" has a different, debatable meaning there (once per VM
process launch, which can happen many times in one session) that deserves
its own decision rather than inheriting the Selector UI's semantics
unreviewed.

## Failure modes

- **No writable application-data location.** `storagePath()` returns an
  empty string when `QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)`
  does; `loadHasLaunchedBefore()`/`saveHasLaunchedBefore()` both treat that as
  "nothing to load/save" and return without touching disk. In that case
  `m_fFirstLaunchEver` is always `true` (nothing was ever loaded), so the
  surprise never fires on a system with no writable app-data path — a safe,
  quiet fallback rather than a crash or a repeated first-run state.
- **Corrupt or foreign-schema state file.** `loadHasLaunchedBefore()` bounds
  the read to 4 KiB, requires valid JSON, an object root, and
  `schemaVersion == 1`; anything else is treated as "never launched before",
  which is the same safe fallback as the missing-file case.
- **No caller ever invites the surprise.** `maybeShowAtStartup()` is only
  called from `UIVirtualBoxManager::polishEvent()`. If that call is ever
  removed, the service still rolls and persists state (harmlessly) but never
  shows anything — there is no other way for the feature to silently
  malfunction into showing when it should not, since display requires an
  explicit invitation from a real top-level window.
- **Empty catalogue.** `attemptShow()` checks `dishes.isEmpty()` before
  indexing; an empty catalogue (which cannot happen with the current static
  table, but is checked defensively) results in nothing shown rather than an
  out-of-bounds access.

## Security and privacy considerations

- No network request of any kind. Every dish name and every illustration is
  compiled into the binary; nothing is fetched, and no user data ever leaves
  the local machine because of this feature.
- The only thing written to disk is one boolean (`hasLaunchedBefore`) plus a
  schema version, in a small bounded JSON file under the application's own
  data directory — no telemetry, no identifiers, no user content.

## Verification

- Read-verified against this checkout: `UIMd3DimSum.{h,cpp}` compile-plausible
  against the exact Qt/IPRT APIs already used successfully elsewhere in this
  directory (`UIMd3NotificationCentre.cpp`'s `QSaveFile`/`QStandardPaths`/
  `QJsonDocument` pattern for the storage functions; `UIMd3Button`/`UIMd3Widget`
  for the toast's interactive chrome and container painting). This lane could
  not run `kmk` (no configured Qt/MSVC toolchain in this checkout, and the
  task explicitly excludes the ~90-minute full build) or launch the rebuilt
  binary (the installed `VirtualBox.exe` on this host predates this change
  and cannot be hot-swapped without rebuilding), so **there is no build or
  runtime capture evidence for this specific feature yet** — this is stated
  plainly rather than assumed. `doc/md3/LocalGates.md` gate 15/16 explain why
  a full local build is infeasible in this lane, and the same blocker applies
  here.
- Source-wiring is covered by a new step in
  `.github/workflows/md3-validation.yml` ("Validate dim sum startup
  surprise"), following the same source-pattern-contract methodology
  (`doc/md3/LocalGates.md` gate 2) already used for every sibling Material 3
  feature — this is a **T1**-class regression guard per
  `CompletenessInventory.md`'s evidence shorthand: it proves the required
  identifiers, kBuild wiring, and doc article exist in the source text, not
  that the feature compiles, links, or behaves correctly at runtime.
- No compiled or executed test exists for this feature (**T0**, in
  `CompletenessInventory.md` terms), matching the honest state of most of
  this codebase's Material 3 surfaces per that same document.

## Suggested articles

- [Local Gates inventory](LocalGates.md) — why a full local build/capture was
  infeasible for this lane, and what evidence class each check here is.
- [Completeness inventory](CompletenessInventory.md) — the per-surface,
  per-contract ledger this feature's section 7 row should be updated
  against.
- [App icon](AppIcon.md) — the project's other precedent for original,
  hand-authored, code-generated visual assets in place of a fetched or
  generated picture.
- [Language modes](../../src/VBox/Frontends/VirtualBox/src/md3/UIMd3Language.h) —
  the `registerText()`/`md3Text()` mechanism this feature's chrome copy uses.
