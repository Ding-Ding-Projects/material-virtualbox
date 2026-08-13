#!/usr/bin/env python3
"""Regenerate the Material Virtual Machine application-icon raster assets.

This script is the reproducible source of truth for every derived raster
file that is committed alongside the vector master
``src/VBox/Artwork/OSE/virtualbox.svg``. It draws the *exact same* geometry
as that SVG (see the coordinates below, which are kept in lockstep with the
SVG by hand) with Pillow instead of an SVG rasterizer, so it has no
dependency beyond Pillow. Nothing here reaches the network; every output is
generated locally and committed, never fetched or generated at build time
from a remote or mutable source.

Run from the repository root:

    py -3 src/VBox/Artwork/generate-icon-assets.py

Regenerates:
  - src/VBox/Frontends/VirtualBox/images/OSE/VirtualBox_{16,20,32,40,48,64}px.png
    (the window/taskbar icon set consumed by VirtualBoxBrand.qrc)
  - src/VBox/Frontends/VirtualBox/images/OSE/about_16px{,_x2,_x3,_x4}.png
    (the small About-dialog mark at 16/32/48/64 px)
  - src/VBox/Artwork/win/OSE/VirtualBox_win.ico
    (the real multi-resolution Windows icon embedded into VirtualBox.exe,
    VirtualBoxVM.exe, VBoxSVC, VBoxSDS and VBoxStub via Config.kmk's
    VBOX_WINDOWS_ICON_FILE)
  - src/VBox/Frontends/VirtualBox/images/OSE/about{,_x2,_x3,_x4}.png
    (the full About-dialog splash background at 640x480 .. 2560x1920 --
    see ``render_about_master()``)
  - src/VBox/Artwork/win/OSE/virtualbox-{vbox,vbox-extpack,ova,ovf}.ico and
    src/VBox/Artwork/win/virtualbox-{vdi,vmdk,vhd,hdd}.ico
    (the Explorer file-type-association icons wired in through
    src/VBox/Installer/win/Resources/Makefile.kmk -- see
    ``FILE_TYPE_ICONS`` below)

Every raster in this file, including the About splash and the file-type
icons, is built only from tones already documented in this fork's own
Material 3 palette (``doc/md3/TonalPalette.md`` and ``doc/md3/AppIcon.md``)
-- nothing here invents a new arbitrary colour, and nothing reaches for
Oracle's wordmark, typography, or trademarked artwork.
"""
from __future__ import annotations

import io
import struct
from pathlib import Path

from PIL import Image, ImageColor, ImageDraw, ImageFilter

REPO_ROOT = Path(__file__).resolve().parents[3]

# ---------------------------------------------------------------------------
# Shared palette -- every tone used anywhere in this file is one already
# documented in doc/md3/TonalPalette.md (the generated HCT scheme for this
# fork's seed colour #6750A4) or doc/md3/AppIcon.md (the app mark's own
# Primary-80 tone). No colour below was invented for this pass.
# ---------------------------------------------------------------------------
TONE = {
    "seed": "#6750A4",                  # Primary P40 -- also the app mark's container colour
    "primary80": "#D0BCFF",             # Primary P80 -- the app mark's "back pane"
    "primary_container": "#4F378A",     # Primary P30
    "on_primary_container": "#E9DDFF",  # Primary P90
    "secondary_container": "#4A4458",   # Secondary S30
    "on_secondary_container": "#E8DEF8",  # Secondary S90
    "surface6": "#141218",              # Neutral N6 (dark surface)
    "on_surface90": "#E6E0E9",          # Neutral N90
    "surface_container_high17": "#2B292F",  # Neutral N17
    "outline60": "#948F99",             # Neutral variant NV60
    "white": "#FFFFFF",
}

# Design grid: 256 units, matching src/VBox/Artwork/OSE/virtualbox.svg exactly.
# (x0, y0, x1, y1, radius, fill)
SHAPES = [
    (0, 0, 256, 256, 56, "#6750A4"),   # Material seed-coloured rounded container
    (54, 46, 166, 158, 24, "#D0BCFF"), # back window (host surface)
    (88, 80, 208, 200, 26, "#6750A4"), # gap ring (== bg colour, separates the panes)
    (94, 86, 202, 194, 22, "#FFFFFF"), # front window (guest surface)
]

SUPERSAMPLE_CANVAS = 2048  # 8x the 256-unit design grid


def render_master() -> Image.Image:
    """Render the mark once at high resolution for downstream LANCZOS resampling."""
    scale = SUPERSAMPLE_CANVAS / 256
    img = Image.new("RGBA", (SUPERSAMPLE_CANVAS, SUPERSAMPLE_CANVAS), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for x0, y0, x1, y1, radius, fill in SHAPES:
        box = (x0 * scale, y0 * scale, x1 * scale, y1 * scale)
        draw.rounded_rectangle(box, radius=radius * scale, fill=fill)
    return img


def render_at(master: Image.Image, size: int) -> Image.Image:
    return master.resize((size, size), Image.LANCZOS)


def write_png(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="PNG", optimize=True)
    print(f"wrote {path.relative_to(REPO_ROOT)} ({img.width}x{img.height})")


def build_ico(master: Image.Image, sizes: list[int], path: Path) -> None:
    """Hand-roll a real ICO: ICONDIR + ICONDIRENTRY[] + PNG-compressed frames.

    Every frame (including sizes below 256) is stored PNG-compressed, which
    is valid for ICO files targeting Windows Vista and later -- this is not
    a renamed PNG, it is a correctly structured icon *directory* whose
    payloads happen to be PNG-encoded, exactly like every icon produced by
    modern tooling (e.g. Visual Studio's own icon editor does the same for
    32/48/256 frames).
    """
    entries = []
    payloads = []
    for size in sizes:
        frame = render_at(master, size)
        buf = io.BytesIO()
        frame.save(buf, format="PNG")
        data = buf.getvalue()
        entries.append((size, data))
        payloads.append(data)

    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        count = len(entries)
        # ICONDIR: reserved(2)=0, type(2)=1 (icon), count(2)
        f.write(struct.pack("<HHH", 0, 1, count))
        header_size = 6
        entry_size = 16
        offset = header_size + entry_size * count
        for size, data in entries:
            wb = size if size < 256 else 0
            hb = size if size < 256 else 0
            # ICONDIRENTRY: width(1) height(1) colorcount(1) reserved(1)
            #               planes(2) bitcount(2) bytesinres(4) imageoffset(4)
            f.write(struct.pack("<BBBBHHII", wb, hb, 0, 0, 1, 32, len(data), offset))
            offset += len(data)
        for _, data in entries:
            f.write(data)
    print(f"wrote {path.relative_to(REPO_ROOT)} ({count} entries: {sizes})")


def verify_ico(path: Path) -> None:
    data = path.read_bytes()
    assert data[:4] == b"\x00\x00\x01\x00", f"bad ICO signature: {data[:4].hex()}"
    count = struct.unpack_from("<H", data, 4)[0]
    print(f"verify: {path.name} signature OK, {count} directory entries:")
    off = 6
    for i in range(count):
        w, h, colors, res, planes, bpp, size, offset = struct.unpack_from("<BBBBHHII", data, off)
        w = w or 256
        h = h or 256
        payload = data[offset:offset + size]
        is_png = payload[:8] == b"\x89PNG\r\n\x1a\n"
        print(f"  entry {i}: {w}x{h} bpp={bpp} bytes={size} png={is_png}")
        off += 16


# ---------------------------------------------------------------------------
# About-dialog splash background (about.png / about_x2.png / about_x3.png /
# about_x4.png -- NOT the small about_16px* mark, which the previous lane
# already replaced). VBoxAboutDlg::prepare() loads this at a fixed logical
# size of 640x480 (the _x2/_x3/_x4 files are plain @2x/@3x/@4x device-pixel
# variants of that same logical canvas, used for HiDPI), and
# VBoxAboutDlg::paintEvent() draws it as the whole dialog background with no
# further compositing. VBoxAboutDlg::prepareLabel() then overlays a
# bottom-right-aligned, Qt::white-by-default QLabel with the product name,
# version and copyright text on top -- so this background must keep that
# quadrant legible for white text, exactly like the Oracle original it
# replaces did with its plain teal field.
# ---------------------------------------------------------------------------
ABOUT_BASE = (640, 480)
ABOUT_SUPERSAMPLE = 4  # matches the largest shipped @4x variant exactly


def draw_mark(draw: ImageDraw.ImageDraw, center: tuple[float, float], size: float) -> None:
    """Draw the app mark (the same SHAPES list used for the app icon itself,
    which live on a 0..256 design grid) at an arbitrary position/scale onto
    an existing ImageDraw target. Used to echo the app icon inside the
    About splash.
    """
    scale = size / 256
    cx, cy = center
    ox, oy = cx - size / 2, cy - size / 2
    for x0, y0, x1, y1, radius, fill in SHAPES:
        box = (ox + x0 * scale, oy + y0 * scale, ox + x1 * scale, oy + y1 * scale)
        draw.rounded_rectangle(box, radius=radius * scale, fill=fill)


def diagonal_gradient(size: tuple[int, int], color_a: str, color_b: str) -> Image.Image:
    """A cheap top-left -> bottom-right gradient: a plain Python loop only
    over a small 64x64 mask (4096 iterations), then Pillow's C-level
    resize/composite do the expensive per-pixel work at full resolution.
    """
    small = 64
    mask = Image.new("L", (small, small))
    px = mask.load()
    for y in range(small):
        for x in range(small):
            px[x, y] = int(255 * (x + y) / (2 * (small - 1)))
    mask = mask.resize(size, Image.BILINEAR)
    layer_a = Image.new("RGB", size, color_a)
    layer_b = Image.new("RGB", size, color_b)
    return Image.composite(layer_b, layer_a, mask)


def render_about_master() -> Image.Image:
    """Render the About-dialog splash background once at the largest shipped
    resolution (2560x1920, i.e. @4x of the 640x480 logical size). Downsampled
    copies are produced for @1x/@2x/@3x by ``write_about_variants``.

    Layout mirrors the composition it replaces -- a dark diagonal field, a
    soft abstract accent shape, and the app mark placed left-of-centre --
    with the bottom-right quadrant kept plain for the overlaid white text
    (see the module comment above).

    No wordmark or product name is baked into these pixels. This fork's
    display name and typography are a separate, not-yet-settled decision
    (see doc/md3/AppIcon.md's "what this lane deliberately did not touch"),
    so the splash stays text-free and lets VBoxAboutDlg's own label carry
    whatever name and version the build is configured with, rather than
    guessing at branding here.
    """
    w = ABOUT_BASE[0] * ABOUT_SUPERSAMPLE
    h = ABOUT_BASE[1] * ABOUT_SUPERSAMPLE

    base = diagonal_gradient((w, h), TONE["surface6"], TONE["surface_container_high17"]).convert("RGBA")

    # Soft abstract accent blob (Primary-container, low opacity, heavily
    # blurred), biased to the upper-right so it never competes with the mark
    # on the left or the text zone at bottom-right.
    blob = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(blob).polygon(
        [(w * 0.35, -h * 0.10), (w * 1.05, h * 0.05), (w * 1.05, h * 0.75), (w * 0.60, h * 0.55)],
        fill=ImageColor.getrgb(TONE["primary_container"]) + (90,),
    )
    blob = blob.filter(ImageFilter.GaussianBlur(radius=w * 0.05))
    base = Image.alpha_composite(base, blob)

    # A second, smaller Primary-80 highlight near the mark for tonal depth.
    # This is background wash, not a silhouette that needs to survive being
    # scaled down -- the "flat shapes, no gradients" rule applies to the
    # mark itself, drawn crisply on top of this a few lines down.
    highlight = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(highlight).ellipse(
        (w * 0.02, h * 0.30, w * 0.55, h * 1.05),
        fill=ImageColor.getrgb(TONE["primary80"]) + (40,),
    )
    highlight = highlight.filter(ImageFilter.GaussianBlur(radius=w * 0.04))
    base = Image.alpha_composite(base, highlight)

    # The app mark, left-of-centre, large and crisp, clear of the
    # bottom-right text zone.
    mark_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw_mark(ImageDraw.Draw(mark_layer), center=(w * 0.26, h * 0.50), size=h * 0.62)
    base = Image.alpha_composite(base, mark_layer)

    return base.convert("RGB")


def write_about_variants(master: Image.Image) -> None:
    frontend_images = REPO_ROOT / "src/VBox/Frontends/VirtualBox/images/OSE"
    variants = {
        "about.png": ABOUT_BASE,
        "about_x2.png": (ABOUT_BASE[0] * 2, ABOUT_BASE[1] * 2),
        "about_x3.png": (ABOUT_BASE[0] * 3, ABOUT_BASE[1] * 3),
        "about_x4.png": (ABOUT_BASE[0] * 4, ABOUT_BASE[1] * 4),
    }
    for name, size in variants.items():
        img = master if size == master.size else master.resize(size, Image.LANCZOS)
        write_png(img, frontend_images / name)


# ---------------------------------------------------------------------------
# File-type-association icons (Explorer icons for .vbox/.vbox-extpack/.ova/
# .ovf/.vdi/.vmdk/.vhd/.hdd). Wired in by
# src/VBox/Installer/win/Resources/Makefile.kmk's VBOX_WINDOWS_ICON_EXT_*
# variables (out of this lane's editable scope; that Makefile already points
# at these exact paths, so replacing file *content* in place is enough).
#
# Two families, both built on the same 256-unit rounded-square container
# (radius 56, matching the app mark) so they read as one family, but with
# distinct glyphs so a disk image never looks like a machine definition:
#
#   - "package" family (vbox, vbox-extpack, ova, ovf): the app mark's own
#     window-pane motif or a document/archive glyph, plus a small top-right
#     "dog-ear" -- reads as *a file*, not the application itself.
#   - "disk" family (vdi, vmdk, vhd, hdd): a stacked-cylinder "disk platter"
#     glyph, no dog-ear -- reads as *data*, not a file.
#
# Every colour is one of the documented TONE entries at the top of this
# file; container colours are deliberately paired across the two families
# (e.g. vbox and vdi both use the seed colour, being this fork's two
# "native, first-party" formats) and disambiguated by glyph, not by
# inventing extra hues.
# ---------------------------------------------------------------------------
OSE_ICON_SIZES = [16, 24, 32, 48, 64, 128]      # matches the existing OSE rc.exe Vista-icon workaround (no 256)
SHARED_ICON_SIZES = [16, 24, 32, 48, 64, 256]   # matches the existing shared (non-OSE-suffixed) icon size set


def s_rrect(draw: ImageDraw.ImageDraw, box, radius: float, fill: str, scale: float) -> None:
    x0, y0, x1, y1 = box
    draw.rounded_rectangle((x0 * scale, y0 * scale, x1 * scale, y1 * scale), radius=radius * scale, fill=fill)


def s_ellipse(draw: ImageDraw.ImageDraw, box, fill: str, scale: float) -> None:
    x0, y0, x1, y1 = box
    draw.ellipse((x0 * scale, y0 * scale, x1 * scale, y1 * scale), fill=fill)


def s_rect(draw: ImageDraw.ImageDraw, box, fill: str, scale: float) -> None:
    x0, y0, x1, y1 = box
    draw.rectangle((x0 * scale, y0 * scale, x1 * scale, y1 * scale), fill=fill)


def s_polygon(draw: ImageDraw.ImageDraw, points, fill: str, scale: float) -> None:
    draw.polygon([(x * scale, y * scale) for x, y in points], fill=fill)


def _vbox_inner(draw, scale):
    """.vbox machine settings XML -- the exact app-mark window-pane motif."""
    s_rrect(draw, (54, 46, 166, 158), 24, TONE["primary80"], scale)
    s_rrect(draw, (88, 80, 208, 200), 26, TONE["seed"], scale)
    s_rrect(draw, (94, 86, 202, 194), 22, TONE["white"], scale)


def _extpack_inner(draw, scale):
    """.vbox-extpack -- the same window-pane motif plus a small "plus" badge
    to read as an add-on rather than the machine definition itself."""
    s_rrect(draw, (54, 46, 166, 158), 24, TONE["primary80"], scale)
    s_rrect(draw, (88, 80, 208, 200), 26, TONE["primary_container"], scale)
    s_rrect(draw, (94, 86, 202, 194), 22, TONE["white"], scale)
    s_ellipse(draw, (160, 160, 240, 240), TONE["secondary_container"], scale)
    s_rect(draw, (193, 178, 207, 222), TONE["on_secondary_container"], scale)
    s_rect(draw, (178, 193, 222, 207), TONE["on_secondary_container"], scale)


def _ova_inner(draw, scale):
    """.ova appliance archive -- a crate tied with a cross-strap."""
    s_rrect(draw, (40, 104, 216, 152), 16, TONE["on_secondary_container"], scale)
    s_rrect(draw, (104, 40, 152, 216), 16, TONE["on_secondary_container"], scale)


def _ovf_inner(draw, scale):
    """.ovf descriptor (XML only, no packaged data) -- a plain text page."""
    s_rrect(draw, (56, 40, 200, 216), 16, TONE["on_surface90"], scale)
    s_rrect(draw, (76, 84, 180, 96), 6, TONE["surface6"], scale)
    s_rrect(draw, (76, 116, 180, 128), 6, TONE["surface6"], scale)
    s_rrect(draw, (76, 148, 150, 160), 6, TONE["surface6"], scale)


def _disk_inner(draw, scale, body: str, cap: str):
    """Shared stacked-cylinder ("disk platter") glyph for every disk-image format."""
    s_rect(draw, (68, 70, 188, 186), body, scale)
    s_ellipse(draw, (68, 50, 188, 90), body, scale)
    s_ellipse(draw, (68, 166, 188, 206), body, scale)
    s_ellipse(draw, (84, 58, 172, 82), cap, scale)


FILE_TYPE_ICONS = [
    # (path relative to src/VBox/Artwork/, icon sizes, container tone, dog-ear tone or None, inner-glyph fn)
    ("win/OSE/virtualbox-vbox.ico", OSE_ICON_SIZES,
     TONE["seed"], TONE["on_primary_container"], _vbox_inner),
    ("win/OSE/virtualbox-vbox-extpack.ico", OSE_ICON_SIZES,
     TONE["primary_container"], TONE["on_primary_container"], _extpack_inner),
    ("win/OSE/virtualbox-ova.ico", OSE_ICON_SIZES,
     TONE["secondary_container"], TONE["white"], _ova_inner),
    ("win/OSE/virtualbox-ovf.ico", OSE_ICON_SIZES,
     TONE["outline60"], TONE["white"], _ovf_inner),
    ("win/virtualbox-vdi.ico", SHARED_ICON_SIZES,
     TONE["seed"], None, lambda draw, scale: _disk_inner(draw, scale, TONE["primary80"], TONE["white"])),
    ("win/virtualbox-vmdk.ico", SHARED_ICON_SIZES,
     TONE["primary_container"], None, lambda draw, scale: _disk_inner(draw, scale, TONE["on_primary_container"], TONE["primary80"])),
    ("win/virtualbox-vhd.ico", SHARED_ICON_SIZES,
     TONE["secondary_container"], None, lambda draw, scale: _disk_inner(draw, scale, TONE["on_secondary_container"], TONE["white"])),
    ("win/virtualbox-hdd.ico", SHARED_ICON_SIZES,
     TONE["surface6"], None, lambda draw, scale: _disk_inner(draw, scale, TONE["outline60"], TONE["on_surface90"])),
]


def render_file_type_icon(container: str, ear: str | None, inner) -> Image.Image:
    scale = SUPERSAMPLE_CANVAS / 256
    img = Image.new("RGBA", (SUPERSAMPLE_CANVAS, SUPERSAMPLE_CANVAS), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle((0, 0, SUPERSAMPLE_CANVAS, SUPERSAMPLE_CANVAS), radius=56 * scale, fill=container)
    inner(draw, scale)
    if ear is not None:
        s_polygon(draw, [(196, 0), (256, 0), (256, 60)], ear, scale)
    return img


def main() -> None:
    master = render_master()

    frontend_images = REPO_ROOT / "src/VBox/Frontends/VirtualBox/images/OSE"
    for size in (16, 20, 32, 40, 48, 64):
        write_png(render_at(master, size), frontend_images / f"VirtualBox_{size}px.png")

    about_sizes = {"about_16px.png": 16, "about_16px_x2.png": 32,
                   "about_16px_x3.png": 48, "about_16px_x4.png": 64}
    for name, size in about_sizes.items():
        write_png(render_at(master, size), frontend_images / name)

    ico_path = REPO_ROOT / "src/VBox/Artwork/win/OSE/VirtualBox_win.ico"
    build_ico(master, [16, 24, 32, 48, 64, 128, 256], ico_path)
    verify_ico(ico_path)

    # About-dialog splash background (about{,_x2,_x3,_x4}.png).
    about_master = render_about_master()
    write_about_variants(about_master)

    # File-type-association icons.
    artwork_root = REPO_ROOT / "src/VBox/Artwork"
    for rel_path, sizes, container, ear, inner in FILE_TYPE_ICONS:
        icon_master = render_file_type_icon(container, ear, inner)
        icon_path = artwork_root / rel_path
        build_ico(icon_master, sizes, icon_path)
        verify_ico(icon_path)


if __name__ == "__main__":
    main()
