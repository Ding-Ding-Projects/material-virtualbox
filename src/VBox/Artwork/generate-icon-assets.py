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

It deliberately does NOT touch the OSE/about*.png splash graphics (640x480
and up) -- those are a separate wordmark/background composition, not the
application icon, and are out of scope for this pass.
"""
from __future__ import annotations

import io
import struct
from pathlib import Path

from PIL import Image, ImageDraw

REPO_ROOT = Path(__file__).resolve().parents[3]

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


if __name__ == "__main__":
    main()
