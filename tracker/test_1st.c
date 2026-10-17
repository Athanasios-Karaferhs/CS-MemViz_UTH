#include <stdio.h>
// Other standard includes go here
#include <stdlib.h>
// Tracker MUST be the last include(to the student)
#include "tracker1st.h"

int main()
{
    int *p;
    *p = 40;
    TRACK_PTR(p);

    *p = *p + 8;
    TRACK_PTR(p);

    p = NULL;
    TRACK_PTR(p);

    int x = 10;
    int *q = &x;
    TRACK_PTR(q);

    printf("Done. Check memory_log.csv!\n");

    return 0;
}