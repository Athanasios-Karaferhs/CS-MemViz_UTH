#include <stdio.h>
// Other standard includes go here
#include <stdlib.h>
// Tracker MUST be the last include(to the student)
#include "tracker1st.h"

int main()
{
    remove("C:/Users/thano/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv");

    printf("Allocating memory...\n");

    // This will be logged as an ALLOC at line 13
    int *array_leaked = (int *)malloc(5 * sizeof(int));
  
    // This will be logged as an ALLOC at line 16, and FREE at line 18
    int *array_safe = (int *)malloc(5 * sizeof(int));

    free(array_safe);

 
    int *p;
    *p =40;
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