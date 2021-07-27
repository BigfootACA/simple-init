#ifndef ARRAY_H
#define ARRAY_H

// src/lib/array.c: convert array to arg list
extern char*array2args(char**arr,char*d);

// src/lib/array.c: append string to a string array
extern char**char_array_append(char**array,char*item,int idx);

// src/lib/array.c: calc string array length
extern int char_array_len(char**arr);

// src/lib/array.c: free a string array and first element in array
extern void free_args_array(char**c);

// src/lib/array.c: parse args string to a string array
extern char**args2array(char*source,char del);

// src/lib/array.c: duplicate string array
extern char**array_dup(char**orig);

// src/lib/array.c: free a string array and all elements in array
extern void array_free(char**arr);

// simple array length
#define ARRLEN(x)(sizeof(x)/sizeof((x)[0]))

// try realloc
#define TRY_ALLOC(_type,_name,_value,_return){\
	_type x_re=_value;\
	if(x_re)(_name)=x_re;\
	else{free(_name);_return;}\
}
#define TRY_REALLOC(_type,_name,_size,_return)TRY_ALLOC(_type,_name,realloc(_name,_size),_return)
#define TRY_ALLOC_CHARS(_name,_value,_return)TRY_ALLOC(char**,_name,_value,_return)
#define TRY_REALLOC_CHARS(_name,_size,_return)TRY_REALLOC(char**,_name,_size,_return)
#define TRY_APPEND(_name,_value,_idx,_return)TRY_ALLOC_CHARS(_name,char_array_append(_name,_value,_idx),_return)

#endif
