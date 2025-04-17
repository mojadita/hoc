/* error.h -- prototipos de las funciones de error.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Tue Dec 31 16:20:44 -05 2024
 * Copyright: (c) 2025 Edward Rivas y Luis Colorado.  All rights reserved.
 * License: BSD.
 */
#ifndef ERROR_H
#define ERROR_H

void execerror(const char *fmt, ...);
void warning(const char *fmt, ...);    /* print warning message */
void vwarning(const char *fmt, va_list args);

void defnonly(int cual, const char *name, ...);

#endif /* ERROR_H */
