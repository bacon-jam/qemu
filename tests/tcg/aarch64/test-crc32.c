/*
 * AArch64 CRC32/CRC32C operand widths and register aliases.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <assert.h>
#include <stdint.h>
#include <stddef.h>

static uint32_t reference(uint64_t acc, uint64_t val, unsigned bytes,
                          uint32_t polynomial)
{
    uint32_t crc = acc;

    for (unsigned i = 0; i < bytes; i++) {
        crc ^= (uint8_t)val;
        val >>= 8;
        for (unsigned bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
        }
    }
    return crc;
}

/* Observe the entire X destination, including zero-extension of the W result. */
#define CHECK_OP(insn, width, rd, rn, rm, expected)                         \
    do {                                                                 \
        uint64_t result;                                                 \
        asm volatile("mov x0, %x[a]\n\t"                                 \
                     "mov x1, %x[v]\n\t"                                 \
                     "mov x2, -1\n\t"                                    \
                     insn " w" rd ", w" rn ", " width rm "\n\t"          \
                     "mov %x[r], x" rd                                   \
                     : [r] "=r" (result)                                 \
                     : [a] "r" (acc), [v] "r" (val)                       \
                     : "x0", "x1", "x2");                                \
        assert(result == (uint64_t)(expected));                          \
    } while (0)

#define CHECK_WIDTH(insn, width, bytes, poly)                              \
    do {                                                                 \
        uint32_t expected = reference(acc, val, bytes, poly);            \
        uint32_t same = reference(acc, acc, bytes, poly);                 \
        CHECK_OP(insn, width, "2", "0", "1", expected);                   \
        CHECK_OP(insn, width, "0", "0", "1", expected);                   \
        CHECK_OP(insn, width, "1", "0", "1", expected);                   \
        CHECK_OP(insn, width, "2", "0", "0", same);                       \
        CHECK_OP(insn, width, "0", "0", "0", same);                       \
        CHECK_OP(insn, width, "2", "zr", "1",                             \
                 reference(0, val, bytes, poly));                         \
        CHECK_OP(insn, width, "2", "0", "zr",                             \
                 reference(acc, 0, bytes, poly));                         \
        CHECK_OP(insn, width, "zr", "0", "1", 0);                         \
    } while (0)

static void test_operands(uint64_t acc, uint64_t val)
{
    CHECK_WIDTH("crc32cb", "w", 1, 0x82f63b78);
    CHECK_WIDTH("crc32ch", "w", 2, 0x82f63b78);
    CHECK_WIDTH("crc32cw", "w", 4, 0x82f63b78);
    CHECK_WIDTH("crc32cx", "x", 8, 0x82f63b78);
    CHECK_WIDTH("crc32b", "w", 1, 0xedb88320);
    CHECK_WIDTH("crc32h", "w", 2, 0xedb88320);
    CHECK_WIDTH("crc32w", "w", 4, 0xedb88320);
    CHECK_WIDTH("crc32x", "x", 8, 0xedb88320);
}

static uint64_t random_value(uint64_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

/* Process complete messages with 8-byte updates and byte tails. */
static uint32_t message_crc(const uint8_t *data, size_t size)
{
    uint32_t crc = UINT32_MAX;

    while (size >= 8) {
        uint64_t value = 0;

        for (unsigned i = 0; i < 8; i++) {
            value |= (uint64_t)data[i] << (8 * i);
        }
        asm("crc32cx %w[c], %w[c], %x[v]"
            : [c] "+r" (crc) : [v] "r" (value));
        data += 8;
        size -= 8;
    }
    while (size--) {
        uint32_t value = *data++;

        asm("crc32cb %w[c], %w[c], %w[v]"
            : [c] "+r" (crc) : [v] "r" (value));
    }
    return ~crc;
}

int main(void)
{
    static const uint64_t values[] = {
        0, UINT64_MAX, 0xaaaaaaaaaaaaaaaaULL, 0x5555555555555555ULL,
        0x0123456789abcdefULL, 0xfedcba9876543210ULL,
    };
    uint64_t state = 0x9e3779b97f4a7c15ULL;
    uint8_t data[137];

    for (unsigned a = 0; a < sizeof(values) / sizeof(values[0]); a++) {
        for (unsigned v = 0; v < sizeof(values) / sizeof(values[0]); v++) {
            test_operands(values[a], values[v]);
        }
    }
    for (unsigned bit = 0; bit < 64; bit++) {
        test_operands(UINT64_C(1) << bit, 0);
        test_operands(0, UINT64_C(1) << bit);
        test_operands(~(UINT64_C(1) << bit), UINT64_MAX);
        test_operands(UINT64_MAX, ~(UINT64_C(1) << bit));
    }
    for (unsigned i = 0; i < 1024; i++) {
        uint64_t acc = random_value(&state);
        uint64_t val = random_value(&state);

        test_operands(acc, val);
    }

    assert(message_crc((const uint8_t *)"123456789", 9) == 0xe3069283);
    for (unsigned i = 0; i < sizeof(data); i++) {
        data[i] = random_value(&state);
    }
    for (unsigned offset = 0; offset < 8; offset++) {
        for (unsigned size = 0; size <= 129; size++) {
            uint32_t expected = UINT32_MAX;

            for (unsigned i = 0; i < size; i++) {
                expected = reference(expected, data[offset + i], 1,
                                     0x82f63b78);
            }
            assert(message_crc(data + offset, size) == ~expected);
        }
    }
    return 0;
}
