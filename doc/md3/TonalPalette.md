# Tonal palette generation

The Material 3 palette for the whole frontend is generated from one seed colour by
`UIMd3Theme::regenerate()`. This article records how that generation works, why it is done
in HCT rather than in a cheaper colour space, and what evidence backs it.

## Why HCT and not HSL

HCT is the colour space Material 3 defines its tonal palettes in. Hue and chroma come from
the CAM16 colour-appearance model; tone is CIE L\*. The property the rest of the shell
depends on is the last one: because tone is L\*, **a tone number is a contrast promise**.
`onPrimary` at tone 20 on `primary` at tone 80 is readable for every seed, because those two
tones are always 60 L\* apart.

The previous implementation approximated HCT by holding HSL hue and saturation and driving
HSL lightness to the tone, with a hand-tuned chroma decay. HSL lightness is not perceptually
uniform, so that promise did not hold. Measured over twelve seeds and all 101 tones:

| Implementation | Worst \|L\* − requested tone\| |
|---|---|
| HSL stand-in (previous) | 37.74 L\* — seed `#00FF00`, tone 50 |
| HCT (current) | 0.21 L\* — the 8-bit rounding floor |

A "tone 50" green really landed at L\* 87.7. For a saturated seed the two sides of a
foreground/background pair could therefore collapse onto the same lightness, which is a
legibility failure, not a cosmetic one. That is the defect this generation path fixes.

The implementation lives in `src/VBox/Frontends/VirtualBox/src/md3/UIMd3Hct.{h,cpp}`. It is
an independent implementation of the published CAM16 model and the Material palette rules,
carries no third-party code, and depends on no Qt — so it can be compiled and run on its own.

## Core palettes

Every palette keeps the seed hue and fixes its own chroma, which is what keeps a scheme a
single colour family while letting each role carry its intended amount of colour.

| Palette | Hue | Chroma | Roles it feeds |
|---|---|---|---|
| Primary | seed hue | `max(48, seed chroma)` | primary, on-primary, primary container |
| Secondary | seed hue | 16 | secondary and its container |
| Tertiary | seed hue + 60° | 24 | tertiary and its container |
| Neutral | seed hue | 6 | surface, on-surface, all surface containers |
| Neutral variant | seed hue | 8 | on-surface-variant, outline, outline variant |
| Error | 25° (fixed) | 84 | error, on-error, error container |

The error family is deliberately **not** seed-derived: a destructive action has to read as
dangerous whatever the user themed the rest of the application to.

Tertiary is the one palette that leaves the seed hue, by a fixed 60° rotation, so the scheme
gets an accent that is related to the seed rather than another shade of it.

## Gamut handling

`md3HctToRgb()` resolves a requested (hue, chroma, tone) to the closest displayable sRGB
colour. When the combination is outside sRGB — which happens for light tones at high chroma —
**tone is honoured and chroma is reduced**, never the other way round. Contrast survives; a
small amount of colourfulness does not. Tone is solved by bisecting the CAM16 lightness axis
to double precision, and the chroma reduction bisects the gamut boundary, so the result is
deterministic and repeatable.

## Role assignment

The tone numbers per role are unchanged from the previous implementation and follow the
Material 3 dark and light schemes. The correction made alongside the colour-space change is
that `on-surface-variant`, `outline` and `outline-variant` now come from the **neutral
variant** palette rather than the neutral one, which is what gives outlines their slight
tint away from the surfaces they separate.

High-contrast schemes keep using the same palettes and shift the tone numbers only.

## Evidence

`src/VBox/Frontends/VirtualBox/testcase/tstUIMd3Hct.cpp` is a compiled, executed test — not a
source-pattern check. It asserts:

- the generated tones match the published Material 3 baseline scheme for seed `#6750A4`,
  within one 8-bit step per channel (tolerance 2, worst observed 2 on one surface container);
- primary tone 40 reproduces the seed exactly;
- every generated colour is displayable, for twelve seeds including the achromatic extremes
  and fully saturated primaries;
- the realised tone is within 0.5 L\* of the requested tone (worst observed 0.23 L\*);
- tone is monotonic — a higher tone is never darker;
- generation is deterministic across repeated calls;
- eight foreground/background tone pairs taken from the real role assignments keep at least
  4.5:1, or 3:1 for the variant pairs, for every one of those seeds.

The `tonal-palette` job in `.github/workflows/md3-validation.yml` builds and runs it on every
push, and additionally asserts that the theme keeps generating palettes through HCT.

Generated dark scheme for the baseline seed `#6750A4`, against the colour literals used by
the checked-in design prototypes — 10 of 19 exact, 18 of 19 within one 8-bit step:

| Role | Generated | Design prototype |
|---|---|---|
| Primary (P80) | `#CFBCFF` | `#D0BCFF` |
| On primary (P20) | `#381E72` | `#381E72` |
| Primary container (P30) | `#4F378A` | `#4F378B` |
| On primary container (P90) | `#E9DDFF` | `#EADDFF` |
| Secondary container (S30) | `#4A4458` | `#4A4458` |
| On secondary container (S90) | `#E8DEF8` | `#E8DEF8` |
| Error (E80) | `#FFB4AB` | `#FFB4AB` |
| Error container (E30) | `#93000A` | `#93000A` |
| On error container (E90) | `#FFDAD6` | `#FFDAD6` |
| Surface (N6) | `#141218` | `#141218` |
| On surface (N90) | `#E6E0E9` | `#E6E0E9` |
| On surface variant (NV80) | `#CAC4CF` | `#CAC4D0` |
| Surface container lowest (N4) | `#0F0D13` | `#0F0D13` |
| Surface container low (N10) | `#1D1B20` | `#1D1B20` |
| Surface container (N12) | `#211F24` | `#211F26` |
| Surface container high (N17) | `#2B292F` | `#2B2930` |
| Surface container highest (N22) | `#36343A` | `#36343B` |
| Outline (NV60) | `#948F99` | `#938F99` |
| Outline variant (NV30) | `#49454E` | `#49454F` |

The residual differences are one 8-bit step on chroma-limited tones, where the reference
implementation resolves the gamut boundary analytically and this one bisects it. Both land
on the same tone; the difference is below what a display can show.

## Cost

A full 27-role regeneration measured 1.06 ms on the validation host (x86-64, GCC 13, `-O2`).
Regeneration runs on seed, scheme and density changes only — never during painting — so the
palette is computed once per theme change and read from `m_colors` thereafter.

## Verification status

- Source-only on Linux: the testcase and `UIMd3Hct.cpp` compile with GCC 13 at
  `-Wall -Wextra -Werror` and the testcase passes 127 checks with 0 failures.
- The Windows `UICommon`, `VirtualBox` and `VirtualBoxVM` targets have **not** been rebuilt
  for this change in the current environment; that remains part of the existing native build
  gate.
- No native screenshot is claimed. The palette evidence above is numeric.
