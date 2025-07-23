#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/*
  list_data returns the data which the list link "link" links give its type
  "type" and member name "member".
*/
#define list_data(link, type, member) ((type*)((uint32_t)(link) - (offsetof(type, member))))

/*
 struct list_link links arbitrary data in a circular doubly linked list without
 needing to know anything of the underlying data.
*/
struct list_link {
  struct list_link* next;
  struct list_link* prev;
};

void list_init(struct list_link* head);
void list_push(struct list_link* head, struct list_link* link);
struct list_link* list_pop(struct list_link* head);
int list_remove(struct list_link* head, struct list_link* link);

#endif
