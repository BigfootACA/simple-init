#ifdef ENABLE_FREETYPE2
#include<ft2build.h>
#include<assets.h>
#include<sys/stat.h>
#include"logger.h"
#include"lvgl.h"
#include"gui.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_SIZES_H
#define TAG "font"
typedef struct{
	char*name;
	unsigned char*data;
	long length;
}lv_face_info_t;
typedef struct{
	void*face_id;
	lv_font_t*font;
	uint16_t style;
	uint16_t height;
}lv_font_fmt_ft_dsc_t;
typedef struct{
	const char*name;
	unsigned char*data;
	long length;
	lv_font_t*font;
	uint16_t weight;
	uint16_t style;
}lv_ft_info_t;
static FT_Library lib;
static FTC_Manager cmgr;
static FTC_CMapCache cmc;
static FTC_SBitCache sc;
static FTC_SBit sb;

static FT_Error font_face_requester(
	FTC_FaceID fid,
	FT_Library lis __attribute__((unused)),
	FT_Pointer rdata __attribute__((unused)),
	FT_Face*af
){
	lv_face_info_t*i=(lv_face_info_t*)fid;
	FT_Error e;
	if(i->name)e=FT_New_Face(lib,i->name,0,af);
	else if(i->data)e=FT_New_Memory_Face(lib,i->data,i->length,0,af);
	else{
		tlog_error("font content not found");
		abort();
	}
	if(e!=FT_Err_Ok)tlog_error("FT_New_Face error: %s\n",FT_Error_String(e));
	return e;
}

bool lv_freetype_init(uint16_t mf,uint16_t ms,uint32_t mb){
	FT_Error e;
	if((e=FT_Init_FreeType(&lib)))return trlog_error(
		false,
		"init freeType error: %s",
		FT_Error_String(e)
	);
	if(FTC_Manager_New(
		lib,mf,ms,mb,
		font_face_requester,
		NULL,&cmgr
	)){
		FT_Done_FreeType(lib);
		return trlog_error(false,"failed to open cache manager");
	}
	if(FTC_CMapCache_New(cmgr,&cmc)){
		tlog_error("failed to open CMap Cache");
		goto Fail;
	}
	if(FTC_SBitCache_New(cmgr,&sc)){
		tlog_error("failed to open sbit cache");
		goto Fail;
	}
	return true;
	Fail:
	FTC_Manager_Done(cmgr);
	FT_Done_FreeType(lib);
	return false;
}

void lv_freetype_destroy(void){
	FTC_Manager_Done(cmgr);
	FT_Done_FreeType(lib);
}

static bool get_glyph_dsc_cb(
	const lv_font_t*f,
	lv_font_glyph_dsc_t*d,
	uint32_t l,
	uint32_t ln __attribute__((unused))
){
	if(l<0x20){
		d->ofs_x=0,d->ofs_y=0;
		d->box_w=0,d->box_h=0;
		d->adv_w=0,d->bpp=0;
		return true;
	}
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)(f->dsc);
	FT_Face face;
	FTC_ImageTypeRec dst;
	FTC_FaceID fid=(FTC_FaceID)dsc->face_id;
	FTC_Manager_LookupFace(cmgr,fid,&face);
	dst.face_id=fid;
	dst.flags=FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
	dst.height=dsc->height,dst.width=dsc->height;
	FT_UInt gi=FTC_CMapCache_Lookup(cmc,fid,FT_Get_Charmap_Index(face->charmap),l);
	if(FTC_SBitCache_Lookup(sc,&dst,gi,&sb,NULL)!=0)telog_error("SBitCache_Lookup error");
	d->box_h=sb->height,d->box_w=sb->width;
	d->ofs_x=sb->left,d->ofs_y=sb->top-sb->height;
	d->adv_w=sb->xadvance,d->bpp=8;
	return true;
}

static const uint8_t*get_glyph_bitmap_cb(
	const lv_font_t*f __attribute__((unused)),
	uint32_t l __attribute__((unused))
){
	return (const uint8_t*)sb->buffer;
}

static lv_font_t*_lv_ft_init(
	char*name,
	unsigned char*data,
	long length,
	int weight,
	lv_ft_style style
){
	lv_font_fmt_ft_dsc_t*dsc=malloc(sizeof(lv_font_fmt_ft_dsc_t));
	if(!dsc)return NULL;
	if(!(dsc->font=malloc(sizeof(lv_font_t)))) {
		free(dsc);
		return NULL;
	}
	lv_face_info_t*fi=NULL;
	if(!(fi=malloc(sizeof(lv_face_info_t))))goto fail;
	memset(fi,0,sizeof(lv_face_info_t));
	if(data)fi->data=data,fi->length=length;
	else if(name)fi->name=name;
	else goto fail;
	dsc->face_id=fi,dsc->height=weight,dsc->style=style;
	FT_Size fs;
	struct FTC_ScalerRec_ s;
	s.face_id=(FTC_FaceID)dsc->face_id;
	s.width=weight,s.height=weight,s.pixel=1;
	if(FTC_Manager_LookupSize(cmgr,&s,&fs)){
		free(fi);
		tlog_error("failed to LookupSize");
		goto fail;
	}
	lv_font_t*f=dsc->font;
	f->dsc=dsc;
	f->get_glyph_dsc=get_glyph_dsc_cb;
	f->get_glyph_bitmap=get_glyph_bitmap_cb;
	f->subpx=LV_FONT_SUBPX_NONE;
	f->line_height=(fs->face->size->metrics.height>>6);
	f->base_line=-(fs->face->size->metrics.descender>>6);
	f->underline_position=fs->face->underline_position;
	f->underline_thickness=fs->face->underline_thickness;
	return f;
	fail:
	free(dsc->font);
	free(dsc);
	if(name)free(name);
	return NULL;
}

lv_font_t*lv_ft_init(const char*name,int weight,lv_ft_style style){
	return _lv_ft_init(strdup(name),NULL,0,weight,style);
}

lv_font_t*lv_ft_init_data(unsigned char*data,long size,int weight,lv_ft_style style){
	return _lv_ft_init(NULL,data,size,weight,style);
}

lv_font_t*lv_ft_init_assets(entry_dir*assets,char*path,int weight,lv_ft_style style){
	entry_file*f=get_assets_file(assets,path);
	if(!f){
		telog_error("failed to load assets font from %s",path);
		return NULL;
	}
	if(f->length<=0||!S_ISREG(f->info.mode)){
		telog_error("load invalid assets font from %s",path);
		return NULL;
	}
	telog_info("assets font %s size %ld bytes",path,f->length);
	return lv_ft_init_data((unsigned char*)f->content,f->length,weight,style);
}

lv_font_t*lv_ft_init_rootfs(char*path,int weight,lv_ft_style style){
	return lv_ft_init_assets(&assets_rootfs,path,weight,style);
}

void lv_ft_destroy(lv_font_t*font){
	if(!font)return;
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)(font->dsc);
	if(dsc){
		lv_face_info_t*fi=dsc->face_id;
		if(fi){
			if(fi->name)free(fi->name);
			free(fi);
		}
		free(dsc->face_id);
		free(dsc->font);
		free(dsc);
	}
}
#endif
