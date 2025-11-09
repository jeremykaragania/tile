#include <kernel/list.h>

/*
  list_init initializes the list head "head".
*/
void list_init(struct list_link* head) {
  head->next = head;
  head->prev = head;
}

/*
  list_push links "link" to the the list head "head".
*/
void list_push(struct list_link* head, struct list_link* link) {
  link->next = head->next;
  link->prev = head;
  head->next->prev = link;
  head->next = link;
}

/*
  list_pop removes the first link after "head" and returns it.
*/
struct list_link* list_pop(struct list_link* head) {
  struct list_link* ret;

  ret = head->next;

  if (list_remove(head, ret) == 0) {
    return ret;
  }

  return NULL;
}

/*
  list_remove removes the link "link" from the list with head "head".
*/
int list_remove(struct list_link* head, struct list_link* link) {
  if (head == link) {
    return -1;
  }

  link->next->prev = link->prev;
  link->prev->next = link->next;

  return 0;
}
