/*****************************************************************************

Copyright (c) 2011, 2015, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2016, 2018, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/ut0crc32.h
CRC32 implementation

Created Aug 10, 2011 Vasil Dimov
*******************************************************/

#ifndef ut0crc32_h
#define ut0crc32_h

#include "univ.i"

/********************************************************************//**
Initializes the data structures used by ut_crc32*(). Does not do any
allocations, would not hurt if called twice, but would be pointless. */
void
ut_crc32_init();
/*===========*/

/********************************************************************//**
Calculates CRC32.
@param ptr - data over which to calculate CRC32.
@param len - data length in bytes.
@return CRC32 (CRC-32C, using the GF(2) primitive polynomial 0x11EDC6F41,
or 0x1EDC6F41 without the high-order bit) */
typedef uint32_t	(*ut_crc32_func_t)(const byte* ptr, ulint len);

/** Pointer to CRC32 calculation function. */
extern ut_crc32_func_t	ut_crc32;

#ifdef INNODB_BUG_ENDIAN_CRC32
/** Pointer to CRC32 calculation function, which uses big-endian byte order
when converting byte strings to integers internally. */
extern uint32_t ut_crc32_legacy_big_endian(const byte* buf, ulint len);
#endif /* INNODB_BUG_ENDIAN_CRC32 */

/** Text description of CRC32 implementation */
extern const char*	ut_crc32_implementation;

#endif /* ut0crc32_h */
