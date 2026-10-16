# imbe-mod

Optimised IMBE (Improved Multi-Band Excitation) vocoder for P25-style
4400 bps digital voice, derived from Pavel Yazev's fixed-point IMBE
encoder/decoder as vendored by op25 (gr-op25_repeater/lib/imbe_vocoder).
GPL v3 or later - see the source headers.

Optimised with a strict bit-exactness policy: profile first, prove each
transform equivalent, verify every encoded frame and decoded sample
against the unmodified baseline.

## What changed relative to the op25 imbe_vocoder

- **CMake build** with a C wrapper API (`imbe_new` / `imbe_encode_frame` /
  `imbe_decode_frame`), 11-byte packed frames (88 bits per 20 ms).
- **Encoder 5.3x faster, decoder 1.75x** (x86-64, -O2/-O3+LTO), with all
  encoded frames and decoded PCM byte-identical to the baseline:
  - `e_p()` result caching: the pitch buffer slides by one frame per
    encode and `e_p()` is a pure function, so each frame's look-ahead
    results are the next frames' current results - up to 3x fewer
    `e_p()` evaluations.
  - Link-time optimization: the ETSI-style saturating helpers
    (`L_add`/`L_mult`/`L_shr`) were out-of-line function calls making
    up ~45% of runtime.
  - `autocorr()` exact-sum fast path with a sound guard: for these
    operands `L_mult` never saturates, and when `sum|2ab| <= MAX_32`
    no intermediate `L_add` can saturate either, so the saturating
    chain is provably equal to the exact integer sum (int64
    accumulation; the original loop is kept as the fallback).
  - Integer-only code: optimization flags and LTO cannot change
    numerics.
- **Decode regression oracle**: golden frame files (encoded by the
  unmodified baseline codec) are committed under `testdata/`; the
  `golden_decode` ctest decodes them and checks the output against
  committed sha256 hashes, so decode determinism is verified without
  keeping audio in-tree.

## Performance (x86-64, 28 s speech corpus, 30 passes)

| Task   | baseline | optimised | speedup |
|--------|----------|-----------|---------|
| encode | 335 us / 20 ms frame | 64 us | 5.3x |
| decode | 45 us / 20 ms frame | 25.5 us | 1.75x |
| init   | 6.3 us | 2.1 us | 3x |

State: `sizeof(imbe_vocoder)` = 6,972 B (single object, no heap after
construction).  Profile after optimisation: FFT ~24%, `e_p` ~23%,
pitch tracking ~11%.

## Frame format

Each 20 ms frame (160 samples at 8 kHz) encodes to 11 bytes (88 bits,
8 x 11-bit words, word 0 in the low bits, big-endian byte order).

## Build and test

```
cmake -B build && cmake --build build
cd build && ctest
./build/imbebench --bench <raw 8 kHz mono s16le speech> 10
```

`imbebench` modes: `--selftest` (synthetic round-trip), `--bench`,
`--encode in.raw out.frames`, `--decode in.frames out.raw`.

## Reproducing the encode bit-exactness check

Encode a speech file with the unmodified baseline codec and with this
build; the frame files must be byte-identical.  The golden files in
`testdata/` were produced this way (150-frame and 1399-frame male
speech corpora).

## Future work

- Split encoder/decoder state (done in the derivative imbe-mod2).
- The fixed-point FFT is now the largest single cost (~24%); an exact
  restructuring may be possible but the saturating butterfly semantics
  make it risky - any change must stay byte-exact.
- Cortex-M4 footprint analysis.
