/* colors.h -- ANSI escape sequences to colourize output.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Jan 21 13:39:23 EET 2025
 * Copyright: (c) 2024-2025 Edward Rivas y Luis Colorado.  All rights reserved.
 */
#ifndef COLORS_H_1734472e_ace6_11f0_af09_0023ae68f329
#define COLORS_H_1734472e_ace6_11f0_af09_0023ae68f329

#ifndef UQ_USE_COLORS  /* { */
#warning define UQ_USE_COLORS in "config.mk" file and then include "config.h" \
    before "colors.h".
#define UQ_USE_COLORS   0
#endif /* UQ_USE_COLORS } */

#if UQ_USE_COLORS /* {{ */
# define BRIGHT   "\033[1m"
# define WHITE    "\033[37m"
# define CYAN     "\033[36m"
# define MAGENTA  "\033[35m"
# define BLUE     "\033[34m"
# define YELLOW   "\033[33m"
# define GREEN    "\033[32m"
# define RED      "\033[31m"
# define ANSI_END "\033[m"
#else /* !UQ_USE_COLORS }{ */
# define BRIGHT   ""
# define WHITE    ""
# define CYAN     ""
# define MAGENTA  ""
# define BLUE     ""
# define YELLOW   ""
# define GREEN    ""
# define RED      ""
# define ANSI_END ""
#endif /* UQ_USE_COLORS }}*/

#endif /* COLORS_H_1734472e_ace6_11f0_af09_0023ae68f329 */
