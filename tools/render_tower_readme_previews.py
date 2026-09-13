#!/usr/bin/env python3
"""Render representative README screens from the game's art and UI layout.

These are interface previews, not captures from a physical Passport display.
Run with Pillow: python3 tools/render_tower_readme_previews.py
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent.parent
ART = ROOT / "assets/images"
OUTPUT = ART / "readme"
OUTPUT.mkdir(exist_ok=True)

# Keep these colors aligned with main/tower_theme.c. Coordinates and labels
# follow the build_menu, build_play, build_city, and build_result functions.
PALETTES = {
    "dark": dict(ink="#30262C", paper="#FFF1DB", grass="#D98560",
                 grass_dark="#88434A", yellow="#F0BD5B", orange="#D97D4D",
                 red="#B34448", muted="#D5C2AD", footer="#423039",
                 on_art="#FFF1DB", on_footer="#FFF1DB", secondary="#78484A",
                 title="#F0BD5B", menu_bg="#423039",
                 menu_selected="#F0BD5B", menu_border="#D5C2AD",
                 menu_selected_border="#FFF1DB", menu_text="#FFF1DB",
                 menu_selected_text="#30262C", map_backing="#423039",
                 window_bright="#F9D698", window_dark="#88434A",
                 floors=("#C66B50", "#D8995B", "#A75B60", "#B77A57")),
    "light": dict(ink="#243443", paper="#FFF9EC", grass="#CBE4D6",
                  grass_dark="#397C70", yellow="#F4C66B", orange="#D98252",
                  red="#B74E58", muted="#BED0D4", footer="#E9F3F0",
                  on_art="#243443", on_footer="#243443", secondary="#456574",
                  title="#843D3F", menu_bg="#FFF9EC",
                  menu_selected="#317F9F", menu_border="#6A8790",
                  menu_selected_border="#243443", menu_text="#243443",
                  menu_selected_text="#FFFFFF", map_backing="#EAF4F2",
                  window_bright="#F5FDFF", window_dark="#6F91A2",
                  floors=("#E5A074", "#E9BB90", "#C98E90", "#D4AF82")),
}


def font(size: int, bold: bool = False):
    face = "SFCompact" if not bold else "SFCompact"
    try:
        return ImageFont.truetype(f"/System/Library/Fonts/{face}.ttf", size)
    except OSError:
        return ImageFont.load_default(size=size)


FONT14 = font(14)
FONT20 = font(20, bold=True)


def screen(theme: str, cover: bool):
    suffix = "-light" if theme == "light" else ""
    kind = "cover" if cover else "backdrop"
    image = Image.open(ART / f"tower-{kind}{suffix}-240x320.png").convert("RGB")
    return image, ImageDraw.Draw(image), PALETTES[theme]


def block(draw, x, y, w, h, fill, border=None, width=0):
    draw.rectangle((x, y, x + w - 1, y + h - 1), fill=fill,
                   outline=border if width else None, width=width)


def label(draw, x, y, value, color, large=False):
    draw.text((x, y), value, fill=color, font=FONT20 if large else FONT14)


def menu(theme: str):
    image, draw, p = screen(theme, cover=True)
    label(draw, 16, 13, "TOWER BLOXX", p["title"], large=True)
    label(draw, 17, 40, "STACK THE CITY", p["on_art"])
    labels = ("01   CITY MODE", "02   QUICK GAME",
              f"03   THEME: {theme.upper()}")
    for i, value in enumerate(labels):
        selected = i == 2
        y = 205 + i * 37
        block(draw, 17, y, 206, 31,
              p["menu_selected"] if selected else p["menu_bg"],
              p["menu_selected_border"] if selected else p["menu_border"], 2)
        label(draw, 29, y + 7, value,
              p["menu_selected_text"] if selected else p["menu_text"])
    return image


def floor(draw, p, cx, y, style):
    x = cx - 24
    block(draw, x, y, 48, 48, p["floors"][style % 4], p["ink"], 2)
    for row in range(4):
        for col in range(4):
            color = p["window_dark"] if (row + col) % 5 == 0 else p["window_bright"]
            block(draw, x + 5 + col * 10, y + 5 + row * 10, 7, 7, color)


def play(theme: str):
    image, draw, p = screen(theme, cover=False)
    label(draw, 15, 11, "TOWER", p["title"], large=True)
    # Transparent field with the same geometry as build_play().
    block(draw, 7, 52, 226, 233, None, p["ink"], 3)
    block(draw, 10, 54, 220, 6, p["orange"], p["ink"], 1)
    count = 3 if theme == "dark" else 2
    for i in range(count):
        y = 52 + 128 - (i + 1) * 48 + count * 48
        floor(draw, p, 7 + 113 + (1 if i % 2 else 0), y, i)
    active_x = 7 + (142 if theme == "dark" else 112)
    block(draw, active_x - 1, 63, 2, 11, p["ink"])
    block(draw, active_x - 6, 57, 13, 9, p["yellow"], p["ink"], 2)
    floor(draw, p, active_x, 66, count)
    if theme == "light":
        label(draw, 72, 138, "PERFECT!", p["ink"], large=True)
    block(draw, 0, 286, 240, 34, p["footer"], p["orange"], 2)
    label(draw, 11, 289, f"F{count:02d}/99  POP00248  X0", p["on_footer"])
    label(draw, 11, 304, "OK DROP   HOLD OK EXIT", p["on_footer"])
    return image


def city(theme: str):
    image, draw, p = screen(theme, cover=False)
    label(draw, 15, 11, "CITY MAP", p["title"], large=True)
    overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
    odraw = ImageDraw.Draw(overlay)
    block(odraw, 10, 58, 220, 211, p["map_backing"] + "80", p["ink"], 2)
    image = Image.alpha_composite(image.convert("RGBA"), overlay).convert("RGB")
    draw = ImageDraw.Draw(image)
    placing = theme == "light"
    for i in range(16):
        x, y = 23 + (i % 4) * 49, 69 + (i // 4) * 48
        value = "10F" if i == 0 and not placing else "+"
        color = p["floors"][0] if value == "10F" else (
            p["yellow"] if placing else p["grass"])
        border = p["red"] if placing and i == 0 else p["ink"]
        block(draw, x, y, 43, 42, color, border, 4 if placing and i == 0 else 2)
        label(draw, x + 5, y + 11, value, p["ink"])
    label(draw, 16, 266, "POP 0  10F  OK PLACE" if placing else
          "CITY POP 248  NEXT:", p["on_art"])
    if not placing:
        for i, tier in enumerate((10, 20, 30, 40)):
            x = 18 + i * 53
            block(draw, x, 288, 47, 24,
                  p["yellow"] if i == 0 else p["muted"], p["ink"], 2)
            label(draw, x + 8, 291, f"{tier}F", p["ink"])
    return image


def result(theme: str):
    image, draw, p = screen(theme, cover=True)
    complete = theme == "light"
    label(draw, 17, 17, "TOWER BLOXX", p["title"], large=True)
    block(draw, 19, 82, 210, 173, p["muted"])
    block(draw, 15, 77, 210, 173, p["paper"], p["ink"], 3)
    label(draw, 30, 93, "TOWER BUILT!" if complete else "GAME OVER",
          p["grass_dark"] if complete else p["red"], large=True)
    label(draw, 31, 138, "FLOORS  10" if complete else "FLOORS  8",
          p["ink"], large=True)
    label(draw, 31, 167, "POP     1200" if complete else "POP     248",
          p["ink"], large=True)
    label(draw, 29, 211, "OK  PLACE IN CITY" if complete else
          "OK  TRY AGAIN", p["secondary"])
    label(draw, 57, 270, "HOLD OK  MENU", p["on_art"])
    return image


def main():
    for theme in ("dark", "light"):
        for name, renderer in (("menu", menu), ("gameplay", play),
                               ("city", city), ("result", result)):
            target = OUTPUT / f"tower-{name}-{theme}.png"
            renderer(theme).save(target, optimize=True)
            print(target.relative_to(ROOT))


if __name__ == "__main__":
    main()
