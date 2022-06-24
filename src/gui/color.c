/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#ifdef ENABLE_GUI
#include"gui.h"
#include"str.h"
#define DECL_COLOR(n,r,g,b){.valid=true,.name=(#n),.color=LV_COLOR_MAKE(r,g,b)}
static struct _colors{
	bool valid;
	char name[31];
	lv_color_t color;
}colors[]={
	DECL_COLOR(AliceBlue            ,0xF0,0xF8,0xFF),
	DECL_COLOR(AntiqueWhite         ,0xFA,0xEB,0xD7),
	DECL_COLOR(Aqua                 ,0x00,0xFF,0xFF),
	DECL_COLOR(Aquamarine           ,0x7F,0xFF,0xD4),
	DECL_COLOR(Azure                ,0xF0,0xFF,0xFF),
	DECL_COLOR(Beige                ,0xF5,0xF5,0xDC),
	DECL_COLOR(Bisque               ,0xFF,0xE4,0xC4),
	DECL_COLOR(Black                ,0x00,0x00,0x00),
	DECL_COLOR(BlanchedAlmond       ,0xFF,0xEB,0xCD),
	DECL_COLOR(Blue                 ,0x00,0x00,0xFF),
	DECL_COLOR(BlueViolet           ,0x8A,0x2B,0xE2),
	DECL_COLOR(Brown                ,0xA5,0x2A,0x2A),
	DECL_COLOR(BurlyWood            ,0xDE,0xB8,0x87),
	DECL_COLOR(CadetBlue            ,0x5F,0x9E,0xA0),
	DECL_COLOR(Chartreuse           ,0x7F,0xFF,0x00),
	DECL_COLOR(Chocolate            ,0xD2,0x69,0x1E),
	DECL_COLOR(Coral                ,0xFF,0x7F,0x50),
	DECL_COLOR(CornflowerBlue       ,0x64,0x95,0xED),
	DECL_COLOR(Cornsilk             ,0xFF,0xF8,0xDC),
	DECL_COLOR(Crimson              ,0xDC,0x14,0x3C),
	DECL_COLOR(Cyan                 ,0x00,0xFF,0xFF),
	DECL_COLOR(DarkBlue             ,0x00,0x00,0x8B),
	DECL_COLOR(DarkCyan             ,0x00,0x8B,0x8B),
	DECL_COLOR(DarkGoldenRod        ,0xB8,0x86,0x0B),
	DECL_COLOR(DarkGray             ,0xA9,0xA9,0xA9),
	DECL_COLOR(DarkGreen            ,0x00,0x64,0x00),
	DECL_COLOR(DarkKhaki            ,0xBD,0xB7,0x6B),
	DECL_COLOR(DarkMagenta          ,0x8B,0x00,0x8B),
	DECL_COLOR(DarkOliveGreen       ,0x55,0x6B,0x2F),
	DECL_COLOR(Darkorange           ,0xFF,0x8C,0x00),
	DECL_COLOR(DarkOrchid           ,0x99,0x32,0xCC),
	DECL_COLOR(DarkRed              ,0x8B,0x00,0x00),
	DECL_COLOR(DarkSalmon           ,0xE9,0x96,0x7A),
	DECL_COLOR(DarkSeaGreen         ,0x8F,0xBC,0x8F),
	DECL_COLOR(DarkSlateBlue        ,0x48,0x3D,0x8B),
	DECL_COLOR(DarkSlateGray        ,0x2F,0x4F,0x4F),
	DECL_COLOR(DarkTurquoise        ,0x00,0xCE,0xD1),
	DECL_COLOR(DarkViolet           ,0x94,0x00,0xD3),
	DECL_COLOR(DeepPink             ,0xFF,0x14,0x93),
	DECL_COLOR(DeepSkyBlue          ,0x00,0xBF,0xFF),
	DECL_COLOR(DimGray              ,0x69,0x69,0x69),
	DECL_COLOR(DodgerBlue           ,0x1E,0x90,0xFF),
	DECL_COLOR(Feldspar             ,0xD1,0x92,0x75),
	DECL_COLOR(FireBrick            ,0xB2,0x22,0x22),
	DECL_COLOR(FloralWhite          ,0xFF,0xFA,0xF0),
	DECL_COLOR(ForestGreen          ,0x22,0x8B,0x22),
	DECL_COLOR(Fuchsia              ,0xFF,0x00,0xFF),
	DECL_COLOR(Gainsboro            ,0xDC,0xDC,0xDC),
	DECL_COLOR(GhostWhite           ,0xF8,0xF8,0xFF),
	DECL_COLOR(Gold                 ,0xFF,0xD7,0x00),
	DECL_COLOR(GoldenRod            ,0xDA,0xA5,0x20),
	DECL_COLOR(Gray                 ,0x80,0x80,0x80),
	DECL_COLOR(Green                ,0x00,0x80,0x00),
	DECL_COLOR(GreenYellow          ,0xAD,0xFF,0x2F),
	DECL_COLOR(HoneyDew             ,0xF0,0xFF,0xF0),
	DECL_COLOR(HotPink              ,0xFF,0x69,0xB4),
	DECL_COLOR(IndianRed            ,0xCD,0x5C,0x5C),
	DECL_COLOR(Indigo               ,0x4B,0x00,0x82),
	DECL_COLOR(Ivory                ,0xFF,0xFF,0xF0),
	DECL_COLOR(Khaki                ,0xF0,0xE6,0x8C),
	DECL_COLOR(Lavender             ,0xE6,0xE6,0xFA),
	DECL_COLOR(LavenderBlush        ,0xFF,0xF0,0xF5),
	DECL_COLOR(LawnGreen            ,0x7C,0xFC,0x00),
	DECL_COLOR(LemonChiffon         ,0xFF,0xFA,0xCD),
	DECL_COLOR(LightBlue            ,0xAD,0xD8,0xE6),
	DECL_COLOR(LightCoral           ,0xF0,0x80,0x80),
	DECL_COLOR(LightCyan            ,0xE0,0xFF,0xFF),
	DECL_COLOR(LightGoldenRodYellow ,0xFA,0xFA,0xD2),
	DECL_COLOR(LightGrey            ,0xD3,0xD3,0xD3),
	DECL_COLOR(LightGreen           ,0x90,0xEE,0x90),
	DECL_COLOR(LightPink            ,0xFF,0xB6,0xC1),
	DECL_COLOR(LightSalmon          ,0xFF,0xA0,0x7A),
	DECL_COLOR(LightSeaGreen        ,0x20,0xB2,0xAA),
	DECL_COLOR(LightSkyBlue         ,0x87,0xCE,0xFA),
	DECL_COLOR(LightSlateBlue       ,0x84,0x70,0xFF),
	DECL_COLOR(LightSlateGray       ,0x77,0x88,0x99),
	DECL_COLOR(LightSteelBlue       ,0xB0,0xC4,0xDE),
	DECL_COLOR(LightYellow          ,0xFF,0xFF,0xE0),
	DECL_COLOR(Lime                 ,0x00,0xFF,0x00),
	DECL_COLOR(LimeGreen            ,0x32,0xCD,0x32),
	DECL_COLOR(Linen                ,0xFA,0xF0,0xE6),
	DECL_COLOR(Magenta              ,0xFF,0x00,0xFF),
	DECL_COLOR(Maroon               ,0x80,0x00,0x00),
	DECL_COLOR(MediumAquaMarine     ,0x66,0xCD,0xAA),
	DECL_COLOR(MediumBlue           ,0x00,0x00,0xCD),
	DECL_COLOR(MediumOrchid         ,0xBA,0x55,0xD3),
	DECL_COLOR(MediumPurple         ,0x93,0x70,0xD8),
	DECL_COLOR(MediumSeaGreen       ,0x3C,0xB3,0x71),
	DECL_COLOR(MediumSlateBlue      ,0x7B,0x68,0xEE),
	DECL_COLOR(MediumSpringGreen    ,0x00,0xFA,0x9A),
	DECL_COLOR(MediumTurquoise      ,0x48,0xD1,0xCC),
	DECL_COLOR(MediumVioletRed      ,0xC7,0x15,0x85),
	DECL_COLOR(MidnightBlue         ,0x19,0x19,0x70),
	DECL_COLOR(MintCream            ,0xF5,0xFF,0xFA),
	DECL_COLOR(MistyRose            ,0xFF,0xE4,0xE1),
	DECL_COLOR(Moccasin             ,0xFF,0xE4,0xB5),
	DECL_COLOR(NavajoWhite          ,0xFF,0xDE,0xAD),
	DECL_COLOR(Navy                 ,0x00,0x00,0x80),
	DECL_COLOR(OldLace              ,0xFD,0xF5,0xE6),
	DECL_COLOR(Olive                ,0x80,0x80,0x00),
	DECL_COLOR(OliveDrab            ,0x6B,0x8E,0x23),
	DECL_COLOR(Orange               ,0xFF,0xA5,0x00),
	DECL_COLOR(OrangeRed            ,0xFF,0x45,0x00),
	DECL_COLOR(Orchid               ,0xDA,0x70,0xD6),
	DECL_COLOR(PaleGoldenRod        ,0xEE,0xE8,0xAA),
	DECL_COLOR(PaleGreen            ,0x98,0xFB,0x98),
	DECL_COLOR(PaleTurquoise        ,0xAF,0xEE,0xEE),
	DECL_COLOR(PaleVioletRed        ,0xD8,0x70,0x93),
	DECL_COLOR(PapayaWhip           ,0xFF,0xEF,0xD5),
	DECL_COLOR(PeachPuff            ,0xFF,0xDA,0xB9),
	DECL_COLOR(Peru                 ,0xCD,0x85,0x3F),
	DECL_COLOR(Pink                 ,0xFF,0xC0,0xCB),
	DECL_COLOR(Plum                 ,0xDD,0xA0,0xDD),
	DECL_COLOR(PowderBlue           ,0xB0,0xE0,0xE6),
	DECL_COLOR(Purple               ,0x80,0x00,0x80),
	DECL_COLOR(Red                  ,0xFF,0x00,0x00),
	DECL_COLOR(RosyBrown            ,0xBC,0x8F,0x8F),
	DECL_COLOR(RoyalBlue            ,0x41,0x69,0xE1),
	DECL_COLOR(SaddleBrown          ,0x8B,0x45,0x13),
	DECL_COLOR(Salmon               ,0xFA,0x80,0x72),
	DECL_COLOR(SandyBrown           ,0xF4,0xA4,0x60),
	DECL_COLOR(SeaGreen             ,0x2E,0x8B,0x57),
	DECL_COLOR(SeaShell             ,0xFF,0xF5,0xEE),
	DECL_COLOR(Sienna               ,0xA0,0x52,0x2D),
	DECL_COLOR(Silver               ,0xC0,0xC0,0xC0),
	DECL_COLOR(SkyBlue              ,0x87,0xCE,0xEB),
	DECL_COLOR(SlateBlue            ,0x6A,0x5A,0xCD),
	DECL_COLOR(SlateGray            ,0x70,0x80,0x90),
	DECL_COLOR(Snow                 ,0xFF,0xFA,0xFA),
	DECL_COLOR(SpringGreen          ,0x00,0xFF,0x7F),
	DECL_COLOR(SteelBlue            ,0x46,0x82,0xB4),
	DECL_COLOR(Tan                  ,0xD2,0xB4,0x8C),
	DECL_COLOR(Teal                 ,0x00,0x80,0x80),
	DECL_COLOR(Thistle              ,0xD8,0xBF,0xD8),
	DECL_COLOR(Tomato               ,0xFF,0x63,0x47),
	DECL_COLOR(Turquoise            ,0x40,0xE0,0xD0),
	DECL_COLOR(Violet               ,0xEE,0x82,0xEE),
	DECL_COLOR(VioletRed            ,0xD0,0x20,0x90),
	DECL_COLOR(Wheat                ,0xF5,0xDE,0xB3),
	DECL_COLOR(White                ,0xFF,0xFF,0xFF),
	DECL_COLOR(WhiteSmoke           ,0xF5,0xF5,0xF5),
	DECL_COLOR(Yellow               ,0xFF,0xFF,0x00),
	DECL_COLOR(YellowGreen          ,0x9A,0xCD,0x32),
	{false,"",LV_COLOR_MAKE(0,0,0)}
};

static bool resolve_color_name(const char*val,lv_color_t*color){
	for(size_t i=0;colors[i].valid;i++){
		if(strcasecmp(val,colors[i].name)!=0)continue;
		memcpy(color,&colors[i].color,sizeof(lv_color_t));
		return true;
	}
	return false;
}

static uint8_t hex2int(char c){
	errno=0;
	if(c>='0'&&c<='9')return c-'0';
	if(c>='a'&&c<='f')return c-'a'+10;
	if(c>='A'&&c<='F')return c-'A'+10;
	errno=ERANGE;
	return 0;
}

static bool parse_color_int(const char*val,lv_color_t*color){
	bool res=true;
	char*v=(char*)val;
	size_t cnt=strlen(val);
	uint8_t r=0,g=0,b=0;
	if(cnt<3)return false;
	if(strncasecmp(v,"0x",2)==0)v+=2,cnt-=2;
	else if(*v=='#')v++,cnt--;
	if(cnt==8)v+=2,cnt-=2;
	if(cnt==3){
		r=hex2int(*v),v++;
		g=hex2int(*v),v++;
		b=hex2int(*v),v++;
	}else if(cnt==6){
		r=hex2int(*v),v++,r<<=4;
		r+=hex2int(*v),v++;
		g=hex2int(*v),v++,g<<=4;
		g+=hex2int(*v),v++;
		b=hex2int(*v),v++,b<<=4;
		b+=hex2int(*v),v++;
	}else res=false;
	if(res){
		color->ch.red=r;
		color->ch.green=g;
		color->ch.blue=b;
	}
	return res;
}

bool lv_parse_color_string(const char*val,lv_color_t*color){
	if(!val||!color)return false;
	memset(color,0,sizeof(lv_color_t));
	if(resolve_color_name(val,color))return true;
	if(parse_color_int(val,color))return true;
	return false;
}
#endif
