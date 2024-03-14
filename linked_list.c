#include "linked_list.h"
#include <assert.h>

void List_init(ListHead* head) {
    head->first = 0;
    head->last = 0;
    head->size = 0;
}

/* findListItem */
ListItem* List_find(ListHead* head, ListItem* item) {
    ListItem* aux = head->first;
    while (aux) {
        if (aux == item) return item;
        aux = aux->next;
    }
    return 0;
}

/* insertListItem */
ListItem* List_insert(ListHead* head, ListItem* prev, ListItem* item) {
    if (item->next || item->prev) return 0;
#ifdef _LIST_DEBUG_
    assert(!List_find(head, item));             /* check that item is not in the list */
    if (prev) assert(List_find(head, prev));    /* check that prev is in the list */
#endif
    ListItem* next = prev ? prev->next : head->first;
    if (prev) { item->prev = prev; prev->next = item; }
    if (next) { item->next = next; next->prev = item; }
    if (!prev) { head->first = item; }
    if (!next) { head->last = item; }
    ++head->size;
    return item;
}

/* getListItem */
ListItem* List_detach(ListHead* head, ListItem* item) {
#ifdef _LIST_DEBUG_
    assert(List_find(head, item));   /* check that item is in the list */
#endif
    ListItem* prev = item->prev;
    ListItem* next = item->next;
    if (prev) { prev->next = next; }
    if (next) { next->prev = prev; }
    if (item == head->first) { head->first = next; }
    if (item == head->last) { head->last = prev; }
    head->size--;
    item->next = item->prev = 0;
    return item;
}

/* append */
ListItem* List_pushBack(ListHead* head, ListItem* item) {
    return List_insert(head, head->last, item);
}

/* prepend */
ListItem* List_pushFront(ListHead* head, ListItem* item) {
    return List_insert(head, 0, item);
}

/* getListHead */
ListItem* List_popFront(ListHead* head) {
    return List_detach(head, head->first);
}
