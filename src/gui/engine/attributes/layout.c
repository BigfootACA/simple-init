/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#ifdef ENABLE_MXML
#include"gui/tools.h"
#include"gui/string.h"
#include"../render_internal.h"

bool xml_attr_handle_apply_flex_flow(xml_render_obj_attr*obj){
	lv_flex_flow_t flow=LV_FLEX_FLOW_ROW;
	if(!obj->obj->parent)return false;
	if(!lv_name_to_flex_flow(obj->value,&flow))
		return trlog_warn(false,"invalid flex flow %s",obj->value);
	lv_obj_set_flex_flow(obj->obj->obj,flow);
	return true;
}

bool xml_attr_handle_apply_flex_grow(xml_render_obj_attr*obj){
	if(!obj->obj->parent)return false;
	lv_obj_set_flex_grow(obj->obj->obj,render_resolve_coord(
		obj->obj,UINT8_MAX,obj->value
	));
	return true;
}

bool xml_attr_handle_apply_flex_align(xml_render_obj_attr*obj){
	char buffer[256],*nm=NULL,*nc=NULL,*nt=NULL,*c;
	lv_flex_align_t main=LV_FLEX_ALIGN_CENTER;
	lv_flex_align_t cross=LV_FLEX_ALIGN_CENTER;
	lv_flex_align_t track=LV_FLEX_ALIGN_CENTER;
	if(!obj->obj->parent)return false;
	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,obj->value,sizeof(buffer)-1);
	if((c=strchr(buffer,':')))*c=0,nm=buffer,nc=c+1;
	else return trlog_warn(false,"invalid flex align %s",obj->value);
	if((c=strchr(nc,':')))*c=0,nt=c+1;
	else return trlog_warn(false,"invalid flex align %s",obj->value);
	if(nm&&nm[0]&&!lv_name_to_flex_align(nm,&main))
		return trlog_warn(false,"invalid flex align main %s",nm);
	if(nc&&nc[0]&&!lv_name_to_flex_align(nc,&cross))
		return trlog_warn(false,"invalid flex align cross %s",nc);
	if(nt&&nt[0]&&!lv_name_to_flex_align(nt,&track))
		return trlog_warn(false,"invalid flex align track cross %s",nt);
	lv_obj_set_flex_align(obj->obj->obj,main,cross,track);
	return true;
}

bool xml_attr_handle_apply_grid_align(xml_render_obj_attr*obj){
	char buffer[256],*nc=NULL,*nr=NULL,*c;
	lv_grid_align_t col=LV_GRID_ALIGN_STRETCH;
	lv_grid_align_t row=LV_GRID_ALIGN_STRETCH;
	if(!obj->obj->parent)return false;
	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,obj->value,sizeof(buffer)-1);
	if((c=strchr(buffer,':')))*c=0,nc=buffer,nr=c+1;
	else return trlog_warn(false,"invalid grid cell %s",obj->value);
	if(nc&&nc[0]&&!lv_name_to_grid_align(nc,&col))
		return trlog_warn(false,"invalid grid align col %s",nc);
	if(nr&&nr[0]&&!lv_name_to_grid_align(nr,&row))
		return trlog_warn(false,"invalid grid align row %s",nr);
	lv_obj_set_grid_align(obj->obj->obj,col,row);
	return true;
}

static lv_coord_t*parse_grid_dsc(xml_render_obj*obj,char*value){
	char*c,*p=value;
	size_t cnt=1,size;
	lv_coord_t*dsc=NULL;
	if(!obj||!value||!value[0])return NULL;
	trim(value);
	cnt+=strcnt(value,"|"),size=(cnt+2)*sizeof(lv_coord_t);
	if(!(dsc=malloc(size)))EDONE(tlog_warn("alloc grid dsc failed"));
	for(size_t i=0;i<cnt;i++){
		if((c=strchr(p,'|')))*c=0;
		if((dsc[i]=render_resolve_coord(obj,LV_GRID_TEMPLATE_LAST,p))==0)
			EDONE(tlog_warn("invalid grid dsc coord %s",p));
		if(c)p=c+1;
	}
	dsc[cnt]=LV_GRID_TEMPLATE_LAST,dsc[cnt+1]=0;
	return dsc;
	done:
	if(dsc)free(dsc);
	return NULL;
}

bool xml_attr_handle_pre_grid_dsc(xml_render_obj_attr*obj){
	char*c,*str=NULL,*nc,*nr;
	lv_coord_t*col=NULL,*row=NULL;
	if(!obj->obj->parent)return false;
	if(obj->obj->grid_dsc_row||obj->obj->grid_dsc_col)return true;
	if(!(str=strdup(obj->value)))return false;
	if(!(c=strchr(str,'@')))EDONE(tlog_warn("invalid grid dsc %s",str));
	*c=0,nc=str,nr=c+1;
	if(!(col=parse_grid_dsc(obj->obj,nc)))
		EDONE(tlog_warn("invalid grid dsc col %s",nc));
	if(!(row=parse_grid_dsc(obj->obj,nr)))
		EDONE(tlog_warn("invalid grid dsc row %s",nr));
	obj->obj->grid_dsc_col=col;
	obj->obj->grid_dsc_row=row;
	free(str);
	return true;
	done:
	if(str)free(str);
	if(col)free(col);
	if(row)free(row);
	return false;
}

bool xml_attr_handle_apply_grid_dsc(xml_render_obj_attr*obj){
	if(
		!obj->obj->parent||
		!obj->obj->grid_dsc_col||
		!obj->obj->grid_dsc_row
	)return false;
	lv_obj_set_grid_dsc_array(
		obj->obj->obj,
		obj->obj->grid_dsc_col,
		obj->obj->grid_dsc_row
	);
	return true;
}

static bool parse_cell(char*val,lv_grid_align_t*align,uint8_t*np,uint8_t*ns){
	char*c=NULL,*pos=NULL,*span=NULL;
	if((c=strchr(val,'@'))){
		*c=0,pos=c+1;
		if((c=strchr(pos,',')))*c=0,span=c+1;
		if(pos&&pos[0])*np=parse_int(pos,*np);
		if(span&&span[0])*ns=parse_int(span,*ns);
		pos=NULL,span=NULL;
	}
	if(val&&val[0]&&!lv_name_to_grid_align(val,align))
		return trlog_warn(false,"invalid grid align %s",val);
	return true;
}

bool xml_attr_handle_apply_grid_cell(xml_render_obj_attr*obj){
	size_t cnt_col=0,cnt_row=0;
	xml_render_obj*o=obj->obj,*a=o->parent;
	lv_grid_align_t col=LV_GRID_ALIGN_STRETCH;
	lv_grid_align_t row=LV_GRID_ALIGN_STRETCH;
	char buffer[256],*nc=NULL,*nr=NULL,*c=NULL;
	uint8_t cp=a->last_col,cs=1,rp=a->last_row,rs=1;
	if(!a||!a->grid_dsc_col||!a->grid_dsc_row)return false;
	while(a->grid_dsc_col[cnt_col]!=LV_GRID_TEMPLATE_LAST)cnt_col++;
	while(a->grid_dsc_row[cnt_row]!=LV_GRID_TEMPLATE_LAST)cnt_row++;
	memset(buffer,0,sizeof(buffer));
	strncpy(buffer,obj->value,sizeof(buffer)-1);
	if((c=strchr(buffer,':'))){
		*c=0,nc=buffer,nr=c+1;
		if(!parse_cell(nc,&col,&cp,&cs))return false;
		if(!parse_cell(nr,&row,&rp,&rs))return false;
	}else{
		uint8_t p=0,s=1;
		lv_grid_align_t align=LV_GRID_ALIGN_STRETCH;
		if(!parse_cell(buffer,&align,&p,&s))return false;
		col=align,row=align;
		if(p!=0)cp=p,rp=p;
		if(s!=1)cs=s,rs=s;
	}
	lv_obj_set_grid_cell(o->obj,col,cp,cs,row,rp,rs);
	if(o->id[0])tlog_debug(
		"%s cell "
		"align %s pos %d span %d "
		"align %s pos %d span %d",
		o->id,
		lv_grid_align_to_name(col),cp,cs,
		lv_grid_align_to_name(row),rp,rs
	);
	cp+=cs;
	if(cp>=cnt_col)cp=0,rp++;
	a->last_col=cp,a->last_row=rp;
	return true;
}
#endif
#endif
