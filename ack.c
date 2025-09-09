/* ack.c -- Program to calculate Ackerman function.
 * Author: Edward Rivas <rivastkw@gmail.com>
 *       y Luis Colorado <luiscoloradourcola@gmail.com>
 * Date: Wed Mar 19 11:43:01 EET 2025
 * Copyright: (c) 2025 Luis Colorado y Edward Rivas.  All rights reserved.
 * License: BSD
 */

#include <stdio.h>

unsigned long n_calls;

unsigned long ack(unsigned long a, unsigned long b)
{
    n_calls++;
    if (a == 0) return b+1;
    if (b == 0) return ack(a-1, 1);
    return ack(a-1, ack(a, b-1));
}

char line[256];

int main()
{
    unsigned long a = 0, b = 0;

    while (fgets(line, sizeof line, stdin)) {
        sscanf(line, "%lu%lu", &a, &b);

        n_calls = 0UL;
        printf("ack(%lu, %lu) -> %lu\n",
               a, b, ack(a, b));
        printf("n_calls = %lu\n",
               n_calls);
    }
}
