#ifndef TRACKER2nd_H
#define TRACKER2nd_H

#include <stdio.h>
#include <stdlib.h>

typedef struct MemoryNode
{
    void *address;
    size_t size;
    struct MemoryNode *next;
} MemoryNode;

//Node tracker to log mem allocation on 2nd .csv
static inline void *nodeTracker(size_t size,const char *file){
    MemoryNode *head = (MemoryNode *)malloc(sizeof(MemoryNode));

    if (head == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for MemoryNode\n");
        return NULL;
    }

    FILE *log = fopen("memory_log_2st_year.csv", "a");
    if(log){
        fprintf(log, "PTR,%p,%zu,%s\n", head, size, file);
        fclose(log);
    }

    MemoryNode *current = (MemoryNode *)malloc(sizeof(MemoryNode));
    if (current == NULL){
        fprintf(stderr, "Failed to allocate memory for MemoryNode\n");
        free(head);
        return NULL;
    }
    FILE *log2=fopen("memory_log_2st_year.csv","r+");
    if(log2){
        fseek(log2, 0, SEEK_END);
        fprintf(log2, "PTR2,%p,%zu,%s\n", current, size, file);
        fclose(log2);
    }
}
//change
#define malloc(size) nodeTracker(size, __FILE__, __LINE__)

#endif TRACKER2nd_H