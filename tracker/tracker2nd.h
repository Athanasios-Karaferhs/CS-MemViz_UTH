#ifndef TRACKER2nd_H
#define TRACKER2nd_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMVIZ_PATH_2ND "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_2nd_year.csv"
#define PROB_CSV "malloc_prob.csv"

static int is_rested = 0;

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

    remove(PROB_CSV); /* fresh probability file for each run */
}

/* returns 1 = pass, 0 = skip, -1 = error; reads the LAST row Python wrote */
static inline int read_last_decision(double *prob)
{
    FILE *f = fopen(PROB_CSV, "r");
    if (!f)
        return -1;

    char buf[256], last[256] = "";
    while (fgets(buf, sizeof(buf), f))
        if (buf[0] >= '0' && buf[0] <= '9') /* skip the header row */
            strcpy(last, buf);
    fclose(f);

    int s, e, passed;
    double p;
    if (sscanf(last, "%d,%d,%lf,%d", &s, &e, &p, &passed) != 4)
        return -1;

    *prob = p;
    return passed;
}

static inline void *track_allocation(size_t size, const char *file, int line)
{
    if (!is_rested)
    {
        track2_reset();
        is_rested = 1;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "python MemProp.py \"%s\" %d %d", file, line, line + 2);
    if (system(cmd) != 0)
    {
        fprintf(stderr, "Error executing Python script\n");
        return NULL;
    }

    double prob = 0.0;
    int passed = read_last_decision(&prob);
    if (passed < 0)
    {
        fprintf(stderr, "Could not read %s\n", PROB_CSV);
        return NULL;
    }

    void *ptr = malloc(size); /* the program must keep working either way */
    if (ptr == NULL)
        return NULL;

    FILE *log = fopen(memviz_log_path_2nd(), "a");
    if (log)
    {
        if (passed)
            fprintf(log, "ALLOC,%p,%zu,%s,%d\n", ptr, size, file, line);
        else
            fprintf(log, "SKIP,%p,%zu,%s,%d\n", ptr, size, file, line);
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