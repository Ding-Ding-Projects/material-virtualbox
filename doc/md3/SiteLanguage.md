# Documentation-site language switcher

`docs/index.html` — the page published at
<https://ding-ding-projects.github.io/material-virtualbox/> — has a three-mode
language switcher. This article records exactly what it does, exactly what it
does *not* translate, and how it was verified.

Read [`NavigationRail.md`](NavigationRail.md) and
[`CommandPalette.md`](CommandPalette.md) for the application-side language
modes this mirrors, and [`CompletenessInventory.md`](CompletenessInventory.md)
row S7 for the honest coverage arithmetic.

## The three modes

The site mirrors `UIMd3LanguageMode` in
`src/VBox/Frontends/VirtualBox/src/md3/UIMd3Language.h:36-41`, in the same
order and with the same semantics:

| Index | Mode | Rendering |
| --- | --- | --- |
| 0 | English | The literal DOM text, unchanged. This is the default. |
| 1 | Cantonese | The Cantonese counterpart, wrapped in `<span lang="zh-HK">`. |
| 2 | Bilingual | English, then `" · "` (U+00B7 with a space either side), then Cantonese — the same join as `QStringLiteral("%1 · %2")` in `UIMd3Language::text()`. |

Other behaviour copied from `UIMd3Language.cpp`:

- **Fallback.** An element with no Cantonese counterpart renders English in
  every mode. This is the site's equivalent of
  `strCantonese.isEmpty() ? strEnglish` at `UIMd3Language.cpp:159-160`, and it
  is how the untranslated bulk of the page stays readable in Cantonese mode.
- **Default.** English, matching `m_enmMode(UIMd3LanguageMode_English)` at
  `UIMd3Language.cpp:77`.
- **Persistence.** `localStorage` key `md3.language.mode`, holding the string
  `"0"`, `"1"` or `"2"`, mirroring the `GUI/Md3/LanguageMode` extra-data key
  and its integer encoding. Reading it clamps exactly like
  `qBound(0, …toInt(), 2)`: an unparseable value is English, an out-of-range
  value clamps to bilingual. Every read and write is wrapped in `try`/`catch`,
  so a browser with storage disabled still gets a working switcher for that
  page view.

**Not mirrored:** the two independent playfulness levels
(`GUI/Md3/FunnyLevelEnglish`, `GUI/Md3/FunnyLevelCantonese`, and the suffix
tables at `UIMd3Language.cpp:26-43`). The site has no playfulness control and
appends no suffixes.

## How the copy is stored

Cantonese lives only in attributes; English is the literal document text. That
ordering is the whole design, and it is what makes the no-JavaScript case
correct rather than merely tolerable.

- `data-md3-zh="…"` on an element supplies that element's Cantonese
  counterpart.
- Inside that attribute, `%1`…`%9` are slots for the element's own child
  elements, in document order. The renderer *moves clones of the original
  children* into those slots. It never retypes their contents.
- `data-md3-label-zh="…"` supplies the Cantonese counterpart of an element's
  `aria-label`.
- Rendering uses `createElement` / `textContent` / `replaceChildren` only.
  There is no `innerHTML` in the file, and CI rejects one.

The slot rule is why every technical fact — the three build hashes, both
commit SHAs, both Actions run numbers, `MSVC 14.44`,
`Windows SDK 10.0.26100.0`, `Qt 6.8.3`, `#6750A4`, the build timestamps, every
`*.md` file name and every `href` — is written **once** in the source and
cannot drift between language modes. The workflow enforces the occurrence
count of each of those facts, so a translation that pastes a hash into a
Cantonese string fails the build.

## The control

- A `<div class="langbar" role="radiogroup" aria-label="Site language">` with
  three `<button type="button" role="radio" aria-checked="…">` children. It is
  deliberately **not** `role="tab"` and **not** `aria-selected`: the existing
  tab strip's script selects `[role=tab]` across the whole document and would
  otherwise swallow these buttons, and the stylesheet's
  `button[aria-selected=true]` fill would otherwise apply to them by accident.
  CI asserts the document contains exactly three `role="tab"` elements.
- Roving `tabindex`: the checked button is `0`, the other two are `-1`.
- Keyboard: ArrowRight/ArrowDown select the next mode, ArrowLeft/ArrowUp the
  previous, both wrapping; Home and End select the first and last; Enter and
  Space select the focused mode. Arrow movement also selects, which is the
  standard radiogroup behaviour.
- Changes are announced through a `role="status"` live region reading
  `Language mode: …` / `語言模式：…`. It is empty on load, so nothing is
  announced until the reader acts.
- The buttons are labelled `English · 英文`, `Cantonese · 粵語`,
  `Bilingual · 雙語` in **all three modes**. This is a deliberate deviation
  from a pure mirror of the application: a Cantonese reader arriving on the
  English default has to be able to find the control. It is called out here
  rather than hidden, and it is why the page's own CJK-free English mode has
  six CJK characters in the switcher chrome.

## No JavaScript, no network, no motion

- **No JavaScript.** English is the literal DOM text, so the page without
  scripting is exactly the page that shipped before this feature: complete and
  readable. The switcher ships with the `hidden` attribute and the
  end-of-document script removes it, so a dead control is never painted. The
  three "the sections below stay in English" notes also ship `hidden`.
- **No network.** The page has zero subresources: no `<script src>`, no
  `<link>`, no `@import`, no `url()`, no web font, no `fetch`. The strings are
  not split into a JSON file — `pages.yml` copies all of `docs/` to the site,
  so a strings file would have silently become an HTTP request. CI rejects any
  of those constructs appearing in the file. Measured in a real browser: one
  request across a page load plus two mode changes, and it is the page itself.
- **No motion.** The switcher introduces no transition and no animation, and
  the stylesheet carries an explicit
  `@media(prefers-reduced-motion:reduce)` block that neutralises animation,
  transition and `scroll-behavior` across the page. Changing mode never
  animates, cross-fades, or scrolls. There were no transitions or animations
  in this file before this change either; the block exists so the guarantee is
  greppable and cannot be lost silently.

**One honest residual:** a reader with Cantonese or bilingual persisted may see
a single frame of English before the end-of-document script swaps the text.
That is a fallback rendering being enhanced, not hidden content. It is *not*
fixed by hiding the body until the script runs, because that would risk a blank
page if the script ever threw.

## What is and is not translated

Of the visible characters in `<body>`, the Cantonese copy covers the page
furniture (status line, lede, footer, `<title>`), all three tab labels, the
tablist's accessible name, all three section headings, all twelve coverage card
titles, the whole Overview panel, and the two status phrases in the
Verification cards.

It does **not** cover:

- the twelve coverage card body paragraphs, which are the bulk of the page's
  prose;
- the four muted footnote paragraphs at the end of the Coverage panel;
- the Verification panel's lede and closing paragraph.

Those untranslated regions are the reason a note reading "The detailed sections
below and every linked document stay in English for now." appears at the top of
each panel in Cantonese and bilingual modes. It is shown to the reader, not
buried in this file.

Beyond the page itself, `pages.yml` publishes 29 `doc/md3/*.md` articles
alongside `index.html`. They are English-only raw Markdown and the switcher
cannot reach them. `index.html` is a small fraction of the published site by
bytes. The correct word for S7 in
[`CompletenessInventory.md`](CompletenessInventory.md) is **Partial**, and that
is what it says.

The product name `Material Virtual Machine` in the `<h1>` is untranslated in
all three modes, deliberately.

## Verification

Two independent checks were run on a Windows 11 host. **Nothing was compiled;
this change touches HTML, YAML and Markdown only, and this host has no C++
toolchain.**

1. **Source contract.** The `Validate MD3 source wiring` step of
   `.github/workflows/md3-validation.yml` — which now carries the switcher
   contract alongside the pre-existing accessible-tabs contract — was
   de-indented verbatim into a scratch `step2-md3-source-wiring.ps1` outside
   the repository and run from the repository root under PowerShell 7.6.5:
   `step2 PASSED`, exit 0. Seven deliberate mutations of `docs/index.html`
   were each confirmed to make it throw. See gate 2 in
   [`LocalGates.md`](LocalGates.md).
2. **Real browser.** A scratch Node 26 script outside the repository drove
   Chrome `--headless=new` over the DevTools Protocol against
   `file:///…/docs/index.html`, using real `Input.dispatchKeyEvent` key
   presses, `Network.enable` request capture and
   `Emulation.setScriptExecutionDisabled`. 48 of 48 checks passed. Screenshots
   of all three modes were captured and read. See gate 20 in
   [`LocalGates.md`](LocalGates.md) for the full list of what was asserted —
   **including the fact that this harness is not checked in and is therefore
   not a repeatable gate.**

No claim is made here about the deployed site. Nothing in this pass fetched
the published URL.
