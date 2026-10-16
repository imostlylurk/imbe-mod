/* Thin C shim over Pavel Yazev's IMBE vocoder (from op25, GPLv2) for ctypes use. */
#include <stdint.h>
#include "imbe_vocoder.h"

extern "C" {

void* imbe_new(void)
{
    return new imbe_vocoder();
}

void imbe_free(void* v)
{
    delete (imbe_vocoder*)v;
}

/* snd: 160 samples in; frame_words: 8 x int16, 11 significant bits each */
void imbe_encode_frame(void* v, int16_t* frame_words, int16_t* snd)
{
    ((imbe_vocoder*)v)->imbe_encode(frame_words, snd);
}

/* frame_words: 8 x int16 (11 bits each); snd: 160 samples out */
void imbe_decode_frame(void* v, int16_t* frame_words, int16_t* snd)
{
    ((imbe_vocoder*)v)->imbe_decode(frame_words, snd);
}

}
