//
// Created by Süleyman ÇİÇEK <suleyman@cicek.pw> on 2/2/26.
//

#include "linked_list.h"

#include <stdlib.h>
#include <string.h>

linked_list_t *ll_append(linked_list_t *list, void *ptr) {
        if (list == NULL) {
                list = (linked_list_t *) malloc(sizeof(linked_list_t));
                memset(list, 0, sizeof(linked_list_t));

                list->ptr = ptr;

                return list;
        }

        list->next = ll_append(list->next, ptr);

        return list;
}

linked_list_t *ll_remove(linked_list_t *list, void *ptr) {
        linked_list_t *prev[] = { NULL };

        linked_list_t *item = ll_search(list, ptr, prev);

        if (item != NULL) {
                if (*prev == NULL) list = item->next;
                else (*prev)->next = item->next;
                free(item);
        }

        return list;
}

linked_list_t *ll_search(linked_list_t *list, void *ptr, linked_list_t **prev) {
        if (list == NULL)
                return NULL;

        if (list->ptr == ptr) return list;

        if (prev != NULL) *prev = list;
        return ll_search(list->next, ptr, prev);
}
