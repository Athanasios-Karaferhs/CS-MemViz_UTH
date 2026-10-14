#ifndef TRACKER1st_H
#define TRACKER1st_H
static inline void track_reset();
int is_rested = 0; // check if the log file has been reset (when we for example we use malloc (on the second file) 2 times in a single file)
#include <stdio.h>
#include <stdlib.h>
#define path "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv"

static inline const char *full_path()
{

    static char full_path[512];

    const char *dir = getenv("USERPROFILE");

    if (dir != NULL)
        snprintf(full_path, sizeof(full_path), "%s%s", dir, path);
    else
        snprintf(full_path, sizeof(full_path), "%s", path);
    return full_path;
}

static inline void track_reset()
{
    FILE *log = fopen(full_path(), "w");
    if (log)
        fclose(log);
}

static inline void track_ptr(const char *name, int **ptr_var, size_t size, const char *file, int line)
{

    if (!is_rested)
    {
        track_reset();
        is_rested = 1;
    }
    FILE *log = fopen(full_path(), "a");
    if (!log)
        return;

    int *target = *ptr_var; // what ptr_var holds

    if (target != NULL)
    {
        fprintf(log, "PTR,%s,%p,%p,%d,%zu,%s,%d\n", name, (void *)ptr_var, (void *)target, *target, size, file, line); // SOS: 2 takes (void *)ptr_var, the address of the pointer variable.3 takes (void *)target, the address it points to .4 takes *target, the value there.
    }
    else
    {
        fprintf(log, "PTR,%s,%p,NULL,-,%zu,%s,%d\n", name, (void *)ptr_var, size, file, line);
    }

    fclose(log);
}

// These macros intercept the student's code and inject __FILE__ and __LINE__
#define TRACK_PTR(p) track_ptr(#p, &(p), sizeof(*(p)), __FILE__, __LINE__)

#endif /* TRACKER1st_H */