# Material 3 regular-expression builder

Every Material 3 search field starts in plain-text mode and owns an independent,
adjacent regular-expression builder. The production implementation is shared by
`UICommon` through `UIMd3SearchField` and `UIMd3RegexBuilder`; a menu-hosted
field expands the complete builder inside that menu, while an ordinary field
opens a screen-bounded modeless dialog beside its builder button.

## Behavior and configuration

- The raw editor uses Qt `QRegularExpression`, whose syntax is PCRE2-compatible.
  The builder states the engine, escaping model, local-only preview behavior,
  and supported `i`, `m`, `s`, and `x` flags in the surface itself.
- Guided controls insert escaped literals, standard or custom character
  classes, subject and word anchors, capturing, non-capturing, named, and
  lookahead groups, alternation, and optional, unbounded, exact, or ranged
  quantifiers. Exact and ranged repetition use bounded number controls instead
  of asking the user to edit placeholder syntax.
- The sample editor updates a tree of matches and capture groups, including
  group names, spans, and bounded captured text. Pattern, flags, mode,
  validation, and the visible owning search field remain synchronized in both
  directions.
- **Copy pattern** copies only the expression. **Export JSON…** writes UTF-8
  schema version 1 with engine, pattern, normalized flags, and an explicit
  `sampleIncluded: false` marker; preview text is never exported implicitly.
- **Use plain text** returns the originating field to case-insensitive literal
  search. **Apply regex** accepts only a valid, completed preview and returns
  focus to the originating field.

Every owning field has a stable field identifier and its own builder state.
Patterns and sample input are local UI state; they are not sent over the network
or written to VirtualBox extra data by this component.

## Failure modes and security

Patterns and preview samples are limited to 4,096 characters. Flags are limited
to the supported set without duplicates, one preview retains at most 256
matches and 2,048 total match/capture rows, and each displayed capture is capped
at 512 characters. Preview work runs off the UI thread, only one worker may be
outstanding per builder, no more than two regex workers may execute process-wide,
and a 300 ms UI budget prevents a slow result from being applied or enabling
**Apply regex**. A newer generation always invalidates an older result.

Qt does not expose PCRE2's native match-limit controls through
`QRegularExpression`. A worker that crosses the UI deadline can therefore keep
finishing its current bounded match in the background, but it cannot block the
interface, cause that builder to enqueue more workers, or exceed the process-wide
two-worker ceiling. Closing the builder invalidates its guarded result. This
limitation is why preview/sample sizes and concurrency are bounded and why
timeout or capacity copy asks the user to shorten the input.

Export uses `QSaveFile` for atomic replacement. Export failure leaves the
previous destination untouched and is announced in the builder. Menu search
uses temporary proxy actions: filtering never changes the visibility of the
shared application-menu or action-pool command it represents.

## Accessibility and localization

All editors, result columns, guided controls, actions, and validation status
have accessible names. Status changes use an accessibility announcement when
the platform accessibility bridge is active. Controls use at least 48 logical
pixels, the content scrolls within a bounded viewport, and focus returns to the
originating field after Apply, plain-text selection, Cancel, close, or a live
language change. English, Cantonese, and bilingual presentation use stable
Material language keys; regular-expression syntax itself remains literal.

## Verification

The native Windows `UICommon` build from exact source commit
[`f29c7eb994d69c0e4ce69c5220110674ed9e846f`](https://github.com/Ding-Ding-Projects/material-virtualbox/commit/f29c7eb994d69c0e4ce69c5220110674ed9e846f)
compiled and linked the search field, builder, menu proxy, tab manager, and
their Qt MOC output with exit 0. Material 3 validation
[run 31328160092](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31328160092)
and Pages [run 31328160088](https://github.com/Ding-Ding-Projects/material-virtualbox/actions/runs/31328160088)
completed successfully; the deployed article returned HTTP 200 with the
process-wide worker ceiling. The Material 3
validation workflow maintains an explicit contract for every guided construct,
input/result bound, worker-generation and timeout control, capture tree,
copy/export path, focus boundary, and menu proxy. Native keyboard,
screen-reader, and visual captures remain part of the deferred real-application
capture gate; a design prototype is not runtime evidence.

Suggested articles: [Settings search](SettingsSearch.md),
[Tab navigation](TabNavigation.md), [Command palette](CommandPalette.md),
[Notification center](NotificationCentre.md), and
[Design coverage](DesignCoverage.md).
