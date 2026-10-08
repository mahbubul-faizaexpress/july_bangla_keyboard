# Third-party notices

## bijoy2unicode (MPL-2.0)

[`layouts/sutonnymj.classic`](layouts/sutonnymj.classic) (the SutonnyMJ Classic glyph
table) is derived from the Unicode→Bijoy mapping table in `bijoy2unicode/converter.py`
of <https://github.com/Mad-FOX/bijoy2unicode>.

That project is licensed under the **Mozilla Public License 2.0**, so this file is also
covered by MPL-2.0. The license text is at <https://mozilla.org/MPL/2.0/>.

Changes from the original:

- converted to a data file of Unicode code points and Windows-1252 bytes
- ZWNJ replaced by ZWJ in the র‍্য entry
- duplicate keys resolved (last entry wins, as in the original Python dict)

## Bijoy keyboard layout

The key positions in [`layouts/bijoy.layout`](layouts/bijoy.layout) follow the Bijoy
keyboard layout. They were read from Bijoy keyboard charts supplied by the project owner.
The Bijoy layout is a proprietary layout by Mustafa Jabbar, registered under the
copyright law of Bangladesh. Get legal advice before distributing this layout publicly.

## Splash artwork

`resources/splash.jpg` ("জুলাই 36": students raising the flag before a red sun) was
supplied by the project owner for the remembrance splash screen. Before any public
release, confirm the artist and the right to redistribute it, and credit the artist here
and in the About box.

## Fonts

- No font files are included.
- Unicode output uses fonts that ship with Windows (for example Nirmala UI).
- Classic output needs the SutonnyMJ font, which the user must obtain separately.

## Website font

The website (`website/assets/fonts/`) self-hosts **Hind Siliguri** by Indian Type
Foundry, licensed under the SIL Open Font License 1.1
(<https://openfontlicense.org>). Only the Bengali and Latin subsets are included.

## Logo

`resources/logo.jpg` and the icons generated from it (`tools/Make-Icons.ps1`) were
supplied by the project owner.
