/*
 *
 * Copyright (C) 2021 BigfootACA <bigfoot@classfun.cn>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 */

#include"internal.h"

extern locate_match_state locate_match_device_path(locate_dest*loc);
extern locate_match_state locate_match_mbr_type(locate_dest*loc);
extern locate_match_state locate_match_mbr_active(locate_dest*loc);
extern locate_match_state locate_match_gpt_name(locate_dest*loc);
extern locate_match_state locate_match_gpt_guid(locate_dest*loc);
extern locate_match_state locate_match_gpt_type(locate_dest*loc);
extern locate_match_state locate_match_disk_label(locate_dest*loc);
extern locate_match_state locate_match_esp(locate_dest*loc);
extern locate_match_state locate_match_fs_name(locate_dest*loc);
extern locate_match_state locate_match_file(locate_dest*loc);
locate_match locate_matches[]={
	locate_match_device_path,
	locate_match_esp,
	locate_match_disk_label,
	locate_match_mbr_type,
	locate_match_mbr_active,
	locate_match_gpt_name,
	locate_match_gpt_guid,
	locate_match_gpt_type,
	locate_match_fs_name,
	locate_match_file,
	NULL
};
