#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>

#include "modeset.h"
#include "bo.h"
#include "dev.h"

static int set_crtc_mode(struct sp_dev *dev, struct sp_crtc *crtc,
			struct sp_connector *conn, drmModeModeInfoPtr mode)
{
	int ret;
	struct drm_mode_create_blob create_blob;
	drmModePropertySetPtr pset;

	memset(&create_blob, 0, sizeof(create_blob));
	create_blob.length = sizeof(struct drm_mode_modeinfo);
	create_blob.data = (__u64)(uintptr_t)mode;

	ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATEPROPBLOB, &create_blob);
	if (ret) {
		printf("Failed to create mode property blob %d", ret);
		return ret;
	}

	pset = drmModePropertySetAlloc();
	if (!pset) {
		printf("Failed to allocate property set");
		return -1;
	}

	ret = drmModePropertySetAdd(pset, crtc->crtc->crtc_id,
				    crtc->mode_pid, create_blob.blob_id) ||
	      drmModePropertySetAdd(pset, crtc->crtc->crtc_id,
				    crtc->active_pid, 1) ||
		drmModePropertySetAdd(pset, conn->conn->connector_id,
				conn->crtc_id_pid, crtc->crtc->crtc_id);
	if (ret) {
		printf("Failed to add blob %d to pset", create_blob.blob_id);
		drmModePropertySetFree(pset);
		return ret;
	}

	ret = drmModePropertySetCommit(dev->fd, DRM_MODE_ATOMIC_ALLOW_MODESET,
					NULL, pset);

	drmModePropertySetFree(pset);

	if (ret) {
		printf("Failed to commit pset ret=%d\n", ret);
		return ret;
	}

	memcpy(&crtc->crtc->mode, mode, sizeof(struct drm_mode_modeinfo));
	crtc->crtc->mode_valid = 1;
	return 0;
}

int initialize_screens(struct sp_dev *dev)
{
	int ret, i, j;
	unsigned crtc_mask = 0;

	for (i = 0; i < dev->num_connectors; i++) {
		struct sp_connector *c = &dev->connectors[i];
		drmModeModeInfoPtr m = NULL;
		drmModeEncoderPtr e = NULL;
		struct sp_crtc *cr = NULL;

		if (c->conn->connection != DRM_MODE_CONNECTED)
			continue;

		if (!c->conn->count_modes) {
			printf("connector has no modes, skipping\n");
			continue;
		}

		/* Take the first unless there's a preferred mode */
		m = &c->conn->modes[0];
		for (j = 0; j < c->conn->count_modes; j++) {
			drmModeModeInfoPtr tmp_m = &c->conn->modes[j];

			if (!(tmp_m->type & DRM_MODE_TYPE_PREFERRED))
				continue;

			m = tmp_m;
			break;
		}

		if (!c->conn->count_encoders) {
			printf("no possible encoders for connector\n");
			continue;
		}

		for (j = 0; j < dev->num_encoders; j++) {
			e = dev->encoders[j];
			if (e->encoder_id == c->conn->encoders[0])
				break;
		}
		if (j == dev->num_encoders) {
			printf("could not find encoder for the connector\n");
			continue;
		}

		for (j = 0; j < dev->num_crtcs; j++) {
			if ((1 << j) & crtc_mask)
				continue;

			cr = &dev->crtcs[j];

			if ((1 << j) & e->possible_crtcs)
				break;
		}
		if (j == dev->num_crtcs) {
			printf("could not find crtc for the encoder\n");
			continue;
		}

		ret = set_crtc_mode(dev, cr, c, m);
		if (ret) {
			printf("failed to set mode!\n");
			continue;
		}
		crtc_mask |= 1 << j;
	}
	return 0;
}

struct sp_plane *get_sp_plane(struct sp_dev *dev, struct sp_crtc *crtc)
{
	int i;

	for(i = 0; i < dev->num_planes; i++) {
		struct sp_plane *p = &dev->planes[i];

		if (p->in_use)
			continue;

		if (!(p->plane->possible_crtcs & (1 << crtc->pipe)))
			continue;

		p->in_use = 1;
		return p;
	}
	return NULL;
}

void put_sp_plane(struct sp_plane *plane)
{
	drmModePlanePtr p;

	/* Get the latest plane information (most notably the crtc_id) */
	p = drmModeGetPlane(plane->dev->fd, plane->plane->plane_id);
	if (p)
		plane->plane = p;

	if (plane->bo) {
		free_sp_bo(plane->bo);
		plane->bo = NULL;
	}
	plane->in_use = 0;
}

int set_sp_plane(struct sp_dev *dev, struct sp_plane *plane,
		struct sp_crtc *crtc, int x, int y)
{
	int ret;
	uint32_t w, h;

	w = plane->bo->width;
	h = plane->bo->height;

	if ((w + x) > crtc->crtc->mode.hdisplay)
		w = crtc->crtc->mode.hdisplay - x;
	if ((h + y) > crtc->crtc->mode.vdisplay)
		h = crtc->crtc->mode.vdisplay - y;

	ret = drmModeSetPlane(dev->fd, plane->plane->plane_id,
			crtc->crtc->crtc_id, plane->bo->fb_id, 0, x, y, w, h,
			0, 0, w << 16, h << 16);
	if (ret) {
		printf("failed to set plane to crtc ret=%d\n", ret);
		return ret;
	}

	return ret;
}
int set_sp_plane_pset(struct sp_dev *dev, struct sp_plane *plane,
		drmModePropertySetPtr pset, struct sp_crtc *crtc, int x, int y)
{
	int ret;
	uint32_t w, h;

	w = plane->bo->width;
	h = plane->bo->height;

	if ((w + x) > crtc->crtc->mode.hdisplay)
		w = crtc->crtc->mode.hdisplay - x;
	if ((h + y) > crtc->crtc->mode.vdisplay)
		h = crtc->crtc->mode.vdisplay - y;

	ret = drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->crtc_pid, crtc->crtc->crtc_id)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->fb_pid, plane->bo->fb_id)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->crtc_x_pid, x)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->crtc_y_pid, y)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->crtc_w_pid, w)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->crtc_h_pid, h)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->src_x_pid, 0)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->src_y_pid, 0)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->src_w_pid, w << 16)
		|| drmModePropertySetAdd(pset, plane->plane->plane_id,
			plane->src_h_pid, h << 16);
	if (ret) {
		printf("failed to add properties to the set\n");
		return -1;
	}

	return ret;
}
