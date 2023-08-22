/*
 *
 * Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define	LID_DEV_NAME	"hall_sensor"
#define HALL_INPUT	"/dev/input/hall_dev"

struct hall_data {
	int gpio;	/* device use gpio number */
	int irq;	/* device request irq number */
	bool wakeup;	/* device can wakeup system or not */
	struct work_struct work;
	struct input_dev *hall_dev;
	struct regulator *vddio;
	u32 min_uv;	/* device allow minimum voltage */
	u32 max_uv;	/* device allow max voltage */
	const char *dev_name;
	const char *gpio_name;
	const char *irq_name;
	const char *input_name;
	int status;
#ifndef CONFIG_FIH_LEO
	int iswitch;
#endif

	struct device* dev;
};

int disp_switch_gpio;
int lcm2_enable=0;

static void hall_sensor_work_handler(struct work_struct *work)
{
	int value;
	struct hall_data *data =
		container_of(work, struct hall_data, work);

	pm_wakeup_event(data->dev,20);

	value = gpio_get_value_cansleep(data->gpio);
	if (value) {
		input_report_key(data->hall_dev, KEY_HALL, 0);
		dev_dbg(&data->hall_dev->dev, "far\n");
		data->status = 1;
	/*
		if(data->gpio == 947)
		{
			lcm2_enable=0;
		}
	*/
	} else {
		input_report_key(data->hall_dev, KEY_HALL, 1);
		dev_dbg(&data->hall_dev->dev, "near\n");
		data->status = 0;
		/*if(data->gpio == 947)
		{
			lcm2_enable=1;
		}*/
	}
	input_sync(data->hall_dev);
	pr_debug("conqueror:hall sensor input event=%d\n",value);
}

struct timer_list hall_timer;
static void hall_timer_func(unsigned long dev)
{
	struct hall_data *data = (struct hall_data*)dev;
	schedule_work(&data->work);
}

static irqreturn_t hall_interrupt_handler(int irq, void *dev)
{
#if 0
	mod_timer(&hall_timer, 50+jiffies);
#else
	int value;
	struct hall_data *data = dev;
	value = gpio_get_value_cansleep(data->gpio);
	if (value) {
		input_report_key(data->hall_dev, KEY_HALL, 0);
		dev_dbg(&data->hall_dev->dev, "far\n");
		data->status = 1;
		if(data->gpio == 36)
		{
#if 0
			if(disp_switch_gpio>0)
			        gpio_direction_output(disp_switch_gpio, 0);
			else
			        pr_err("request gpio spi_cs_gpio failed: %d\n", disp_switch_gpio);
#endif
			lcm2_enable=0;
		}

	} else {
		input_report_key(data->hall_dev, KEY_HALL, 1);
		dev_dbg(&data->hall_dev->dev, "near\n");
		data->status = 0;
		if(data->gpio == 36)
		{
			lcm2_enable=1;
#if 0
			if(disp_switch_gpio>0)
			        gpio_direction_output(disp_switch_gpio, 1);
			else
			        pr_err("request gpio spi_cs_gpio failed: %d\n", disp_switch_gpio);
#endif
		}
	}
	input_sync(data->hall_dev);

	pm_wakeup_event(data->dev,20);
	pr_info("conqueror:hall sensor INT input event=%d\n",value);

#endif
	return IRQ_HANDLED;
}

static int hall_input_init(struct platform_device *pdev,
		struct hall_data *data)
{
	int err = -1;

	data->hall_dev = devm_input_allocate_device(&pdev->dev);
	if (!data->hall_dev) {
		dev_err(&data->hall_dev->dev,
				"input device allocation failed\n");
		return -EINVAL;
	}
	data->hall_dev->name = data->dev_name;
	data->hall_dev->phys = data->input_name;
	//input register type change from switch type to key type.
	/*	
	__set_bit(EV_SW, data->hall_dev->evbit);
	__set_bit(SW_LID, data->hall_dev->swbit);
	*/

    __set_bit(EV_KEY, data->hall_dev->evbit);
	__set_bit(KEY_HALL, data->hall_dev->keybit);

	err = input_register_device(data->hall_dev);
	if (err < 0) {
		dev_err(&data->hall_dev->dev,
				"unable to register input device %s\n",
				LID_DEV_NAME);
		return err;
	}

	return 0;
}

static int hall_config_regulator(struct platform_device *dev, bool on)
{
	struct hall_data *data = dev_get_drvdata(&dev->dev);
	int rc = 0;

	if (on) {
		data->vddio = devm_regulator_get(&dev->dev, "vddio");
		if (IS_ERR(data->vddio)) {
			rc = PTR_ERR(data->vddio);
			dev_err(&dev->dev, "Regulator vddio get failed rc=%d\n",
					rc);
			data->vddio = NULL;
			return rc;
		}

		if (regulator_count_voltages(data->vddio) > 0) {
			rc = regulator_set_voltage(
					data->vddio,
					data->min_uv,
					data->max_uv);
			if (rc) {
				dev_err(&dev->dev, "Regulator vddio Set voltage failed rc=%d\n",
						rc);
				goto deinit_vregs;
			}
		}
		return rc;
	} else {
		goto deinit_vregs;
	}

deinit_vregs:
	if (regulator_count_voltages(data->vddio) > 0)
		regulator_set_voltage(data->vddio, 0, data->max_uv);

	return rc;
}

static int hall_set_regulator(struct platform_device *dev, bool on)
{
	struct hall_data *data = dev_get_drvdata(&dev->dev);
	int rc = 0;

	if (on) {
		if (!IS_ERR_OR_NULL(data->vddio)) {
			rc = regulator_enable(data->vddio);
			if (rc) {
				dev_err(&dev->dev, "Enable regulator vddio failed rc=%d\n",
					rc);
				goto disable_regulator;
			}
		}
		return rc;
	} else {
		if (!IS_ERR_OR_NULL(data->vddio)) {
			rc = regulator_disable(data->vddio);
			if (rc)
				dev_err(&dev->dev, "Disable regulator vddio failed rc=%d\n",
					rc);
		}
		return 0;
	}

disable_regulator:
	if (!IS_ERR_OR_NULL(data->vddio))
		regulator_disable(data->vddio);
	return rc;
}

#ifdef CONFIG_OF
static int hall_parse_dt(struct device *dev, struct hall_data *data)
{
	unsigned int tmp;
	u32 tempval;
	int rc;
	int disp_switch_gpio_tmp,value;
	struct device_node *np = dev->of_node;

	//default hall gpio status is high

	data->gpio = of_get_named_gpio_flags(dev->of_node,
			"linux,gpio-int", 0, &tmp);
	if (!gpio_is_valid(data->gpio)) {
		dev_err(dev, "hall gpio is not valid\n");
		return -EINVAL;
	}

    disp_switch_gpio_tmp = of_get_named_gpio(dev->of_node,
               "qcom,platform-spi-switch-gpio", 0);

	value = gpio_get_value_cansleep(data->gpio);
	pr_info("conqueror:%s, switch=%d,val=%d \n",__func__,disp_switch_gpio_tmp,value);

	if (!gpio_is_valid(disp_switch_gpio_tmp))
	{
               pr_err("%s:%d, spi-switch-gpio gpio not specified\n",
                                               __func__, __LINE__);
		setup_timer(&hall_timer, hall_timer_func,(unsigned long)data);
		INIT_WORK(&data->work,hall_sensor_work_handler);
	}
	else
	{
		rc = gpio_request(disp_switch_gpio_tmp, "panel_switch_gpio");
		if (rc) {
			pr_err("%s: panel switch gpio request fail!\n", __func__);
		}
		else
			disp_switch_gpio = disp_switch_gpio_tmp;
	}


	data->wakeup = of_property_read_bool(np, "linux,wakeup");

	rc = of_property_read_u32(np, "linux,max-uv", &tempval);
	if (rc) {
		dev_err(dev, "unable to read max-uv\n");
		return -EINVAL;
	}
	data->max_uv = tempval;

	rc = of_property_read_u32(np, "linux,min-uv", &tempval);
	if (rc) {
		dev_err(dev, "unable to read min-uv\n");
		return -EINVAL;
	}
	data->min_uv = tempval;

	rc = of_property_read_string(np, "hall,gpio_name", &data->gpio_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "unable to read gpio name!\n");
		return -EINVAL;
	}

	rc = of_property_read_string(np, "hall,irq_name", &data->irq_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "unable to read irq name!\n");
		return -EINVAL;
	}

	rc = of_property_read_string(np, "hall,dev_name", &data->dev_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "unable to read device name!\n");
		return -EINVAL;
	}

	rc = of_property_read_string(np, "hall,input_name", &data->input_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "unable to read input name!\n");
		return -EINVAL;
	}

	pr_info("conqueror:%s, gpioname=%s,irqname=%s,dev_name=%s,input_name=%s \n",
			__func__,data->gpio_name,data->irq_name,data->dev_name,data->input_name);

	return 0;
}
#else
static int hall_parse_dt(struct device *dev, struct hall_data *data)
{
	return -EINVAL;
}
#endif


ssize_t hall_status_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct hall_data * data = dev_get_drvdata(dev);
	return sprintf(buf,"%d",data->status ? 0 : 1);
}

DEVICE_ATTR(hall_status,S_IRUGO,hall_status_show,NULL);

#ifndef CONFIG_FIH_LEO
extern void mdss_spi_panel_lighton_ext(int enable);

ssize_t hall_switch_show(struct device *dev,struct device_attribute *attr,char *buf)
{
	struct hall_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->iswitch);
}

ssize_t hall_switch_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	struct hall_data *data = dev_get_drvdata(dev);
	u32 value = -1;
	int ret = 0;

	ret = kstrtouint(buf, 10, &value);
	if (ret) {
		pr_err("kstrtoint failed. ret=%d\n", ret);
		return ret;
	}
	pr_info("hall_sensor0 data->gpio = %d,iswitch=%d, value=%d, disp_switch_gpio = %d\n", data->gpio, data->iswitch, value, disp_switch_gpio);

	if (data->gpio != 36) {
		pr_err("hall_sensor0 data->gpio =%d, value=%d\n", data->gpio, value);
		return count;
	}

	if (data->iswitch == value) {
		pr_err("hall_sensor0 iswitch is same as value, so return.\n");
		return count;
	}

	if (value == 0) {
		dev_dbg(&data->hall_dev->dev, "main display\n");

		if (disp_switch_gpio >0) {
			ret = gpio_direction_output(disp_switch_gpio, 0); //main
			//mdss_spi_panel_lighton_ext(0);
			data->iswitch = 0;
		}

		pr_info("conqueror:switch to main display");
	} else if (value == 1) {
		dev_dbg(&data->hall_dev->dev, "ext display\n");

		if (disp_switch_gpio > 0) {
		    ret =gpio_direction_output(disp_switch_gpio, 1); //ext
		    mdss_spi_panel_lighton_ext(1);
		    data->iswitch = 1;
		}

		pr_info("conqueror:switch to ext display");
	}

	pr_info("conqueror:hall_status_store done\n");

	return count;
}

DEVICE_ATTR(hall_switch, 0640, hall_switch_show, hall_switch_store);
#endif

static int hall_driver_probe(struct platform_device *dev)
{
	struct hall_data *data;
	int err = 0;
	int irq_flags;

	dev_dbg(&dev->dev, "hall_driver probe\n");
	data = devm_kzalloc(&dev->dev, sizeof(struct hall_data), GFP_KERNEL);
	if (data == NULL) {
		err = -ENOMEM;
		dev_err(&dev->dev,
				"failed to allocate memory %d\n", err);
		goto exit;
	}
	dev_set_drvdata(&dev->dev, data);
	if (dev->dev.of_node) {
		err = hall_parse_dt(&dev->dev, data);
		if (err < 0) {
			dev_err(&dev->dev, "Failed to parse device tree\n");
			goto exit;
		}
	} else if (dev->dev.platform_data != NULL) {
		memcpy(data, dev->dev.platform_data, sizeof(*data));
	} else {
		dev_err(&dev->dev, "No valid platform data.\n");
		err = -ENODEV;
		goto exit;
	}

	err = hall_input_init(dev, data);
	if (err < 0) {
		dev_err(&dev->dev, "input init failed\n");
		goto exit;
	}

	if (!gpio_is_valid(data->gpio)) {
		dev_err(&dev->dev, "gpio is not valid\n");
		err = -EINVAL;
		goto exit;
	}

	irq_flags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
		| IRQF_ONESHOT;
	err = gpio_request_one(data->gpio, GPIOF_DIR_IN, data->gpio_name);
	if (err) {
		dev_err(&dev->dev, "unable to request gpio %d\n", data->gpio);
		goto exit;
	}

	data->dev = &dev->dev;
	data->irq = gpio_to_irq(data->gpio);
	err = devm_request_threaded_irq(&dev->dev, data->irq, NULL,
			hall_interrupt_handler,
			irq_flags, data->irq_name, data);
	if (err < 0) {
		dev_err(&dev->dev, "request irq failed : %d\n", data->irq);
		goto free_gpio;
	}

	device_init_wakeup(&dev->dev, data->wakeup);
	enable_irq_wake(data->irq);

	err = hall_config_regulator(dev, true);
	if (err < 0) {
		dev_err(&dev->dev, "Configure power failed: %d\n", err);
		goto free_irq;
	}

	err = hall_set_regulator(dev, true);
	if (err < 0) {
		dev_err(&dev->dev, "power on failed: %d\n", err);
		goto err_regulator_init;
	}

	device_create_file(&dev->dev,&dev_attr_hall_status);
#ifndef CONFIG_FIH_LEO
	device_create_file(&dev->dev,&dev_attr_hall_switch);
#endif

	//the logic is same as interrupt handler
#ifdef CONFIG_FIH_LEO
	data->status = 1;
#else
	data->status = gpio_get_value_cansleep(data->gpio);
	data->iswitch = 0;
#endif

	pr_info("conqueror:%s, hall=%d \n",__func__,data->status);

	if (data->status) {
		input_report_key(data->hall_dev, KEY_HALL, 0);
	} else {
		input_report_key(data->hall_dev, KEY_HALL, 1);
	}
	input_sync(data->hall_dev);

	return 0;

err_regulator_init:
	hall_config_regulator(dev, false);
free_irq:
	disable_irq_wake(data->irq);
	device_init_wakeup(&dev->dev, 0);
free_gpio:
	gpio_free(data->gpio);
exit:
	return err;
}

static int hall_driver_remove(struct platform_device *dev)
{
	struct hall_data *data = dev_get_drvdata(&dev->dev);

	disable_irq_wake(data->irq);
	device_init_wakeup(&dev->dev, 0);
	if (data->gpio)
		gpio_free(data->gpio);
	hall_set_regulator(dev, false);
	hall_config_regulator(dev, false);

	return 0;
}

static struct platform_device_id hall_id[] = {
	{LID_DEV_NAME, 0 },
	{ },
};


#ifdef CONFIG_OF
static struct of_device_id hall_match_table[] = {
	{.compatible = "hall-switch", },
	{ },
};
#endif

static struct platform_driver hall_driver = {
	.driver = {
		.name = LID_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hall_match_table),
	},
	.probe = hall_driver_probe,
	.remove = hall_driver_remove,
	.id_table = hall_id,
};

static int __init hall_init(void)
{
	return platform_driver_register(&hall_driver);
}

static void __exit hall_exit(void)
{
	platform_driver_unregister(&hall_driver);
}

module_init(hall_init);
module_exit(hall_exit);
MODULE_DESCRIPTION("Hall sensor driver");
MODULE_LICENSE("GPL v2");
