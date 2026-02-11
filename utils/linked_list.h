//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/2/26.
//

#ifndef COMMON_IO_BASIC_LINKED_LIST_H
#define COMMON_IO_BASIC_LINKED_LIST_H

typedef struct __attribute__((packed)) linked_list_s {
        void *ptr;
        struct linked_list_s *next;
} linked_list_t;

linked_list_t *ll_append(linked_list_t *list, void *ptr);
linked_list_t *ll_remove(linked_list_t *list, void *ptr);
linked_list_t *ll_search(linked_list_t *list, void *ptr, linked_list_t **prev);

#endif //COMMON_IO_BASIC_LINKED_LIST_H