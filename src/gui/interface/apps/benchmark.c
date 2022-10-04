#ifdef ENABLE_GUI
#define _GNU_SOURCE
#include<unistd.h>
#include<stdlib.h>
#include"gui.h"
#include"logger.h"
#include"pathnames.h"
#include"gui/sysbar.h"
#include"gui/guidrv.h"
#include"gui/activity.h"
#undef time
#define TAG "benchmark"
#define RND_NUM              64
#define SCENE_TIME           1000
#define ANIM_TIME_MIN        ((2*SCENE_TIME)/10)
#define ANIM_TIME_MAX        (SCENE_TIME)
#define OBJ_NUM              8
#define OBJ_SIZE_MIN         (LV_MAX(LV_DPI/20,5))
#define OBJ_SIZE_MAX         ((int)gui_sw/2)
#define RADIUS               LV_MAX(LV_DPI/15,2)
#define BORDER_WIDTH         LV_MAX(LV_DPI/40,1)
#define SHADOW_WIDTH_SMALL   LV_MAX(LV_DPI/15,5)
#define SHADOW_OFS_X_SMALL   LV_MAX(LV_DPI/20,2)
#define SHADOW_OFS_Y_SMALL   LV_MAX(LV_DPI/20,2)
#define SHADOW_SPREAD_SMALL  LV_MAX(LV_DPI/30,2)
#define SHADOW_WIDTH_LARGE   LV_MAX(LV_DPI/5, 10)
#define SHADOW_OFS_X_LARGE   LV_MAX(LV_DPI/10,5)
#define SHADOW_OFS_Y_LARGE   LV_MAX(LV_DPI/10,5)
#define SHADOW_SPREAD_LARGE  LV_MAX(LV_DPI/30,2)
#define IMG_WIDTH            100
#define IMG_HEIGHT           100
#define IMG_NUM              LV_MAX((int)(gui_sw*gui_sh)/5/IMG_WIDTH/IMG_HEIGHT,1)
#define IMG_ZOOM_MIN         128
#define IMG_ZOOM_MAX         (256+64)
#define TXT                  "hello world\nit is a multi line text to test\nthe performance of text rendering"
#define LINE_WIDTH           LV_MAX(LV_DPI/50,2)
#define LINE_POINT_NUM       16
#define LINE_POINT_DIFF_MIN  (LV_DPI/10)
#define LINE_POINT_DIFF_MAX  LV_MAX((int)gui_sw/(LINE_POINT_NUM+2),LINE_POINT_DIFF_MIN*2)
#define ARC_WIDTH_THIN       LV_MAX(LV_DPI/50,2)
#define ARC_WIDTH_THICK      LV_MAX(LV_DPI/10,5)
typedef struct{
	const char*name;
	void (*create_cb)(void);
	uint32_t time_sum_normal,time_sum_opa;
	uint32_t refr_cnt_normal,refr_cnt_opa;
	uint32_t fps_normal,fps_opa;
	uint8_t weight;
}scene_dsc_t;
static lv_obj_t*screen;
static bool run=false;
static lv_style_t style_common;
static bool opa_mode=true;
static int32_t scene_act=-1;
static lv_obj_t*scene_bg,*title,*subtitle;
static uint32_t rnd_act;
static uint32_t rnd_map[]={
	0xbd13204f,0x67d8167f,0x20211c99,0xb0a7cc05,0x06d5c703,0xeafb01a7,0xd0473b5c,0xc999aaa2,
	0x86f9d5d9,0x294bdb29,0x12a3c207,0x78914d14,0x10a30006,0x6134c7db,0x194443af,0x142d1099,
	0x376292d5,0x20f433c5,0x074d2a59,0x4e74c293,0x072a0810,0xdd0f136d,0x5cca6dbc,0x623bfdd8,
	0xb645eb2f,0xbe50894a,0xc9b56717,0xe0f912c8,0x4f6b5e24,0xfe44b128,0xe12d57a8,0x9b15c9cc,
	0xab2ae1d3,0xb4dc5074,0x67d457c8,0x8e46b00c,0xa29a1871,0xcee40332,0x80f93aa1,0x85286096,
	0x09bd6b49,0x95072088,0x2093924b,0x6a27328f,0xa796079b,0xc3b488bc,0xe29bcce0,0x07048a4c,
	0x7d81bd99,0x27aacb30,0x44fc7a0e,0xa2382241,0x8357a17d,0x97e9c9cc,0xad10ff52,0x9923fc5c,
	0x8f2c840a,0x20356ba2,0x7997a677,0x9a7f1800,0x35c7562b,0xd901fe51,0x8f4e053d,0xa5b94923,
};
static void rnd_reset(void){rnd_act=0;}
static int32_t rnd_next(int32_t min,int32_t max){
	if(min==max)return min;
	if(min>max){
		int32_t t=min;
		min=max,max=t;
	}
	int32_t d=max-min,r=(rnd_map[rnd_act]%d)+min;
	rnd_act++;
	if(rnd_act>=RND_NUM)rnd_act=0;
	return r;
}
static void fall_anim(lv_obj_t*obj){
	lv_obj_update_layout(scene_bg);
	lv_obj_update_layout(obj);
	lv_obj_set_x(obj,rnd_next(0,lv_obj_get_width(scene_bg)-lv_obj_get_width(obj)));
	uint32_t t=rnd_next(ANIM_TIME_MIN,ANIM_TIME_MAX);
	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a,obj);
	lv_anim_set_exec_cb(&a,(lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_values(&a,0,lv_obj_get_height(scene_bg)-lv_obj_get_height(obj));
	lv_anim_set_time(&a,t);
	lv_anim_set_playback_time(&a,t);
	lv_anim_set_repeat_count(&a,LV_ANIM_REPEAT_INFINITE);
	a.act_time=a.time/2;
	lv_anim_start(&a);
}
static void rect_create(lv_style_t*style){
	for(uint32_t i=0;i<OBJ_NUM;i++){
		lv_obj_t*obj=lv_obj_create(scene_bg);
		lv_obj_remove_style_all(obj);
		lv_obj_add_style(obj,style,0);
		lv_obj_set_style_bg_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		lv_obj_set_style_border_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		lv_obj_set_style_shadow_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		lv_obj_set_size(obj,rnd_next(OBJ_SIZE_MIN,OBJ_SIZE_MAX),rnd_next(OBJ_SIZE_MIN,OBJ_SIZE_MAX));
		fall_anim(obj);
	}
}
static void img_create(lv_style_t*style,const void*src,bool rotate,bool zoom,bool aa){
	for(uint32_t i=0;i<(uint32_t)IMG_NUM;i++){
		lv_obj_t*obj=lv_img_create(scene_bg);
		lv_obj_remove_style_all(obj);
		lv_obj_add_style(obj,style,0);
		lv_img_set_src(obj,src);
		lv_obj_set_style_img_recolor(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		if(rotate)lv_img_set_angle(obj,rnd_next(0,3599));
		if(zoom)lv_img_set_zoom(obj,rnd_next(IMG_ZOOM_MIN,IMG_ZOOM_MAX));
		lv_img_set_antialias(obj,aa);
		fall_anim(obj);
	}
}
static void txt_create(lv_style_t*style){
	for(uint32_t i=0;i<OBJ_NUM;i++){
		lv_obj_t*obj=lv_label_create(scene_bg);
		lv_obj_remove_style_all(obj);
		lv_obj_add_style(obj,style,0);
		lv_obj_set_style_text_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		lv_label_set_text(obj,TXT);
		fall_anim(obj);
	}
}
static void line_create(lv_style_t*style){
	static lv_point_t points[OBJ_NUM][LINE_POINT_NUM];
	for(uint32_t i=0;i<OBJ_NUM;i++){
		points[i][0].x=0,points[i][0].y=0;
		for(uint32_t j=1;j<LINE_POINT_NUM;j++){
			points[i][j].x=points[i][j-1].x+rnd_next(LINE_POINT_DIFF_MIN,LINE_POINT_DIFF_MAX);
			points[i][j].y=rnd_next(LINE_POINT_DIFF_MIN,LINE_POINT_DIFF_MAX);
		}
		lv_obj_t*obj=lv_line_create(scene_bg);
		lv_obj_remove_style_all(obj);
		lv_obj_add_style(obj,style,0);
		lv_obj_set_style_line_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),0);
		lv_line_set_points(obj,points[i],LINE_POINT_NUM);
		fall_anim(obj);
	}
}
static inline void anim_lv_arc_set_end_angle(void*obj,int32_t val){
	lv_arc_set_end_angle(obj,val);
}
static void arc_create(lv_style_t*style){
	for(uint32_t i=0;i<OBJ_NUM;i++){
		lv_obj_t*obj=lv_arc_create(scene_bg);
		lv_obj_remove_style_all(obj);
		lv_obj_set_size(obj,rnd_next(OBJ_SIZE_MIN,OBJ_SIZE_MAX),rnd_next(OBJ_SIZE_MIN,OBJ_SIZE_MAX));
		lv_obj_add_style(obj,style,LV_PART_INDICATOR);
		lv_obj_set_style_arc_color(obj,lv_color_hex(rnd_next(0,0xFFFFF0)),LV_PART_INDICATOR);
		lv_arc_set_start_angle(obj,0);
		uint32_t t=rnd_next(ANIM_TIME_MIN/4,ANIM_TIME_MAX/4);
		lv_anim_t a;
		lv_anim_init(&a);
		lv_anim_set_var(&a,obj);
		lv_anim_set_exec_cb(&a,anim_lv_arc_set_end_angle);
		lv_anim_set_values(&a,0,359);
		lv_anim_set_time(&a,t);
		lv_anim_set_playback_time(&a,t);
		lv_anim_set_repeat_count(&a,LV_ANIM_REPEAT_INFINITE);
		lv_anim_start(&a);
		fall_anim(obj);
	}
}
static void rectangle_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_bg_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void rectangle_rounded_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void rectangle_circle_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,LV_RADIUS_CIRCLE);
	lv_style_set_bg_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void border_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void border_rounded_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void border_circle_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,LV_RADIUS_CIRCLE);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	rect_create(&style_common);
}
static void border_top_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_border_side(&style_common,LV_BORDER_SIDE_TOP);
	rect_create(&style_common);
}
static void border_left_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_border_side(&style_common,LV_BORDER_SIDE_LEFT);
	rect_create(&style_common);
}
static void border_top_left_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_border_side(&style_common,LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP);
	rect_create(&style_common);
}
static void border_left_right_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_border_side(&style_common,LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT);
	rect_create(&style_common);
}
static void border_top_bottom_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_border_side(&style_common,LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM);
	rect_create(&style_common);
}
static void shadow_small_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,LV_OPA_COVER);
	lv_style_set_shadow_opa(&style_common,opa_mode?LV_OPA_80:LV_OPA_COVER);
	lv_style_set_shadow_width(&style_common,SHADOW_WIDTH_SMALL);
	rect_create(&style_common);
}
static void shadow_small_ofs_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,LV_OPA_COVER);
	lv_style_set_shadow_opa(&style_common,opa_mode?LV_OPA_80:LV_OPA_COVER);
	lv_style_set_shadow_width(&style_common,SHADOW_WIDTH_SMALL);
	lv_style_set_shadow_ofs_x(&style_common,SHADOW_OFS_X_SMALL);
	lv_style_set_shadow_ofs_y(&style_common,SHADOW_OFS_Y_SMALL);
	lv_style_set_shadow_spread(&style_common,SHADOW_SPREAD_SMALL);
	rect_create(&style_common);
}
static void shadow_large_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,LV_OPA_COVER);
	lv_style_set_shadow_opa(&style_common,opa_mode?LV_OPA_80:LV_OPA_COVER);
	lv_style_set_shadow_width(&style_common,SHADOW_WIDTH_LARGE);
	rect_create(&style_common);
}
static void shadow_large_ofs_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,LV_OPA_COVER);
	lv_style_set_shadow_opa(&style_common,opa_mode?LV_OPA_80:LV_OPA_COVER);
	lv_style_set_shadow_width(&style_common,SHADOW_WIDTH_LARGE);
	lv_style_set_shadow_ofs_x(&style_common,SHADOW_OFS_X_LARGE);
	lv_style_set_shadow_ofs_y(&style_common,SHADOW_OFS_Y_LARGE);
	lv_style_set_shadow_spread(&style_common,SHADOW_SPREAD_LARGE);
	rect_create(&style_common);
}
static void img_rgb_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_rgb.png",false,false,false);
}
static void img_argb_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_argb.png",false,false,false);
}
static void img_ckey_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_chroma_keyed.png",false,false,false);
}
static void img_index_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_indexed16.png",false,false,false);
}
static void img_rgb_recolor_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_img_recolor_opa(&style_common,LV_OPA_50);
	img_create(&style_common,"img_cogwheel_rgb.png",false,false,false);
}
static void img_argb_recolor_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_img_recolor_opa(&style_common,LV_OPA_50);
	img_create(&style_common,"img_cogwheel_argb.png",false,false,false);
}
static void img_ckey_recolor_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_img_recolor_opa(&style_common,LV_OPA_50);
	img_create(&style_common,"img_cogwheel_chroma_keyed.png",false,false,false);
}
static void img_index_recolor_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_img_recolor_opa(&style_common,LV_OPA_50);
	img_create(&style_common,"img_cogwheel_indexed16.png",false,false,false);
}
static void img_rgb_rot_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_rgb.png",true,false,false);
}
static void img_rgb_rot_aa_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_rgb.png",true,false,true);
}
static void img_argb_rot_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_argb.png",true,false,false);
}
static void img_argb_rot_aa_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_argb.png",true,false,true);
}
static void img_rgb_zoom_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_rgb.png",false,true,false);
}
static void img_rgb_zoom_aa_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_rgb.png",false,true,true);
}
static void img_argb_zoom_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_argb.png",false,true,false);
}
static void img_argb_zoom_aa_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	img_create(&style_common,"img_cogwheel_argb.png",false,true,true);
}
static void txt_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_text_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	txt_create(&style_common);
}
static void line_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_line_width(&style_common,LINE_WIDTH);
	lv_style_set_line_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	line_create(&style_common);
}
static void arc_think_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_arc_width(&style_common,ARC_WIDTH_THIN);
	lv_style_set_arc_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	arc_create(&style_common);
}
static void arc_thick_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_arc_width(&style_common,ARC_WIDTH_THICK);
	lv_style_set_arc_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	arc_create(&style_common);
}
static void sub_rectangle_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	rect_create(&style_common);
}
static void sub_border_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_border_width(&style_common,BORDER_WIDTH);
	lv_style_set_border_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	rect_create(&style_common);
}
static void sub_shadow_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_radius(&style_common,RADIUS);
	lv_style_set_bg_opa(&style_common,LV_OPA_COVER);
	lv_style_set_shadow_opa(&style_common,opa_mode?LV_OPA_80:LV_OPA_COVER);
	lv_style_set_shadow_width(&style_common,SHADOW_WIDTH_SMALL);
	lv_style_set_shadow_spread(&style_common,SHADOW_WIDTH_SMALL);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	rect_create(&style_common);
}
static void sub_img_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_img_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	img_create(&style_common,"img_cogwheel_argb.png",false,false,false);
}
static void sub_line_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_line_width(&style_common,LINE_WIDTH);
	lv_style_set_line_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	line_create(&style_common);
}
static void sub_arc_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_arc_width(&style_common,ARC_WIDTH_THICK);
	lv_style_set_arc_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	arc_create(&style_common);
}
static void sub_text_cb(void){
	lv_style_reset(&style_common);
	lv_style_set_text_font(&style_common,lv_theme_get_font_normal(NULL));
	lv_style_set_text_opa(&style_common,opa_mode?LV_OPA_50:LV_OPA_COVER);
	lv_style_set_blend_mode(&style_common,LV_BLEND_MODE_SUBTRACTIVE);
	txt_create(&style_common);
}
static scene_dsc_t scenes[]={
	{.name="Rectangle",			 .weight=30,.create_cb=rectangle_cb},
	{.name="Rectangle rounded",		 .weight=20,.create_cb=rectangle_rounded_cb},
	{.name="Circle",			 .weight=10,.create_cb=rectangle_circle_cb},
	{.name="Border",			 .weight=20,.create_cb=border_cb},
	{.name="Border rounded",		 .weight=30,.create_cb=border_rounded_cb},
	{.name="Circle border",			 .weight=10,.create_cb=border_circle_cb},
	{.name="Border top",			 .weight=3, .create_cb=border_top_cb},
	{.name="Border left",			 .weight=3, .create_cb=border_left_cb},
	{.name="Border top + left",		 .weight=3, .create_cb=border_top_left_cb},
	{.name="Border left + right",		 .weight=3, .create_cb=border_left_right_cb},
	{.name="Border top + bottom",		 .weight=3, .create_cb=border_top_bottom_cb},
	{.name="Shadow small",			 .weight=3, .create_cb=shadow_small_cb},
	{.name="Shadow small offset",		 .weight=5, .create_cb=shadow_small_ofs_cb},
	{.name="Shadow large",			 .weight=5, .create_cb=shadow_large_cb},
	{.name="Shadow large offset",		 .weight=3, .create_cb=shadow_large_ofs_cb},
	{.name="Image RGB",			 .weight=20,.create_cb=img_rgb_cb},
	{.name="Image ARGB",			 .weight=20,.create_cb=img_argb_cb},
	{.name="Image chorma keyed",		 .weight=5, .create_cb=img_ckey_cb},
	{.name="Image indexed",			 .weight=5, .create_cb=img_index_cb},
	{.name="Image RGB recolor",		 .weight=5, .create_cb=img_rgb_recolor_cb},
	{.name="Image ARGB recolor",		 .weight=20,.create_cb=img_argb_recolor_cb},
	{.name="Image chorma keyed recolor",	 .weight=3, .create_cb=img_ckey_recolor_cb},
	{.name="Image indexed recolor",		 .weight=3, .create_cb=img_index_recolor_cb},
	{.name="Image RGB rotate",		 .weight=3, .create_cb=img_rgb_rot_cb},
	{.name="Image RGB rotate anti aliased",	 .weight=3, .create_cb=img_rgb_rot_aa_cb},
	{.name="Image ARGB rotate",		 .weight=5, .create_cb=img_argb_rot_cb},
	{.name="Image ARGB rotate anti aliased", .weight=5, .create_cb=img_argb_rot_aa_cb},
	{.name="Image RGB zoom",		 .weight=3, .create_cb=img_rgb_zoom_cb},
	{.name="Image RGB zoom anti aliased",	 .weight=3, .create_cb=img_rgb_zoom_aa_cb},
	{.name="Image ARGB zoom",		 .weight=5, .create_cb=img_argb_zoom_cb},
	{.name="Image ARGB zoom anti aliased",	 .weight=5, .create_cb=img_argb_zoom_aa_cb},
	{.name="Text",				 .weight=20,.create_cb=txt_cb},
	{.name="Line",				 .weight=10,.create_cb=line_cb},
	{.name="Arc think",			 .weight=10,.create_cb=arc_think_cb},
	{.name="Arc thick",			 .weight=10,.create_cb=arc_thick_cb},
	{.name="Substr. rectangle",		 .weight=10,.create_cb=sub_rectangle_cb},
	{.name="Substr. border",		 .weight=10,.create_cb=sub_border_cb},
	{.name="Substr. shadow",		 .weight=10,.create_cb=sub_shadow_cb},
	{.name="Substr. image",			 .weight=10,.create_cb=sub_img_cb},
	{.name="Substr. line",			 .weight=10,.create_cb=sub_line_cb},
	{.name="Substr. arc",			 .weight=10,.create_cb=sub_arc_cb},
	{.name="Substr. text",			 .weight=10,.create_cb=sub_text_cb},
	{.name="",				 .create_cb=NULL}
};
static void result_out(bool opa){
	int i=scene_act;
	if(opa)i--;
	const char*name=scenes[i].name;
	uint32_t fps=scenes[i].fps_normal;
	lv_label_set_text_fmt(
		subtitle,
		"Result of \"%s%s\": %d FPS",
		name,opa?" + opa":"",fps
	);
	tlog_info(
		"result of \"%s%s\": %d FPS",
		name,opa?" + opa":"",fps
	);
}
static void scene_next_task_cb(lv_timer_t*timer __attribute__((unused))){
	if(!run)return;
	lv_disp_trig_activity(NULL);
	lv_obj_clean(scene_bg);
	if(opa_mode){
		if(scene_act>=0){
			if(scenes[scene_act].time_sum_opa==0)scenes[scene_act].time_sum_opa=1;
			scenes[scene_act].fps_opa=(1000*scenes[scene_act].refr_cnt_opa)/scenes[scene_act].time_sum_opa;
			if(scenes[scene_act].create_cb)scene_act++;
		}else scene_act++;
		opa_mode=false;
	}else{
		if(scenes[scene_act].time_sum_normal==0)scenes[scene_act].time_sum_normal=1;
		scenes[scene_act].fps_normal=(1000*scenes[scene_act].refr_cnt_normal)/scenes[scene_act].time_sum_normal;
		opa_mode=true;
	}
	if(scenes[scene_act].create_cb){
		lv_label_set_text_fmt(
			title,
			"%d/%zu: %s%s",
			scene_act*2+(opa_mode?1:0),
			(sizeof(scenes)/sizeof(scene_dsc_t)*2)-2,
			scenes[scene_act].name,
			opa_mode?"+opa":""
		);
		if(opa_mode)result_out(false);
		else if(scene_act>0)result_out(true);
		else lv_label_set_text(subtitle,"");
		rnd_reset();
		scenes[scene_act].create_cb();
		lv_timer_t*t=lv_timer_create(scene_next_task_cb,SCENE_TIME,NULL);
		lv_timer_set_repeat_count(t,1);
	}else{
		uint32_t weight_sum=0;
		uint32_t weight_normal_sum=0;
		uint32_t weight_opa_sum=0;
		uint32_t fps_sum=0;
		uint32_t fps_normal_sum=0;
		uint32_t fps_opa_sum=0;
		uint32_t i;
		for(i=0;scenes[i].create_cb;i++){
			fps_normal_sum+=scenes[i].fps_normal*scenes[i].weight;
			weight_normal_sum+=scenes[i].weight;
			uint32_t w=LV_MAX(scenes[i].weight/2,1);
			fps_opa_sum+=scenes[i].fps_opa*w;
			weight_opa_sum+=w;
		}
		fps_sum=fps_normal_sum+fps_opa_sum;
		weight_sum=weight_normal_sum+weight_opa_sum;
		if(fps_sum<=0)fps_sum=1;
		if(weight_sum<=0)weight_sum=1;
		if(weight_normal_sum<=0)weight_normal_sum=1;
		if(weight_opa_sum<=0)weight_opa_sum=1;
		uint32_t fps_weighted=fps_sum/weight_sum;
		if(fps_weighted<=0)fps_weighted=1;
		uint32_t fps_normal_unweighted=fps_normal_sum/weight_normal_sum;
		if(fps_normal_unweighted<=0)fps_normal_unweighted=1;
		uint32_t fps_opa_unweighted=fps_opa_sum/weight_opa_sum;
		if(fps_opa_unweighted<=0)fps_opa_unweighted=1;
		uint32_t opa_speed_pct=(fps_opa_unweighted*100)/fps_normal_unweighted;
		lv_obj_clean(screen);
		sysbar_set_full_screen(false);
		lv_obj_update_layout(screen);
		scene_bg=NULL;
		lv_obj_set_flex_flow(screen,LV_FLEX_FLOW_COLUMN);
		title=lv_label_create(screen);
		lv_label_set_text_fmt(title,"Weighted FPS: %d",fps_weighted);
		tlog_info("opa speed: %d%%",opa_speed_pct);
		subtitle=lv_label_create(screen);
		lv_label_set_text_fmt(subtitle,"Opa. speed: %d%%",opa_speed_pct);
		tlog_info("weighted fps: %d",fps_weighted);
		lv_coord_t w=lv_obj_get_content_width(screen);
		lv_obj_t*table=lv_table_create(screen);
		lv_table_set_col_cnt(table,2);
		lv_table_set_col_width(table,0,(w*3)/4-3);
		lv_table_set_col_width(table,1,w/4-3);
		lv_obj_set_width(table,lv_pct(100));
		uint16_t row=0;
		lv_table_add_cell_ctrl(table,row,0,LV_TABLE_CELL_CTRL_MERGE_RIGHT);
		lv_table_set_cell_value(table,row,0,"Slow but common cases");
		row++;
		char buf[256];
		for(i=0;i<sizeof(scenes)/sizeof(scene_dsc_t)-1;i++){
			if(scenes[i].fps_normal<20&&scenes[i].weight>=10){
				lv_table_set_cell_value(table,row,0,scenes[i].name);
				lv_snprintf(buf,sizeof(buf),"%d",scenes[i].fps_normal);
				lv_table_set_cell_value(table,row,1,buf);
				row++;
			}
			if(scenes[i].fps_opa<20&&LV_MAX(scenes[i].weight/2,1)>=10){
				lv_snprintf(buf,sizeof(buf),"%s + opa",scenes[i].name);
				lv_table_set_cell_value(table,row,0,buf);
				lv_snprintf(buf,sizeof(buf),"%d",scenes[i].fps_opa);
				lv_table_set_cell_value(table,row,1,buf);
				row++;
			}
		}
		if(row==1){
			lv_table_add_cell_ctrl(table,row,0,LV_TABLE_CELL_CTRL_MERGE_RIGHT);
			lv_table_set_cell_value(table,row,0,"All good");
			row++;
		}
		lv_table_add_cell_ctrl(table,row,0,LV_TABLE_CELL_CTRL_MERGE_RIGHT);
		lv_table_set_cell_value(table,row,0,"All cases");
		row++;
		for(i=0;i<sizeof(scenes)/sizeof(scene_dsc_t)-1;i++){
			lv_table_set_cell_value(table,row,0,scenes[i].name);
			lv_snprintf(buf,sizeof(buf),"%d",scenes[i].fps_normal);
			lv_table_set_cell_value(table,row,1,buf);
			row++;
			lv_snprintf(buf,sizeof(buf),"%s + opa",scenes[i].name);
			lv_table_set_cell_value(table,row,0,buf);
			lv_snprintf(buf,sizeof(buf),"%d",scenes[i].fps_opa);
			lv_table_set_cell_value(table,row,1,buf);
			row++;
		}
	}
}

static void monitor_cb(
	lv_disp_drv_t*drv __attribute__((unused)),
	uint32_t time,
	uint32_t px __attribute__((unused))
){
	if(opa_mode){
		scenes[scene_act].refr_cnt_opa++;
		scenes[scene_act].time_sum_opa+=time;
	}else{
		scenes[scene_act].refr_cnt_normal++;
		scenes[scene_act].time_sum_normal+=time;
	}
}

static void benchmark_draw(lv_obj_t*scr){
	screen=scr;
	lv_disp_get_next(NULL)->driver->monitor_cb=monitor_cb;
	lv_obj_set_style_pad_all(scr,0,0);
	lv_obj_set_flex_flow(scr,LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_bg_opa(scr,LV_OPA_COVER,0);
	title=lv_label_create(scr);
	lv_obj_set_width(title,lv_pct(100));
	subtitle=lv_label_create(scr);
	lv_obj_set_width(subtitle,lv_pct(100));
	scene_bg=lv_obj_create(scr);
	lv_obj_remove_style_all(scene_bg);
	lv_obj_set_flex_grow(scene_bg,1);
	lv_obj_set_width(scene_bg,lv_pct(100));
	lv_style_init(&style_common);
	lv_obj_update_layout(scr);
	scene_next_task_cb(NULL);
}

int benchmark_main(int argc __attribute__((unused)),char**argv __attribute__((unused))){
	open_socket_logfd_default();
	gui_pre_init();
	gui_screen_init();
	run=true;
	benchmark_draw(lv_scr_act());
	for(;;){
		lv_task_handler();
		guidrv_taskhandler();
		usleep(30000);
	}
}

static int benchmark_cleanup(struct gui_activity*act __attribute__((unused))){
	lv_disp_get_next(NULL)->driver->monitor_cb=NULL;
	scene_act=-1,run=false,opa_mode=true;
	return 0;
}

static int benchmark_init(struct gui_activity*act __attribute__((unused))){
	return (lv_disp_get_next(NULL)->driver->monitor_cb)?-1:0;
}

static int benchmark_wrapper_draw(struct gui_activity*act){
	rnd_reset();
	run=true;
	benchmark_draw(act->page);
	return 0;
}

struct gui_register guireg_benchmark={
	.name="gui-benchmark",
	.title="GUI Benchmark",
	.full_screen=true,
	.show_app=true,
	.init=benchmark_init,
	.quiet_exit=benchmark_cleanup,
	.draw=benchmark_wrapper_draw,
	.back=true,
	.mask=false,
};
#endif
