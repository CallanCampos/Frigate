<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="logo-dark.svg">
    <img alt="Frigate" src="logo.svg" width="400">
  </picture>
</p>

<p align="center">
  <img alt="Frigate Black 2's icon" src="icon-black2.svg" width="128">
  <img alt="Frigate White 2's icon" src="icon-white2.svg" width="128">
</p>

# Frigate's artwork

Frigate's mark, logo and app icons: the logo at the top of the landing page and the READMEs, and the icons of
`Frigate Black 2.app` and `Frigate White 2.app` and of the program's window on Linux and Windows. Every file is drawn
as code by [tools/frigate_logo.py](tools/frigate_logo.py), so a change is a change to that script.

The ship is a stylized depiction of the Plasma Frigate, Team Plasma's flying ship in Pokémon Black 2 and White 2, which
the port is named after. It is drawn as original flat shapes in this tree, from the ship's parts and proportions as
the games show them; it is not traced or copied from any image, and it carries no emblem, logo, lettering, character
or Pokémon, and nothing from the ROM. Pokémon, and the designs of its games, including this ship's, belong to
Nintendo, Game Freak, Creatures and The Pokémon Company. Frigate is a fan project, not affiliated with or endorsed by
them.

## The design

Frigate's mark is the Plasma Frigate drawn flat, in profile, with the parts that tell it from any other tall ship:

- one main mast with its square sails set as a single grid of panels, three tiers of four, the feet of each tier
  bellied by the wind, and a pennant at the truck;
- two swept jibs, nested like a pair of blades, over a long ram that runs on from the deck, the bow sweeping up into
  it in one curve like a beak;
- a tall, narrow sail on a short mast at the very stern, and forward of it, on the raised stern deck, a round tower
  with a dome, the deck stepping down from it to the main deck in a curve;
- a swept fin under the bow, and the outrigger pontoon slung below the keel on raked struts, with sky between, its
  thruster cone parted from its tail: the ship flies.

The rig and the fittings are steel blue, standing for the games' white canvas and cyan fittings, over a hull with its
strake of five ports cut out, so the strake shows whatever the ship sits on. On White 2's icon that gives a black hull
whose six light runs between the ports make a white chequered stripe; on Black 2's, the reverse, a white hull with five
black ports: the plate carries the version, and the ship is its negative. The cold steel blue nods to the games' icy
theme, and the FRIGATE wordmark is drawn from geometric shapes, the ends of its bars filled forward like a sail. The
ship sails to the right, the games' ship seen from its other side, so that in the logo its ram points at the
wordmark.

## Files

| File | What it is | Where it is used |
| --- | --- | --- |
| `logo.svg` | The mark and the FRIGATE wordmark, for light backgrounds | The top of [.github/README.md](../../../.github/README.md) (the landing page), [port/README.md](../../README.md) and this file, chosen by colour scheme with `logo-dark.svg` |
| `logo-dark.svg` | The same, for dark backgrounds | As `logo.svg` |
| `logo.png`, `logo-dark.png` | The logos rendered 1200 px wide (1200 x 333) | Wherever an SVG can't go: a forum post, a slide, a social card |
| `mark.svg`, `mark-dark.svg` | The ship alone, for light and for dark backgrounds | Square places: an avatar, a favicon |
| `icon-black2.svg`, `icon-white2.svg` | The app icons: the ship on Black 2's dark plate or White 2's light one, on Apple's 1024 px macOS icon grid | The sources of the two masters below |
| `icon-black2-1024.png`, `icon-white2-1024.png` | The icons' 1024 x 1024 masters, transparent outside the plate | The build: `PORT_APP_ICON` in [CMakeLists.txt](../../CMakeLists.txt) picks the version's, and [How the build uses the icons](#how-the-build-uses-the-icons) says what is made from it |
| `icon-black2-16.svg`, `icon-black2-32.svg`, `icon-white2-16.svg`, `icon-white2-32.svg` | The icons' 16 and 32 px images, drawn pixel by pixel for their size on Apple's grid: the ship with its sails as whole-pixel panels (three tiers of four at 32 px, of two at 16 px), the stern sail, the jibs over the ram, the hull (its ports at 32 px) and the pontoon on its struts, and a stronger edge on White 2's plate | The sources of the PNGs below |
| `icon-black2-16.png`, `icon-black2-32.png`, `icon-white2-16.png`, `icon-white2-32.png` | Those images rendered at their size | The `.icns`'s 16 and 32 px images (`PORT_APP_ICON_16` and `_32`) |
| `window-black2.svg`, `window-white2.svg` | The 32 px image again with its plate filling the square, as icons on Linux and Windows do | The source of the PNGs below |
| `window-black2-32.png`, `window-white2-32.png` | Those rendered at 32 px | The window's icon on Linux and Windows (`PORT_WINDOW_ICON`) |
| [tools/frigate_logo.py](tools/frigate_logo.py) | Writes the twelve SVGs: every shape, colour and spacing is set there, and the small images' pixels (`SHIP_16`, `SHIP_32`) | Run by `render.py`, and by the test `assets_branding` |
| [tools/render.py](tools/render.py) | Writes the SVGs, renders the ten PNGs, and with `--sheet` a contact sheet (macOS) | [Changing the artwork](#changing-the-artwork) |
| [tools/sheet.swift](tools/sheet.swift) | The renderer: SVGs to PNGs with AppKit, and contact sheets | Built by `render.py` in a temporary directory |
| [tools/contact.py](tools/contact.py) | Lays out the contact sheet: the logos on GitHub's backgrounds, every icon and mark from 128 down to 16 px, and the small images at their size and enlarged | `render.py --sheet` |

Each SVG has a `<title>` ("Frigate", or "Frigate Black 2" and "Frigate White 2" for the icons) and draws with paths
alone, plus one gradient on each icon's plate: no text, fonts or images, so GitHub, browsers and macOS draw them alike.

## Colours

| Name | Hex | Used for | Contrast |
| --- | --- | --- | --- |
| Ink | `#10151C` | The hull and the wordmark on light backgrounds | 18:1 on white |
| Steel | `#4A7FB0` | The rig and fittings (masts, sails, jibs, pennant, the tower's dome, the bow fin, the pontoon and its struts) on light backgrounds | 4.2:1 on white |
| Snow | `#F1F5F9` | The hull and the wordmark on dark backgrounds | 17:1 on `#0D1117` |
| Ice | `#9DC0E0` | The rig and fittings on dark backgrounds | 10:1 on `#0D1117` |

Black 2's plate runs from `#1C2431` at the top to `#0D1117` and carries the dark-background ship; White 2's runs from
`#FFFFFF` to `#E3E9F0`, with a faint 2 px edge of ink at 12% just inside it so that it keeps its shape on white, and
carries the light-background ship. At 16 and 32 px that edge is a pixel of ink at 35%, so that the plate still shows on
white.

## Using the logo

Neither logo file switches colours by itself: they are a pair. On a page that follows the reader's colour scheme, as
GitHub does, use both:

```html
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="logo-dark.svg">
  <img alt="Frigate" src="logo.svg" width="400">
</picture>
```

`mark.svg` is for light backgrounds only, and `mark-dark.svg` for dark ones. The logo reads down to about 160 px wide.
The mark is plainly the Plasma Frigate from 64 px up, where the icons are the master scaled. At 32 and 16 px they are
drawn for their size, with the sail block wider than the master's so that its panels part on whole pixels: at 32 px
the gridded sails on their mast with the pennant, the stern sail and the tower, the jibs over the ram, the hull with
its ports and the pontoon on its struts; at 16 px the sails in two columns of panels, the stern sail, the jibs, the ram,
the hull and the pontoon. Use the files as they are: a new colour or shape belongs in `frigate_logo.py`, so that every
file keeps the same drawing.

## How the build uses the icons

Nothing made from the masters is committed: the build makes it again whenever a master changes.

- **The macOS app's icon:** [app/macos/make_icns.sh](../../app/macos/make_icns.sh) makes the ten images of a macOS
  icon set (16 to 512 points at 1x and 2x): the 16 and 32 px ones are the small images drawn for their size (the 32 px
  one also serves the 16 point image at 2x), and the others the version's master scaled with `sips`. `iconutil` packs
  them into an `.icns` in the build directory ([cmake/macos_app.cmake](../../cmake/macos_app.cmake)).
  [make_app.sh](../../app/macos/make_app.sh) puts it in the app as `Contents/Resources/AppIcon.icns`, which the
  Info.plist's `CFBundleIconFile` names; Finder, the Dock and the About window show it. The program's own window sets
  no icon on macOS, so the Dock keeps the app's.
- **The window's icon on Linux and Windows:** [tools/gen_window_icon.c](../../tools/gen_window_icon.c), built for the
  machine that builds, writes two bitmaps into a C file ([cmake/app.cmake](../../cmake/app.cmake)): the window's
  32 px image as it is, and the master with its transparent margin trimmed (the macOS grid's, which icons on Linux and
  Windows don't have) and averaged down to 64 px. [app/window_icon.c](../../app/window_icon.c) gives the window the
  32 px one with `SDL_SetWindowIcon`, and the 64 px one as its alternate image, which SDL uses on a display scaled to
  200%; the taskbar and the title bar scale from those.

The tests: `assets_branding` ([tests/assets/test_branding.py](../../tests/assets/test_branding.py)) checks that the
SVGs are what `frigate_logo.py` writes, that they draw with paths alone, the PNGs' sizes, and that each small image's
PNG holds its ship pixel for pixel; `app_make_app` checks the `.icns` (its 16 and 32 px images the drawn ones) and that
the app holds it; `app_window_icon_plain` and `_sanitized` check the window's bitmaps.

## Changing the artwork

1. Change [tools/frigate_logo.py](tools/frigate_logo.py), the only place the shapes and colours are set.
2. On macOS, with Xcode's command line tools (for `swiftc`), write the SVGs and render the PNGs again, with a contact
   sheet to look at, outside the tree or under the ignored `build/`:

   ```sh
   python3 port/assets/branding/tools/render.py --sheet build/branding/contact-sheet.png
   ```

   Look at the sheet: the logos on GitHub's light and dark backgrounds, every icon and mark at 128, 64, 32 and 16 px,
   the last two also enlarged to show their pixels, and the small images drawn for 16 and 32 px. A change to the ship
   is drawn twice more, in the pixels of `SHIP_16` and `SHIP_32`.
3. Check and see the result in the product:
   `ctest --test-dir build/port -R 'assets_branding|app_window_icon|app_make_app'`, then
   `cmake --build build/port --target frigate-black2_app` and open the app (the same for White 2 in its own build).
4. Commit the script, the SVGs and the PNGs together. The PNGs are rendered by AppKit, so they come from a Mac; on
   another system `frigate_logo.py` still writes the SVGs, but the masters stay as they were until a Mac renders them.

## License

The drawing is part of this repository and under its license, the [GNU GPL v3.0](../../../LICENSE): it was made for
Frigate, as code in this tree. In the GPL's terms, its source is `tools/frigate_logo.py` with the SVGs it writes; the
PNGs are renders of them. The license covers our drawing only, not what it depicts: the Plasma Frigate is a design
from Pokémon Black 2 and White 2, and it and the games' names belong to Nintendo, Game Freak, Creatures and The Pokémon
Company, with which Frigate is not affiliated. The icons' plate follows Apple's macOS icon grid, the rounded square
that macOS app icons share. Nor does the license cover a name: a build that isn't Frigate's, such as a fork under
another name, should carry its own name and icon, as it takes its own bundle identifier
([port/README.md](../../README.md#the-macos-app)).

## Known limits

- The 64 px images, the `.icns`'s 32 point image at 2x and the window's for 200%, are still the master scaled; they
  read as the Plasma Frigate, but its panels' seams, struts and ports blur there, and a drawing for 64 px would be
  crisper.
- macOS 26 shows icons from a plain `.icns` inside a dark plate of its own at 16 and 32 px, as it does other apps'
  `.icns` icons; from 64 px up it shows Frigate's plate as drawn. An Icon Composer icon (macOS 26's layered format,
  with the plate, the rig and the hull as layers) would avoid that.
- The mark depicts a ship designed for the games, not a generic tall ship, so it is not free of the games'
  intellectual property: it is a fan project's depiction of it, as the name Frigate is a reference to it. No
  trademark search was made.
- The pontoon is our simplification of the games' outriggers, which are wide plates with tubes close under the hull;
  in profile it can read as a seaplane's float.
