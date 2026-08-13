# Application logo and packaged icon

This article documents the fork's own application mark, its committed vector
master, the generated raster derivatives, where each is wired into the built
application, and what still needs a change outside this lane's allowed files
to finish wiring it into the Windows installer.

## Why this exists

The release checklist for this fork requires an original, project-appropriate
logo and a real packaged application icon: a committed master source, valid
generated platform formats (including a multi-resolution `.ico` for Windows),
the mark wired into application chrome/executable/installer/update metadata,
and verification of the built artifact at both small and standard icon sizes.
A framework default icon, a missing mark, a mutable/unreachable icon URL, or a
raster file renamed to an icon extension all block that gate.

The icon this fork shipped with until now was Oracle's own trademarked
VirtualBox mark (`src/VBox/Artwork/OSE/virtualbox.svg`, a teal/orange
checkmark authored 2024-08-22, upstream commit `d5ed5ae5694`). That is not
this project's mark to ship, so it has been replaced with an original design.

## The mark

A rounded Material 3 "container" square, filled with this fork's own
generated seed colour (`#6750A4`, see [`TonalPalette.md`](TonalPalette.md)),
holding two overlapping rounded "window" panes -- a light lavender pane
(`#D0BCFF`, the fork's own documented Primary-80 tone) behind a white pane,
separated by a thin gap in the background colour. It reads as one machine's
window running another's: a host surface with a guest surface layered on top,
which is what the application actually does. The whole mark is three flat
shapes with no gradients or fine detail, so the silhouette still reads at
16x16.

| Colour | Hex | Role |
| --- | --- | --- |
| Container / gap | `#6750A4` | Fork's generated Material seed (documented in `TonalPalette.md`) |
| Back pane | `#D0BCFF` | Fork's own documented Primary-80 tone |
| Front pane | `#FFFFFF` | Maximum-contrast foreground |

## Committed master and generator

- **Vector master:** [`src/VBox/Artwork/OSE/virtualbox.svg`](../../src/VBox/Artwork/OSE/virtualbox.svg).
  This is not a new file living alongside the old one -- it *is* the file the
  build already resolves through `Config.kmk`'s `VBOX_BRAND_GUI_VBOX_SVG` and
  loads at runtime via `QIcon(":/VirtualBox.svg")` in
  `UIVirtualBoxManager.cpp` (the VirtualBox Manager window icon), so replacing
  its content in place rewires that chrome with no build-script edits needed.
  It is committed, static, and requires no network access or external tool to
  resolve -- opening it is the whole story.
- **Reproducible raster generator:**
  [`src/VBox/Artwork/generate-icon-assets.py`](../../src/VBox/Artwork/generate-icon-assets.py).
  A small, dependency-light (Pillow only) script that draws the *exact same*
  coordinates as the SVG master and renders every derived PNG and the Windows
  `.ico` from one supersampled render pass. Run it from the repository root
  with `py -3 src/VBox/Artwork/generate-icon-assets.py` to regenerate every
  raster derivative deterministically after the master changes. It makes no
  network calls.

## Where the mark is wired in

| File | Consumer | Wiring point |
| --- | --- | --- |
| `src/VBox/Artwork/OSE/virtualbox.svg` | VirtualBox Manager window icon | `QIcon(":/VirtualBox.svg")` in `UIVirtualBoxManager.cpp`, resolved through `VirtualBoxBrand.qrc` -> `Config.kmk`'s `VBOX_BRAND_GUI_VBOX_SVG` |
| `src/VBox/Frontends/VirtualBox/images/OSE/VirtualBox_{16,20,32,40,48,64}px.png` | Window/taskbar icon set on non-Windows chrome | `VirtualBoxBrand.qrc` -> `VBOX_BRAND_GUI_VBOX_*PX_PNG` |
| `src/VBox/Frontends/VirtualBox/images/OSE/about_16px{,_x2,_x3,_x4}.png` | Small About-dialog mark (16/32/48/64 px) | `VirtualBoxBrand.qrc` -> `VBOX_BRAND_GUI_ABOUT_16PX*_PNG` |
| `src/VBox/Artwork/win/OSE/VirtualBox_win.ico` | The real Windows executable icon | `Config.kmk`'s `VBOX_WINDOWS_ICON_FILE` (OSE build), embedded via `VBOX_SET_VER_INFO_EXE`/`VBOX_SET_VER_INFO_DLL` and the generated `*-icon.rc` resource scripts into **every** Windows PE the build produces: `VirtualBox.exe` (Manager), `VirtualBoxVM.exe`, `VirtualBoxHardenedVM`, `VBoxSVC`, `VBoxSDS`, `VBoxStub`, `VBoxHeadless`, `VBoxSDL`, `VBoxService`, `VBoxNetDHCP`, `VBoxNetNAT`, `VBoxIntNetSwitch`, and more -- anything that pulls in `$(VBOX_WINDOWS_ICON_FILE)` |

Because `VBOX_WINDOWS_ICON_FILE` and the `VBOX_BRAND_GUI_*` variables in
`Config.kmk` are fixed, already-existing paths, replacing the *content* at
those paths was enough to rewire the whole chrome and every packaged Windows
executable -- no edit to `Config.kmk` itself was necessary or made.

## Windows `.ico` verification

`src/VBox/Artwork/win/OSE/VirtualBox_win.ico` is a real, structurally valid
Windows icon, verified two independent ways (the generator's own hand-rolled
directory reader, and an unrelated Pillow load):

- **Signature:** first 6 bytes `00 00 01 00 07 00` -- reserved=0, type=1
  (icon), 7 directory entries. This is the mandatory `ICONDIR` header; a PNG
  renamed to `.ico` would not have it.
- **Directory entries** (all 32bpp, PNG-compressed payloads, each payload's
  own PNG signature `89 50 4E 47 0D 0A 1A 0A` verified present):

  | Size | Payload bytes | PNG-compressed |
  | --- | --- | --- |
  | 16x16 | 634 | yes |
  | 24x24 | 918 | yes |
  | 32x32 | 1170 | yes |
  | 48x48 | 1662 | yes |
  | 64x64 | 2240 | yes |
  | 128x128 | 4669 | yes |
  | 256x256 | 9688 | yes |

- **Independent cross-check:** `PIL.Image.open(...).info['sizes']` reports
  `{(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)}` and loads a
  256x256 RGBA default frame -- confirms Pillow's own ICO parser agrees with
  the hand-rolled directory reader used above.
- **Legibility at small size:** at 16x16 the mark is a solid purple rounded
  square with a visible lighter pane peeking from behind a white pane -- the
  two-pane silhouette still reads; there is no fine detail to lose.

## What this lane deliberately did not touch

- **`src/VBox/Frontends/VirtualBox/images/OSE/about.png` (and its `_x2`/`_x3`/`_x4`
  variants, 640x480 up to 2560x1920).** These are Oracle's full "VirtualBox"
  wordmark splash graphic used as the About-dialog background -- a distinct
  art asset (gradient background, wordmark typography), not the application
  icon. They still carry Oracle's trademark and should be replaced in a
  follow-up pass once this fork's product name and typography are settled
  elsewhere, rather than guessed at here.
- **File-type icons** (`virtualbox-vbox.ico`, `virtualbox-vbox-extpack.ico`,
  `virtualbox-ova.ico`, `virtualbox-ovf.ico`, `virtualbox-vdi.ico`,
  `virtualbox-vmdk.ico`, `virtualbox-vhd.ico`, `virtualbox-hdd.ico` under
  `src/VBox/Artwork/win/`) used for Explorer file-association icons on
  `.vbox`/`.ova`/`.ovf`/disk-image files. Lower priority than the application
  icon itself and out of scope for this pass.
- **macOS `.icns`** (`src/VBox/Artwork/darwin/OSE/VirtualBox.icns`). This
  fork's active delivery scope is Windows-only; the macOS icon was left
  untouched.

## Outstanding change needed in files this lane could not edit

The Squirrel.Windows packaging path
(`.github/workflows/windows-package-release.yml` and
`tools/build-windows.ps1`, both out of bounds for this lane) generates a
NuGet `.nuspec` with an `<id>`, `<version>`, `<authors>`, and `<description>`
but **no icon reference at all** -- not a broken/mutable `<iconUrl>`, simply
none. `<iconUrl>` itself would be the wrong fix even if this lane could edit
those files: it requires a live, reachable HTTPS URL, which fails the "not a
mutable or unreachable icon URL" half of the release gate outright.

The correct fix is to pass Squirrel's own `--icon <path>` flag to
`Squirrel.exe --releasify`, pointing at this repository's
`src/VBox/Artwork/win/OSE/VirtualBox_win.ico` (the same file now embedded in
`VirtualBox.exe` itself, so the installer and the installed app agree). That
embeds the icon directly into the generated `Setup.exe`, the Start Menu
shortcut, and the Programs-and-Features entry -- fully offline, no URL
involved. Concretely, both of the following calls need the added flag:

- `tools/build-windows.ps1`, `New-SquirrelInstaller`, the
  `& $Tools.Squirrel --releasify $package.FullName --releaseDir $release --no-msi`
  line.
- `.github/workflows/windows-package-release.yml`, the "Build unsigned
  Squirrel full installer" step's
  `& $squirrel --releasify $package --releaseDir $releaseDir --no-msi` line.

Both need `--icon "<repo-root>\src\VBox\Artwork\win\OSE\VirtualBox_win.ico"`
appended. Neither file was edited by this lane, per its scope boundary; this
is reported here so the lane that owns those files can make the one-line
change.

## Suggested articles

- [`TonalPalette.md`](TonalPalette.md) -- where the mark's seed colour and its
  Primary-80 tone come from.
- [`CodexHandoff.md`](CodexHandoff.md) -- the standing native-implementation
  contract this mark's wiring follows.
