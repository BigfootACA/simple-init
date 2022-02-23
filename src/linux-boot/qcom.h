#ifndef _QCOM_H
#define _QCOM_H
#include<stddef.h>
#include<stdint.h>
#include<stdbool.h>
#include"linux_boot.h"
#define DTB_PAD_SIZE 2048
#define DTBO_CUSTOM_MAX 4
#define PLATFORM_FOUNDRY_SHIFT 16
#define SOC_MASK (0xffff)
#define VARIANT_MASK (0x000000ff)
#define VARIANT_MINOR_MASK (0x0000ff00)
#define VARIANT_MAJOR_MASK (0x00ff0000)
#define PMIC_MODEL_MASK (0x000000ff)
#define PMIC_REV_MASK (0xffffff00)
#define PMIC_SHIFT_IDX (2)
#define PLATFORM_SUBTYPE_SHIFT_ID (0x18)
#define FOUNDRY_ID_MASK (0x00ff0000)
#define PLATFORM_SUBTYPE_MASK (0x000000ff)
#define DDR_MASK (0x00000700)
#define DT_V1_SIZE (3*sizeof(uint32_t))

typedef struct dtbo_table_header{
	uint32_t magic;
	uint32_t total_size;
	uint32_t header_size;
	uint32_t dt_entry_size;
	uint32_t dt_entry_count;
	uint32_t dt_entry_offset;
	uint32_t page_size;
	uint32_t reserved[1];
}dtbo_table_header;

typedef struct dtbo_table_entry{
	uint32_t dt_size;
	uint32_t dt_offset;
	uint32_t id;
	uint32_t rev;
	uint32_t custom[DTBO_CUSTOM_MAX];
}dtbo_table_entry;

typedef struct qcom_msm_id{
	uint32_t platform_id;
	uint32_t soc_rev;
}qcom_msm_id;

typedef struct qcom_board_id{
	uint32_t variant_id;
	uint32_t platform_subtype;
}qcom_board_id;

typedef struct qcom_pmic_id{
	uint32_t pmic_version[4];
}qcom_pmic_id;

extern int qcom_parse_id(void*dtb,qcom_chip_info*info);
extern int qcom_dump_info(qcom_chip_info*info);
extern int qcom_get_chip_info(linux_boot*lb,qcom_chip_info*info);
extern int64_t qcom_check_dtb(qcom_chip_info*dtb,qcom_chip_info*chip);
#endif
