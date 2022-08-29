add_library(ul-shared STATIC
	libs/util-linux/lib/mangle.c
	libs/util-linux/lib/crc32.c
	libs/util-linux/lib/crc32c.c
	libs/util-linux/lib/path.c
	libs/util-linux/lib/loopdev.c
	libs/util-linux/lib/encode.c
	libs/util-linux/lib/buffer.c
	libs/util-linux/lib/blkdev.c
	libs/util-linux/lib/strutils.c
	libs/util-linux/lib/canonicalize.c
	libs/util-linux/lib/mbsalign.c
	libs/util-linux/lib/sysfs.c
	libs/util-linux/lib/linux_version.c
	libs/util-linux/lib/fileutils.c
	libs/util-linux/lib/monotonic.c
	libs/util-linux/lib/match.c
	libs/util-linux/lib/jsonwrt.c
)
add_library(uuid STATIC
	libs/util-linux/libuuid/unpack.c
	libs/util-linux/libuuid/parse.c
	libs/util-linux/libuuid/uuid_time.c
	libs/util-linux/libuuid/clear.c
	libs/util-linux/libuuid/pack.c
	libs/util-linux/libuuid/gen_uuid.c
	libs/util-linux/libuuid/compare.c
	libs/util-linux/libuuid/predefined.c
	libs/util-linux/libuuid/copy.c
	libs/util-linux/libuuid/unparse.c
	libs/util-linux/libuuid/isnull.c
)
add_library(blkid STATIC
	libs/util-linux/libblkid/cache.c
	libs/util-linux/libblkid/config.c
	libs/util-linux/libblkid/dev.c
	libs/util-linux/libblkid/devname.c
	libs/util-linux/libblkid/devno.c
	libs/util-linux/libblkid/encode.c
	libs/util-linux/libblkid/evaluate.c
	libs/util-linux/libblkid/getsize.c
	libs/util-linux/libblkid/partitions/aix.c
	libs/util-linux/libblkid/partitions/atari.c
	libs/util-linux/libblkid/partitions/bsd.c
	libs/util-linux/libblkid/partitions/dos.c
	libs/util-linux/libblkid/partitions/gpt.c
	libs/util-linux/libblkid/partitions/mac.c
	libs/util-linux/libblkid/partitions/minix.c
	libs/util-linux/libblkid/partitions/partitions.c
	libs/util-linux/libblkid/partitions/sgi.c
	libs/util-linux/libblkid/partitions/solaris_x86.c
	libs/util-linux/libblkid/partitions/sun.c
	libs/util-linux/libblkid/partitions/ultrix.c
	libs/util-linux/libblkid/partitions/unixware.c
	libs/util-linux/libblkid/probe.c
	libs/util-linux/libblkid/read.c
	libs/util-linux/libblkid/resolve.c
	libs/util-linux/libblkid/save.c
	libs/util-linux/libblkid/superblocks/adaptec_raid.c
	libs/util-linux/libblkid/superblocks/apfs.c
	libs/util-linux/libblkid/superblocks/bcache.c
	libs/util-linux/libblkid/superblocks/befs.c
	libs/util-linux/libblkid/superblocks/bfs.c
	libs/util-linux/libblkid/superblocks/bitlocker.c
	libs/util-linux/libblkid/superblocks/bluestore.c
	libs/util-linux/libblkid/superblocks/btrfs.c
	libs/util-linux/libblkid/superblocks/cramfs.c
	libs/util-linux/libblkid/superblocks/ddf_raid.c
	libs/util-linux/libblkid/superblocks/drbd.c
	libs/util-linux/libblkid/superblocks/drbdmanage.c
	libs/util-linux/libblkid/superblocks/drbdproxy_datalog.c
	libs/util-linux/libblkid/superblocks/erofs.c
	libs/util-linux/libblkid/superblocks/exfat.c
	libs/util-linux/libblkid/superblocks/exfs.c
	libs/util-linux/libblkid/superblocks/ext.c
	libs/util-linux/libblkid/superblocks/f2fs.c
	libs/util-linux/libblkid/superblocks/gfs.c
	libs/util-linux/libblkid/superblocks/hfs.c
	libs/util-linux/libblkid/superblocks/highpoint_raid.c
	libs/util-linux/libblkid/superblocks/hpfs.c
	libs/util-linux/libblkid/superblocks/iso9660.c
	libs/util-linux/libblkid/superblocks/isw_raid.c
	libs/util-linux/libblkid/superblocks/jfs.c
	libs/util-linux/libblkid/superblocks/jmicron_raid.c
	libs/util-linux/libblkid/superblocks/linux_raid.c
	libs/util-linux/libblkid/superblocks/lsi_raid.c
	libs/util-linux/libblkid/superblocks/luks.c
	libs/util-linux/libblkid/superblocks/lvm.c
	libs/util-linux/libblkid/superblocks/minix.c
	libs/util-linux/libblkid/superblocks/mpool.c
	libs/util-linux/libblkid/superblocks/netware.c
	libs/util-linux/libblkid/superblocks/nilfs.c
	libs/util-linux/libblkid/superblocks/ntfs.c
	libs/util-linux/libblkid/superblocks/nvidia_raid.c
	libs/util-linux/libblkid/superblocks/ocfs.c
	libs/util-linux/libblkid/superblocks/promise_raid.c
	libs/util-linux/libblkid/superblocks/refs.c
	libs/util-linux/libblkid/superblocks/reiserfs.c
	libs/util-linux/libblkid/superblocks/romfs.c
	libs/util-linux/libblkid/superblocks/silicon_raid.c
	libs/util-linux/libblkid/superblocks/squashfs.c
	libs/util-linux/libblkid/superblocks/stratis.c
	libs/util-linux/libblkid/superblocks/superblocks.c
	libs/util-linux/libblkid/superblocks/swap.c
	libs/util-linux/libblkid/superblocks/sysv.c
	libs/util-linux/libblkid/superblocks/ubi.c
	libs/util-linux/libblkid/superblocks/ubifs.c
	libs/util-linux/libblkid/superblocks/udf.c
	libs/util-linux/libblkid/superblocks/ufs.c
	libs/util-linux/libblkid/superblocks/vdo.c
	libs/util-linux/libblkid/superblocks/vfat.c
	libs/util-linux/libblkid/superblocks/via_raid.c
	libs/util-linux/libblkid/superblocks/vmfs.c
	libs/util-linux/libblkid/superblocks/vxfs.c
	libs/util-linux/libblkid/superblocks/xfs.c
	libs/util-linux/libblkid/superblocks/zfs.c
	libs/util-linux/libblkid/superblocks/zonefs.c
	libs/util-linux/libblkid/tag.c
	libs/util-linux/libblkid/topology/dm.c
	libs/util-linux/libblkid/topology/evms.c
	libs/util-linux/libblkid/topology/ioctl.c
	libs/util-linux/libblkid/topology/lvm.c
	libs/util-linux/libblkid/topology/md.c
	libs/util-linux/libblkid/topology/sysfs.c
	libs/util-linux/libblkid/topology/topology.c
	libs/util-linux/libblkid/verify.c
	libs/util-linux/libblkid/version.c

)
add_library(mount STATIC
	libs/util-linux/libmount/btrfs.c
	libs/util-linux/libmount/cache.c
	libs/util-linux/libmount/context.c
	libs/util-linux/libmount/context_loopdev.c
	libs/util-linux/libmount/context_mount.c
	libs/util-linux/libmount/context_umount.c
	libs/util-linux/libmount/context_veritydev.c
	libs/util-linux/libmount/fs.c
	libs/util-linux/libmount/iter.c
	libs/util-linux/libmount/lock.c
	libs/util-linux/libmount/monitor.c
	libs/util-linux/libmount/optmap.c
	libs/util-linux/libmount/optstr.c
	libs/util-linux/libmount/tab.c
	libs/util-linux/libmount/tab_diff.c
	libs/util-linux/libmount/tab_parse.c
	libs/util-linux/libmount/tab_update.c
	libs/util-linux/libmount/utils.c
	libs/util-linux/libmount/version.c
)
add_library(fdisk STATIC
	libs/util-linux/libfdisk/alignment.c
	libs/util-linux/libfdisk/ask.c
	libs/util-linux/libfdisk/bsd.c
	libs/util-linux/libfdisk/context.c
	libs/util-linux/libfdisk/dos.c
	libs/util-linux/libfdisk/field.c
	libs/util-linux/libfdisk/gpt.c
	libs/util-linux/libfdisk/item.c
	libs/util-linux/libfdisk/iter.c
	libs/util-linux/libfdisk/label.c
	libs/util-linux/libfdisk/partition.c
	libs/util-linux/libfdisk/parttype.c
	libs/util-linux/libfdisk/script.c
	libs/util-linux/libfdisk/sgi.c
	libs/util-linux/libfdisk/sun.c
	libs/util-linux/libfdisk/table.c
	libs/util-linux/libfdisk/utils.c
	libs/util-linux/libfdisk/version.c
	libs/util-linux/libfdisk/wipe.c
)
add_library(smartcols STATIC
	libs/util-linux/libsmartcols/calculate.c
	libs/util-linux/libsmartcols/cell.c
	libs/util-linux/libsmartcols/column.c
	libs/util-linux/libsmartcols/grouping.c
	libs/util-linux/libsmartcols/iter.c
	libs/util-linux/libsmartcols/line.c
	libs/util-linux/libsmartcols/print-api.c
	libs/util-linux/libsmartcols/print.c
	libs/util-linux/libsmartcols/symbols.c
	libs/util-linux/libsmartcols/table.c
	libs/util-linux/libsmartcols/version.c
	libs/util-linux/libsmartcols/walk.c
)
target_include_directories(uuid PRIVATE libs/util-linux/include)
target_include_directories(fdisk PRIVATE libs/util-linux/include)
target_include_directories(blkid PRIVATE libs/util-linux/include)
target_include_directories(mount PRIVATE libs/util-linux/include)
target_include_directories(smartcols PRIVATE libs/util-linux/include)
target_include_directories(ul-shared PRIVATE libs/util-linux/include)
target_link_libraries(uuid PRIVATE ul-shared)
target_link_libraries(fdisk PRIVATE ul-shared smartcols)
target_link_libraries(blkid PRIVATE ul-shared)
target_link_libraries(mount PRIVATE ul-shared)
target_link_libraries(smartcols PRIVATE ul-shared)
if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(ul-shared PRIVATE -Wno-format-overflow -Wno-stringop-overflow)
endif()
target_compile_options(ul-shared PRIVATE -Wno-array-bounds)
include_directories(libs/util-linux)
