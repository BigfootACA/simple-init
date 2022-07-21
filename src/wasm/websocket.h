#ifndef SIMPLE_INIT_WASM_WEBSOCKET_H
#define SIMPLE_INIT_WASM_WEBSOCKET_H
#include<json.h>
#include<stdbool.h>
#include<sys/types.h>
#include <emscripten/websocket.h>
#ifndef MIN
#define MIN(a,b)((b)>(a)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b)((b)<(a)?(a):(b))
#endif
#define ARRLEN(x)(sizeof(x)/sizeof((x)[0]))
struct ws_cmd_proc;
struct ws_data_hand;
struct http_hand_websocket;
typedef int(*ws_cmd_hand)(
	struct ws_cmd_proc*,
	struct http_hand_websocket*,
	char**,size_t*
);
struct ws_cmd_proc{
	const char cmd[64];
	ws_cmd_hand hand;
};
typedef int(*ws_data_handler)(
	struct http_hand_websocket*h,
	struct ws_data_hand*hand,
	const char*data,
	size_t len
);
typedef int(*ws_json_handler)(
	struct http_hand_websocket*h,
	struct ws_data_hand*hand,
	const json_object*jo
);
typedef int(*ws_status_handler)(
	EMSCRIPTEN_WEBSOCKET_T
);

struct ws_data_hand{
	char tag[64];
	ws_data_handler recv_data;
	ws_json_handler recv_json;
};

#define WS_CMDS(cmds...)((struct ws_cmd_proc*[]){cmds NULL})
#define WS_HANDS(hands...)((struct ws_data_hand*[]){hands NULL})
#define WS_CMD_PROC(_name,_hand)(&(struct ws_cmd_proc){.cmd=(_name),.hand=(_hand)}),
#define WS_HAND_PROC(_name,_json,_data)(&(struct ws_data_hand){.tag=(_name),.recv_json=(_json),.recv_data=(_data)}),
struct http_hand_websocket{
	EMSCRIPTEN_WEBSOCKET_T ws;
	size_t*counter;
	ws_status_handler establish;
	ws_status_handler disconnect;
	struct ws_data_hand**hands;
	struct ws_cmd_proc**cmds;
};
extern EM_BOOL ws_on_message(
	int type,
	const EmscriptenWebSocketMessageEvent*event,
	void*data
);
extern int ws_send_json_payload(struct http_hand_websocket*h,const char*tag,json_object*jo);
extern int ws_send_payload(struct http_hand_websocket*h,const char*tag,const void*payload,size_t len);
extern int ws_print_payload(struct http_hand_websocket*h,const char*tag,const char*payload);
extern int ws_send_cmd_r(int r,struct http_hand_websocket*h,const char*cmd);
extern int ws_send_cmd(struct http_hand_websocket*h,const char*cmd);
extern int ws_printf(struct http_hand_websocket*h,const char*fmt,...)__attribute__((format(printf,2,3)));
extern int ws_print(struct http_hand_websocket*h,const char*data);
extern int ws_write(struct http_hand_websocket*h,const char*data,size_t len);
#endif
