#ifndef TRACKER2nd_H
#define TRACKER2nd_H
int is_rested = 0;
#include <stdio.h>
#include <stdlib.h>
#include "malloc_prob.h"
static inline void track2_reset(void);
#define MEMVIZ_PATH_2ND "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_2nd_year.csv"
void file_delete();

void file_delete()
{
    FILE *filename = fopen("malloc_prob.h", "w");
    if (filename)
    {
        fprintf(filename, "#ifndef MALLOC_PROB_H\n#define MALLOC_PROB_H\n\n#define MALLOC_PROBABILITY 0.1\n#define ANALYZED_LINES 0\n\n#endif ");
        fclose(filename);
    }
}

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

    char start[100];
    snprintf(start, sizeof(start), "python MemProp.py test_2nd.c malloc_prob.h %d %d", line, line + 2);

    int pyt_result = system(start);
    if (pyt_result != 0)
    {
        fprintf(stderr, "Error executing Python script\n");
        return NULL;
    }
    if (MALLOC_PROBABILITY_ + {count} <= 0.0)
    {
        fprintf(stderr, "Prop <0.0 Nothing to do line %d\n", line);
        FILE *log = fopen(memviz_log_path_2nd(), "a");
        if (log)
        {
            fprintf(log, "false,%s,%d\n", file, line);
            fclose(log);
        }
        // file_delete();
    }
    else
    {
        void *ptr = malloc(size);
        if (ptr == NULL)
            return NULL;

        FILE *log = fopen(memviz_log_path_2nd(), "a");
        if (log)
        {
            fprintf(log, "ALLOC,%p,%zu,%s,%d\n", ptr, size, file, line);
            fclose(log);
        }
        // file_delete();
        return ptr;
    }
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