/*
 * Simple implementation of bidirectional linked list
 */

#ifndef _LIST_H
#define _LIST_H
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

// src/lib/list.c: merge two list
extern list*list_merge(list*lst1,list*lst2);

// src/lib/list.c: duplicate list
extern list*list_duplicate(list*lst,list*end);

// src/lib/list.c: duplicate chars list
extern list*list_duplicate_chars(list*lst,list*end);

// src/lib/list.c: return true if there is only one item in the list
extern bool list_is_alone(list*point);

// src/lib/list.c: add item to an list
extern int list_obj_add(list**lst,list*item);

// src/lib/list.c: new and add item to an list
extern int list_obj_add_new(list**lst,void*data);

// src/lib/list.c: delete item from an list
extern int list_obj_del(list**lst,list*item,runnable_t*datafree);

// src/lib/list.c: default free runnable
extern int list_default_free(void*data);

// require not null
static inline list*list_new_notnull(void*data){if(!data)EPRET(EINVAL);return list_new(data);}
static inline int list_add_new_notnull(list*point,void*data){if(!data)ERET(EINVAL);return list_add_new(point,data);}
static inline int list_push_new_notnull(list*point,void*data){if(!data)ERET(EINVAL);return list_push_new(point,data);}
static inline int list_insert_new_notnull(list*point,void*data){if(!data)ERET(EINVAL);return list_insert_new(point,data);}
static inline int list_unshift_new_notnull(list*point,void*data){if(!data)ERET(EINVAL);return list_unshift_new(point,data);}
static inline int list_obj_add_new_notnull(list**lst,void*data){if(!data)ERET(EINVAL);return list_obj_add_new(lst,data);}

// duplicate and new
#define list_new_dup(data,len)                   list_new_notnull(memdup(data,len))
#define list_new_strdup(data)                    list_new_notnull(strdup(data))
#define list_new_strndup(data,len)               list_new_notnull(strndup(data,len))
#define list_add_new_dup(point,data,len)         list_add_new_notnull(point,memdup(data,len))
#define list_add_new_strdup(point,data)          list_add_new_notnull(point,strdup(data))
#define list_add_new_strndup(point,data,len)     list_add_new_notnull(point,strndup(data,len))
#define list_push_new_dup(point,data,len)        list_push_new_notnull(point,memdup(data,len))
#define list_push_new_strdup(point,data)         list_push_new_notnull(point,strdup(data))
#define list_push_new_strndup(point,data,len)    list_push_new_notnull(point,strndup(data,len))
#define list_insert_new_dup(point,data,len)      list_insert_new_notnull(point,memdup(data,len))
#define list_insert_new_strdup(point,data)       list_insert_new_notnull(point,strdup(data))
#define list_insert_new_strndup(point,data,len)  list_insert_new_notnull(point,strndup(data,len))
#define list_unshift_new_dup(point,data,len)     list_unshift_new_notnull(point,memdup(data,len))
#define list_unshift_new_strdup(point,data)      list_unshift_new_notnull(point,strdup(data))
#define list_unshift_new_strndup(point,data,len) list_unshift_new_notnull(point,strndup(data,len))
#define list_obj_add_new_dup(lst,data,len)     list_obj_add_new_notnull(lst,memdup(data,len))
#define list_obj_add_new_strdup(lst,data)      list_obj_add_new_notnull(lst,strdup(data))
#define list_obj_add_new_strndup(lst,data,len) list_obj_add_new_notnull(lst,strndup(data,len))

// use default free
#define list_free_item_def(point)list_free_item(point,list_default_free)
#define list_free_all_def(point)list_free_all(point,list_default_free)
#define list_remove_free_def(point)list_remove_free(point,list_default_free)

// get item data with type
#define LIST_DATA(_list,_type)((_type)((_list)->data))
#define LIST_DATA_DECLARE(_name,_list,_type) _type _name=((_type)((_list)->data))

#endif
