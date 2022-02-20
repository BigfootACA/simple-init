/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _EFIPLATFORMINFO_H
#define _EFIPLATFORMINFO_H
typedef enum {
  PLATFORM_INFO_TYPE_UNKNOWN = 0x00,     /**< Unknown target device. */
  PLATFORM_INFO_TYPE_CDP = 0x01,         /**< CDP device. */
  PLATFORM_INFO_TYPE_FFA = 0x02,         /**< Form-fit accurate device. */
  PLATFORM_INFO_TYPE_FLUID = 0x03,       /**< Forward looking user interface demonstration device. */
  PLATFORM_INFO_TYPE_OEM = 0x05,         /**< Original equipment manufacturer device. */
  PLATFORM_INFO_TYPE_QT = 0x06,          /**< Qualcomm tablet device. */
  PLATFORM_INFO_TYPE_MTP = 0x08,         /**< MTP device. */
  PLATFORM_INFO_TYPE_LIQUID = 0x09,      /**< LiQUID device. */
  PLATFORM_INFO_TYPE_DRAGONBOARD = 0x0A, /**< DragonBoard@tm device. */
  PLATFORM_INFO_TYPE_QRD = 0x0B,         /**< QRD device. */
  PLATFORM_INFO_TYPE_EVB = 0x0C,         /**< EVB device. */
  PLATFORM_INFO_TYPE_HRD = 0x0D,         /**< HRD device. */
  PLATFORM_INFO_TYPE_DTV = 0x0E,  /**< DTV device. */
  PLATFORM_INFO_TYPE_RUMI = 0x0F, /**< Target is on Rumi (ASIC emulation). */
  PLATFORM_INFO_TYPE_VIRTIO = 0x10,  /**< Target is on Virtio (system-level simulation). */
  PLATFORM_INFO_TYPE_GOBI = 0x11, /**< Gobi@tm device. */
  PLATFORM_INFO_TYPE_CBH  = 0x12,  /**< CBH device. */
  PLATFORM_INFO_TYPE_BTS = 0x13,  /**< BTS device. */
  PLATFORM_INFO_TYPE_XPM = 0x14,  /**< XPM device. */
  PLATFORM_INFO_TYPE_RCM = 0x15,  /**< RCM device. */
  PLATFORM_INFO_TYPE_DMA = 0x16,  /**< DMA device. */
  PLATFORM_INFO_TYPE_STP = 0x17,  /**< STP device. */
  PLATFORM_INFO_TYPE_SBC = 0x18,  /**< SBC device. */
  PLATFORM_INFO_TYPE_ADP = 0x19,  /**< ADP device. */
  PLATFORM_INFO_TYPE_CHI = 0x1A,  /**< CHI device. */
  PLATFORM_INFO_TYPE_SDP = 0x1B,  /**< SDP device. */
  PLATFORM_INFO_TYPE_RRP = 0x1C,  /**< RRP device. */
  PLATFORM_INFO_TYPE_CLS = 0x1D,  /**< CLS device. */
  PLATFORM_INFO_TYPE_TTP = 0x1E,  /**< TTP device. */
  PLATFORM_INFO_TYPE_HDK = 0x1F,  /**< HDK device. */
  PLATFORM_INFO_TYPE_IOT = 0x20,  /**< IOT device. */
  PLATFORM_INFO_TYPE_ATP = 0x21,  /**< ATP device. */
  PLATFORM_INFO_TYPE_IDP = 0x22,  /**< IDP device. */
  PLATFORM_INFO_NUM_TYPES,
  PLATFORM_INFO_TYPE_32BITS = 0x7FFFFFFF
} PLATFORM_TYPE;
typedef enum {
  PLATFORM_INFO_KEY_UNKNOWN = 0x00,
  PLATFORM_INFO_KEY_DDR_FREQ = 0x01,
  PLATFORM_INFO_KEY_GFX_FREQ = 0x02,
  PLATFORM_INFO_KEY_CAMERA_FREQ = 0x03,
  PLATFORM_INFO_KEY_FUSION = 0x04,
  PLATFORM_INFO_KEY_CUST = 0x05,
  PLATFORM_INFO_NUM_KEYS = 0x06,
  PLATFORM_INFO_KEY_32BITS = 0x7FFFFFFF
} PLATFORM_INFO_KEY_TYPE;
#define PLATFORM_INFO_PROTOCOL_VERSION 0x0000000000020000
#define PLATFORM_INFO_PROTOCOL_GUID { 0x157a5c45, 0x21b2, 0x43c5, { 0xba, 0x7c, 0x82, 0x2f, 0xee, 0x5f, 0xe5, 0x99 } }
extern EFI_GUID gEfiPlatformInfoProtocolGuid;
typedef struct _PLATFORM_INFO_PROTOCOL PLATFORM_INFO_PROTOCOL;
typedef struct {
	PLATFORM_TYPE platform;
	UINT32 version;
	UINT32 subtype;
	BOOLEAN fusion;
} PLATFORM_INFO_TYPE;
typedef EFI_STATUS (EFIAPI *PLATFORM_INFO_GET_INFO) (
	IN PLATFORM_INFO_PROTOCOL *This,
	OUT PLATFORM_INFO_TYPE *PlatformInfo
);
typedef EFI_STATUS (EFIAPI *PLATFORM_INFO_GET_KEYVALUE) (
	IN PLATFORM_INFO_PROTOCOL *This,
	IN PLATFORM_INFO_KEY_TYPE Key,
	OUT UINT32 *Value
);
struct _PLATFORM_INFO_PROTOCOL {
	UINT64 Version;
	PLATFORM_INFO_GET_INFO GetPlatformInfo;
	PLATFORM_INFO_GET_KEYVALUE GetKeyValue;
};
#endif
