#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <drm.h>
#include <drm_fourcc.h>
#include <errno.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "bo.h"
#include "dev.h"
#include "modeset.h"

static void show_usage(char *name)
{
	printf("Usage: %s [OPTION]\n", name);
	printf("   -c, --card      Index of dri card (ie: /dev/dri/cardN)\n");
	printf("   -r, --crtc      Index of crtc to use for test\n");
	printf("\n\n");
}

void parse_arguments(int argc, char *argv[], int *card, int *crtc)
{
	static struct option options[] = {
		{ "card", required_argument, NULL, 'c' },
		{ "crtc", required_argument, NULL, 'r' },
		{ "help", no_argument, NULL, 'h' },
	};
	int option_index = 0;
	int c;

	*card = -1;
	*crtc = -1;
	do {
		c = getopt_long(argc, argv, "c:r:h", options, &option_index);
		switch (c) {
		case 0:
		case 'h':
			show_usage(argv[0]);
			exit(0);
		case -1:
			break;
		case 'c':
			if (optarg[0] < '0' || optarg[0] > '9') {
				printf("Invalid card value '%s'!\n", optarg);
				show_usage(argv[0]);
				exit(-1);
			}
			*card = optarg[0] - '0';
			break;
		case 'r':
			if (optarg[0] < '0' || optarg[0] > '9') {
				printf("Invalid crtc value '%s'!\n", optarg);
				show_usage(argv[0]);
				exit(-1);
			}
			*crtc = optarg[0] - '0';
			break;
		}
	} while (c != -1);

	if (*card < 0 || *crtc < 0) {
		show_usage(argv[0]);
		exit(-1);
	}
}

static uint32_t get_prop_id(struct sp_dev *dev,
			drmModeObjectPropertiesPtr props, const char *name)
{
	drmModePropertyPtr p;
	uint32_t i, prop_id = 0; /* Property ID should always be > 0 */

	for (i = 0; !prop_id && i < props->count_props; i++) {
		p = drmModeGetProperty(dev->fd, props->props[i]);
		if (!strcmp(p->name, name))
			prop_id = p->prop_id;
		drmModeFreeProperty(p);
	}
	if (!prop_id)
		printf("Could not find %s property\n", name);
	return prop_id;
}

static int get_supported_format(struct sp_plane *plane, uint32_t *format)
{
	uint32_t i;

	for (i = 0; i < plane->plane->count_formats; i++) {
		if (plane->plane->formats[i] == DRM_FORMAT_XRGB8888 ||
		    plane->plane->formats[i] == DRM_FORMAT_ARGB8888 ||
		    plane->plane->formats[i] == DRM_FORMAT_RGBA8888) {
			*format = plane->plane->formats[i];
			return 0;
		}
	}
	printf("No suitable formats found!\n");
	return -ENOENT;
}

struct sp_dev *create_sp_dev(int card)
{
	struct sp_dev *dev;
	int ret, fd, i, j;
	drmModeRes *r = NULL;
	drmModePlaneRes *pr = NULL;
	char card_path[256];

	snprintf(card_path, sizeof(card_path), "/dev/dri/card%d", card);

	fd = open(card_path, O_RDWR);
	if (fd < 0) {
		printf("failed to open card0\n");
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		printf("failed to allocate dev\n");
		return NULL;
	}

	dev->fd = fd;

	ret = drmSetClientCap(dev->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (ret) {
		printf("failed to set client cap\n");
		goto err;
	}

	ret = drmSetClientCap(dev->fd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (ret) {
		printf("Failed to set atomic cap %d", ret);
		goto err;
	}

	r = drmModeGetResources(dev->fd);
	if (!r) {
		printf("failed to get r\n");
		goto err;
	}

	dev->num_connectors = r->count_connectors;
	dev->connectors = calloc(dev->num_connectors,
				sizeof(struct sp_connector));
	if (!dev->connectors) {
		printf("failed to allocate connectors\n");
		goto err;
	}
	for (i = 0; i < dev->num_connectors; i++) {
		drmModeObjectPropertiesPtr props;
		dev->connectors[i].conn = drmModeGetConnector(dev->fd,
					r->connectors[i]);
		if (!dev->connectors[i].conn) {
			printf("failed to get connector %d\n", i);
			goto err;
		}

		props = drmModeObjectGetProperties(dev->fd, r->connectors[i],
				DRM_MODE_OBJECT_CONNECTOR);
		if (!props) {
			printf("failed to get connector properties\n");
			goto err;
		}

		dev->connectors[i].crtc_id_pid = get_prop_id(dev, props,
								"CRTC_ID");
		drmModeFreeObjectProperties(props);
		if (!dev->connectors[i].crtc_id_pid)
			goto err;
	}

	dev->num_encoders = r->count_encoders;
	dev->encoders = calloc(dev->num_encoders, sizeof(*dev->encoders));
	if (!dev->encoders) {
		printf("failed to allocate encoders\n");
		goto err;
	}
	for (i = 0; i < dev->num_encoders; i++) {
		dev->encoders[i] = drmModeGetEncoder(dev->fd, r->encoders[i]);
		if (!dev->encoders[i]) {
			printf("failed to get encoder %d\n", i);
			goto err;
		}
	}

	dev->num_crtcs = r->count_crtcs;
	dev->crtcs = calloc(dev->num_crtcs, sizeof(struct sp_crtc));
	if (!dev->crtcs) {
		printf("failed to allocate crtcs\n");
		goto err;
	}
	for (i = 0; i < dev->num_crtcs; i++) {
		drmModeObjectPropertiesPtr props;

		dev->crtcs[i].crtc = drmModeGetCrtc(dev->fd, r->crtcs[i]);
		if (!dev->crtcs[i].crtc) {
			printf("failed to get crtc %d\n", i);
			goto err;
		}
		dev->crtcs[i].pipe = i;
		dev->crtcs[i].num_planes = 0;

		props = drmModeObjectGetProperties(dev->fd, r->crtcs[i],
				DRM_MODE_OBJECT_CRTC);
		if (!props) {
			printf("failed to get crtc properties\n");
			goto err;
		}

		dev->crtcs[i].mode_pid = get_prop_id(dev, props, "MODE_ID");
		dev->crtcs[i].active_pid = get_prop_id(dev, props, "ACTIVE");
		drmModeFreeObjectProperties(props);
		if (!dev->crtcs[i].mode_pid || !dev->crtcs[i].active_pid)
			goto err;
	}

	pr = drmModeGetPlaneResources(dev->fd);
	if (!pr) {
		printf("failed to get plane resources\n");
		goto err;
	}
	dev->num_planes = pr->count_planes;
	dev->planes = calloc(dev->num_planes, sizeof(struct sp_plane));
	for(i = 0; i < dev->num_planes; i++) {
		drmModeObjectPropertiesPtr props;
		struct sp_plane *plane = &dev->planes[i];

		plane->dev = dev;
		plane->plane = drmModeGetPlane(dev->fd, pr->planes[i]);
		if (!plane->plane) {
			printf("failed to get plane %d\n", i);
			goto err;
		}
		plane->bo = NULL;
		plane->in_use = 0;

		ret = get_supported_format(plane, &plane->format);
		if (ret) {
			printf("failed to get supported format: %d\n", ret);
			goto err;
		}

		for (j = 0; j < dev->num_crtcs; j++) {
			if (plane->plane->possible_crtcs & (1 << j))
				dev->crtcs[j].num_planes++;
		}

		props = drmModeObjectGetProperties(dev->fd, pr->planes[i],
				DRM_MODE_OBJECT_PLANE);
		if (!props) {
			printf("failed to get plane properties\n");
			goto err;
		}
		plane->crtc_pid = get_prop_id(dev, props, "CRTC_ID");
		if (!plane->crtc_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->fb_pid = get_prop_id(dev, props, "FB_ID");
		if (!plane->fb_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->crtc_x_pid = get_prop_id(dev, props, "CRTC_X");
		if (!plane->crtc_x_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->crtc_y_pid = get_prop_id(dev, props, "CRTC_Y");
		if (!plane->crtc_y_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->crtc_w_pid = get_prop_id(dev, props, "CRTC_W");
		if (!plane->crtc_w_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->crtc_h_pid = get_prop_id(dev, props, "CRTC_H");
		if (!plane->crtc_h_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->src_x_pid = get_prop_id(dev, props, "SRC_X");
		if (!plane->src_x_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->src_y_pid = get_prop_id(dev, props, "SRC_Y");
		if (!plane->src_y_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->src_w_pid = get_prop_id(dev, props, "SRC_W");
		if (!plane->src_w_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		plane->src_h_pid = get_prop_id(dev, props, "SRC_H");
		if (!plane->src_h_pid) {
			drmModeFreeObjectProperties(props);
			goto err;
		}
		drmModeFreeObjectProperties(props);
	}

	if (pr)
		drmModeFreePlaneResources(pr);
	if (r)
		drmModeFreeResources(r);

	return dev;
err:
	if (pr)
		drmModeFreePlaneResources(pr);
	if (r)
		drmModeFreeResources(r);
	destroy_sp_dev(dev);
	return NULL;
}

void destroy_sp_dev(struct sp_dev *dev)
{
	int i;

	if (dev->planes) {
		for (i = 0; i< dev->num_planes; i++) {
			if (dev->planes[i].in_use)
				put_sp_plane(&dev->planes[i]);
			if (dev->planes[i].plane)
				drmModeFreePlane(dev->planes[i].plane);
			if (dev->planes[i].bo)
				free_sp_bo(dev->planes[i].bo);
		}
		free(dev->planes);
	}
	if (dev->crtcs) {
		for (i = 0; i< dev->num_crtcs; i++) {
			if (dev->crtcs[i].crtc)
				drmModeFreeCrtc(dev->crtcs[i].crtc);
		}
		free(dev->crtcs);
	}
	if (dev->encoders) {
		for (i = 0; i< dev->num_encoders; i++) {
			if (dev->encoders[i])
				drmModeFreeEncoder(dev->encoders[i]);
		}
		free(dev->encoders);
	}
	if (dev->connectors) {
		for (i = 0; i< dev->num_connectors; i++) {
			if (dev->connectors[i].conn)
				drmModeFreeConnector(dev->connectors[i].conn);
		}
		free(dev->connectors);
	}

	close(dev->fd);
	free(dev);
}
