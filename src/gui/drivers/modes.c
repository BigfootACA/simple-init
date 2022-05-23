/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"gui/guidrv.h"
struct display_mode builtin_modes[]={
	{ .name="540x960",   .width=540,  .height=960  },
	{ .name="1080x1920", .width=1080, .height=1920 },
	{ .name="640x480",   .width=640,  .height=480  },
	{ .name="800x480",   .width=800,  .height=480  },
	{ .name="800x600",   .width=800,  .height=600  },
	{ .name="832x624",   .width=832,  .height=624  },
	{ .name="960x640",   .width=960,  .height=640  },
	{ .name="1024x600",  .width=1024, .height=600  },
	{ .name="1024x768",  .width=1024, .height=768  },
	{ .name="1152x864",  .width=1152, .height=864  },
	{ .name="1152x870",  .width=1152, .height=870  },
	{ .name="1280x720",  .width=1280, .height=720  },
	{ .name="1280x760",  .width=1280, .height=760  },
	{ .name="1280x768",  .width=1280, .height=768  },
	{ .name="1280x800",  .width=1280, .height=800  },
	{ .name="1280x960",  .width=1280, .height=960  },
	{ .name="1280x1024", .width=1280, .height=1024 },
	{ .name="1360x768",  .width=1360, .height=768  },
	{ .name="1366x768",  .width=1366, .height=768  },
	{ .name="1400x1050", .width=1400, .height=1050 },
	{ .name="1440x900",  .width=1440, .height=900  },
	{ .name="1600x900",  .width=1600, .height=900  },
	{ .name="1600x1200", .width=1600, .height=1200 },
	{ .name="1680x1050", .width=1680, .height=1050 },
	{ .name="1920x1080", .width=1920, .height=1080 },
	{ .name="1920x1200", .width=1920, .height=1200 },
	{ .name="1920x1440", .width=1920, .height=1440 },
	{ .name="2000x2000", .width=2000, .height=2000 },
	{ .name="2048x1536", .width=2048, .height=1536 },
	{ .name="2048x2048", .width=2048, .height=2048 },
	{ .name="2560x1440", .width=2560, .height=1440 },
	{ .name="2560x1600", .width=2560, .height=1600 },
	{ .name="2560x2048", .width=2560, .height=2048 },
	{ .name="2800x2100", .width=2800, .height=2100 },
	{ .name="3200x2400", .width=3200, .height=2400 },
	{ .name="3840x2160", .width=3840, .height=2160 },
	{ .name="4096x2160", .width=4096, .height=2160 },
	{ .name="7680x4320", .width=7680, .height=4320 },
	{ .name="8192x4320", .width=8192, .height=4320 },
	{ .name="", .width=0, .height=0 }
};
