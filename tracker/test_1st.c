#include <stdio.h>
#include <stdlib.h>
// Tracker MUST be the last include(to the student)
#include "tracker1st.h"

#define path "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv"

int main()
{
    // remove is not mandatory since the logs on the files are automaticly cleared. For testing tho I would recommend keeping it in
    remove(full_path());

    printf("Allocating memory...\n");

    // This will be logged as an ALLOC at line 13
    int *array_leaked = (int *)malloc(5 * sizeof(int));

    // This will be logged as an ALLOC at line 16, and FREE at line 18
    int *array_safe = (int *)malloc(5 * sizeof(int));

    free(array_safe);

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