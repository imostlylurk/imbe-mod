/* Thin C shim over Pavel Yazev's IMBE vocoder (from op25, GPLv2).
 * Frames are packed to 11 bytes (88 bits, 8 x 11-bit words).
 */

#include <stdint.h>
#include <string.h>
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

/* Standard IMBE 88-bit prioritized info stream: u0..u3 are 12-bit
   words, u4..u6 11-bit, u7 7-bit; u0's MSB is the first bit of the
   frame, big-endian byte order.  (The previous 8x11-bit packing
   truncated the top bit of u0..u3 - the four lowest-priority
   spectral bits of each frame - on ~half of all frames; forwarded
   from imbe-mod2.) */
static const int word_bits[8] = {12, 12, 12, 12, 11, 11, 11, 7};

static void pack_words(const int16_t *w, uint8_t *out)
{
    int bit = 0;
    memset(out, 0, 11);
    for (int i = 0; i < 8; i++)
        for (int k = word_bits[i] - 1; k >= 0; k--)
        {
            out[bit >> 3] |= ((w[i] >> k) & 1) << (7 - (bit & 7));
            bit++;
        }
}

static void unpack_words(const uint8_t *in, int16_t *w)
{
    int bit = 0;
    for (int i = 0; i < 8; i++)
    {
        int v = 0;
        for (int k = 0; k < word_bits[i]; k++)
        {
            v = (v << 1) | ((in[bit >> 3] >> (7 - (bit & 7))) & 1);
            bit++;
        }
        w[i] = (int16_t)v;
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
