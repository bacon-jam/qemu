/*
 * x86 CRC32C acceleration.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef X86_HOST_CRYPTO_CRC32C_H
#define X86_HOST_CRYPTO_CRC32C_H

#include "host/cpuinfo.h"
#include <immintrin.h>

#if defined(__SSE4_2__)
# define HAVE_CRC32C_ACCEL  true
# define ATTR_CRC32C_ACCEL
#else
# define HAVE_CRC32C_ACCEL  likely(cpuinfo & CPUINFO_SSE42)
# define ATTR_CRC32C_ACCEL  __attribute__((target("sse4.2")))
#endif

/* Update an uncomplemented CRC with the low 1, 2, 4 or 8 bytes of val. */
static inline uint32_t ATTR_CRC32C_ACCEL
crc32c_accel(uint32_t crc, uint64_t val, uint32_t bytes)
{
    switch (bytes) {
    case 1:
        return _mm_crc32_u8(crc, val);
    case 2:
        return _mm_crc32_u16(crc, val);
    case 4:
        return _mm_crc32_u32(crc, val);
    case 8:
        return _mm_crc32_u64(crc, val);
    default:
        g_assert_not_reached();
    }
}

#endif /* X86_HOST_CRYPTO_CRC32C_H */
