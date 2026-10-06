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
| `master` | verified on the master Bijoy chart (see below) | allowed |
| `chart` | not on the master, but on another Bijoy chart and contradicted by none | allowed |
| `user-list` | from the user-supplied conjunct sequences | allowed |
| `memory` | engineer's recollection, not yet confirmed | **rejected** |

**This product follows the original Bijoy layout only.** No row comes from Avro or any
other layout.

**Master reference:** the Marks PC Solution Bijoy chart with its typing-rules table. The
user designated it the master on 2026-10-06. When charts disagree, the master wins.

Secondary Bijoy charts (also supplied by the user on 2026-10-06):

- the official "বিজয় বাংলা কীবোর্ড, তৃতীয় সংস্করণ" chart
- a Unicode Bijoy chart that shows the link forms separately

There are 80 rows: 79 are verified on the master. The only exception is ॥ =
`g` + Shift+`g`, which appears on both secondary charts but not on the master. A unit test
fails if any other non-master row is added, so a mapping from another layout cannot slip
in unnoticed.

Cases the master settles:

- ঔ = `g` + Shift+`x` (its rules table)
- ৎ / ঃ are on the backslash key
- Ctrl+Alt+B switches the keyboard, which matches this product's hotkey

Unmapped keys pass through to the application unchanged. On the master these keys are
plain English, so they are intentionally left unmapped:

- Shift+6 (^)
- `,` `.` `/`
- `;` `-` `=` `[` `]` and their shifted forms

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
