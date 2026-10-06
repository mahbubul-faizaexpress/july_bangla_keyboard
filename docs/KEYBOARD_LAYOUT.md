# Keyboard layout

The Bijoy layout is defined in [`layouts/bijoy.layout`](../layouts/bijoy.layout). That
file is the only source of truth. At build time `tools/layoutgen` validates it and compiles
it into `BijoyLayout.inc`. The engine turns that into a constant 4 × 128 table, so a key
lookup is a single array index with no allocation.

## Model

- Keys are identified by **scan code** (physical position), not by the character the active
  Latin layout would produce.
- Each key has up to four **layers**, matching the four quadrants on a Bijoy key cap:
  `normal`, `shift`, `link` (after the linker `g`), and `link-shift`.
- A row maps one key/layer to a **token**: a kind (consonant, vowel-sign, vowel-sign-pre,
  phala, …) plus 1–3 UTF-16 code units.
- How tokens combine is the composer's job (Phase 3), not the layout's. Examples: `g` + `d`
  → ই; pre-base kar reordering; ে + া → ো.

## Provenance

Every row carries a `source`:

| source | meaning | release builds (`-DSTRICT_LAYOUT=ON`) |
|---|---|---|
| `chart` | read from the key caps; no reference chart contradicts it (see below) | allowed |
| `user-list` | from the user-supplied conjunct sequences | allowed |
| `memory` | engineer's recollection, not yet confirmed | **rejected** |

**Reference charts** (all supplied by the user on 2026-10-06):

- (a) the official "বিজয় বাংলা কীবোর্ড, তৃতীয় সংস্করণ" chart
- (b) a Unicode Bijoy chart that shows the link forms separately
- (c) the Marks PC Solution chart with its typing-rules table

All 80 rows are confirmed. Chart (a) puts some link forms at mid-height on the key cap,
which makes them easy to misread. Charts (b) and (c) settle those cases:

- ঔ = `g` + Shift+`x`
- ॥ = `g` + Shift+`g`
- ৎ / ঃ are on the backslash key

The following are not mapped yet, so they pass through to the application unchanged:

- the link layers of the number row and of the `,` `.` keys (fractions, Assamese ৰ ৱ;
  the charts disagree or are unreadable)
- Shift+6

## Typing order (visual order), for the Phase 3 composer

Bijoy is typed in **visual order**:

- **Pre-base vowel signs (ি ে ৈ) are typed before** the consonant cluster they belong to.
  Examples: কি = `d j`, ক্ষি = `d j g N`.
- ো = `c` + cluster + `f`, and ৌ = `c` + cluster + Shift+`x`.
- **Reph (Shift+`a`) is typed after** the cluster it sits on. If a post-base vowel sign
  follows the cluster, the reph comes after that sign too. Examples: কর্ম = `j m A`,
  কার্য = `j f w A`.

Evidence:

- Classic Bijoy writes glyphs in the order they are typed, so legacy SutonnyMJ text records
  the typing order. The well-known sample "Avwg" = আমি stores `w` (ি) before `g` (ম).
- The Bijoy→Unicode reordering logic in
  [Mad-FOX/bijoy2unicode](https://github.com/Mad-FOX/bijoy2unicode)
  (`converter.py`, `reArrangeUnicodeConvertedText`) moves pre-base kars after the cluster,
  joins ে + া/ৗ into ো/ৌ, and moves the reph glyph `©` from after the cluster (and after
  one kar) to র্ before it.

These cases are in `tests/golden/bijoy_typing_order.tsv`. They will run once the
composer exists (Phase 3). Accepting logical-order input as well can be added later as a
setting.

## Rules from the typing-rules table (c), for the Phase 3 composer

- Shift selects the upper letter on a key. The linker `g` joins consonants, e.g.
  হ্ন = `i g b`.
- Vowel signs, ্য and ্র are never joined with `g`. They follow the consonant directly,
  e.g. কৃ = ক + ৃ, ব্যা = ব + ্য + া.
- Caps Lock must be off in Bijoy. The engine ignores Caps Lock and uses only Shift.

> The two other chart images supplied together with the Bijoy chart show a **different
> layout**, not Bijoy (for example K = ক, `/` = hasant). They are not used.

## Tests

- `tests/unit/LayoutTests.cpp` restates every confirmed letter-key mapping by hand,
  using independently written scan codes, so an error in the layout file or in the
  generator surfaces as a mismatch.
- `tests/golden/bijoy_conjuncts.tsv` holds the user-supplied conjunct key sequences. Phase 2
  checks that their tokens concatenate to the exact expected code points. Phase 3 runs them
  through the composer.
- `tests/layoutgen/*.layout` holds malformed inputs that layoutgen must reject with a
  diagnostic: duplicate key, unknown key/layer/kind, bad or surrogate code point, missing
  code point, empty file, and a `memory` row under `--strict`.
