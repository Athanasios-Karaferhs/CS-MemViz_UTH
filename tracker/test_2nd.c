#include <stdio.h>
#include <stdlib.h>
// last
#include "tracker2nd.h"

typedef struct Node
{
  int data;
  struct Node *next;
} Node;

typedef struct List
{
  Node *head;
  size_t length;
} List;

int main()
{

  List *my_list = (List *)malloc(sizeof(List));
  my_list->head = NULL;
  my_list->length = 0;

  Node *first_node = (Node *)malloc(sizeof(Node));
  first_node->data = 42;
  first_node->next = NULL;

  my_list->head = first_node;
  my_list->length = 1;

  printf("List created with length %zu and data %d\n", my_list->length, my_list->head->data);
  /*everyone from here on is currently been fixed. Do NOT use malloc without been needed in a linked list.(fix-idea:track if a pointer is currently seen the space you just malloced)*/
  int *x = malloc(20);

  printf("Malloc has been complited\n");
  //  free(x);
  // printf("Free has been complited\n");

  return 0;
}