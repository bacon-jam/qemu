/*
 * No host specific CRC32C acceleration.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef GENERIC_HOST_CRYPTO_CRC32C_H
#define GENERIC_HOST_CRYPTO_CRC32C_H

#define HAVE_CRC32C_ACCEL  false
#define ATTR_CRC32C_ACCEL

uint32_t crc32c_accel(uint32_t, uint64_t, uint32_t)
    QEMU_ERROR("unsupported accel");

#endif /* GENERIC_HOST_CRYPTO_CRC32C_H */
