#include <stdio.h>
#include "TRACKER2nd.H"

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

int main(void)
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

  return 0;
}