/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

/*
 * Simple implementation of bidirectional linked list
 */

#ifndef _LIST_H
#define _LIST_H
#include"defines.h"
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

// bidirectional linked list
struct list{
	struct list*prev;
	struct list*next;
	void*data;
};
typedef struct list list;

typedef bool(*list_sorter)(list*f1,list*f2);
typedef bool(*list_comparator)(list*f,void*data);

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

// src/lib/list.c: free an item
extern int list_free_item(list*point,runnable_t*datafree);

// src/lib/list.c: free all items in list
extern int list_free_all(list*point,runnable_t*datafree);

// src/lib/list.c: count items
extern int list_count(list*point);

// src/lib/list.c: swap item and prev item
extern int list_swap_prev(list*point);

// src/lib/list.c: swap item and next item
extern int list_swap_next(list*point);

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

// src/lib/list.c: lookup list by data
extern list*list_lookup_data(list*lst,void*data);

// src/lib/list.c: return true if there is only one item in the list
extern bool list_is_alone(list*point);

// src/lib/list.c: add item to a list
extern int list_obj_add(list**lst,list*item);

// src/lib/list.c: new and add item to a list
extern int list_obj_add_new(list**lst,void*data);

// src/lib/list.c: strip item from a list
extern int list_obj_strip(list**lst,list*item);

// src/lib/list.c: delete item from a list
extern int list_obj_del(list**lst,list*item,runnable_t*datafree);

// src/lib/list.c: lookup item and delete from a list
extern int list_obj_del_data(list**lst,void*data,runnable_t*datafree);

// src/lib/list.c: sort a list
extern int list_sort(list*lst,list_sorter sorter);

// src/lib/list.c: search a list object
extern list*list_search_one(list*lst,list_comparator comparator,void*data);

// src/lib/list.c: compare pointer
extern bool list_pointer_comparator(list*f,void*v);

// src/lib/list.c: compare string
extern bool list_string_comparator(list*f,void*v);

// src/lib/list.c: compare string without case
extern bool list_string_case_comparator(list*f,void*v);

// src/lib/list.c: sort string
extern bool list_string_sorter(list*l1,list*l2);

// src/lib/list.c: search a string
extern list*list_search_string(list*lst,const char*str);

// src/lib/list.c: search a string ignore case
extern list*list_search_case_string(list*lst,const char*str);

// src/lib/list.c: default free runnable
extern int list_default_free(void*data);

// src/lib/list.c: reverse a list
extern int list_reverse(list*lst);

// src/lib/list.c: convert list to string
extern char*list_string_append(list*lst,char*buff,size_t len,char*sep);

// require not null
extern void*_memdup(void*mem,size_t len);
#define memdup _memdup
#define _IN static inline
#define _NOTNULL(data,ret,rx) {if(!data){rx(EINVAL);}else{return ret;}}
#define _NF_NOTNULL(data,ret,rx) {if(!data){rx(EINVAL);}else{int i=ret;if(i<00)free(data);return i;}}
#define _PF_NOTNULL(data,ret,rx) {if(!data){rx(EINVAL);}else{void*i=ret;if(!i)free(data);return i;}}
#define _N_NOTNULL(data,ret) _NOTNULL(data,ret,ERET)
#define _P_NOTNULL(data,ret) _NOTNULL(data,ret,EPRET)
#define _DECLARE_NOT_NULL(_name,_base,_ret,_notnull,...)_IN _ret _name(__VA_ARGS__ void*data)_notnull(data,_base)
#define _DECLARE_N_NOT_NULL(_name,_base,...)_DECLARE_NOT_NULL(_name,_base,int,_N_NOTNULL,__VA_ARGS__)
#define _DECLARE_NX_NOT_NULL(_name,_base)_DECLARE_N_NOT_NULL(_name,_base(point,data),list*point,)
_DECLARE_NX_NOT_NULL(list_add_new_notnull,list_add_new)
_DECLARE_NX_NOT_NULL(list_push_new_notnull,list_push_new)
_DECLARE_NX_NOT_NULL(list_insert_new_notnull,list_insert_new)
_DECLARE_NX_NOT_NULL(list_unshift_new_notnull,list_unshift_new)
_DECLARE_N_NOT_NULL(list_obj_add_new_notnull,list_obj_add_new(point,data),list**point,)
_DECLARE_NOT_NULL(list_new_notnull,list_new(data),list*,_P_NOTNULL,)

// duplicate and new
#define _DECLARE_DUP(_name,_base,_ret,_dups,_arg,_if,...)_IN _ret _name(__VA_ARGS__){void*dup=(void*)_dups;_ret ret=_base _arg;if(_if)free(dup);return ret;}
#define _DECLARE_X_DUP(_name,_ret,_arg,_ifs,...) \
	_DECLARE_DUP(_name##_dup,_name##_notnull,_ret,memdup(data,len),_arg,_ifs,__VA_ARGS__ void*data,size_t len)\
	_DECLARE_DUP(_name##_strdup,_name##_notnull,_ret,strdup(data),_arg,_ifs,__VA_ARGS__ const char*data)\
	_DECLARE_DUP(_name##_strndup,_name##_notnull,_ret,strndup(data,len),_arg,_ifs,__VA_ARGS__ const char*data,size_t len)
#define _DECLARE_PX_DUP(_name) _DECLARE_X_DUP(_name,int,(point,dup),ret<0,list*point,)
_DECLARE_X_DUP(list_new,list*,(dup),!ret)
_DECLARE_PX_DUP(list_add_new)
_DECLARE_PX_DUP(list_push_new)
_DECLARE_PX_DUP(list_insert_new)
_DECLARE_PX_DUP(list_unshift_new)
_DECLARE_X_DUP(list_obj_add_new,int,(point,dup),ret<0,list**point,)

// use default free
#define list_free_item_def(point)list_free_item(point,list_default_free)
#define list_free_all_def(point)list_free_all(point,list_default_free)
#define list_remove_free_def(point)list_remove_free(point,list_default_free)

// get item data with type
#define LIST_DATA(_list,_type)((_type)((_list)->data))
#define LIST_DATA_DECLARE(_name,_list,_type) _type _name=((_type)((_list)->data))

#endif
