/* error.h -- prototipos de las funciones de error.
 * Date: Tue Dec 31 16:20:44 -05 2024
 */
#ifndef ERROR_H
#define ERROR_H

void execerror(const char *fmt, ...);
void warning(const char *fmt, ...);    /* print warning message */
void vwarning(const char *fmt, va_list args);

void defnonly(int cual, const char *name, ...);

#endif /* ERROR_H */
