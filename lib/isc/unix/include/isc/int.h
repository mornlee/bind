/*
 * Copyright (C) 1999-2001, 2004, 2005, 2007, 2016  Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* $Id: int.h,v 1.16 2007/06/19 23:47:19 tbox Exp $ */

#ifndef ISC_INT_H
#define ISC_INT_H 1

/*! \file */

typedef char				isc_int8_t;
typedef unsigned char			isc_uint8_t;
typedef short				isc_int16_t;
typedef unsigned short			isc_uint16_t;
typedef int				isc_int32_t;
typedef unsigned int			isc_uint32_t;
typedef long long			isc_int64_t;
typedef unsigned long long		isc_uint64_t;

#define ISC_INT8_MIN	-128
#define ISC_INT8_MAX	127
#define ISC_UINT8_MAX	255

#define ISC_INT16_MIN	-32768
#define ISC_INT16_MAX	32767
#define ISC_UINT16_MAX	65535

/*%
 * Note that "int" is 32 bits on all currently supported Unix-like operating
 * systems, but "long" can be either 32 bits or 64 bits, thus the 32 bit
 * constants are not qualified with "L".
 */
#define ISC_INT32_MIN	-2147483648
#define ISC_INT32_MAX	2147483647
#define ISC_UINT32_MAX	4294967295U

#define ISC_INT64_MIN	-9223372036854775808LL
#define ISC_INT64_MAX	9223372036854775807LL
#define ISC_UINT64_MAX	18446744073709551615ULL

#endif /* ISC_INT_H */
