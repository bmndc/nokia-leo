/*
 * Copyright (C) ROCKCHIP, Inc.
 * Author:yzq<yzq@rock-chips.com>
 *
 * based on exynos_drm.c
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include <xf86drm.h>

#include "rockchip_drm.h"
#include "rockchip_drmif.h"

/*
 * Create rockchip drm device object.
 *
 * @fd: file descriptor to rockchip drm driver opened.
 *
 * if true, return the device object else NULL.
 */
struct rockchip_device *rockchip_device_create(int fd)
{
	struct rockchip_device *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		fprintf(stderr, "failed to create device[%s].\n",
				strerror(errno));
		return NULL;
	}

	dev->fd = fd;

	return dev;
}

/*
 * Destroy rockchip drm device object
 *
 * @dev: rockchip drm device object.
 */
void rockchip_device_destroy(struct rockchip_device *dev)
{
	free(dev);
}

/*
 * Create a rockchip buffer object to rockchip drm device.
 *
 * @dev: rockchip drm device object.
 * @size: user-desired size.
 * flags: user-desired memory type.
 *	user can set one or more types among several types to memory
 *	allocation and cache attribute types. and as default,
 *	ROCKCHIP_BO_NONCONTIG and ROCKCHIP-BO_NONCACHABLE types would
 *	be used.
 *
 * if true, return a rockchip buffer object else NULL.
 */
struct rockchip_bo *rockchip_bo_create(struct rockchip_device *dev,
					size_t size, uint32_t flags)
{
	struct rockchip_bo *bo;
	struct drm_rockchip_gem_create req = {
		.size = size,
		.flags = flags,
	};

	if (size == 0) {
		fprintf(stderr, "invalid size.\n");
		return NULL;
	}

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to create bo[%s].\n",
				strerror(errno));
		goto fail;
	}

	bo->dev = dev;

	if (drmIoctl(dev->fd, DRM_IOCTL_ROCKCHIP_GEM_CREATE, &req)){
		fprintf(stderr, "failed to create gem object[%s].\n",
				strerror(errno));
		goto err_free_bo;
	}

	bo->handle = req.handle;
	bo->size = size;
	bo->flags = flags;

	return bo;

err_free_bo:
	free(bo);
fail:
	return NULL;
}

struct rockchip_bo *rockchip_bo_from_handle(struct rockchip_device *dev,
			uint32_t handle, uint32_t flags, uint32_t size)
{
	struct rockchip_bo *bo;

	if (size == 0) {
		fprintf(stderr, "invalid size.\n");
		return NULL;
	}

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to create bo[%s].\n",
				strerror(errno));
		return NULL;
	}

	bo->dev = dev;
	bo->handle = handle;
	bo->size = size;
	bo->flags = flags;

	return bo;
}

/*
 * Destroy a rockchip buffer object.
 *
 * @bo: a rockchip buffer object to be destroyed.
 */
void rockchip_bo_destroy(struct rockchip_bo *bo)
{
	if (!bo)
		return;

	if (bo->vaddr)
		munmap(bo->vaddr, bo->size);

	if (bo->handle) {
		struct drm_gem_close req = {
			.handle = bo->handle,
		};

		drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
	}

	free(bo);
}


/*
 * Get a rockchip buffer object from a gem global object name.
 *
 * @dev: a rockchip device object.
 * @name: a gem global object name exported by another process.
 *
 * this interface is used to get a rockchip buffer object from a gem
 * global object name sent by another process for buffer sharing.
 *
 * if true, return a rockchip buffer object else NULL.
 *
 */
struct rockchip_bo *rockchip_bo_from_name(struct rockchip_device *dev,
						uint32_t name)
{
	struct rockchip_bo *bo;
	struct drm_gem_open req = {
		.name = name,
	};

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to allocate bo[%s].\n",
				strerror(errno));
		return NULL;
	}

	if (drmIoctl(dev->fd, DRM_IOCTL_GEM_OPEN, &req)) {
		fprintf(stderr, "failed to open gem object[%s].\n",
				strerror(errno));
		goto err_free_bo;
	}

	bo->dev = dev;
	bo->name = name;
	bo->handle = req.handle;

	return bo;

err_free_bo:
	free(bo);
	return NULL;
}

/*
 * Get a gem global object name from a gem object handle.
 *
 * @bo: a rockchip buffer object including gem handle.
 * @name: a gem global object name to be got by kernel driver.
 *
 * this interface is used to get a gem global object name from a gem object
 * handle to a buffer that wants to share it with another process.
 *
 * if true, return 0 else negative.
 */
int rockchip_bo_get_name(struct rockchip_bo *bo, uint32_t *name)
{
	if (!bo->name) {
		struct drm_gem_flink req = {
			.handle = bo->handle,
		};
		int ret;

		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_FLINK, &req);
		if (ret) {
			fprintf(stderr, "failed to get gem global name[%s].\n",
					strerror(errno));
			return ret;
		}

		bo->name = req.name;
	}

	*name = bo->name;

	return 0;
}

uint32_t rockchip_bo_handle(struct rockchip_bo *bo)
{
	return bo->handle;
}

/*
 * Mmap a buffer to user space.
 *
 * @bo: a rockchip buffer object including a gem object handle to be mmapped
 *	to user space.
 *
 * if true, user pointer mmaped else NULL.
 */
void *rockchip_bo_map(struct rockchip_bo *bo)
{
	if (!bo->vaddr) {
		struct rockchip_device *dev = bo->dev;
		struct drm_rockchip_gem_map_off req = {
			.handle = bo->handle,
		};
		int ret;

		ret = drmIoctl(dev->fd, DRM_IOCTL_ROCKCHIP_GEM_MAP_OFFSET, &req);
		if (ret) {
			fprintf(stderr, "failed to ioctl gem map offset[%s].\n",
				strerror(errno));
			return NULL;
		}

		bo->vaddr = mmap(0, bo->size, PROT_READ | PROT_WRITE,
			   MAP_SHARED, dev->fd, req.offset);
		if (bo->vaddr == MAP_FAILED) {
			fprintf(stderr, "failed to mmap buffer[%s].\n",
				strerror(errno));
			return NULL;
		}
	}

	return bo->vaddr;
}
