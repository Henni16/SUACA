#include <stdio.h>
#include "iacaMarks.h"

int main()
{
    IACA_START
    int a = 3-7;
    int b = 4;
    int c = a+b;
    int d = c-a;
    IACA_END
    printf("d: %i.\n", d);
    return 0;
}
