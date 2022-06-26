#define _GNU_SOURCE
#ifdef ENABLE_GUI
#ifdef ENABLE_FREETYPE2
#include<ft2build.h>
#include<sys/stat.h>
#include"gui.h"
#include"assets.h"
#include"logger.h"
#include"gui/font.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_CACHE_H
#include FT_SIZES_H
#include FT_OUTLINE_H
#define TAG "font"
typedef struct{
	const char*name;
	const void*mem;
	size_t mem_size;
	lv_font_t*font;
	uint16_t weight;
	uint16_t style;
}lv_ft_info_t;
typedef struct name_refer_t{
	const char*name;
	int32_t cnt;
}name_refer_t;
typedef struct{
	const void*mem;
	const char*name;
	size_t mem_size;
	lv_font_t*font;
	uint16_t style;
	uint16_t height;
	FT_Face face;
}lv_font_fmt_ft_dsc_t;
static FT_Library library;
static lv_ll_t names_ll;
static FTC_Manager cache_manager;
static FTC_CMapCache cmap_cache;
static FTC_SBitCache sbit_cache;
static FTC_SBit sbit;
static FT_Error font_face_requester(
	FTC_FaceID face_id,
	FT_Library library_is __attribute__((unused)),
	FT_Pointer req_data __attribute__((unused)),
	FT_Face*aface
){
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)face_id;
	FT_Error e=dsc->mem?
		   FT_New_Memory_Face(library,dsc->mem,dsc->mem_size,0,aface):
		   FT_New_Face(library,dsc->name,0,aface);
	if(e)tlog_error("new face error: %s",FT_Error_String(e));
	return e;
}

bool lv_freetype_init(uint16_t mf,uint16_t ms,uint32_t mb){
	FT_Error e;
	if((e=FT_Init_FreeType(&library)))return trlog_error(
		false,
		"init freetype error: %s",
		FT_Error_String(e)
	);
	_lv_ll_init(&names_ll,sizeof(name_refer_t));
	;
	if((e=FTC_Manager_New(
		library,mf,ms,mb,
		font_face_requester,
		NULL,
		&cache_manager
	)))EDONE(tlog_error(
		"failed to open cache manager: %s",
		FT_Error_String(e)
	));
	if((e=FTC_CMapCache_New(
		cache_manager,&cmap_cache
	)))EDONE(tlog_error(
		"failed to open CMap Cache: %s",
		FT_Error_String(e)
	));
	if((e=FTC_SBitCache_New(
		cache_manager,&sbit_cache
	)))EDONE(tlog_error(
		"failed to open SBit cache: %s",
		FT_Error_String(e)
	));
	return true;
	done:
	if(cache_manager)FTC_Manager_Done(cache_manager);
	if(library)FT_Done_FreeType(library);
	cache_manager=NULL,library=NULL;
	return false;
}

void lv_freetype_destroy(void){
	FTC_Manager_Done(cache_manager);
	FT_Done_FreeType(library);
}

static bool get_bold_glyph(
	const lv_font_t*font,
	FT_Face face,
	FT_UInt glyph_index,
	lv_font_glyph_dsc_t*dsc_out
){
	if(FT_Load_Glyph(face,glyph_index,FT_LOAD_DEFAULT))return false;
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)(font->dsc);
	if(
		face->glyph->format==FT_GLYPH_FORMAT_OUTLINE&&
		dsc->style&FT_FONT_STYLE_BOLD
	)FT_Outline_Embolden(&face->glyph->outline,1<<6);
	if(FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL))return false;
	dsc_out->adv_w=(face->glyph->metrics.horiAdvance>>6);
	dsc_out->box_h=face->glyph->bitmap.rows;
	dsc_out->box_w=face->glyph->bitmap.width;
	dsc_out->ofs_x=face->glyph->bitmap_left;
	dsc_out->ofs_y=face->glyph->bitmap_top-face->glyph->bitmap.rows;
	dsc_out->bpp=8;
	return true;
}

static bool get_glyph_dsc_cb(
	const lv_font_t*font,
	lv_font_glyph_dsc_t*dsc_out,
	uint32_t unicode_letter,
	uint32_t unicode_letter_next __attribute__((unused))
){
	FT_Error e;
	if(unicode_letter<0x20){
		dsc_out->adv_w=0;
		dsc_out->box_h=0;
		dsc_out->box_w=0;
		dsc_out->ofs_x=0;
		dsc_out->ofs_y=0;
		dsc_out->bpp=0;
		return true;
	}
	lv_font_fmt_ft_dsc_t*dsc=
		(lv_font_fmt_ft_dsc_t*)(font->dsc);
	FTC_FaceID face_id=(FTC_FaceID)dsc;
	FT_Size face_size;
	struct FTC_ScalerRec_ scaler;
	scaler.face_id=face_id;
	scaler.width=dsc->height;
	scaler.height=dsc->height;
	scaler.pixel=1;
	if(FTC_Manager_LookupSize(
		cache_manager,
		&scaler,&face_size
	))return false;
	FT_Face face=face_size->face;
	FT_UInt charmap_index=FT_Get_Charmap_Index(face->charmap);
	FT_UInt glyph_index=FTC_CMapCache_Lookup(
		cmap_cache,face_id,
		charmap_index,unicode_letter
	);
	dsc_out->is_placeholder=glyph_index==0;
	if(dsc->style&FT_FONT_STYLE_ITALIC){
		FT_Matrix italic_matrix;
		italic_matrix.xx=1<<16;
		italic_matrix.xy=0x5800;
		italic_matrix.yx=0;
		italic_matrix.yy=1<<16;
		FT_Set_Transform(face,&italic_matrix,NULL);
	}
	if(dsc->style&FT_FONT_STYLE_BOLD){
		dsc->face=face;
		if(!get_bold_glyph(font,face,glyph_index,dsc_out)){
			dsc->face=NULL;
			return false;
		}
		goto end;
	}
	FTC_ImageTypeRec desc_type;
	desc_type.face_id=face_id;
	desc_type.flags=FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
	desc_type.height=dsc->height;
	desc_type.width=dsc->height;
	if((e=FTC_SBitCache_Lookup(
		sbit_cache,&desc_type,
		glyph_index,&sbit,NULL
	))){
		tlog_error(
			"S-Bit cache lookup error: %s",
			FT_Error_String(e)
		);
		return false;
	}
	dsc_out->adv_w=sbit->xadvance;
	dsc_out->box_h=sbit->height;
	dsc_out->box_w=sbit->width;
	dsc_out->ofs_x=sbit->left;
	dsc_out->ofs_y=sbit->top-sbit->height;
	dsc_out->bpp=8;
	end:
	if(
		(dsc->style&FT_FONT_STYLE_ITALIC)&&
		(unicode_letter_next=='\0')
	)dsc_out->adv_w=dsc_out->box_w+dsc_out->ofs_x;
	return true;
}

static const uint8_t*get_glyph_bitmap_cb(
	const lv_font_t*font,
	uint32_t ul __attribute__((unused))
){
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)(font->dsc);
	if(dsc->style&FT_FONT_STYLE_BOLD){
		if(dsc->face&&dsc->face->glyph->format==FT_GLYPH_FORMAT_BITMAP)
			return (const uint8_t*)(dsc->face->glyph->bitmap.buffer);
		return NULL;
	}
	return (const uint8_t*)sbit->buffer;
}
static const char*name_refer_find(const char*name){
	name_refer_t*refer=_lv_ll_get_head(&names_ll);
	while(refer){
		if(strcmp(refer->name,name)==0){
			refer->cnt+=1;
			return refer->name;
		}
		refer=_lv_ll_get_next(&names_ll,refer);
	}
	return NULL;
}
static const char*name_refer_save(const char*name){
	const char*pos=name_refer_find(name);
	if(pos)return pos;
	name_refer_t*refer=_lv_ll_ins_tail(&names_ll);
	if(refer){
		uint32_t len=strlen(name)+1;
		refer->name=lv_mem_alloc(len);
		if(refer->name){
			lv_memcpy(
				(void*)refer->name,
				name,len
			);
			refer->cnt=1;
			return refer->name;
		}
		_lv_ll_remove(&names_ll,refer);
		lv_mem_free(refer);
	}
	return "";
}

static lv_font_t*_lv_ft_init(
	char*name,
	unsigned char*data,
	long length,
	int weight,
	lv_ft_style style
){
	FT_Error e;
	size_t need_size=
		sizeof(lv_font_fmt_ft_dsc_t)+
		sizeof(lv_font_t);
	lv_font_fmt_ft_dsc_t*dsc=lv_mem_alloc(need_size);
	if(!dsc)return false;
	lv_memset_00(dsc,need_size);
	dsc->font=
		(lv_font_t*)(((char*)dsc)+
		sizeof(lv_font_fmt_ft_dsc_t));
	if(data)dsc->mem=data,dsc->mem_size=length;
	else if(name)dsc->name=name_refer_save(name);
	else{
		lv_mem_free(dsc);
		return NULL;
	}
	dsc->height=weight;
	dsc->style=style;
	FT_Size face_size;
	struct FTC_ScalerRec_ scaler;
	scaler.face_id=(FTC_FaceID)dsc;
	scaler.width=weight;
	scaler.height=weight;
	scaler.pixel=1;
	if((e=FTC_Manager_LookupSize(
		cache_manager,&scaler,&face_size
	))){
		tlog_error(
			"failed to lookup size: %s",
			FT_Error_String(e)
		);
		lv_mem_free(dsc);
		return NULL;
	}
	lv_font_t*font=dsc->font;
	font->dsc=dsc;
	font->get_glyph_dsc=get_glyph_dsc_cb;
	font->get_glyph_bitmap=get_glyph_bitmap_cb;
	font->subpx=LV_FONT_SUBPX_NONE;
	font->line_height=(face_size->face->size->metrics.height>>6);
	font->base_line=-(face_size->face->size->metrics.descender>>6);
	FT_Fixed scale=face_size->face->size->metrics.y_scale;
	int8_t thickness=FT_MulFix(scale,face_size->face->underline_thickness)>>6;
	font->underline_position=FT_MulFix(scale,face_size->face->underline_position)>>6;
	font->underline_thickness=thickness<1?1:thickness;
	return font;
}
static void name_refer_del(const char*name){
	name_refer_t*refer=_lv_ll_get_head(&names_ll);
	while(refer){
		if(strcmp(refer->name,name)==0){
			refer->cnt-=1;
			if(refer->cnt<=0){
				_lv_ll_remove(&names_ll,refer);
				lv_mem_free((void*)refer->name);
				lv_mem_free(refer);
			}
			return;
		}
		refer=_lv_ll_get_next(&names_ll,refer);
	}
}
void lv_ft_font_destroy(lv_font_t*font){
	if(!font)return;
	lv_font_fmt_ft_dsc_t*dsc=(lv_font_fmt_ft_dsc_t*)(font->dsc);
	if(dsc){
		FTC_Manager_RemoveFaceID(cache_manager,(FTC_FaceID)dsc);
		name_refer_del(dsc->name);
		lv_mem_free(dsc);
	}
}

lv_font_t*lv_ft_init(const char*name,int weight,lv_ft_style style){
	return name?_lv_ft_init(strdup(name),NULL,0,weight,style):NULL;
}

lv_font_t*lv_ft_init_data(unsigned char*data,long size,int weight,lv_ft_style style){
	return data?_lv_ft_init(NULL,data,size,weight,style):NULL;
}

lv_font_t*lv_ft_init_assets(entry_dir*assets,char*path,int weight,lv_ft_style style){
	if(!assets||!path)return NULL;
	entry_file*f=get_assets_file(assets,path);
	if(!f){
		telog_error("failed to load assets font from %s",path);
		return NULL;
	}
	if(f->length<=0||!S_ISREG(f->info.mode)){
		telog_error("load invalid assets font from %s",path);
		return NULL;
	}
	telog_info("assets font %s size %zu bytes",path,f->length);
	return lv_ft_init_data((unsigned char*)f->content,f->length,weight,style);
}

lv_font_t*lv_ft_init_rootfs(char*path,int weight,lv_ft_style style){
	return lv_ft_init_assets(&assets_rootfs,path,weight,style);
}

#endif
#endif
