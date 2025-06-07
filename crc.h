/* crc.h -- definitions for crc calculations.
 * AUTHOR: Luis Colorado <luiscoloradourcola@gmail.com>
 * DATE: Fri Nov  4 12:06:21 CET 2005
 * Copyright: Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef CRC_H
#define CRC_H
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdint.h>

/* constants */
#define CRC_TABLE_SIZE	256
#define CRC_BYTE_SIZE	8
#define CRC_BYTE_MASK	0xff

/* types */
typedef uint8_t  CRC_BYTE;
typedef uint32_t CRC_STATE;

/* prototypes */

CRC_STATE do_crc(
	CRC_STATE   state,
	const char *buff,
	size_t      nbytes,
	CRC_STATE  *table);

#ifdef __cplusplus
} // extern "C"
#endif /* __cplusplus */
#endif /* CRC_H */
