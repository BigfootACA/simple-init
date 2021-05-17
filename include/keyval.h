#ifndef kv_H
#define kv_H
#include<sys/types.h>

// keyval struct
struct keyval{char*key,*value;};
typedef struct keyval keyval;

// keyval single usage

// src/lib/keyval.c: free keyval struct
extern void kv_free(keyval*kv);

// src/lib/keyval.c: fill keyval struct with zero
extern keyval*kv_init(keyval*kv);

// src/lib/keyval.c: allocate a new keyval struct
extern keyval*kv_malloc();

// src/lib/keyval.c: allocate a new keyval struct (exit when failed)
extern keyval*kv_xmalloc();

// src/lib/keyval.c: create and init a keyval (call kv_malloc and kv_init)
extern keyval*kv_new();

// src/lib/keyval.c: convert keyval to a string (eg: kv_print({"K","V"},buf,32,"=") = "K=V")
extern char*kv_print(keyval*kv,char*buff,size_t bs,char*del);

// src/lib/keyval.c: dump keyval to stdout (call kv_print)
extern void kv_dump(keyval*kv,char*del);

// src/lib/keyval.c: set key and value
extern keyval*kv_set(keyval*kv,char*key,char*value);

// src/lib/keyval.c: duplicate key and value
extern keyval*kv_set_dup(keyval*kv,char*key,char*value);

// src/lib/keyval.c: duplicate key and value with length
extern keyval*kv_set_ndup(keyval*kv,char*key,size_t ks,char*value,size_t vs);

// src/lib/keyval.c: convert string line to keyval (eg: kv_parse({},"K=V",'=') = {"K","V"})
extern keyval*kv_parse(keyval*kv,char*line,char del);

// src/lib/keyval.c: create and init a keyval with key and value (call kv_new and kv_set)
extern keyval*kv_new_set(char*key,char*value);

// src/lib/keyval.c: create and init a keyval with key and value (call kv_new and kv_set_dup)
extern keyval*kv_new_set_dup(char*key,char*value);

// src/lib/keyval.c: create and init a keyval with key and value (call kv_new and kv_set_ndup)
extern keyval*kv_new_set_ndup(char*key,size_t ks,char*value,size_t vs);

// src/lib/keyval.c: create and init a keyval from a line (call kv_new and kv_parse)
extern keyval*kv_new_parse(char*line,char del);

// keyval array usage

// src/lib/keyval.c: free a keyval array and free all elements in array
extern void kvarr_free(keyval**kvs);

// src/lib/keyval.c: fill keyval array with zero
extern keyval**kvarr_init(keyval**kvs,int len);

// src/lib/keyval.c: allocate a new keyval array with length
extern keyval**kvarr_malloc(size_t len);

// src/lib/keyval.c: allocate a new keyval array with length (exit when failed)
extern keyval**kvarr_xmalloc(size_t len);

// src/lib/keyval.c: create and init a keyval array with length (call kvarr_malloc and kvarr_init)
extern keyval**kvarr_new(int len);

// src/lib/keyval.c: convert string to keyval array (eg: kvarr_parse({},["K=V\nA=B"],'\n','=') = [{"K","V"},{"A","B"}])
extern keyval**kvarr_parse(keyval**kvs,size_t s,char*lines,char ldel,char del);

// src/lib/keyval.c: convert string array to keyval array (eg: kvarr_parse_arr({},["K=V","A=B"],'=') = [{"K","V"},{"A","B"}])
extern keyval**kvarr_parse_arr(keyval**kvs,size_t s,char**lines,char del);

// src/lib/keyval.c: create and init a keyval array from a string= (call kvarr_new and kvarr_parse)
extern keyval**kvarr_new_parse(char*lines,char ldel,char del);

// src/lib/keyval.c: create and init a keyval array from a string array (call kvarr_new and kvarr_parse_arr)
extern keyval**kvarr_new_parse_arr(char**lines,char del);

// src/lib/keyval.c: dump keyval array to stdout (call kv_print) (eg: kvarr_dump([{"K","V"},{"A","B"}],"=","\n") = "K=V\nA=B")
extern void kvarr_dump(keyval**kvs,char*del,char*ldel);

// src/lib/keyval.c: calc keyval array elements count
extern size_t kvarr_count(keyval**kvs);

// src/lib/keyval.c: find a keyval in keyval array by key, return def if not found
extern keyval*kvarr_get_by_key(keyval**kvs,char*key,keyval*def);

// src/lib/keyval.c: find a keyval in keyval array by value, return def if not found
extern keyval*kvarr_get_by_value(keyval**kvs,char*value,keyval*def);

// src/lib/keyval.c: get a value in keyval array by key, return def if not found
extern char*kvarr_get_value_by_key(keyval**kvs,char*key,char*def);

// src/lib/keyval.c: get a key in keyval array by value, return def if not found
extern char*kvarr_get_key_by_value(keyval**kvs,char*value,char*def);

// declare a keyval
#define KV(_key,_value)(keyval){.key=(_key),.value=(_value)}

// foreach a keyval array
#define KVARR_FOREACH(kvs,item,idx) \
        keyval*item;\
	for(size_t idx=0;((item)=(kvs)[idx]);(idx)++)

// alias of kvarr_get_value_by_key
#define kvarr_get kvarr_get_value_by_key
#endif
