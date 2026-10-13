#ifndef TRACKER2nd_H
#define TRACKER2nd_H
int is_rested = 0;
#include <stdio.h>
#include <stdlib.h>
static inline void track2_reset(void);
#define MEMVIZ_PATH_2ND "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_2nd_year.csv"

static inline const char *memviz_log_path_2nd(void)
{
    static char buf[512];
    const char *dir = getenv("USERPROFILE");

    if (dir != NULL)
        snprintf(buf, sizeof(buf), "%s%s", dir, MEMVIZ_PATH_2ND);
    else
        snprintf(buf, sizeof(buf), "%s", MEMVIZ_PATH_2ND);
    return buf;
}

static inline void track2_reset(void)
{
    FILE *log = fopen(memviz_log_path_2nd(), "w");
    if (log)
        fclose(log);
}

static inline void *track_allocation(size_t size, const char *file, int line)
{

    if (!is_rested)
    {
        track2_reset();
        is_rested = 1;
    }

    void *ptr = malloc(size);
    if (ptr == NULL)
        return NULL;

    FILE *log = fopen(memviz_log_path_2nd(), "a");
    if (log)
    {
        fprintf(log, "ALLOC,%p,%zu,%s,%d\n", ptr, size, file, line);
        fclose(log);
    }

    return ptr;
}

static inline void track_free(void *ptr, const char *file, int line)
{
    if (!is_rested)
    {
        track2_reset();
        is_rested = 1;
    }

    FILE *log = fopen(memviz_log_path_2nd(), "a");
    if (log)
    {
        if (ptr != NULL)
            fprintf(log, "FREE,%p,0,%s,%d\n", ptr, file, line);
        fclose(log);
    }
    free(ptr);
}

#define malloc(size) track_allocation(size, __FILE__, __LINE__)
#define free(ptr) track_free(ptr, __FILE__, __LINE__)

#endif