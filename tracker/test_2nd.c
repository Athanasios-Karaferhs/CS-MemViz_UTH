#include <stdio.h>
#include <stdlib.h>
//last
#include "tracker2nd.h"

int main()
{
    // Clear the old log file for fresh testing
    remove("memory_log_2st_year.csv");

    printf("Allocating memory...\n");

    // This will be logged as an ALLOC at line 13
    int *array_leaked = (int *)nodeTracker(10 * sizeof(int), __FILE__);

    // This will be logged as an ALLOC at line 16, and FREE at line 18
    int *array_safe = (int *)nodeTracker(5 * sizeof(int), __FILE__);

  //  free(array_safe);

    // We intentionally "forget" to free array_leaked
    printf("Done. Check memory_log.csv!\n");

    return 0;
}