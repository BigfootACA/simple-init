#ifndef MSGBOX_H
#define MSGBOX_H
enum msgbox_mode{
	MODE_OK,
	MODE_YESNO,
	MODE_CUSTOM
};
typedef bool(*msgbox_callback)(uint16_t id,const char*btn);
struct msgbox{
	char text[BUFSIZ];
	const char**buttons;
	enum msgbox_mode mode;
	msgbox_callback callback;
};
extern void msgbox_create(
	enum msgbox_mode mode,
	msgbox_callback callback,
	const char**buttons,
	const char*content,
	...
);
extern void msgbox_create_yesno(msgbox_callback callback,const char*content,...);
extern void msgbox_create_ok(msgbox_callback callback,const char*content,...);
extern void msgbox_alert(const char*content,...);
#endif
