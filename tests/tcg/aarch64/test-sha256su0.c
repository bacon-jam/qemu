/* SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Copyright:
 *   2023      Yi-Yen Chung <eric681@andestech.com> (Copyright owned by Andes Technology)
 */


/*
 * Test SHA256SU0 lane ordering and overlapping source/destination registers.
 * Non-alias known answers are from SIMDe test/arm/neon/sha256.c at
 * 206506dd50a4b0c21d32a848d9a71b76a93d9404.  The notice above covers them.
 */
#include <asm/hwcap.h>
#include <sys/auxv.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HWCAP_SHA2
#define HWCAP_SHA2 (1 << 6)
#endif

static const struct {
    uint32_t d[4], n[4], expected[4];
} vectors[] = {
    { { 0x76b9ff64, 0x11853a46, 0xdabaf725, 0xf96e37a1 },
      { 0x093545b3, 0x3090ef69, 0xa26d9761, 0xa18c39dc },
      { 0x373c28c1, 0xfeb097ea, 0xabf25be5, 0x2fc63811 } },
    { { 0xc13c8bd2, 0x2212c66f, 0x9c1a0932, 0x4698d3bf },
      { 0x87c302c8, 0xdf25a0c7, 0xfbc2c86a, 0xcea03494 },
      { 0x2cda4197, 0x170a98a1, 0xdecc03a8, 0x87de9b6b } },
    { { 0x1b4051b9, 0x554511f2, 0x13791b13, 0xa055d0a3 },
      { 0x021165a3, 0x52d67ae0, 0x472f5c89, 0xde26274f },
      { 0xc5bf0f05, 0xb7d2277c, 0x3adc14b3, 0xbf849f9e } },
    { { 0x4afa7fbe, 0x1abccf55, 0xc3c33a89, 0xc108ecc5 },
      { 0x656717e1, 0x99351dec, 0xfef920aa, 0x5abed152 },
      { 0xe5b22699, 0xe01a6129, 0x6c55b68c, 0xcca7624f } },
    { { 0x60a07f8b, 0xdc97571c, 0x5d3e81bf, 0x05ad4b57 },
      { 0xa6838a46, 0x9f08b4ae, 0x3530ad0a, 0x2cd742a5 },
      { 0x588d72f3, 0xb209d197, 0x59a9b456, 0x80ba2b53 } },
    { { 0x53f8db06, 0x2d56613d, 0xf3080102, 0x6dda4c43 },
      { 0x94f6e65a, 0x6997d8d7, 0xe53b2961, 0x01f49211 },
      { 0x3bb806b6, 0x491e0e1d, 0x0b782768, 0x8bfbe07d } },
    { { 0xf3e69a7c, 0xd7ee3e24, 0x74264597, 0xe08a77ef },
      { 0xcdc99492, 0x7d5b1c9e, 0xd58f33bf, 0x545529a8 },
      { 0xd0c1c8bf, 0x88f79754, 0xd251e7c7, 0x39908ab8 } },
    { { 0x59901957, 0xf9b1dfe7, 0xbe196b77, 0xc53cbf1c },
      { 0xfe93332f, 0xc838e9f5, 0x6ef5b105, 0x5b82887e },
      { 0x00ccbf86, 0x9c14d025, 0xcd044b49, 0x52217ec3 } },
};

static const struct {
    uint32_t d[4], expected[4];
} alias_vectors[] = {
    { { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
      { 0x00000000, 0x00000000, 0x00000000, 0x00000000 } },
    { { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
      { 0x1ffffffe, 0x1ffffffe, 0x1ffffffe, 0x1ffffffe } },
    { { 0x00000000, 0x00000001, 0x00000002, 0x00000003 },
      { 0x02004000, 0x04008001, 0x0600c002, 0x00000003 } },
    { { 0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210 },
      { 0x3e8111b3, 0x8a2bdf80, 0x217eee4b, 0x95d4207e } },
};

static void sha256su0(uint32_t out[4], const uint32_t d[4],
                     const uint32_t n[4])
{
    asm volatile("ld1 {v0.4s}, [%1]\n\t"
                 "ld1 {v1.4s}, [%2]\n\t"
                 "sha256su0 v0.4s, v1.4s\n\t"
                 "st1 {v0.4s}, [%0]"
                 : : "r"(out), "r"(d), "r"(n)
                 : "v0", "v1", "memory");
}

static void sha256su0_alias(uint32_t out[4], const uint32_t d[4])
{
    asm volatile("ld1 {v0.4s}, [%1]\n\t"
                 "sha256su0 v0.4s, v0.4s\n\t"
                 "st1 {v0.4s}, [%0]"
                 : : "r"(out), "r"(d) : "v0", "memory");
}

int main(void)
{
    uint32_t out[4];

    if (!(getauxval(AT_HWCAP) & HWCAP_SHA2)) {
        puts("SKIP: no HWCAP_SHA2 on this system");
        return EXIT_SUCCESS;
    }

    for (unsigned i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        sha256su0(out, vectors[i].d, vectors[i].n);
        for (unsigned lane = 0; lane < 4; lane++) {
            assert(out[lane] == vectors[i].expected[lane]);
        }
    }

    /* Lane3 must use the original D[0], even when the register aliases. */
    for (unsigned i = 0;
         i < sizeof(alias_vectors) / sizeof(alias_vectors[0]); i++) {
        sha256su0_alias(out, alias_vectors[i].d);
        for (unsigned lane = 0; lane < 4; lane++) {
            assert(out[lane] == alias_vectors[i].expected[lane]);
        }
        sha256su0(out, alias_vectors[i].d, alias_vectors[i].d);
        for (unsigned lane = 0; lane < 4; lane++) {
            assert(out[lane] == alias_vectors[i].expected[lane]);
        }
    }
    return EXIT_SUCCESS;
}
