#!/usr/bin/env python3
"""Convert Tower Bloxx art to the Passport's flash-resident RGB565 images."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT = 240, 320


def convert(name: str) -> None:
    asset_name = name.replace("_", "-")
    source = ROOT / f"assets/images/tower-{asset_name}-source.png"
    preview_path = ROOT / f"assets/images/tower-{asset_name}-240x320.png"
    raw_path = ROOT / f"main/tower_{name}.rgb565"
    image = Image.open(source).convert("RGB")
    image = image.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    packed = bytearray()
    preview = Image.new("RGB", (WIDTH, HEIGHT))
    preview_pixels = preview.load()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            red, green, blue = image.getpixel((x, y))
            color = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)
            packed.extend((color & 0xFF, color >> 8))
            preview_pixels[x, y] = (
                ((color >> 11) & 0x1F) * 255 // 31,
                ((color >> 5) & 0x3F) * 255 // 63,
                (color & 0x1F) * 255 // 31,
            )
    raw_path.write_bytes(packed)
    preview.save(preview_path)
    assert len(packed) == WIDTH * HEIGHT * 2
    print(f"{raw_path}: {len(packed)} bytes; preview {preview_path}")


def main() -> None:
    convert("cover")
    convert("backdrop")
    convert("cover_light")
    convert("backdrop_light")


if __name__ == "__main__":
    main()
