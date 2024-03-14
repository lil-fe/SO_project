#pragma once

typedef struct ListItem {
    struct ListItem* prev;
    struct ListItem* next;
} ListItem;

typedef struct ListHead {
    ListItem* first;
    ListItem* last;
    int size;
} ListHead;

void List_init(ListHead*);
ListItem* List_find(ListHead*, ListItem*);
ListItem* List_insert(ListHead*, ListItem* prev, ListItem* item);
ListItem* List_detach(ListHead*, ListItem*);
ListItem* List_pushBack(ListHead*, ListItem*);
ListItem* List_pushFront(ListHead*, ListItem*);
ListItem* List_popFront(ListHead*);
