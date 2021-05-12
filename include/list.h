/*
 * Simple implementation of bidirectional linked list
 */

#ifndef _list_H
#define _list_H
#include"defines.h"
#include<stdbool.h>

// bidirectional linked list
struct list{
	struct list*prev;
	struct list*next;
	void*data;
};
typedef struct list list;

// src/lib/list.c: add new after point
extern int list_add(list*point,list*new);

// src/lib/list.c: new and add new after point
extern int list_add_new(list*point,void*data);

// src/lib/list.c: add new before point
extern int list_insert(list*point,list*new);

// src/lib/list.c: new and add new before point
extern int list_insert_new(list*point,void*data);

// src/lib/list.c: add items to the end of the list
extern int list_push(list*point,list*new);

// src/lib/list.c: new and add items to the end of the list
extern int list_push_new(list*point,void*data);

// src/lib/list.c: add items to the head of the list
extern int list_unshift(list*point,list*new);

// src/lib/list.c: new and add items to the head of the list
extern int list_unshift_new(list*point,void*data);

// src/lib/list.c: remove an item from list
extern int list_remove(list*point);

// src/lib/list.c: remove an item from list and free
extern int list_remove_free(list*point,runnable_t*datafree);

// src/lib/list.c: free a item
extern int list_free_item(list*point,runnable_t*datafree);

// src/lib/list.c: free all items in list
extern int list_free_all(list*point,runnable_t*datafree);

// src/lib/list.c: count items
extern int list_count(list*point);

// src/lib/list.c: new item with data
extern list*list_new(void*data);

// src/lib/list.c: seek to list end (last item)
extern list*list_last(list*point);

// src/lib/list.c: seek to list start (first item)
extern list*list_first(list*point);

// src/lib/list.c: return true if there is only one item in the list
extern bool list_is_alone(list*point);

// get item data with type
#define LIST_DATA(_list,_type)((_type)((_list)->data))

#endif
