#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include "msm_sensor.h"

//#define CAM_TCT_DEBUG
#ifdef CAM_TCT_DEBUG
#define CDBG(fmt, args...) printk(KERN_ERR"[CAM_TCT]"fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define CAM_ERR(fmt, args...) printk(KERN_ERR"[CAM_TCT]"fmt, ##args)

int32_t sensor_sysfs_init(char* sensor_name,int position);
void cur_sensor_update(struct msm_sensor_ctrl_t *s_ctrl);
int32_t camera_dynamic_regs_debug(struct msm_sensor_ctrl_t *s_ctrl);
