# Settings search and regex builder

The Global Preferences and Machine Settings shell now uses the shared `UIMd3SearchField` rather than a second, settings-only filter widget. Plain text remains the default: typing in the field filters the existing `UISettingsPageFrame` descriptions and their child editors without replacing the selector, serializers, validators, or page models.

The `.*` affordance opens a non-blocking, anchored `UIMd3RegexBuilder` for that field. It accepts a bounded pattern, the supported `i`, `m`, `s`, and `x` flags, and sample text. Validation is local through Qt's `QRegularExpression`; invalid patterns stay in the builder and are not applied. Applying a valid pattern keeps the query and flags attached to that field, while **Use plain text** returns to the normal case-insensitive literal match. A field never shares regex state with another search surface.

The same settings header exposes the persisted language mode (English, Hong Kong Cantonese, or bilingual) and two independent 1–5 funny-level sliders. They call the shared `UIMd3Language` instance directly, emit live change notifications, and restore their values through the existing extra-data lifecycle rather than a second preferences store.

## Behavior and failure modes

- Patterns and samples are capped at 4096 characters and flags at 16 characters.
- A malformed pattern reports Qt's validation error and leaves the current filter unchanged.
- Regex matching is bounded to the local settings descriptions; there is no network fetch or provider-authored code execution.
- A blank plain-text query restores the normal expert/basic visibility rules. A valid regex reveals pages whose descriptions or descendants match it.
- The existing selector remains the source of truth for category identity and visibility. Search only changes filtering and does not discard settings edits.

## Accessibility and localization

The field, regex affordance, pattern, flags, sample, validation status, and actions have accessible names. Focus stays in the originating settings surface when the modeless builder closes. Copy is routed through Qt translations so English, Cantonese, and bilingual presentation can be supplied by the shared language layer without changing the filter contract.

## Verification

The MD3 validation workflow checks that both sources and both MOC headers are owned by `UICommon`. Qt `moc` is run against each new header before integration. Native GUI capture is still required for the final evidence: a static source check or design preview does not prove that the real settings dialog opens, filters pages, preserves focus, or renders at narrow/high-DPI sizes.

Suggested articles: [`NavigationRail.md`](NavigationRail.md), [`DesignCoverage.md`](DesignCoverage.md), and the repository [build instructions](https://www.virtualbox.org/wiki/Build_instructions).
