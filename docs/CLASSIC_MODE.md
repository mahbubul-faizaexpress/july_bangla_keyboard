# Classic mode (Bijoy ANSI / SutonnyMJ)

Classic mode produces the same text that Bijoy Classic produces: SutonnyMJ glyph codes,
for documents and applications that use legacy Bangla fonts. It shares the keyboard
layout and the composer with Unicode mode. Only the output backend differs.

```
Bijoy keys -> composer (syllable of typed keys) -> Classic backend -> SutonnyMJ codes
                                                -> Unicode backend -> logical Unicode
```

## Encoding assumptions

- SutonnyMJ is an ANSI font. It draws Bangla glyph shapes on Windows-1252 byte values; it
  is **not** Unicode, and changing the font of Unicode text does not produce Classic text.
- The text service inserts the **Unicode equivalent of each byte** (cp1252 decoding).
  Byte `0x87` (ে) is inserted as U+2021 `‡`, and `0xB6` (ক্ষ) as U+00B6 `¶`.
  - In a Unicode application with SutonnyMJ selected, this displays as Bangla and saves
    exactly what Bijoy would.
  - In an ANSI (non-Unicode) application, Windows converts it back to the original bytes,
    but **only when the system ANSI code page is 1252**. On any other code page the text
    would degrade, so the diagnostics (`checkClassicEnvironment`) report the code page and
    the UI warns.
- The product never downloads fonts. The user selects SutonnyMJ in the application, as
  with Bijoy, and the diagnostics report whether it is installed.

## Visual order

Classic text is stored in visual order, matching Bijoy typing:

| Text | Classic glyph codes | Rule |
|---|---|---|
| কি | `wK` | pre-base kar (ি) before the consonant |
| কো | `‡Kv` | ে before the cluster, া after |
| কৌ | `‡KŠ` | ে before the cluster, ৗ after |
| কর্ম | `Kg©` | reph glyph `©` after the consonant it sits on |
| ক্ষি | `w¶` | kar before a whole conjunct |

The composer already keeps ো/ৌ as ে + া/ৗ, so the Classic backend emits the halves in
place without decomposing anything.

## Glyph table

[`layouts/sutonnymj.classic`](../layouts/sutonnymj.classic) is the source of truth. It
has 222 rows: Unicode sequence → cp1252 bytes. At build time `tools/layoutgen/ClassicGen`
validates it (undefined cp1252 bytes, duplicates, lengths) and generates a sorted table.
The backend does a greedy longest match, so ক্ষ্ম becomes `²` rather than `¶` + `&g`.

**Provenance:** the rows come from the Unicode→Bijoy table of
[Mad-FOX/bijoy2unicode](https://github.com/Mad-FOX/bijoy2unicode). The only change at
import was that র‍্য uses ZWJ, as the Unicode standard requires.

**Verified in the font (2026-10-07):** all 222 rows are `source=verified`. Each row was
rendered with the SutonnyMJ font (supplied by the project owner and never committed) next
to the Unicode text in Nirmala UI, and also inside real words, then checked by eye.
Fifteen rows were corrected; the reasons are listed in the table header:

- **ল-ফলা (7 rows):** used the soft hyphen `AD`, which renders as "-" or not at all. They
  now use `AC`.
- **রু, দ্রু, ন্ত, স্ত, ন্ত্ব (5 rows):** used `93` and `97`, which render detached or in a
  different shape in this font. They are now written with plain letters.
- **হ্ব (1 row):** `9F` read as "হব"; it now uses `A1`.
- **ণ্ড (1 row):** now written with a visible hasant, because the `Ê` glyph renders broken.
- **চ্ঞ (1 row):** kept, but it shows as half-চ + ঞ rather than a ligature.

`STRICT_LAYOUT` is now ON by default, so any new unverified row fails the build.

**Independent evidence:** the widely quoted SutonnyMJ samples "Avwg evsjvq Mvb MvB"
(আমি বাংলায় গান গাই) and "Kv‡R" (কাজে) are reproduced exactly by typing the Bijoy keys
(`tests/golden/classic_sutonnymj.tsv`).

## Known gaps and limits

- **৳ (Shift+4)** has no SutonnyMJ code in the table. It is emitted unchanged until a
  verified code is found. The fuzzer allows exactly this one Bengali character through.
- SutonnyMJ has alternative glyphs for some signs (several ু, ূ, ৃ, ে, ৈ variants). The
  table uses one variant each, plus the converter's special forms (গু, রু, রূ, হু, হৃ, শু,
  ন্তু, স্তু). Bijoy itself may choose other variants in some contexts, so this needs
  checking against real Bijoy output.
- A reph typed after a post-base kar is emitted before the kar (`K©v` for র্কা). Both
  orders render the same in SutonnyMJ according to the converter's handling; this needs
  confirming in the font.

## Tests

- **Golden Classic output:** 24 cases, 5 of them known samples.
- **Table checks:** every table glyph round-trips through cp1252 (`WideCharToMultiByte`,
  no best-fit), and the table is sorted and duplicate-free.
- **Coverage:** every character the layout can type has a glyph, except ৳.
- **Fuzzing:** `july_fuzz` runs Classic mode and fails if any Bengali code point other than
  ৳ (or a ZWJ) reaches Classic output.
- **classicgen validation:** malformed tables are rejected with a diagnostic.
