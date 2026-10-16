# imbe-mod

imbe-mod: optimised IMBE (Improved Multi-Band Excitation) vocoder for
P25-style 4400 bps digital voice, derived from Pavel Yazev's fixed-point
IMBE encoder/decoder as vendored by op25
(gr-op25_repeater/lib/imbe_vocoder).
GPL v3 or later - see the source headers.

Sibling project of [codec2-vhf](https://github.com/imostlylurk/codec2-vhf),
optimised with the same methods: profile first, stay bit-exact against
the unmodified baseline, measure every claim.

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
- **Golden-file oracle**: encoded frames and round-trip PCM from the
  baseline are committed under `testdata/`; `imbebench --encode` must
  reproduce them byte-for-byte.

## Performance (x86-64, 28 s speech corpus, 30 passes)

| Task   | baseline | optimised | speedup |
|--------|----------|-----------|---------|
| encode | 335 us / 20 ms frame | 64 us | 5.3x |
| decode | 45 us / 20 ms frame | 25.5 us | 1.75x |
| init   | 6.3 us | 2.1 us | 3x |

State: `sizeof(imbe_vocoder)` = 6,972 B (single object, no heap after
construction).  Profile after optimisation: FFT ~24%, `e_p` ~23%,
pitch tracking ~11%.

## Build and test

```
cmake -B build && cmake --build build
cd build && ctest
./build/imbebench --bench testdata/librivox_male.raw 10
```

## Frame format

Each 20 ms frame (160 samples at 8 kHz) encodes to 11 bytes (88 bits),
packed from 8 x 11-bit words, word 0 in the low bits.  This matches the
wire format used by the `vocoder` tool suite
(`~/Projects/vocoder`), whose `.imbe` files interoperate directly.

## Future work

- Split encoder/decoder state for half-duplex RAM savings (the vocoder
  class holds both; members are largely separable).
- The fixed-point FFT is now the largest single cost (~24%); an exact
  restructuring may be possible but the saturating butterfly semantics
  make it risky - any change must stay byte-exact.
- Cortex-M4 footprint analysis as in codec2-vhf once the state split
  lands.
