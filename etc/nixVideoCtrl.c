// https://www.kernel.org/doc/html/v4.14/media/uapi/v4l/control.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/videodev2.h>

static int fd;
static struct v4l2_queryctrl queryctrl;
static struct v4l2_querymenu querymenu;

static const char * typeStr(__u32 type) {
	switch (type) {
	case V4L2_CTRL_TYPE_INTEGER: return "integer 32-bit"; break;
	case V4L2_CTRL_TYPE_BOOLEAN: return "boolean"; break;
	case V4L2_CTRL_TYPE_MENU: return "menu"; break;
	case V4L2_CTRL_TYPE_INTEGER_MENU: return "integer 64-bit menu"; break;
	case V4L2_CTRL_TYPE_BITMASK: return "32-bit bitmask"; break;
	case V4L2_CTRL_TYPE_BUTTON: return "button"; break;
	case V4L2_CTRL_TYPE_INTEGER64: return "integer 64-bit"; break;
	case V4L2_CTRL_TYPE_STRING: return "string"; break;
	case V4L2_CTRL_TYPE_CTRL_CLASS: return "control class"; break;
#ifdef V4L2_CTRL_TYPE_U8
	case V4L2_CTRL_TYPE_U8: return "unsigned integer 8-bit"; break;
#endif /* V4L2_CTRL_TYPE_U8 */
#ifdef V4L2_CTRL_TYPE_U16
	case V4L2_CTRL_TYPE_U16: return "unsigned integer 16-bit"; break;
#endif /* V4L2_CTRL_TYPE_U16 */
#ifdef V4L2_CTRL_TYPE_U32
	case V4L2_CTRL_TYPE_U32: return "unsigned integer 64-bit"; break;
#endif /* V4L2_CTRL_TYPE_U32 */
	default: return "unknown type";
	}
}

static void enumerateMenu(__u32 id) {
	printf("  Menu items:\n");
	memset(&querymenu, 0, sizeof(querymenu));
	querymenu.id = id;
	for (querymenu.index = queryctrl.minimum; querymenu.index <= queryctrl.maximum; querymenu.index++) {
		if (0 == ioctl(fd, VIDIOC_QUERYMENU, &querymenu)) {
			printf("  %s\n", querymenu.name);
		}
	}
}

int main(int argc, char ** argv) {
	int res = EXIT_FAILURE;
	fd = -1;
	if (argc < 2) {
		fprintf(stderr, "Error: Missing path to video4linux2 device.\n");
		goto onError;
	}
	
	errno = 0;
	fd = open(argv[1], O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open \"%s\". %s\n", argv[1], strerror(errno));
		goto onError;
	}
	
	printf("Enumerating all controls\n");
	printf("========================\n");
	memset(&queryctrl, 0, sizeof(queryctrl));
	queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
	while (ioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) == 0) {
		if ( ! (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) ) {
			printf("Control %s (%s)\n", queryctrl.name, typeStr(queryctrl.type));
			if (queryctrl.type == V4L2_CTRL_TYPE_MENU) {
				enumerateMenu(queryctrl.id);
			}
		}
		queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	}
	if (errno != EINVAL) {
		fprintf(stderr, "Error: ioctl failed for VIDIOC_QUERYCTRL. %s\n", strerror(errno));
		goto onError;
	}
	
	res = EXIT_SUCCESS;
onError:
	if (fd >= 0) close(fd);
	return res;
}
