#ifndef TRACKER1st_H
#define TRACKER1st_H

#ifdef TRACKER2nd_H
#error "Include only one tracker header: tracker1st.h and tracker2nd.h both redefine malloc."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#ifndef full_path
#define full_path  "C:/Users/thano/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv"
#endif
*/

#define path "/CS-MemViz/source/repos/CS_MemViz/CS_MemViz/memory_log_1st_year.csv"

static inline const char *full_path()
{

    static char full_path[512];

    const char *dir = getenv("USERPROFILE");

    if (dir != NULL)
    {
        snprintf(full_path, sizeof(full_path), "%s%s", dir, path);
    }
    else
    {
        snprintf(full_path, sizeof(full_path), "%s", path);
    }
    return full_path;
}

static inline void track_reset()
{
    FILE *log = fopen(full_path(), "w");
    if (log)
        fclose(log);
}

static inline void *track_malloc(size_t size, const char *file, int line)
{
    void *ptr = malloc(size);

    FILE *log = fopen(full_path(), "a");
    if (log)
    {
        /* ALLOC,address,size,file,line */
        fprintf(log, "ALLOC,%p,%zu,%s,%d\n", ptr, size, file, line);
        fclose(log);
    }
    return ptr;
}

static inline void track_free(void *ptr, const char *file, int line)
{
    FILE *log = fopen(full_path(), "a");
    if (log)
    {
        if (ptr != NULL)
            fprintf(log, "FREE,%p,0,%s,%d\n", ptr, file, line);
        fclose(log);
    }
    free(ptr);
}

//  One snapshot of a pointer variable: PTR,name,address_of_pointer_variable,address_it_points_to,value_there,size,file,line
static inline void track_ptr(const char *name, int **ptr_var,
                             size_t size, const char *file, int line)
{
    FILE *log = fopen(full_path(), "a");
    if (!log)
        return;

    int *target = *ptr_var;

    if (target != NULL)
    {
        fprintf(log, "PTR,%s,%p,%p,%d,%zu,%s,%d\n",
                name, (void *)ptr_var, (void *)target, *target, size, file, line);
    }
    else
    {
        fprintf(log, "PTR,%s,%p,NULL,-,%zu,%s,%d\n",
                name, (void *)ptr_var, size, file, line);
    }

    fclose(log);
}

/* These macros intercept the student's code and inject __FILE__ and __LINE__
   They must stay after the functions above (which use the real malloc/free) */
#define malloc(size) track_malloc(size, __FILE__, __LINE__)
#define free(ptr) track_free(ptr, __FILE__, __LINE__)
#define TRACK_PTR(p) track_ptr(#p, &(p), sizeof(*(p)), __FILE__, __LINE__)

#endif /* TRACKER1st_H */