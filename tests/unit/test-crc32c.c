/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "qemu/crc32c.h"
#include "host/crypto/crc32c.h"

static uint32_t reference(uint32_t crc, const uint8_t *data, unsigned size)
{
    while (size--) {
        crc ^= *data++;
        for (unsigned bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0x82f63b78U : 0);
        }
    }
    return crc;
}

static void test_accel_value(uint32_t acc, uint64_t val)
{
    uint8_t data[8];

    stq_le_p(data, val);
    for (unsigned bytes = 1; bytes <= 8; bytes *= 2) {
        uint32_t expected = reference(acc, data, bytes);

        if (HAVE_CRC32C_ACCEL) {
            g_assert_cmphex(crc32c_accel(acc, val, bytes), ==, expected);
        }
    }
}

static void test_accel(void)
{
    uint64_t state = 0x9e3779b97f4a7c15ULL;

    if (!HAVE_CRC32C_ACCEL) {
        g_test_skip("No host CRC32C acceleration");
        return;
    }
    test_accel_value(0, 0);
    test_accel_value(UINT32_MAX, UINT64_MAX);
    for (unsigned bit = 0; bit < 64; bit++) {
        test_accel_value(0, UINT64_C(1) << bit);
        test_accel_value(UINT32_MAX, ~(UINT64_C(1) << bit));
        if (bit < 32) {
            test_accel_value(UINT32_C(1) << bit, 0);
            test_accel_value(~(UINT32_C(1) << bit), UINT64_MAX);
        }
    }
    for (unsigned i = 0; i < 4096; i++) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        test_accel_value(state >> 32, state);
    }
}

static void test_buffer(void)
{
    uint8_t data[137];
    static const uint32_t accumulators[] = {
        0, UINT32_MAX, 0x12345678, 0x80000000,
    };

    g_assert_cmphex(crc32c(UINT32_MAX, (const uint8_t *)"123456789", 9),
                    ==, 0xe3069283);
    for (unsigned i = 0; i < sizeof(data); i++) {
        data[i] = (i * 37) ^ (i >> 1);
    }
    for (unsigned offset = 0; offset < 8; offset++) {
        for (unsigned size = 0; size <= 129; size++) {
            for (unsigned a = 0; a < ARRAY_SIZE(accumulators); a++) {
                uint32_t expected = reference(accumulators[a], data + offset,
                                              size);
                uint32_t actual = accumulators[a];
                unsigned i = 0;

                g_assert_cmphex(crc32c(actual, data + offset, size),
                                ==, ~expected);
                if (HAVE_CRC32C_ACCEL) {
                    for (; size - i >= 8; i += 8) {
                        actual = crc32c_accel(actual,
                                             ldq_le_p(data + offset + i), 8);
                    }
                    for (; i < size; i++) {
                        actual = crc32c_accel(actual, data[offset + i], 1);
                    }
                    g_assert_cmphex(actual, ==, expected);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/crc32c/buffer", test_buffer);
    g_test_add_func("/crc32c/accel", test_accel);
    return g_test_run();
}
