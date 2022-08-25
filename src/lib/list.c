/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#define _GNU_SOURCE
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include"str.h"
#include"list.h"

list*list_new(void*data){
	errno=ENOMEM;
	struct list*list=malloc(sizeof(struct list));
	if(!list)return NULL;
	errno=0;
	memset(list,0,sizeof(struct list));
	list->data=data;
	return list;
}

int list_add(list*point,list*new){
	errno=0;
	if(!point||!new)ERET(EINVAL);
	if(point==new)ERET(ELOOP);
	if(!point->next)point->next=new,new->prev=point;
	else{
		list*f,*l;
		if(!(f=list_first(new)))return -errno;
		if(!(l=list_last(new)))return -errno;
		point->next->prev=l;
		l->next=point->next;
		point->next=f;
		f->prev=point;
	}
	return (errno=0);
}

int list_add_new(list*point,void*data){
	list*new=list_new(data);
	if(!new)return -errno;
	int r=list_add(point,new);
	if(r<0)free(new);
	return r;
}

int list_insert(list*point,list*new){
	errno=0;
	list*f,*l;
	if(!point||!new)ERET(EINVAL);
	if(!(l=list_last(new)))return -errno;
	if(!point->prev)point->prev=l,l->next=point;
	else{
		if(!(f=list_first(new)))return -errno;
		point->prev->next=l;
		l->prev=point->prev;
		point->prev=f;
		f->next=point;
	}
	ERET(0);
}

int list_insert_new(list*point,void*data){
	list*new=list_new(data);
	if(!new)return -errno;
	int r=list_insert(point,new);
	if(r<0)free(new);
	return r;
}

int list_push(list*point,list*new){
	return list_add(list_last(point),new);
}

int list_push_new(list*point,void*data){
	return list_add_new(list_last(point),data);
}

int list_unshift(list*point,list*new){
	return list_insert(list_first(point),new);
}

int list_unshift_new(list*point,void*data){
	return list_insert_new(list_first(point),data);
}

int list_count(list*point){
	errno=0;
	if(!point)ERET(EINVAL);
	int t=1;
	list*cur=list_first(point);
	if(!cur)return -errno;
	while((cur=cur->next)&&cur!=point)t++;
	return t;
}

static int list_swap_neighbor(list*p1,list*p2){
	errno=0;
	if(!p1||!p2)ERET(EINVAL);
	if(p1->prev)p1->prev->next=p2;
	if(p2->next)p2->next->prev=p1;
	p1->next=p2->next,p2->prev=p1->prev;
	p1->prev=p2,p2->next=p1;
	return 0;
}

int list_swap_prev(list*point){
	errno=0;
	if(!point)ERET(EINVAL);
	if(!point->prev)ERET(ENOENT);
	return list_swap_neighbor(point->prev,point);
}

int list_swap_next(list*point){
	errno=0;
	if(!point)ERET(EINVAL);
	if(!point->next)ERET(ENOENT);
	return list_swap_neighbor(point,point->next);
}

list*list_last(list*point){
	errno=0;
	if(!point)EPRET(EINVAL);
	list*cur=point->next;
	if(!cur)return point;
	while(cur){
		if(cur==point)EPRET(ELOOP)
		if(cur->next)cur=cur->next;
		else break;
	}
	return cur;
}

list*list_duplicate(list*lst,list*end){
	if(!lst)EPRET(EINVAL);
	list*x,*r;
	if(
		!(x=list_first(lst))||
		!(r=list_new(x->data))
	)return NULL;
	while((x=x->next)&&x!=end){
		if(list_push_new(r,x->data)<0){
			list_free_all(r,NULL);
			return NULL;
		}
	}
	return r;
}

list*list_duplicate_chars(list*lst,list*end){
	if(!lst)EPRET(EINVAL);
	list*x,*r;
	if(
		!(x=list_first(lst))||
		!(r=list_new_strdup(x->data))
	)return NULL;
	while((x=x->next)&&x!=end){
		if(list_push_new_strdup(r,x->data)<0){
			list_free_all(r,NULL);
			return NULL;
		}
	}
	return r;
}

list*list_first(list*point){
	errno=0;
	if(!point)EPRET(EINVAL);
	list*cur=point->prev;
	if(!cur)return point;
	while(cur){
		if(cur==point)EPRET(ELOOP)
		if(cur->prev)cur=cur->prev;
		else break;
	}
	return cur;
}

list*list_merge(list*lst1,list*lst2){
	if(!lst1||!lst2)EPRET(EINVAL);
	list*l1=list_last(lst1),*l2=list_first(lst2);
	if(!l1||!l2||list_add(l1,l2)!=0)return NULL;
	return lst1;
}

int list_remove(list*point){
	errno=0;
	if(!point)ERET(EINVAL);
	if(point->next&&point->prev){
		point->prev->next=point->next;
		point->next->prev=point->prev;
	}else if(point->next&&!point->prev)point->next->prev=NULL;
	else if(!point->next&&point->prev)point->prev->next=NULL;
	else ERET(ENOENT);
	point->prev=NULL;
	point->next=NULL;
	return 0;
}

int list_default_free(void*data){
	if(data)free(data);
	return 0;
}

int list_free_item(list*point,runnable_t*datafree){
	errno=0;
	if(!point)ERET(EINVAL);
	if(point->prev)point->prev->next=NULL;
	if(point->next)point->next->prev=NULL;
	if(point->data&&datafree)datafree(point->data);
	point->data=NULL;
	free(point);
	return 0;
}

int list_free_all(list*point,runnable_t*datafree){
	errno=0;
	if(!point)ERET(EINVAL);
	list*cur=list_first(point),*next;
	if(!cur)cur=point;
	do{
		next=cur->next;
		list_free_item(cur,datafree);
	}while((cur=next));
	return 0;
}

int list_remove_free(list*point,runnable_t*datafree){
	return list_remove(point)<0?-errno:list_free_item(point,datafree);
}

bool list_is_alone(list*point){
	return point&&!point->prev&&!point->next;
}

int list_obj_add(list**lst,list*item){
	if(!item||!lst)ERET(EINVAL);
	if(!*lst)*lst=item;
	else if(list_push(*lst,item)!=0)return -errno;
	return 0;
}

int list_obj_add_new(list**lst,void*data){
	if(!lst)ERET(EINVAL);
	list*item=list_new(data);
	if(!item)return -errno;
	int r=list_obj_add(lst,item);
	if(r<0)list_free_item(item,NULL);
	return r;
}

int list_obj_strip(list**lst,list*item){
	if(!item||!lst)ERET(EINVAL);
	if(list_is_alone(*lst)){
		if(*lst!=item)return -1;
		*lst=NULL;
	}else if(*lst==item){
		if(item->prev)*lst=item->prev;
		if(item->next)*lst=item->next;
	}
	list_remove(item);
	return 0;
}

int list_obj_del(list**lst,list*item,runnable_t*datafree){
	if(!item||!lst)ERET(EINVAL);
	if(list_obj_strip(lst,item)!=0)return -1;
	list_free_item(item,datafree);
	return 0;
}

list*list_lookup_data(list*lst,void*data){
	if(!lst||!data)EPRET(EINVAL);
	list*cur=list_first(lst);
	if(cur)while(cur->data!=data&&(cur=cur->next));
	if(!cur||cur->data!=data)EPRET(ENOENT);
	return cur;
}

int list_obj_del_data(list**lst,void*data,runnable_t*datafree){
	if(!lst)ERET(EINVAL);
	list*item=list_lookup_data(*lst,data);
	return item?list_obj_del(lst,item,datafree):-errno;
}

int list_sort(list*lst,list_sorter sorter){
	if(!lst||!sorter)ERET(EINVAL);
	int r=0;
	list*f;
	bool changed=false;
	do{
		changed=false;
		if(!(f=list_first(lst)))continue;
		do{
			if(!f->next)continue;
			if(!sorter(f,f->next))continue;
			list_swap_neighbor(f,f->next);
			changed=true;
		}while((f=f->next));
		r++;
	}while(changed);
	return r;
}

list*list_search_one(list*lst,list_comparator comparator,void*data){
	if(!lst||!comparator)EPRET(EINVAL);
	list*f;
	if((f=list_first(lst)))do{
		if(f&&comparator(f,data))return f;
	}while((f=f->next));
	return NULL;
}

bool list_pointer_comparator(list*f,void*v){
	return LIST_DATA(f,void*)==v;
}

bool list_string_comparator(list*f,void*v){
	LIST_DATA_DECLARE(str,f,char*);
	return str&&v&&strcmp(str,(char*)v)==0;
}

bool list_string_case_comparator(list*f,void*v){
	LIST_DATA_DECLARE(str,f,char*);
	return str&&v&&strcasecmp(str,(char*)v)==0;
}

bool list_string_sorter(list*l1,list*l2){
	LIST_DATA_DECLARE(s1,l1,char*);
	LIST_DATA_DECLARE(s2,l2,char*);
	for(size_t i=0;s1[i]&&s2[i];i++)
		if(s1[i]!=s2[i])return s1[i]>s2[i];
	return false;
}

list*list_search_string(list*lst,const char*str){
	return list_search_one(lst,list_string_comparator,(void*)str);
}

list*list_search_case_string(list*lst,const char*str){
	return list_search_one(lst,list_string_case_comparator,(void*)str);
}

int list_reverse(list*lst){
	list*l,*n;
	if((l=list_first(lst)))do{
		n=l->prev;
		l->prev=l->next;
		l->next=n;
	}while((l=l->prev));
	return 0;
}

char*list_string_append(list*lst,char*buff,size_t len,char*sep){
	list*l;
	if((l=list_first(lst)))do{
		LIST_DATA_DECLARE(str,l,char*);
		if(!str)continue;
		if(sep)strlcat(buff,sep,len);
		strlcat(buff,str,len);
	}while((l=l->next));
	return buff;
}
