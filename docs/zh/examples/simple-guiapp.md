# 简单的GUI App示例

## 1. 编写src/gui/interface/apps/example.c

```C
#ifdef ENABLE_GUI

#include<errno.h>
#include<stdbool.h>

// GUI库
#include"gui.h"

// GUI活动管理
#include"gui/activity.h"

// Snack Bar
#include"gui/snackbar.h"

// 日志
#include"logger.h"

// 日志tag
#define TAG "example"

struct example{
	bool exit;

	// "Logger"按钮
	lv_obj_t*btn;

	// 标题
	lv_obj_t*txt;
};

// 获取到焦点
static int example_get_focus(struct gui_activity*act){
	// 将所有可控制的按钮加入组，以获取按键控制
	struct example*data=act->data;
	if(!data)return 0;
	lv_group_add_obj(gui_grp,data->btn);
	tlog_debug("hey %s, get focus",act->name);
	return 0;
}

// 失去焦点
static int example_lost_focus(struct gui_activity*act){
	// 让出按钮控制权
	struct example*data=act->data;
	if(!data)return 0;
	lv_group_remove_obj(data->btn);
	tlog_debug("hey %s, lost focus",act->name);
	return 0;
}

// 点击"Logger"按钮
static void logger_click(lv_obj_t*obj,lv_event_t e){
	if(!obj||e!=LV_EVENT_CLICKED)return;
	tlog_debug("hello");
}

// 初始
static int example_init(struct gui_activity*act){
	struct example*data=malloc(sizeof(struct example));
	if(!data)return -ENOMEM;
	memset(data,0,sizeof(struct example));
	act->data=data;
	return 0;
}

// 图形界面大小改变事件
static int example_resize(struct gui_activity*act){
	struct example*data=act->data;
	if(!data)return 0;
	lv_obj_set_width(data->txt,act->w);
	lv_obj_set_width(data->txt,act->w/2);
	lv_obj_align(data->txt,NULL,LV_ALIGN_IN_TOP_MID,0,gui_dpi/2);
	lv_obj_align(data->btn,data->txt,LV_ALIGN_OUT_BOTTOM_MID,0,gui_dpi/5);
	return 0;
}

// 初始化图形界面
static int example_draw(struct gui_activity*act){
	struct example*data=act->data;
	if(!data)return -1;

	// 添加标题
	data->txt=lv_label_create(act->page,NULL);
	lv_label_set_text(data->txt,_("This is example app"));
	lv_label_set_long_mode(data->txt,LV_LABEL_LONG_SROLL_CIRC);
	lv_label_set_align(data->txt,LV_LABEL_ALIGN_CENTER);

	// 添加按钮
	data->btn=lv_btn_create(act->page,NULL);
	lv_obj_set_event_cb(data->btn,logger_click);
	lv_label_set_text(lv_label_create(data->btn,NULL),_("Logger"));

	return 0;
}

// 正在销毁活动
static int example_quiet_exit(struct gui_activity*act){
	tlog_info("bye, %s",act->name);
	if(act->data)free(act->data);
	act->data=NULL;
	return 0;
}

// 返回超时
static int on_dismiss(void*d){
	struct example*data=d;
	data->exit=false;
	return 0;
}

// 是否同意退出（或返回）
static int example_ask_exit(struct gui_activity*act){
	struct example*data=act->data;
	int r=data->exit?0:-1;
	snackbar_set_show_time(3000);
	if(data->exit){
		snackbar_set_on_dismiss(NULL,NULL);
		snackbar_hide();
	}else{
		snackbar_set_on_dismiss(on_dismiss,data);
		snackbar_show_text(_("Press back again to exit"));
	}
	data->exit=true;
	return r;
}

// GUI App 注册
struct gui_register guireg_example={
	// App ID
	.name="example-app",

	// App标题，显示在主界面
	.title="Example App",

	// App图标，显示在主界面
	.icon="example.png",

	// 控制是否在主界面显示图标
	.show_app=true,

	// 事件注册
	.init=example_init,
	.draw=example_draw,
	.quiet_exit=example_quiet_exit,
	.resize=example_resize,
	.ask_exit=example_ask_exit,
	.lost_focus=example_lost_focus,
	.get_focus=example_get_focus,

	// 是否允许返回或退出？
	.back=true,

	// 背景容器是否使用灰色半透明的mask？
	.mask=false,
};
#endif
```

## 2. 将GUI App添加到src/gui/activities.c

注册GUI App

```diff
diff --git a/src/gui/activities.c b/src/gui/activities.c
--- a/src/gui/activities.c
+++ b/src/gui/activities.c
@@ -9,6 +9,7 @@ extern struct gui_register guireg_language;
 extern struct gui_register guireg_filemgr;
 extern struct gui_register guireg_reboot;
 extern struct gui_register guireg_guiapp;
+extern struct gui_register guireg_example;
 struct gui_register*guiact_register[]={
 	#ifdef ENABLE_UEFI
 	&guireg_uefi_bootmenu,
@@ -24,6 +25,7 @@ struct gui_register*guiact_register[]={
 	&guireg_language,
 	&guireg_reboot,
 	&guireg_guiapp,
+	&guireg_example,
 	NULL
 };
 #endif
```

## 3. 添加源码编译

#### Linux Target: 添加到src/gui/CMakeLists.txt

```diff
diff --git a/src/gui/CMakeLists.txt b/src/gui/CMakeLists.txt
--- a/src/gui/CMakeLists.txt
+++ b/src/gui/CMakeLists.txt
@@ -14,6 +14,7 @@ add_library(init_gui STATIC
 	interface/settings/language.c
 	interface/apps/logviewer.c
 	interface/apps/benchmark.c
+	interface/apps/example.c
 	drivers/drm.c
 	drivers/fbdev.c
 	drivers/input.c
```

#### UEFI Target: 添加到src/gui/SimpleInitGUI.inf

```diff
diff --git a/src/gui/SimpleInitGUI.inf b/src/gui/SimpleInitGUI.inf
--- a/src/gui/SimpleInitGUI.inf
+++ b/src/gui/SimpleInitGUI.inf
@@ -62,3 +62,4 @@
   interface/core/sysbar.c
   interface/apps/uefi_bootmenu.c
   interface/apps/uefi_shell.c
+  interface/apps/example.c
```
