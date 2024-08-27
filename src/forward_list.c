
#include <stdio.h>
#include <stdlib.h>

#include "forward_list.h"

Node *node_create(data_type value, Node *next)
{
    Node *n = (Node *)malloc(sizeof(Node));
    n->value = value;
    n->next = next;
    return n;
}

void node_destroy(Node *n)
{
    free(n);
}

ForwardList *forward_list_create()
{
    ForwardList *l = (ForwardList *)calloc(1, sizeof(ForwardList));

    l->head = NULL;
    l->last = NULL;
    l->size = 0;

    return l;
}

int forward_list_size(ForwardList *l)
{
    return l->size;
}

data_type forward_list_find(ForwardList *l, void *key, int (*cmp_fn)(data_type data, void *key))
{
    Node *n = l->head;

    while (n != NULL)
    {
        if (!cmp_fn(n->value, key))
            return n->value;

        n = n->next;
    }

    return NULL;
}

void forward_list_push_front(ForwardList *l, data_type data)
{
    l->head = node_create(data, l->head);
    l->size++;

    if (l->size == 1)
        l->last = l->head;
}

void forward_list_push_back(ForwardList *l, data_type data)
{
    Node *new_node = node_create(data, NULL);

    if (l->last == NULL)
        l->head = l->last = new_node;
    else
        l->last = l->last->next = new_node;

    l->size++;
}

void forward_list_print(ForwardList *l, void (*print_fn)(data_type data))
{
    Node *n = l->head;

    printf("[");

    while (n != NULL)
    {
        print_fn(n->value);
        n = n->next;

        if (n != NULL)
            printf(", ");
    }

    printf("]");
}

data_type forward_list_get(ForwardList *l, int i)
{
    if (i < 0 || i >= l->size)
        exit(printf("Error: forward_list_get(): index out of bounds."));

    Node *n = l->head;
    for (int j = 0; j < i; j++)
        n = n->next;

    return n->value;
}

data_type forward_list_pop_front(ForwardList *l)
{
    if (l->head == NULL)
        exit(printf("Error: forward_list_pop_front(): list is empty."));

    Node *n = l->head;
    l->head = l->head->next;
    data_type data = n->value;
    node_destroy(n);
    l->size--;

    if (l->size <= 1)
        l->last = l->head;

    return data;
}

void forward_list_destroy(ForwardList *l)
{
    Node *n = l->head;
    while (n != NULL)
    {
        Node *next = n->next;
        node_destroy(n);
        n = next;
    }

    free(l);
}