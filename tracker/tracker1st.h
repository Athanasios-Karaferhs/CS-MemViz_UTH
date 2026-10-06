#ifndef TRACKER1st_H
#define TRACKER1st_H

#include <stdio.h>
#include <stdlib.h>

// 1. The custom wrapper functions.We use 'static inline' so this header can be included in multiple .c files without causing linker errors.
static inline void *track_malloc(size_t size, const char *file, int line)
{
    void *ptr = malloc(size); // Execute the real malloc

    FILE *log = fopen("C:/Users/thano/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv", "a");
    if (log)
    {
        // Log format: EVENT, ADDRESS, SIZE, FILENAME, LINE_NUMBER
        fprintf(log, "ALLOC,%p,%zu,%s,%d\n", ptr, size, file, line);
        fclose(log);
    }
    return ptr;
}

static void track_free(void *ptr, const char *file, int line)
{
    FILE *log = fopen("C:/Users/thano/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv", "a");
    if (log && ptr != NULL)
    {
        fprintf(log, "FREE,%p,0,%s,%d\n", ptr, file, line);
        fclose(log);
    }

    free(ptr); // Execute the real free
}

//The Macro HijackThis intercepts the student's code and injects __FILE__ and __LINE__.
#define malloc(size) track_malloc(size, __FILE__, __LINE__)
#define free(ptr) track_free(ptr, __FILE__, __LINE__)

#endif TRACKER1st_H