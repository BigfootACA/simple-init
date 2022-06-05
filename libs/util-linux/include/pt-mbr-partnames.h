MBR_TYPE(0x00, ("Empty")),
MBR_TYPE(0x01, ("FAT12")),
MBR_TYPE(0x02, ("XENIX root")),
MBR_TYPE(0x03, ("XENIX usr")),
MBR_TYPE(0x04, ("FAT16 <32M")),
MBR_TYPE(0x05, ("Extended")),		/* DOS 3.3+ extended partition */
MBR_TYPE(0x06, ("FAT16")),		/* DOS 16-bit >=32M */
MBR_TYPE(0x07, ("HPFS/NTFS/exFAT")),	/* OS/2 IFS, eg, HPFS or NTFS or QNX or exFAT */
MBR_TYPE(0x08, ("AIX")),		/* AIX boot (AIX -- PS/2 port) or SplitDrive */
MBR_TYPE(0x09, ("AIX bootable")),	/* AIX data or Coherent */
MBR_TYPE(0x0a, ("OS/2 Boot Manager")),/* OS/2 Boot Manager */
MBR_TYPE(0x0b, ("W95 FAT32")),
MBR_TYPE(0x0c, ("W95 FAT32 (LBA)")),/* LBA really is `Extended Int 13h' */
MBR_TYPE(0x0e, ("W95 FAT16 (LBA)")),
MBR_TYPE(0x0f, ("W95 Ext'd (LBA)")),
MBR_TYPE(0x10, ("OPUS")),
MBR_TYPE(0x11, ("Hidden FAT12")),
MBR_TYPE(0x12, ("Compaq diagnostics")),
MBR_TYPE(0x14, ("Hidden FAT16 <32M")),
MBR_TYPE(0x16, ("Hidden FAT16")),
MBR_TYPE(0x17, ("Hidden HPFS/NTFS")),
MBR_TYPE(0x18, ("AST SmartSleep")),
MBR_TYPE(0x1b, ("Hidden W95 FAT32")),
MBR_TYPE(0x1c, ("Hidden W95 FAT32 (LBA)")),
MBR_TYPE(0x1e, ("Hidden W95 FAT16 (LBA)")),
MBR_TYPE(0x24, ("NEC DOS")),
MBR_TYPE(0x27, ("Hidden NTFS WinRE")),
MBR_TYPE(0x39, ("Plan 9")),
MBR_TYPE(0x3c, ("PartitionMagic recovery")),
MBR_TYPE(0x40, ("Venix 80286")),
MBR_TYPE(0x41, ("PPC PReP Boot")),
MBR_TYPE(0x42, ("SFS")),
MBR_TYPE(0x4d, ("QNX4.x")),
MBR_TYPE(0x4e, ("QNX4.x 2nd part")),
MBR_TYPE(0x4f, ("QNX4.x 3rd part")),
MBR_TYPE(0x50, ("OnTrack DM")),
MBR_TYPE(0x51, ("OnTrack DM6 Aux1")),	/* (or Novell) */
MBR_TYPE(0x52, ("CP/M")),		/* CP/M or Microport SysV/AT */
MBR_TYPE(0x53, ("OnTrack DM6 Aux3")),
MBR_TYPE(0x54, ("OnTrackDM6")),
MBR_TYPE(0x55, ("EZ-Drive")),
MBR_TYPE(0x56, ("Golden Bow")),
MBR_TYPE(0x5c, ("Priam Edisk")),
MBR_TYPE(0x61, ("SpeedStor")),
MBR_TYPE(0x63, ("GNU HURD or SysV")),	/* GNU HURD or Mach or Sys V/386 (such as ISC UNIX) */
MBR_TYPE(0x64, ("Novell Netware 286")),
MBR_TYPE(0x65, ("Novell Netware 386")),
MBR_TYPE(0x70, ("DiskSecure Multi-Boot")),
MBR_TYPE(0x75, ("PC/IX")),
MBR_TYPE(0x80, ("Old Minix")),	/* Minix 1.4a and earlier */
MBR_TYPE(0x81, ("Minix / old Linux")),/* Minix 1.4b and later */
MBR_TYPE(0x82, ("Linux swap / Solaris")),
MBR_TYPE(0x83, ("Linux")),
MBR_TYPE(0x84, ("OS/2 hidden or Intel hibernation")),/* OS/2 hidden C: drive,
				   hibernation type Microsoft APM
				   or hibernation Intel Rapid Start */
MBR_TYPE(0x85, ("Linux extended")),
MBR_TYPE(0x86, ("NTFS volume set")),
MBR_TYPE(0x87, ("NTFS volume set")),
MBR_TYPE(0x88, ("Linux plaintext")),
MBR_TYPE(0x8e, ("Linux LVM")),
MBR_TYPE(0x93, ("Amoeba")),
MBR_TYPE(0x94, ("Amoeba BBT")),	/* (bad block table) */
MBR_TYPE(0x9f, ("BSD/OS")),		/* BSDI */
MBR_TYPE(0xa0, ("IBM Thinkpad hibernation")),
MBR_TYPE(0xa5, ("FreeBSD")),		/* various BSD flavours */
MBR_TYPE(0xa6, ("OpenBSD")),
MBR_TYPE(0xa7, ("NeXTSTEP")),
MBR_TYPE(0xa8, ("Darwin UFS")),
MBR_TYPE(0xa9, ("NetBSD")),
MBR_TYPE(0xab, ("Darwin boot")),
MBR_TYPE(0xaf, ("HFS / HFS+")),
MBR_TYPE(0xb7, ("BSDI fs")),
MBR_TYPE(0xb8, ("BSDI swap")),
MBR_TYPE(0xbb, ("Boot Wizard hidden")),
MBR_TYPE(0xbc, ("Acronis FAT32 LBA")),/* hidden (+0xb0) Acronis Secure Zone (backup software) */
MBR_TYPE(0xbe, ("Solaris boot")),
MBR_TYPE(0xbf, ("Solaris")),
MBR_TYPE(0xc1, ("DRDOS/sec (FAT-12)")),
MBR_TYPE(0xc4, ("DRDOS/sec (FAT-16 < 32M)")),
MBR_TYPE(0xc6, ("DRDOS/sec (FAT-16)")),
MBR_TYPE(0xc7, ("Syrinx")),
MBR_TYPE(0xda, ("Non-FS data")),
MBR_TYPE(0xdb, ("CP/M / CTOS / ...")),/* CP/M or Concurrent CP/M or
				   Concurrent DOS or CTOS */
MBR_TYPE(0xde, ("Dell Utility")),	/* Dell PowerEdge Server utilities */
MBR_TYPE(0xdf, ("BootIt")),		/* BootIt EMBRM */
MBR_TYPE(0xe1, ("DOS access")),	/* DOS access or SpeedStor 12-bit FAT
				   extended partition */
MBR_TYPE(0xe3, ("DOS R/O")),		/* DOS R/O or SpeedStor */
MBR_TYPE(0xe4, ("SpeedStor")),	/* SpeedStor 16-bit FAT extended
				   partition < 1024 cyl. */

/* Linux https://www.freedesktop.org/wiki/Specifications/BootLoaderSpec/ */
MBR_TYPE(0xea, ("Linux extended boot")),

MBR_TYPE(0xeb, ("BeOS fs")),
MBR_TYPE(0xee, ("GPT")),		/* Intel EFI GUID Partition Table */
MBR_TYPE(0xef, ("EFI (FAT-12/16/32)")),/* Intel EFI System Partition */
MBR_TYPE(0xf0, ("Linux/PA-RISC boot")),/* Linux/PA-RISC boot loader */
MBR_TYPE(0xf1, ("SpeedStor")),
MBR_TYPE(0xf4, ("SpeedStor")),	/* SpeedStor large partition */
MBR_TYPE(0xf2, ("DOS secondary")),	/* DOS 3.3+ secondary */
MBR_TYPE(0xf8, ("EBBR protective")),	/* Arm EBBR firmware protective partition */
MBR_TYPE(0xfb, ("VMware VMFS")),
MBR_TYPE(0xfc, ("VMware VMKCORE")),	/* VMware kernel dump partition */
MBR_TYPE(0xfd, ("Linux raid autodetect")),/* Linux raid partition with
				       autodetect using persistent
				       superblock */
MBR_TYPE(0xfe, ("LANstep")),		/* SpeedStor >1024 cyl. or LANstep */
MBR_TYPE(0xff, ("BBT")),		/* Xenix Bad Block Table */
