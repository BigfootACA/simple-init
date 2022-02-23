/* Copyright (c) 2015-2016, 2018 The Linux Foundation. All rights reserved.
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

#ifndef __EFIRAMPARTITION_H__
#define __EFIRAMPARTITION_H__
typedef struct _EFI_RAMPARTITION_PROTOCOL EFI_RAMPARTITION_PROTOCOL;
#define EFI_RAMPARTITION_PROTOCOL_REVISION 0x0000000000010001
#define EFI_RAMPARTITION_PROTOCOL_GUID { 0x5172FFB5, 0x4253, 0x7D51, { 0xC6, 0x41, 0xA7, 0x01, 0xF9, 0x73, 0x10, 0x3C } }
extern EFI_GUID gEfiRamPartitionProtocolGuid;
typedef struct _RamPartition{
	UINT64 Base;
	UINT64 AvailableLength;
}RamPartitionEntry;
typedef EFI_STATUS(EFIAPI*EFI_RAMPARTITION_GETRAMPARTITIONVERSION)(
	IN EFI_RAMPARTITION_PROTOCOL *This,
	OUT UINT32                   *MajorVersion,
	OUT UINT32                   *MinorVersion
);
typedef EFI_STATUS(EFIAPI*EFI_RAMPARTITION_GETHIGHESTBANKBIT)(
    IN EFI_RAMPARTITION_PROTOCOL *This,
    OUT UINT32                   *HighestBankBit
);
typedef EFI_STATUS(EFIAPI*EFI_RAMPARTITION_GETMINPASRSIZE)(
	IN EFI_RAMPARTITION_PROTOCOL *This,
	OUT UINT32                   *MinPasrSize
);
typedef EFI_STATUS(EFIAPI*EFI_RAMPARTITION_GETRAMPARTITIONS)(
	IN EFI_RAMPARTITION_PROTOCOL *This,
	OUT RamPartitionEntry        *RamPartitions,
	IN OUT UINT32                *NumPartition
);
struct _EFI_RAMPARTITION_PROTOCOL{
	UINT64 Revision;
	EFI_RAMPARTITION_GETRAMPARTITIONVERSION GetRamPartitionVersion;
	EFI_RAMPARTITION_GETHIGHESTBANKBIT GetHighestBankBit;
	EFI_RAMPARTITION_GETRAMPARTITIONS GetRamPartitions;
	EFI_RAMPARTITION_GETMINPASRSIZE GetMinPasrSize;
};
#endif
