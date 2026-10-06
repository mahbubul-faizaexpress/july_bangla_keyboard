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
| `photo3` | read from the Bijoy keyboard chart, 3rd edition | allowed |
| `user-list` | from the user-supplied conjunct sequences | allowed |
| `memory` | engineer's recollection, not yet confirmed | **rejected** |

These rows are currently `memory` and need confirmation:

- Shift+G (।)
- x (ও)
- Shift+X (ৗ)
- link-shift X (ঔ)
- Shift+4 (৳)
- Shift+7 (ঁ)

The following are not mapped yet, so they pass through to the application unchanged:

- Shift+A
- the link layer of `x`
- the other shifted number-row symbols
- the `` ` `` `,` `.` keys and their shifted forms

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
