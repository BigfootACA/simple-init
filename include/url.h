/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifndef _URL_H
#define _URL_H
#include<stdint.h>
#include<stdbool.h>
#include"list.h"
#include"keyval.h"

typedef struct url{
	char*scheme;
	char*username;
	char*password;
	char*host;
	int port;
	char*path;
	char*query;
	char*fragment;
}url;

// src/lib/url.c: encode (escape) url
extern char*url_encode(const char*src,size_t src_len,char*out,size_t out_len);

// src/lib/url.c: encode (escape) url with custom char map
extern char*url_encode_map(const char*src,size_t src_len,char*out,size_t out_len,const char*map);

// src/lib/url.c: encode (escape) url skip some chars
extern char*url_encode_skip(const char*src,size_t src_len,char*out,size_t out_len,const char*skip);

// src/lib/url.c: encode (escape) url and append to a buffer
extern char*url_encode_append(const char*src,size_t src_len,char*buf,size_t buf_len);

// src/lib/url.c: encode (escape) url and append to a buffer with custom char map
extern char*url_encode_append_map(const char*src,size_t src_len,char*buf,size_t buf_len,const char*map);

// src/lib/url.c: encode (escape) url and append to a buffer skip some chars
extern char*url_encode_append_skip(const char*src,size_t src_len,char*buf,size_t buf_len,const char*skip);

// src/lib/url.c: encode (escape) url and return allocated buffer
extern char*url_encode_alloc(const char*src,size_t src_len);

// src/lib/url.c: encode (escape) url and return allocated buffer with custom char map
extern char*url_encode_alloc_map(const char*src,size_t src_len,const char*map);

// src/lib/url.c: encode (escape) url and return allocated buffer skip some chars
extern char*url_encode_alloc_skip(const char*src,size_t src_len,const char*skip);

// src/lib/url.c: decode (unescape) url
extern char*url_decode(const char*src,size_t src_len,char*out,size_t out_len);

// src/lib/url.c: decode (unescape) url and return allocated buffer
extern char*url_decode_alloc(const char*src,size_t src_len);

// src/lib/url.c: parse url query eg: url_parse_query_list("http://localhost/?a=b&c&d=%20") = {"a":"b","c":NULL,"d":" "}
extern list*url_parse_query_list(const char*url,size_t len);

// src/lib/url.c: parse url query eg: url_parse_query_array("http://localhost/?a=b&c&d=%20") = {"a":"b","c":NULL,"d":" "}
extern keyval**url_parse_query_array(const char*url,size_t len);

// src/lib/url.c: generate query string from keyval list
extern char*url_generate_query_list(char*buf,size_t len,list*queries);

// src/lib/url.c: generate query string from keyval list and return allocated buffer
extern char*url_generate_query_list_alloc(list*queries);

// src/lib/url.c: generate query string from keyval array
extern char*url_generate_query_array(char*buf,size_t len,keyval**queries);

// src/lib/url.c: generate query string from keyval array and return allocated buffer
extern char*url_generate_query_array_alloc(keyval**queries);

// src/lib/url.c: allocate url struct
extern url*url_new();

// src/lib/url.c: parse all url components
extern bool url_parse(url*u,const char*url,size_t len);

// src/lib/url.c: allocate url struct and parse all url components
extern url*url_parse_new(const char*url,size_t len);

// src/lib/url.c: parse a relative url
extern url*url_parse_relative_path(url*u,url*n,const char*path,size_t len);

// src/lib/url.c: clean all fields in url struct
extern void url_clean(url*u);

// src/lib/url.c: free url struct
extern void url_free(url*u);

// src/lib/url.c: copy url all fields
extern url*url_copy(url*old,url*new);

// src/lib/url.c: duplicate url struct and all fields
extern url*url_dup(url*u);

// src/lib/url.c: compare two url struct and all fields
extern bool url_equals(url*u1,url*u2);

// src/lib/url.c: set url path go back
extern bool url_go_back(url*u,bool clean);

// src/lib/url.c: check url path is in top level
extern bool url_is_on_top(url*u);

// src/lib/url.c: generate url from url struct
extern char*url_generate(char*buf,size_t len,url*u);

// src/lib/url.c: generate url from url struct and return allocated buffer
extern char*url_generate_alloc(url*u);

// src/lib/url.c: generate url authority from url struct
extern char*url_generate_authority(char*buf,size_t len,url*u);

// src/lib/url.c: dump all fields in url struct
extern void url_dump(char*buf,size_t len,url*u);

// src/lib/url.c: dump all fields in url struct and return allocated buffer
extern char*url_dump_alloc(url*u);

// src/lib/url.c: dump all fields in url struct to fd
extern void url_dump_fd(int fd,url*u);

// src/lib/url.c: set url scheme value
extern char*url_set_scheme(url*u,const char*val,size_t len);

// src/lib/url.c: set url username value
extern char*url_set_username(url*u,const char*val,size_t len);

// src/lib/url.c: set url username value without decode
extern char*url_set_username_decoded(url*u,const char*val,size_t len);

// src/lib/url.c: set url password value
extern char*url_set_password(url*u,const char*val,size_t len);

// src/lib/url.c: set url password value without decode
extern char*url_set_password_decoded(url*u,const char*val,size_t len);

// src/lib/url.c: set url host value
extern char*url_set_host(url*u,const char*val,size_t len);

// src/lib/url.c: set url host value without decode
extern char*url_set_host_decoded(url*u,const char*val,size_t len);

// src/lib/url.c: set url path value
extern char*url_set_path(url*u,const char*val,size_t len);

// src/lib/url.c: set url path value without decode
extern char*url_set_path_decoded(url*u,const char*val,size_t len);

// src/lib/url.c: set url fragment value
extern char*url_set_fragment(url*u,const char*val,size_t len);

// src/lib/url.c: set url query value
extern char*url_set_query(url*u,const char*val,size_t len);

// src/lib/url.c: set url query from list
extern char*url_set_query_list(url*u,list*queries);

// src/lib/url.c: set url query from array
extern char*url_set_query_array(url*u,keyval**queries);

// src/lib/url.c: get url query as list
extern list*url_get_query_list(url*u);

// src/lib/url.c: get url query as array
extern keyval**url_get_query_array(url*u);

#define url_dump_stdout(u)url_dump_fd(STDOUT_FILENO,u)
#define url_dump_stderr(u)url_dump_fd(STDERR_FILENO,u)
#endif
