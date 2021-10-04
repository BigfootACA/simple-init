#ifndef MSGBOX_H
#define MSGBOX_H
enum msgbox_mode{
	MODE_OK,
	MODE_YESNO,
	MODE_CUSTOM
};
typedef bool(*msgbox_callback)(uint16_t id,const char*btn);
struct msgbox{
	char*title;
	const char**buttons;
	enum msgbox_mode mode;
	msgbox_callback callback;
};
extern void msgbox_create(
	char*content,
	enum msgbox_mode mode,
	msgbox_callback callback,
	const char**buttons
);
#endif
