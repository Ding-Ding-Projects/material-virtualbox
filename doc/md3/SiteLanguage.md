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
  every mode. This is the site's equivalent of `UIMd3Language.cpp:160`, which
  reads verbatim
  `const QString strCantonese = pair.second.isEmpty() ? strEnglish : pair.second;`,
  and it is how the untranslated bulk of the page stays readable in Cantonese
  mode.
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
  `Language mode: …` / `語言模式：…`. The restored mode is written into that
  line as **text on load**, so the line is never blank, but it is deliberately
  **not announced** on load — nothing changed, and a live region that speaks on
  arrival talks over the page the reader just opened. The two are separated by
  *when the role is on the node*, not by a flag: the script reads the `role`
  off `#lang-status`, removes it, renders the text, and restores it in a
  `requestAnimationFrame` callback, so the accessibility tree adopts that text
  as the region's starting content instead of as a change. The attribute stays
  in the markup, so a reader with JavaScript disabled still meets the role, and
  every later mode change is a genuine live-region update.

  **Corrected 2026-08-16.** This bullet previously read "It is empty on load,
  so nothing is announced until the reader acts." The second clause was true;
  the first was a defect, not a design. Measured in headless Chrome at
  `7d5c9dece14`, reloading with `md3.language.mode=1` stored: the page was
  fully Cantonese and the Cantonese radio was correctly checked, yet
  `#lang-status` `textContent` on load was `""` in all three modes, and only
  became `"語言模式：粵語"` after the control was clicked. At this commit it is
  `"Language mode: English"`, `"語言模式：粵語"` and
  `"Language mode: Bilingual · 語言模式：雙語"` on a fresh, untouched load.
  A `MutationObserver` installed at document-start recorded the same three-step
  order in every mode: `role attribute: "status" -> null`, then
  `status text written: … while role=null`, then
  `role attribute: null -> "status"`.
- The buttons are labelled `English · 英文`, `Cantonese · 粵語`,
  `Bilingual · 雙語` in **all three modes**, and each Chinese half is its own
  `<span lang="zh-HK">`. Showing them in every mode is a deliberate deviation
  from a pure mirror of the application: a Cantonese reader arriving on the
  English default has to be able to find the control. It is called out here
  rather than hidden, and it is why the page's own otherwise CJK-free English
  mode has six CJK characters in the switcher chrome.

  **Fixed 2026-08-16 — WCAG 2.2 SC 3.1.2 Language of Parts.** Those six
  characters used to be the only untagged CJK on the page: everything else
  Cantonese is built by `cantoneseRun()`/`slotNode()`, which set `lang="zh-HK"`
  themselves, but the three labels are literal DOM text and inherited the
  document's `<html lang="en">`. A screen reader with an English voice
  therefore mispronounced the very control a Cantonese-seeking reader needs.
  Measured `[lang="zh-HK"]` element counts on a fresh, untouched load — at
  `7d5c9dece14`: English **0**, Cantonese **41**, bilingual **41**; at this
  commit: English **3**, Cantonese **45**, bilingual **45**. The `+4` in the
  translated modes is the three label spans plus the status line's own
  Cantonese run, which now exists on load. (An independent verifier reported
  42/42/0 for the old page; that is the same page measured *after* touching the
  control, which added the status line's one Cantonese run. Both numbers are
  reproduced above: `[lang="zh-HK"] on load 41` / `after click 42`.) The
  visible text is unchanged. CI asserts the shape of all three labels and that
  the source contains exactly three literal `lang="zh-HK"` occurrences.

## No JavaScript, no network, no motion

- **No JavaScript.** English is the literal DOM text, so the page without
  scripting is exactly the page that shipped before this feature: complete and
  readable. The switcher ships with the `hidden` attribute and the
  end-of-document script removes it, so a dead control is never painted. The
  three "the sections below stay in English" notes also ship `hidden`.
- **No network.** The page has zero subresources: no `<script src>`, no
  `<link>`, no `@import`, no `url()`, no `@font-face`, no `fetch`, no
  `XMLHttpRequest`. The strings are not split into a JSON file — `pages.yml`
  copies all of `docs/` to the site, so a strings file would have silently
  become an HTTP request. **CI rejects every one of those constructs**, and
  that is now literally true rather than approximately true: the guard at
  `.github/workflows/md3-validation.yml`'s "Validate MD3 source wiring" step
  used to read `'<script src|<link |@import|fetch\(|XMLHttpRequest'` — it did
  not check `url(` or `@font-face`, so this bullet was claiming two guards CI
  did not have. Rather than narrow the sentence, the two missing alternatives
  were added to the guard: it now reads
  `'<script src|<link |@import|fetch\(|XMLHttpRequest|url\(|@font-face'`.
  `docs/index.html` contains zero occurrences of either
  (`grep -c 'url(' docs/index.html` → `0`,
  `grep -c '@font-face' docs/index.html` → `0`), so the widened guard went
  green on arrival; injecting either construct into a scratch copy was
  confirmed to make the step throw `The documentation site must stay
  network-free` and exit 1. Measured in a real browser: one request across a
  page load plus two mode changes, and it is the page itself.
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

Beyond the page itself, `pages.yml` publishes 30 `doc/md3/*.md` articles
alongside `index.html`. They are English-only raw Markdown and the switcher
cannot reach them. `index.html` is a small fraction of the published site by
bytes. The correct word for S7 in
[`CompletenessInventory.md`](CompletenessInventory.md) is **Partial**, and that
is what it says.

The product name `Material Virtual Machine` in the `<h1>` is untranslated in
all three modes, deliberately.

### The unsigned-driver ceiling, and why its identifier is slotted

Added 2026-08-16. The page's most important sentence — **no virtual machine can
start**, because `VBoxSup.sys` is unsigned, 64-bit Windows will not load an
unsigned kernel driver, and code signing is permanently prohibited for this
project — sits outside the tab strip, so it is visible whichever panel is
selected, and it is translated like everything else on this page.

It is worth reading as the reference example of the `%N` slot mechanism, because
it is the case where getting it wrong would matter most. The paragraph is:

```html
<p id="ceiling" class="ceiling" data-md3-zh="%1 主機虛擬化驅動程式 %2 未經簽署，…">
  <strong data-md3-zh="冇任何虛擬機開得到。">No virtual machine can start.</strong>
  The host hypervisor driver <code>VBoxSup.sys</code> is unsigned, …
</p>
```

`%1` is the `<strong>`, which carries its own `data-md3-zh` and is therefore
re-texted and `lang`-tagged in Cantonese. `%2` is the `<code>`, which carries
none — so `slotNode()` clones it unchanged and the driver name is **the same
element, written once in the markup**, in every mode. That is the point: a
technical identifier that is typed twice can drift, and this one names the file
whose signature status is the whole ceiling. `md3-validation.yml` freezes its
source occurrence count at exactly one, and asserts the `%1`/`%2` shape, so
un-slotting it fails the build rather than passing quietly with two copies.

Rendered, all three modes confirmed in headless Chrome:

- **English** — `<strong>No virtual machine can start.</strong> … <code>VBoxSup.sys</code> …`, one occurrence.
- **Cantonese** — `<strong lang="zh-HK">冇任何虛擬機開得到。</strong><span lang="zh-HK"> 主機虛擬化驅動程式 </span><code>VBoxSup.sys</code><span lang="zh-HK"> 未經簽署，…</span>`, one occurrence, every Cantonese run `lang`-tagged.
- **Bilingual** — the English run, the `·` join, then the Cantonese run; two rendered occurrences of the identifier, both clones of the one source element.

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

### Re-verification of the two accessibility repairs (2026-08-16)

Both repairs above were measured, not asserted. Nothing was compiled; this
host still has no C++ toolchain.

1. **Source contract.** The `docs/index.html` block of the
   `Validate MD3 source wiring` step — the run of assertions that opens with
   `$pagesIndex = Get-Content docs/index.html -Raw` and ends where the
   `$managerWidget` assertions begin, carrying the accessible-tabs contract, the
   switcher contract, the new `lang="zh-HK"` label contract and the new
   status-line ordering contract — was de-indented verbatim into a
   scratch `docs-index-contract.ps1` outside the repository, with GitHub's own
   `if ((Test-Path -LiteralPath variable:\LASTEXITCODE)) { exit $LASTEXITCODE }`
   epilogue appended, and run from the repository root under PowerShell 7. It
   printed nothing and exited **0**. It was also proved able to fail rather
   than assumed to be: reverting the three label spans in a scratch copy made
   it print `Language switcher label must tag its Chinese half lang=zh-HK:
   english` and exit **1**, and deleting the single line
   `if (statusLine) statusLine.removeAttribute('role');` made it print
   `Pages status line must render without announcing on load:
   statusLine.removeAttribute('role')` and exit **1**.
2. **Real browser, before and after.** A scratch Node 26 script outside the
   repository served `docs/` over `http://127.0.0.1` (HTTP, not `file://`, so
   `localStorage` has a real origin) and drove Chrome `--headless=new` over the
   DevTools Protocol. For each mode it stored the index, **reloaded**, and read
   the fresh page. The pre-fix numbers quoted in the two bullets above come
   from running the same harness against `git show 7d5c9dece14:docs/index.html`.
   This harness is **not checked in and is therefore not a repeatable gate.**

**Technical facts still do not diverge between modes**, re-measured on this
tree: the distinct sets of `<code>` values (14), `href` values (23) and `<kbd>`
values (3) are byte-identical in all three modes, and each of the nine pinned
facts occurs once in `document.body.textContent` in English and in Cantonese.
The one honest exception is arithmetic, not drift: in **bilingual** mode
`#6750A4` occurs **twice**, because bilingual renders the English run and the
Cantonese run of the same paragraph and `%1` clones the `<code>` element into
both. The *value* is the same clone in both places, which is the property that
matters and the reason the source still contains it exactly once. This is the
same cloning already recorded for five facts in gate 20 of
[`LocalGates.md`](LocalGates.md).
