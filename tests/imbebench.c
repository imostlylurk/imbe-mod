/* imbebench: baseline measurement and bit-exactness oracle for the
 * IMBE vocoder.
 *
 * usage:
 *   imbebench --selftest              round-trip sanity check
 *   imbebench --bench in.raw [passes] timing per frame
 *   imbebench --encode in.raw out.imbeframes   dump 11-byte frames
 *   imbebench --decode in.imbeframes out.raw   frames -> pcm
 *
 * Encoded frames are 11 bytes (88 bits) per 160 samples (20 ms).
 */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "imbe_vocoder_wrapper.h"

static double now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e6 + ts.tv_nsec / 1e3;
}

static int16_t *load_raw(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror(path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    int16_t *buf = malloc(sz & ~1);
    *n = fread(buf, 1, sz, f) / 2;
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s --selftest | --bench in.raw [passes] | "
                        "--encode in.raw out | --decode in out.raw\n",
                argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], "--selftest"))
    {
        void *v = imbe_new();
        int16_t in[160], out[160];
        uint8_t frame[11];
        double acc = 0;
        for (int i = 0; i < 160; i++)
            in[i] = (int16_t)(6000 * sin(2 * 3.14159265 * 190 * i / 8000.0));
        /* 50 frames of tone + silence tail */
        int nonzero = 0;
        for (int f = 0; f < 50; f++)
        {
            for (int i = 0; i < 160; i++)
                in[i] = f < 40 ? (int16_t)(6000 * sin(2 * 3.14159265 *
                                                     (150 + 50 * (f % 3)) * (f * 160 + i) / 8000.0))
                               : 0;
            imbe_encode_frame(v, frame, in);
            imbe_decode_frame(v, out, frame);
            for (int i = 0; i < 160; i++)
                acc += (double)out[i] * out[i];
            for (int i = 0; i < 11; i++)
                if (frame[i])
                    nonzero = 1;
        }
        imbe_free(v);
        double rms = sqrt(acc / (50.0 * 160));
        int ok = rms > 100 && rms < 30000 && nonzero;
        printf("selftest: rms %.1f %s\n", rms, ok ? "PASS" : "FAIL");
        return !ok;
    }

    if (!strcmp(argv[1], "--bench"))
    {
        size_t n;
        int16_t *pcm = load_raw(argv[2], &n);
        int passes = argc > 3 ? atoi(argv[3]) : 20;
        size_t frames = n / 160;
        void *v = imbe_new();
        uint8_t frame[11];
        int16_t out[160];

        double t0 = now_us();
        for (int r = 0; r < 1000; r++)
        {
            void *w = imbe_new();
            imbe_free(w);
        }
        printf("init        %8.1f us\n", (now_us() - t0) / 1000);

        double et = 0, dt = 0, emax = 0, dmax = 0;
        for (int p = 0; p < passes; p++)
        {
            for (size_t f = 0; f < frames; f++)
            {
                t0 = now_us();
                imbe_encode_frame(v, frame, &pcm[f * 160]);
                double d = now_us() - t0;
                et += d;
                if (d > emax)
                    emax = d;

                t0 = now_us();
                imbe_decode_frame(v, out, frame);
                d = now_us() - t0;
                dt += d;
                if (d > dmax)
                    dmax = d;
            }
        }
        size_t cnt = frames * passes;
        printf("frames      %zu x %d\n", frames, passes);
        printf("encode      %8.1f us avg  %8.1f us max\n", et / cnt, emax);
        printf("decode      %8.1f us avg  %8.1f us max\n", dt / cnt, dmax);
        printf("full-duplex CPU load (20 ms frames) %.1f %%\n",
               (et / cnt + dt / cnt) / 20000.0 * 100);
        return 0;
    }

    if (!strcmp(argv[1], "--encode"))
    {
        size_t n;
        int16_t *pcm = load_raw(argv[2], &n);
        FILE *f = fopen(argv[3], "wb");
        void *v = imbe_new();
        uint8_t frame[11];
        size_t frames = (n + 159) / 160;
        for (size_t i = 0; i < frames; i++)
        {
            int16_t buf[160] = {0};
            size_t have = n - i * 160;
            if (have > 160)
                have = 160;
            memcpy(buf, &pcm[i * 160], have * 2);
            imbe_encode_frame(v, frame, buf);
            fwrite(frame, 1, 11, f);
        }
        fclose(f);
        fprintf(stderr, "%s: %zu frames\n", argv[3], frames);
        return 0;
    }

    if (!strcmp(argv[1], "--decode"))
    {
        size_t n;
        uint8_t *data = (uint8_t *)load_raw(argv[2], &n); /* byte pairs ok */
        FILE *f = fopen(argv[3], "wb");
        void *v = imbe_new();
        size_t frames = n / 11;
        int16_t out[160];
        for (size_t i = 0; i < frames; i++)
        {
            imbe_decode_frame(v, out, &data[i * 11]);
            fwrite(out, 2, 160, f);
        }
        fclose(f);
        fprintf(stderr, "%s: %zu frames\n", argv[3], frames);
        return 0;
    }

    return 1;
}
