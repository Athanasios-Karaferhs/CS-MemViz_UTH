#ifndef TRACKER2nd_H
#define TRACKER2nd_H

#ifdef TRACKER1st_H
#error "Include only one tracker header: tracker1st.h and tracker2nd.h both redefine malloc."
#endif

#include <stdio.h>
#include <stdlib.h>

#define path "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_2st_year.csv"

static inline const char *full_path(void)
{
    static char full_path[512]; // static to let the OS claim its memory back

    const char *dir = getenv("USERPROFILE");
    if (dir == NULL)
    {
        printf("Error trying to find the users profile");
        snprintf(full_path, sizeof(full_path), ".%s", path);
        return full_path;
    }

    snprintf(full_path, sizeof(full_path), "%s%s", dir, path); // expects two string arguments
    return full_path;
}

// Node tracker to log mem allocation on 2nd .csv
static inline void *track_allocation(size_t size, const char *file, int line)
{
    void *ptr = malloc(size); // Allocates exact memory requested for struct
    if (ptr == NULL)
        return NULL;

    FILE *log = fopen(full_path(), "a");
    if (log)
    {
        // Logs allocation: PTR,address,size,file,line
        fprintf(log, "PTR,%p,%zu,%s,%d\n", ptr, size, file, line);
        fclose(log);
    }

    return ptr;
}

// macro
#define malloc(size) track_allocation(size, __FILE__, __LINE__)

#endif