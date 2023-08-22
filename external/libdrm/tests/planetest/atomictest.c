#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <xf86drm.h>

#include "dev.h"
#include "bo.h"
#include "modeset.h"

static int terminate = 0;

static void sigint_handler(int arg)
{
	terminate = 1;
}

static void
page_flip_handler(int fd, unsigned int sequence, unsigned int tv_sec,
		unsigned int tv_usec, void *user_data)
{
}

static void incrementor(int *inc, int *val, int increment, int lower, int upper)
{
	if(*inc > 0)
		*inc = *val + increment >= upper ? -1 : 1;
	else
		*inc = *val - increment <= lower ? 1 : -1;
	*val += *inc * increment;
}

int main(int argc, char *argv[])
{
	int ret, i, j, num_test_planes;
	int x_inc = 1, x = 0, y_inc = 1, y = 0;
	uint32_t plane_w = 128, plane_h = 128;
	struct sp_dev *dev;
	struct sp_plane **plane = NULL;
	struct sp_crtc *test_crtc;
	fd_set fds;
	drmModePropertySetPtr pset;
	drmEventContext event_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = page_flip_handler,
	};
	int card = 0, crtc = 0;

	signal(SIGINT, sigint_handler);

	parse_arguments(argc, argv, &card, &crtc);

	dev = create_sp_dev(card);
	if (!dev) {
		printf("Failed to create sp_dev\n");
		return -1;
	}

	if (crtc >= dev->num_crtcs) {
		printf("Invalid crtc %d (num=%d)\n", crtc, dev->num_crtcs);
		return -1;
	}

	ret = initialize_screens(dev);
	if (ret) {
		printf("Failed to initialize screens\n");
		goto out;
	}
	test_crtc = &dev->crtcs[crtc];

	plane = calloc(dev->num_planes, sizeof(*plane));
	if (!plane) {
		printf("Failed to allocate plane array\n");
		goto out;
	}

	/* Create our planes */
	num_test_planes = test_crtc->num_planes;
	for (i = 0; i < num_test_planes; i++) {
		plane[i] = get_sp_plane(dev, test_crtc);
		if (!plane[i]) {
			printf("no unused planes available\n");
			goto out;
		}

		plane[i]->bo = create_sp_bo(dev, plane_w, plane_h, 16, 32,
				plane[i]->format, 0);
		if (!plane[i]->bo) {
			printf("failed to create plane bo\n");
			goto out;
		}

		fill_bo(plane[i]->bo, 0xFF, 0xFF, 0xFF, 0xFF);
	}

	pset = drmModePropertySetAlloc();
	if (!pset) {
		printf("Failed to allocate the property set\n");
		goto out;
	}

	while (!terminate) {
		FD_ZERO(&fds);
		FD_SET(dev->fd, &fds);

		incrementor(&x_inc, &x, 5, 0,
			test_crtc->crtc->mode.hdisplay - plane_w);
		incrementor(&y_inc, &y, 5, 0, test_crtc->crtc->mode.vdisplay -
						plane_h * num_test_planes);

		for (j = 0; j < num_test_planes; j++) {
			ret = set_sp_plane_pset(dev, plane[j], pset, test_crtc,
					x, y + j * plane_h);
			if (ret) {
				printf("failed to move plane %d\n", ret);
				goto out;
			}
		}

		ret = drmModePropertySetCommit(dev->fd,
				DRM_MODE_PAGE_FLIP_EVENT, NULL, pset);
		if (ret) {
			printf("failed to commit properties ret=%d\n", ret);
			goto out;
		}

		do {
			ret = select(dev->fd + 1, &fds, NULL, NULL, NULL);
		} while (ret == -1 && errno == EINTR);

		if (FD_ISSET(dev->fd, &fds))
			drmHandleEvent(dev->fd, &event_context);
	}

	drmModePropertySetFree(pset);

	for (i = 0; i < num_test_planes; i++)
		put_sp_plane(plane[i]);

out:
	destroy_sp_dev(dev);
	free(plane);
	return ret;
}
