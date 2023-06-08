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

	while (!terminate) {
		incrementor(&x_inc, &x, 5, 0,
			test_crtc->crtc->mode.hdisplay - plane_w);
		incrementor(&y_inc, &y, 5, 0, test_crtc->crtc->mode.vdisplay -
						plane_h * num_test_planes);

		for (j = 0; j < num_test_planes; j++) {
			ret = set_sp_plane(dev, plane[j], test_crtc,
					x, y + j * plane_h);
			if (ret) {
				printf("failed to set plane %d %d\n", j, ret);
				goto out;
			}
		}
		usleep(15 * 1000);
	}

	for (i = 0; i < num_test_planes; i++)
		put_sp_plane(plane[i]);

out:
	destroy_sp_dev(dev);
	free(plane);
	return ret;
}
