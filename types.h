/* types.h -- type definitions used by type related things.
 * Author: Edward Rivas <rivastkw@gmail.com>
 * Date: Mon Sep 29 04:48:19 -05 2025
 * Copyright: (c) 2025 Edward Rivas.  All rights reserved.
 * License: BSD
 */
#ifndef TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329
#define TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329

#include "config.h"

#ifndef   FMT_CHAR  /* { */
#warning  FMT_CHAR should be defined in config.mk
#define   FMT_CHAR "0x%02hhx"
#endif /* FMT_CHAR     } */

#ifndef   FMT_DOUBLE  /* { */
#warning  FMT_DOUBLE should be defined in config.mk
#define   FMT_DOUBLE "%.15lg"
#endif /* FMT_DOUBLE     } */

#ifndef   FMT_FLOAT  /* { */
#warning  FMT_FLOAT should be defined in config.mk
#define   FMT_FLOAT "%.7g"
#endif /* FMT_FLOAT     } */

#ifndef   FMT_INT  /* { */
#warning  FMT_INT should be defined in config.mk
#define   FMT_INT "%i"
#endif /* FMT_INT     } */

#ifndef   FMT_LONG  /* { */
#warning  FMT_LONG should be defined in config.mk
#define   FMT_LONG "%liL"
#endif /* FMT_LONG     } */

#ifndef   FMT_SHORT  /* { */
#warning  FMT_SHORT should be defined in config.mk
#define   FMT_SHORT "0x%04hx"
#endif /* FMT_SHORT     } */

extern const Symbol
       *Char,
       *Double,
       *Float,
       *Integer,
       *Long,
       *Short,
       *String,
       *Prev;

#endif /* TYPES_H_9fc2bf3a_9d19_11f0_8203_0023ae68f329 */
