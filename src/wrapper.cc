/* Thin C shim over Pavel Yazev's IMBE vocoder (from op25, GPLv2).
 * Frames are packed to 11 bytes (88 bits, 8 x 11-bit words).
 */

#include <stdint.h>
#include "imbe_vocoder.h"
#include "imbe_vocoder_wrapper.h"

extern "C" {

void *imbe_new(void)
{
    return new imbe_vocoder();
}

void imbe_free(void *v)
{
    delete (imbe_vocoder *)v;
}

static void pack_words(const int16_t *w, uint8_t *out)
{
    /* 8 words x 11 bits = 88 bits: w7 in the low bits (matches the
       vocoder.py convention); needs more than 64 bits */
    unsigned __int128 all = 0;
    for (int i = 0; i < 8; i++)
        all = (all << 11) | (unsigned)(w[i] & 0x7FF);
    for (int i = 0; i < 11; i++)
        out[i] = (uint8_t)((all >> (8 * i)) & 0xFF); /* little-endian */
}

static void unpack_words(const uint8_t *in, int16_t *w)
{
    unsigned __int128 all = 0;
    for (int i = 0; i < 11; i++)
        all |= (unsigned __int128)in[i] << (8 * i);
    for (int i = 0; i < 8; i++)
    {
        w[i] = (int16_t)(all & 0x7FF);
        all >>= 11;
    }
}

void imbe_encode_frame(void *v, uint8_t *frame11, const int16_t *snd)
{
    int16_t words[8];
    ((imbe_vocoder *)v)->imbe_encode(words, const_cast<int16_t *>(snd));
    pack_words(words, frame11);
}

void imbe_decode_frame(void *v, int16_t *snd, const uint8_t *frame11)
{
    int16_t words[8];
    unpack_words(frame11, words);
    ((imbe_vocoder *)v)->imbe_decode(words, snd);
}

} /* extern "C" */
