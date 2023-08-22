/**
 * libf2fs.c
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 *
 * Dual licensed under the GPL or LGPL version 2 licenses.
 */
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mntent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>

#include <f2fs_fs.h>

struct f2fs_configuration config;

/*
 * IO interfaces
 */
int dev_read_version(void *buf, __u64 offset, size_t len)
{
	if (lseek64(config.kd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (read(config.kd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_read(void *buf, __u64 offset, size_t len)
{
	if (lseek64(config.fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (read(config.fd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_readahead(__u64 offset, size_t len)
{
#ifdef POSIX_FADV_WILLNEED
	return posix_fadvise(config.fd, offset, len, POSIX_FADV_WILLNEED);
#else
	return 0;
#endif
}

int dev_write(void *buf, __u64 offset, size_t len)
{
	if (lseek64(config.fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (write(config.fd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_write_block(void *buf, __u64 blk_addr)
{
	return dev_write(buf, blk_addr * F2FS_BLKSIZE, F2FS_BLKSIZE);
}

int dev_write_dump(void *buf, __u64 offset, size_t len)
{
	if (lseek64(config.dump_fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (write(config.dump_fd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_fill(void *buf, __u64 offset, size_t len)
{
	/* Only allow fill to zero */
	if (*((__u8*)buf))
		return -1;
	if (lseek64(config.fd, (off64_t)offset, SEEK_SET) < 0)
		return -1;
	if (write(config.fd, buf, len) < 0)
		return -1;
	return 0;
}

int dev_read_block(void *buf, __u64 blk_addr)
{
	return dev_read(buf, blk_addr * F2FS_BLKSIZE, F2FS_BLKSIZE);
}

int dev_read_blocks(void *buf, __u64 addr, __u32 nr_blks)
{
	return dev_read(buf, addr * F2FS_BLKSIZE, nr_blks * F2FS_BLKSIZE);
}

int dev_reada_block(__u64 blk_addr)
{
	return dev_readahead(blk_addr * F2FS_BLKSIZE, F2FS_BLKSIZE);
}

void f2fs_finalize_device(struct f2fs_configuration *c)
{
	/*
	 * We should call fsync() to flush out all the dirty pages
	 * in the block device page cache.
	 */
	if (fsync(c->fd) < 0)
		MSG(0, "\tError: Could not conduct fsync!!!\n");

	if (close(c->fd) < 0)
		MSG(0, "\tError: Failed to close device file!!!\n");

	close(c->kd);
}
