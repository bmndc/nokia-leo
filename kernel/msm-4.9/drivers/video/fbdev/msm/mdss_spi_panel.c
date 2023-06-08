/* Copyright (c) 2017-2018, 2020, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/qpnp/pin.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/leds.h>
#include <linux/qpnp/pwm.h>
#include <linux/of_device.h>

#include "mdss.h"
#include "mdss_panel.h"
#include "mdss_spi_panel.h"
#include "mdss_spi_client.h"
#include "mdp3.h"

extern int disp_switch_gpio;
extern int lcm2_enable;
static int lcd_vendor_source = -1;
static int lcd_first_lighton = 0;

#ifdef DEBUG_EN
	#define mdss_spi_dbg(format, arg...)   pr_info(format, ##arg)
#else
	#define mdss_spi_dbg(format, arg...) do { } while (0)
#endif

#define MDSS_INIT_WINDOW_EACH_FRAME

#ifndef CONFIG_FIH_LEO
static unsigned int iV285 =0;
#endif

struct spi_panel_data	*extpan_ctrl_pdata = NULL;
static int mdss_spi_panel_pinctrl_set_state(
		struct spi_panel_data *ctrl_pdata,
		bool active);

static int mdss_spi_panel_reset(struct mdss_panel_data *pdata, int enable);
static int mdss_spi_panel_unblank(struct mdss_panel_data *pdata);

/*
* try to directly operate the ext display
*/
void mdss_spi_panel_lighton_ext(int enable)
{
	int ret;
	struct mdss_panel_data* panel_data;

	pr_info("conqueror: mdss_spi_panel_lighton_ext(%d)\n", enable);

	if (!enable) {
		pr_err("conqueror:%s, power off ext lcm? \n", __func__);
		return;
	}

	panel_data = &extpan_ctrl_pdata->panel_data;
	if (panel_data == NULL) {
		pr_err("conqueror:%s, panel_data is null \n", __func__);
		return;
	}

	//extpan_ctrl_pdata is
	//rc = mdss_spi_panel_power_ctrl(pdata, MDSS_PANEL_POWER_ON);
	ret = msm_mdss_enable_vreg(
		extpan_ctrl_pdata->panel_power_data.vreg_config,
		extpan_ctrl_pdata->panel_power_data.num_vreg, 1);

	if (ret)
		pr_err("%s: failed to enable vregs for %s\n", __func__, "PANEL_PM");


	mdss_spi_panel_pinctrl_set_state(extpan_ctrl_pdata, true);
	mdss_spi_panel_reset(panel_data, 1);

	//case MDSS_EVENT_UNBLANK:
	pr_err("conqueror: %s, UNBLANK \n", __func__);
	ret = mdss_spi_panel_unblank(panel_data);

	//case MDSS_EVENT_PANEL_ON:
	extpan_ctrl_pdata->ctrl_state |= CTRL_STATE_MDP_ACTIVE;
	pr_info("conqueror: %s, PANEL_ON, ret=%d\n", __func__, ret );

	return;

}

DEFINE_LED_TRIGGER(bl_led_trigger);
static int mdss_spi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct spi_panel_data *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_err("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	if (!gpio_is_valid(ctrl_pdata->disp_dc_gpio)) {
		pr_err("%s:%d, dc line not configured\n",
			   __func__, __LINE__);

		gpio_free(ctrl_pdata->rst_gpio);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("display reset gpio %d request failed,rc=%d\n",ctrl_pdata->rst_gpio,rc);
		return rc;
	}

	rc = gpio_request(ctrl_pdata->disp_dc_gpio, "disp_dc");
	if (rc) {
		pr_err("display dc gpio request failed,rc=%d\n",rc);
		return rc;
	}

	rc = gpio_direction_output(ctrl_pdata->rst_gpio,1);
	rc = gpio_direction_output(ctrl_pdata->disp_dc_gpio,1);

	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {

		if (!pinfo->cont_splash_enabled) {
			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
					pdata->panel_info.rst_seq[i]);

				if (pdata->panel_info.rst_seq[++i])
					usleep_range(pinfo->rst_seq[i] * 1000,
					pinfo->rst_seq[i] * 1000);
			}
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_info("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_err("%s: Reset panel done\n", __func__);
		}
	} else {

		pr_info("conqueror:mdss_spi_panel_reset, light off display,will make square via rest[%d] \n",ctrl_pdata->rst_gpio);

		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		usleep_range(1*1000,1000);
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		usleep_range(10*1000,10000);
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
		usleep_range(5*1000,5000);

		gpio_set_value(ctrl_pdata->disp_dc_gpio, 1);
	}

	gpio_free(ctrl_pdata->rst_gpio);
	gpio_free(ctrl_pdata->disp_dc_gpio);

	return rc;
}


static int mdss_spi_panel_pinctrl_set_state(
	struct spi_panel_data *ctrl_pdata,
	bool active)
{
	struct pinctrl_state *pin_state;
	int rc = -EFAULT;

	if (IS_ERR_OR_NULL(ctrl_pdata->pin_res.pinctrl))
		return PTR_ERR(ctrl_pdata->pin_res.pinctrl);

	pin_state = active ? ctrl_pdata->pin_res.gpio_state_active
				: ctrl_pdata->pin_res.gpio_state_suspend;
	if (!IS_ERR_OR_NULL(pin_state)) {
		rc = pinctrl_select_state(ctrl_pdata->pin_res.pinctrl,
				pin_state);
		if (rc)
			pr_err("%s: can not set %s pins\n", __func__,
			       active ? MDSS_PINCTRL_STATE_DEFAULT
			       : MDSS_PINCTRL_STATE_SLEEP);
	} else {
		pr_err("%s: invalid '%s' pinstate\n", __func__,
		       active ? MDSS_PINCTRL_STATE_DEFAULT
		       : MDSS_PINCTRL_STATE_SLEEP);
	}
	return rc;
}


static int mdss_spi_panel_pinctrl_init(struct platform_device *pdev)
{
	struct spi_panel_data *ctrl_pdata;

	ctrl_pdata = platform_get_drvdata(pdev);
	ctrl_pdata->pin_res.pinctrl = devm_pinctrl_get(&pdev->dev);

	if (IS_ERR_OR_NULL(ctrl_pdata->pin_res.pinctrl)) {
		pr_err("%s: failed to get pinctrl\n", __func__);
		return PTR_ERR(ctrl_pdata->pin_res.pinctrl);
	}

	ctrl_pdata->pin_res.gpio_state_active
		= pinctrl_lookup_state(ctrl_pdata->pin_res.pinctrl,
				MDSS_PINCTRL_STATE_DEFAULT);

	if (IS_ERR_OR_NULL(ctrl_pdata->pin_res.gpio_state_active))
		pr_warn("%s: can not get default pinstate\n", __func__);

	ctrl_pdata->pin_res.gpio_state_suspend
		= pinctrl_lookup_state(ctrl_pdata->pin_res.pinctrl,
				MDSS_PINCTRL_STATE_SLEEP);

	if (IS_ERR_OR_NULL(ctrl_pdata->pin_res.gpio_state_suspend))
		pr_warn("%s: can not get sleep pinstate\n", __func__);

	return 0;
}


static int mdss_spi_panel_power_on(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);
	if (!mdp3_res->secure_reg_on) {
#ifndef CONFIG_FIH_LEO
	if (iV285 == 0) {
#endif
		ret = msm_mdss_enable_vreg(
			ctrl_pdata->panel_power_data.vreg_config,
			ctrl_pdata->panel_power_data.num_vreg, 1);
		if (ret) {
			pr_err("%s: failed to enable vregs for %s\n",
				__func__, "PANEL_PM");
		}
#ifndef CONFIG_FIH_LEO
		else {
			iV285++;
		}
	}
#endif
	}

	/*
	 * If continuous splash screen feature is enabled, then we need to
	 * request all the GPIOs that have already been configured in the
	 * bootloader. This needs to be done irresepective of whether
	 * the lp11_init flag is set or not.
	 */
	if (pdata->panel_info.cont_splash_enabled) {
		if (mdss_spi_panel_pinctrl_set_state(ctrl_pdata, true))
			pr_debug("reset enable: pinctrl not enabled\n");

		ret = mdss_spi_panel_reset(pdata, 1);
		if (ret)
			pr_err("%s: Panel reset failed. rc=%d\n",
					__func__, ret);
	}

	lcd_first_lighton = 1;

	return ret;
}


static int mdss_spi_panel_power_off(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		ret = -EINVAL;
		goto end;
	}
	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	ret = mdss_spi_panel_reset(pdata, 0);
	if (ret) {
		pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
		ret = 0;
	}

	if (mdss_spi_panel_pinctrl_set_state(ctrl_pdata, false))
		pr_warn("reset disable: pinctrl not enabled\n");

	if (!mdp3_res->secure_reg_on) {
#ifndef CONFIG_FIH_LEO
		if (iV285 > 1) {
#endif
		ret = msm_mdss_enable_vreg(
			ctrl_pdata->panel_power_data.vreg_config,
			ctrl_pdata->panel_power_data.num_vreg, 0);
		if (ret)
			pr_err("%s: failed to disable vregs for %s\n",
				__func__, "PANEL_PM");
#ifndef CONFIG_FIH_LEO
		else
			iV285--;
		}
#endif
	}

end:
	return ret;
}


static int mdss_spi_panel_power_ctrl(struct mdss_panel_data *pdata,
	int power_state)
{
	int ret;
	struct mdss_panel_info *pinfo;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pinfo = &pdata->panel_info;
	pr_debug("%s: cur_power_state=%d req_power_state=%d\n", __func__,
		pinfo->panel_power_state, power_state);

	if (pinfo->panel_power_state == power_state) {
		pr_debug("%s: no change needed\n", __func__);
		return 0;
	}

	switch (power_state) {
	case MDSS_PANEL_POWER_OFF:
		ret = mdss_spi_panel_power_off(pdata);
		break;
	case MDSS_PANEL_POWER_ON:
		ret = mdss_spi_panel_power_on(pdata);
		break;
	default:
		pr_err("%s: unknown panel power state requested (%d)\n",
			__func__, power_state);
		ret = -EINVAL;
	}

	if (!ret)
		pinfo->panel_power_state = power_state;

	return ret;
}

static int mdss_spi_panel_unblank(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	if (!(ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT)) {
		ret = ctrl_pdata->on(pdata);
		if (ret) {
			pr_err("%s: unable to initialize the panel\n",
							__func__);
			return ret;
		}
		ctrl_pdata->ctrl_state |= CTRL_STATE_PANEL_INIT;
	}

	return ret;
}

static int mdss_spi_panel_blank(struct mdss_panel_data *pdata, int power_state)
{
	int ret = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
		ret = ctrl_pdata->off(pdata);
		if (ret) {
			pr_err("%s: Panel OFF failed\n", __func__);
			return ret;
		}
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
	}

	return ret;
}


static int mdss_spi_panel_event_handler(struct mdss_panel_data *pdata,
				  int event, void *arg)
{
	int rc = 0;
	struct spi_panel_data *ctrl_pdata = NULL;
	int power_state;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	pr_info("mdss_spi_panel_event_handler event=0x%x \n",event);

	switch (event) {
	case MDSS_EVENT_LINK_READY:
#ifndef CONFIG_FIH_LEO
		if (lcm2_enable && extpan_ctrl_pdata) {
			ctrl_pdata = extpan_ctrl_pdata;
			pdata = &extpan_ctrl_pdata->panel_data;
		}
		if (strstr(ctrl_pdata->panel_data.panel_info.panel_name, "ext panel")) {
			//pr_err("%s:open ext panel \n",__func__);
			pr_info("%s: open ext panel, sw=%d \n", __func__, disp_switch_gpio);
			if (disp_switch_gpio > 0)
				gpio_direction_output(disp_switch_gpio, 1);
		} else {
			//pr_err("%s:open main panel \n",__func__);
			pr_info("%s: open main panel, sw=%d\n", __func__, disp_switch_gpio);
			if (disp_switch_gpio > 0)
				gpio_direction_output(disp_switch_gpio, 0);
		}
#endif
		rc = mdss_spi_panel_power_ctrl(pdata, MDSS_PANEL_POWER_ON);
		if (rc) {
			pr_err("%s:Panel power on failed. rc=%d\n",
							__func__, rc);
			return rc;
		}
		mdss_spi_panel_pinctrl_set_state(ctrl_pdata, true);
		mdss_spi_panel_reset(pdata, 1);
		break;
	case MDSS_EVENT_UNBLANK:
		rc = mdss_spi_panel_unblank(pdata);
		break;
	case MDSS_EVENT_PANEL_ON:
		ctrl_pdata->ctrl_state |= CTRL_STATE_MDP_ACTIVE;
		break;
	case MDSS_EVENT_BLANK:
		power_state = (int) (unsigned long) arg;
		break;
	case MDSS_EVENT_PANEL_OFF:
		if (!lcm2_enable && extpan_ctrl_pdata) {
			ctrl_pdata = extpan_ctrl_pdata;
			pdata = &extpan_ctrl_pdata->panel_data;
		}
		power_state = (int) (unsigned long) arg;
		ctrl_pdata->ctrl_state &= ~CTRL_STATE_MDP_ACTIVE;
		rc = mdss_spi_panel_blank(pdata, power_state);
		rc = mdss_spi_panel_power_ctrl(pdata, power_state);
		break;
	default:
		pr_debug("%s: unhandled event=%d\n", __func__, event);
		break;
	}
	pr_info("%s-:event=%d, rc=%d\n", __func__, event, rc);
	return rc;
}

int is_spi_panel_continuous_splash_on(struct mdss_panel_data *pdata)
{
	int i = 0, voltage = 0;
	struct mdss_vreg *vreg;
	int num_vreg;
	struct spi_panel_data *ctrl_pdata = NULL;

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
			panel_data);
	vreg = ctrl_pdata->panel_power_data.vreg_config;
	num_vreg = ctrl_pdata->panel_power_data.num_vreg;

	for (i = 0; i < num_vreg; i++) {
		if (regulator_is_enabled(vreg[i].vreg) <= 0)
			return false;
		voltage = regulator_get_voltage(vreg[i].vreg);
		if (!(voltage >= vreg[i].min_voltage &&
			 voltage <= vreg[i].max_voltage))
			return false;
	}

	return true;
}

static void enable_spi_panel_te_irq(struct spi_panel_data *ctrl_pdata,
							bool enable)
{
	static int te_irq_count;

	if (!gpio_is_valid(ctrl_pdata->disp_te_gpio)) {
		pr_err("%s:%d,SPI panel TE GPIO not configured\n",
			   __func__, __LINE__);
		return;
	}
	mutex_lock(&ctrl_pdata->te_mutex);
	if (enable) {
		if (++te_irq_count == 1)
			enable_irq(gpio_to_irq(ctrl_pdata->disp_te_gpio));
	} else {
		if (--te_irq_count == 0)
			disable_irq(gpio_to_irq(ctrl_pdata->disp_te_gpio));
	}
	mutex_unlock(&ctrl_pdata->te_mutex);
}

#ifdef MDSS_INIT_WINDOW_EACH_FRAME
static int mdss_spi_init_window_cmds(struct spi_panel_data* ctrl)
{
	int i;
	int dc_status = 0;

	if (ctrl == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}


	for (i = 0; i < ctrl->init_window_cmds.cmd_cnt; i++) {
		dc_status = gpio_get_value(ctrl->disp_dc_gpio);
		/* pull down dc gpio indicate this is command */
		gpio_set_value(ctrl->disp_dc_gpio, 0);
		mdss_spi_tx_command(ctrl->init_window_cmds.cmds[i].command);
		gpio_set_value((ctrl->disp_dc_gpio), 1);

		if (ctrl->init_window_cmds.cmds[i].dchdr.dlen > 1) {
			mdss_spi_tx_parameter(ctrl->init_window_cmds.cmds[i].parameter,
					ctrl->init_window_cmds.cmds[i].dchdr.dlen-1);
		}

		if (ctrl->init_window_cmds.cmds[i].dchdr.wait != 0)
			msleep(ctrl->init_window_cmds.cmds[i].dchdr.wait);
	}

	if(dc_status != 0 )
		gpio_set_value(ctrl->disp_dc_gpio, dc_status);

	pr_debug("%s:-\n", __func__);
	return 0;
}
//
#endif

int mdss_spi_panel_kickoff(struct mdss_panel_data *pdata,
			char __iomem *buf, int len, int dma_stride)
{
	struct spi_panel_data *ctrl_pdata = NULL;
	char *tx_buf;
	int rc = 0;
	int panel_yres;
	int panel_xres;
	int padding_length = 0;
	int actual_stride = 0;
	int byte_per_pixel = 0;
	int scan_count = 0;
	int idelay = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	tx_buf = ctrl_pdata->tx_buf;
	panel_xres = ctrl_pdata->panel_data.panel_info.xres;
	panel_yres = ctrl_pdata->panel_data.panel_info.yres;

	byte_per_pixel = ctrl_pdata->panel_data.panel_info.bpp / 8;
	actual_stride = panel_xres * byte_per_pixel;
	padding_length = dma_stride - actual_stride;

	/* remove the padding and copy to continuous buffer */
	while (scan_count < panel_yres) {
		memcpy((tx_buf + scan_count * actual_stride),
			(buf + scan_count * (actual_stride + padding_length)),
				actual_stride);
		scan_count++;
	}

#ifdef MDSS_INIT_WINDOW_EACH_FRAME
	mdss_spi_init_window_cmds(ctrl_pdata);
#endif

	if (!strstr(ctrl_pdata->panel_data.panel_info.panel_name, "ext panel")) {
		idelay = SPI_PANEL_TE_TIMEOUT;
		enable_spi_panel_te_irq(ctrl_pdata, true);
	} else {
		idelay = 0;
		rc = -1;
	}

	mutex_lock(&ctrl_pdata->spi_tx_mutex);
	if (idelay == SPI_PANEL_TE_TIMEOUT) {
		reinit_completion(&ctrl_pdata->spi_panel_te);

		rc = wait_for_completion_timeout(&ctrl_pdata->spi_panel_te,
					msecs_to_jiffies(idelay /*SPI_PANEL_TE_TIMEOUT*/));
	}
	if (rc == 0)
		pr_err("wait panel TE time out\n");

#ifdef __MDSS_INIT_WINDOW_EACH_FRAME
	mdss_spi_init_window_cmds(ctrl_pdata);
#endif

	rc = mdss_spi_tx_pixel(tx_buf, ctrl_pdata->byte_pre_frame);
	mutex_unlock(&ctrl_pdata->spi_tx_mutex);
	enable_spi_panel_te_irq(ctrl_pdata, false);

	return rc;
}

unsigned char rotatel_left(unsigned char *p,unsigned char len)
{
	unsigned char valBit;
	if(!len)
		return 0;
	len--;
	valBit = *p>>7;
	do{
		*p = (*p<<1)|(*(p+1)>>7);
		p++;
	}while(--len);
	*p = (*p<<1)|valBit;
	return 1;
}

static int mdss_spi_read_panel_data(struct mdss_panel_data *pdata,
		u8 reg_addr, u8 *data, u8 len)
{
	int rc = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
		panel_data);

	mutex_lock(&ctrl_pdata->spi_tx_mutex);
	gpio_direction_output(ctrl_pdata->disp_dc_gpio, 0);
	rc = mdss_spi_read_data(reg_addr, data, len);
	pr_info("%s:org value=0x%x\n",__func__,data[0]);
#ifndef CONFIG_FIH_LEO
	rotatel_left(data, len);
#endif
	gpio_direction_output(ctrl_pdata->disp_dc_gpio, 1);
	mutex_unlock(&ctrl_pdata->spi_tx_mutex);

	return rc;
}

static int mdss_spi_panel_on(struct mdss_panel_data *pdata)
{
	struct spi_panel_data *ctrl = NULL;
	struct mdss_panel_info *pinfo;
	int i;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}
	pinfo = &pdata->panel_info;
	ctrl = container_of(pdata, struct spi_panel_data,
				panel_data);
	if (lcm2_enable && extpan_ctrl_pdata) {
		ctrl = extpan_ctrl_pdata;
		pinfo = &ctrl->panel_data.panel_info;
	}

	for (i = 0; i < ctrl->on_cmds.cmd_cnt; i++) {
		/* pull down dc gpio indicate this is command */
		gpio_direction_output(ctrl->disp_dc_gpio, 0);
		mdss_spi_tx_command(ctrl->on_cmds.cmds[i].command);
		gpio_direction_output((ctrl->disp_dc_gpio), 1);

		if (ctrl->on_cmds.cmds[i].dchdr.dlen > 1) {
			mdss_spi_tx_parameter(ctrl->on_cmds.cmds[i].parameter,
					ctrl->on_cmds.cmds[i].dchdr.dlen-1);
		}
		if (ctrl->on_cmds.cmds[i].dchdr.wait != 0)
			msleep(ctrl->on_cmds.cmds[i].dchdr.wait);
	}

	pinfo->blank_state = MDSS_PANEL_BLANK_UNBLANK;

	pr_info("%s:-\n", __func__);

	return 0;
}


static int mdss_spi_panel_off(struct mdss_panel_data *pdata)
{
	struct spi_panel_data *ctrl = NULL;
	struct mdss_panel_info *pinfo;
	int i;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pinfo = &pdata->panel_info;
	ctrl = container_of(pdata, struct spi_panel_data,
				panel_data);

	for (i = 0; i < ctrl->off_cmds.cmd_cnt; i++) {
		/* pull down dc gpio indicate this is command */
		gpio_direction_output(ctrl->disp_dc_gpio, 0);
		mdss_spi_tx_command(ctrl->off_cmds.cmds[i].command);
		gpio_direction_output((ctrl->disp_dc_gpio), 1);

		if (ctrl->off_cmds.cmds[i].dchdr.dlen > 1) {
			mdss_spi_tx_parameter(ctrl->off_cmds.cmds[i].parameter,
					ctrl->off_cmds.cmds[i].dchdr.dlen-1);
		}

		if (ctrl->off_cmds.cmds[i].dchdr.wait != 0)
			msleep(ctrl->off_cmds.cmds[i].dchdr.wait);
	}

	pinfo->blank_state = MDSS_PANEL_BLANK_BLANK;

	pr_info("%s:-\n", __func__);
	return 0;
}

static void mdss_spi_put_dt_vreg_data(struct device *dev,
	struct mdss_module_power *module_power)
{
	if (!module_power) {
		pr_err("%s: invalid input\n", __func__);
		return;
	}

	if (module_power->vreg_config) {
		devm_kfree(dev, module_power->vreg_config);
		module_power->vreg_config = NULL;
	}
	module_power->num_vreg = 0;
}


static int mdss_spi_get_panel_vreg_data(struct device *dev,
			struct mdss_module_power *mp)
{
	int i = 0, rc = 0;
	u32 tmp = 0;
	struct device_node *of_node = NULL, *supply_node = NULL;
	struct device_node *supply_root_node = NULL;

	if (!dev || !mp) {
		pr_err("%s: invalid input\n", __func__);
		rc = -EINVAL;
		return rc;
	}

	of_node = dev->of_node;

	mp->num_vreg = 0;

	supply_root_node = of_get_child_by_name(of_node,
				"qcom,panel-supply-entries");

	for_each_available_child_of_node(supply_root_node, supply_node) {
		mp->num_vreg++;
	}
	if (mp->num_vreg == 0) {
		pr_debug("%s: no vreg\n", __func__);
		goto novreg;
	} else {
		pr_debug("%s: vreg found. count=%d\n", __func__, mp->num_vreg);
	}

	mp->vreg_config = kcalloc(mp->num_vreg, sizeof(struct mdss_vreg),
				GFP_KERNEL);

	if (mp->vreg_config != NULL) {
		for_each_available_child_of_node(supply_root_node,
				supply_node) {
			const char *st = NULL;
			/* vreg-name */
			rc = of_property_read_string(supply_node,
				"qcom,supply-name", &st);
			if (rc) {
				pr_err("%s: error reading name. rc=%d\n",
					__func__, rc);
				goto error;
			}
			snprintf(mp->vreg_config[i].vreg_name,
				ARRAY_SIZE((mp->vreg_config[i].vreg_name)),
					"%s", st);
			/* vreg-min-voltage */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-min-voltage", &tmp);
			if (rc) {
				pr_err("%s: error reading min volt. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].min_voltage = tmp;

			/* vreg-max-voltage */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-max-voltage", &tmp);
			if (rc) {
				pr_err("%s: error reading max volt. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].max_voltage = tmp;

			/* enable-load */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-enable-load", &tmp);
			if (rc) {
				pr_err("%s: error read enable load. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].load[DSS_REG_MODE_ENABLE] = tmp;

			/* disable-load */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-disable-load", &tmp);
			if (rc) {
				pr_err("%s: error read disable load. rc=%d\n",
					__func__, rc);
				goto error;
			}
			mp->vreg_config[i].load[DSS_REG_MODE_DISABLE] = tmp;

			/* pre-sleep */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-pre-on-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error read pre on value\n",
						__func__);
				rc = 0;
			} else {
				mp->vreg_config[i].pre_on_sleep = tmp;
			}

			rc = of_property_read_u32(supply_node,
				"qcom,supply-pre-off-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error read pre off value\n",
						__func__);
				rc = 0;
			} else {
				mp->vreg_config[i].pre_off_sleep = tmp;
			}

			/* post-sleep */
			rc = of_property_read_u32(supply_node,
				"qcom,supply-post-on-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error read post on value\n",
						__func__);
				rc = 0;
			} else {
				mp->vreg_config[i].post_on_sleep = tmp;
			}

			rc = of_property_read_u32(supply_node,
				"qcom,supply-post-off-sleep", &tmp);
			if (rc) {
				pr_debug("%s: error read post off value\n",
						__func__);
				rc = 0;
			} else {
				mp->vreg_config[i].post_off_sleep = tmp;
			}

			++i;
		}
	}
	return rc;
error:
	kfree(mp->vreg_config);
	mp->vreg_config = NULL;

novreg:
	mp->num_vreg = 0;

	return rc;

}

static int mdss_spi_panel_parse_cmds(struct device_node *np,
		struct spi_panel_cmds *pcmds, char *cmd_key)
{
	const char *data;
	int blen = 0, len;
	char *buf, *bp;
	struct spi_ctrl_hdr *dchdr;
	int i, cnt;

	data = of_get_property(np, cmd_key, &blen);
	if (!data) {
		pr_err("%s: failed, key=%s\n", __func__, cmd_key);
		return -ENOMEM;
	}

	buf = kcalloc(blen, sizeof(char), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, data, blen);

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len >= sizeof(*dchdr)) {
		dchdr = (struct spi_ctrl_hdr *)bp;
		if (dchdr->dlen > len) {
			pr_err("%s: dtsi parse error, len=%d",
				__func__, dchdr->dlen);
			goto exit_free;
		}
		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		pr_err("%s: dcs_cmd=%x len=%d error",
				__func__, buf[0], len);
		goto exit_free;
	}

	pcmds->cmds = kcalloc(cnt, sizeof(struct spi_cmd_desc),
						GFP_KERNEL);
	if (!pcmds->cmds)
		goto exit_free;

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct spi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].command = bp;
		pcmds->cmds[i].parameter = bp + sizeof(char);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	pr_debug("%s: dcs_cmd=%x, len=%d, cmd_cnt=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt);

	return 0;

exit_free:
	kfree(buf);
	return -ENOMEM;
}

static int mdss_spi_panel_parse_reset_seq(struct device_node *np,
		u32 rst_seq[MDSS_SPI_RST_SEQ_LEN], u32 *rst_len,
		const char *name)
{
	int num = 0, i;
	int rc;
	struct property *data;
	u32 tmp[MDSS_SPI_RST_SEQ_LEN];

	*rst_len = 0;
	data = of_find_property(np, name, &num);
	num /= sizeof(u32);
	if (!data || !num || num > MDSS_SPI_RST_SEQ_LEN || num % 2) {
		pr_err("%s:%d, error reading %s, length found = %d\n",
			__func__, __LINE__, name, num);
	} else {
		rc = of_property_read_u32_array(np, name, tmp, num);
		if (rc)
			pr_err("%s:%d, error reading %s, rc = %d\n",
				__func__, __LINE__, name, rc);
		else {
			for (i = 0; i < num; ++i)
				rst_seq[i] = tmp[i];
			*rst_len = num;
		}
	}
	return 0;
}

static bool mdss_send_panel_cmd_for_esd(struct spi_panel_data *ctrl_pdata)
{

	if (ctrl_pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return false;
	}

	mutex_lock(&ctrl_pdata->spi_tx_mutex);
	mdss_spi_panel_on(&ctrl_pdata->panel_data);
	mutex_unlock(&ctrl_pdata->spi_tx_mutex);

	return true;
}

int  mdss_spi_esd_recovery(struct spi_panel_data *ctrl_pdata,struct mdss_panel_data *pdata )
{
	int rc;

	pr_info("%s: %d\n",__func__, __LINE__);
	//reset panel
	mdss_spi_panel_reset(pdata,1);

	//send init cmd
	mdss_spi_panel_on(pdata);

	//refresh framebuffer
	mutex_lock(&ctrl_pdata->spi_tx_mutex);

	reinit_completion(&ctrl_pdata->spi_panel_te);
	rc = wait_for_completion_timeout(&ctrl_pdata->spi_panel_te,
				 msecs_to_jiffies(SPI_PANEL_TE_TIMEOUT));

	if (rc == 0)
	{
		if(strstr(ctrl_pdata->panel_data.panel_info.panel_name,"ext panel"))
			pr_err("wait ext panel TE time out\n");
		else
			pr_err("wait main panel TE time out\n");
	}

	rc = mdss_spi_tx_pixel(ctrl_pdata->tx_buf, ctrl_pdata->byte_pre_frame);
	mutex_unlock(&ctrl_pdata->spi_tx_mutex);

	return rc;

}

static int mdss_spi_no_esd_cmd(struct mdss_panel_data *pdata)
{
	struct spi_panel_data *ctrl = NULL;
	int i;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl = container_of(pdata, struct spi_panel_data,
			panel_data);

	for (i = 0; i < ctrl->no_esd_cmds.cmd_cnt; i++) {
		/* pull down dc gpio indicate this is command */
		gpio_set_value(ctrl->disp_dc_gpio, 0);
		mdss_spi_tx_command(ctrl->no_esd_cmds.cmds[i].command);
		gpio_set_value((ctrl->disp_dc_gpio), 1);

		if (ctrl->no_esd_cmds.cmds[i].dchdr.dlen > 1) {
			mdss_spi_tx_parameter(ctrl->no_esd_cmds.cmds[i].parameter,
					ctrl->no_esd_cmds.cmds[i].dchdr.dlen-1);
		}

		if (ctrl->no_esd_cmds.cmds[i].dchdr.wait != 0)
			msleep(ctrl->no_esd_cmds.cmds[i].dchdr.wait);
	}
	pr_info("%s:-\n", __func__);
	return 0;
}


static bool mdss_spi_reg_status_check(struct spi_panel_data *ctrl_pdata)
{
	struct mdss_panel_data *pdata;
	bool esd_check_failed;
	int i, check_count;
	int ret = 0;
#define ESD_CHECK_MAX	(3)

	if (ctrl_pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return false;
	}

	pr_debug("%s: Checking Register status\n", __func__);

	pdata = &ctrl_pdata->panel_data;

	if (lcm2_enable && extpan_ctrl_pdata) {
		ctrl_pdata = extpan_ctrl_pdata;
		pdata = &ctrl_pdata->panel_data;
	}

	pr_warn("%s: Checking Register status\n", __func__);

#ifdef MMITEST
	return true;
#endif

	for (check_count = 0; check_count < ESD_CHECK_MAX; check_count++) {

		ret = mdss_spi_read_panel_data(&ctrl_pdata->panel_data,
				ctrl_pdata->panel_status_reg,
				ctrl_pdata->act_status_value,
				ctrl_pdata->status_cmds_rlen);

		if (ret >= 0) {

			for (i = 0, esd_check_failed = false; i < ctrl_pdata->status_cmds_rlen; i++) {
				pr_warn("%s: Checking Register status act_status_value[%d]=0x%x\n", __func__,i,ctrl_pdata->act_status_value[i]);
				pr_warn("%s: Checking Register status exp_status_value[%d]=0x%x\n", __func__,i,ctrl_pdata->exp_status_value[i]);
			}

			for (i = 0, esd_check_failed = false; i < ctrl_pdata->status_cmds_rlen; i++) 
			{
				if (ctrl_pdata->act_status_value[i] != ctrl_pdata->exp_status_value[i]) {
					pr_err("%s: found the esd happend.\n", __func__);
					//recovery display
					mdss_spi_esd_recovery(ctrl_pdata,pdata);
					pr_err("%s: already recover the esd\n", __func__);

					return true;

					esd_check_failed = true;
					break;
				}
			}

			if (!esd_check_failed) {
			    mdss_spi_no_esd_cmd(pdata);
				return true;
			}
		}
	}

//failure judgement: (spi read protocol fail || read data not match)
	//mdss_send_panel_cmd_for_esd(ctrl_pdata);
	return false;
}


static void mdss_spi_parse_esd_params(struct device_node *np,
		struct spi_panel_data	*ctrl)
{
	u32 tmp;
	int rc;
	struct property *data;
	const char *string;
	struct mdss_panel_info *pinfo = &ctrl->panel_data.panel_info;

	pinfo->esd_check_enabled = of_property_read_bool(np,
		"qcom,esd-check-enabled");
	pr_info("mdss_spi_panel.c esd=%d\n",pinfo->esd_check_enabled);

	if (!pinfo->esd_check_enabled)
		return;

	ctrl->status_mode = SPI_ESD_MAX;

	rc = of_property_read_string(np,
			"qcom,mdss-spi-panel-status-check-mode", &string);
	if (!rc) {
		if (!strcmp(string, "reg_read")) {
			ctrl->status_mode = SPI_ESD_REG;
			ctrl->check_status =
				mdss_spi_reg_status_check;
		} else if (!strcmp(string, "send_init_command")) {
			ctrl->status_mode = SPI_SEND_PANEL_COMMAND;
			ctrl->check_status =
				mdss_send_panel_cmd_for_esd;
				return;
		} else {
			pr_err("No valid panel-status-check-mode string\n");
			pinfo->esd_check_enabled = false;
			return;
		}
	}

	rc = of_property_read_u8(np, "qcom,mdss-spi-panel-status-reg",
				&ctrl->panel_status_reg);
	if (rc) {
		pr_warn("%s:%d, Read status reg failed, disable ESD check\n",
				__func__, __LINE__);
		pinfo->esd_check_enabled = false;
		return;
	}

	rc = of_property_read_u32(np, "qcom,mdss-spi-panel-status-read-length",
					&tmp);
	if (rc) {
		pr_warn("%s:%d, Read reg length failed, disable ESD check\n",
				__func__, __LINE__);
		pinfo->esd_check_enabled = false;
		return;
	}

	ctrl->status_cmds_rlen = (!rc ? tmp : 1);

	ctrl->exp_status_value = kzalloc(sizeof(u8) *
				 (ctrl->status_cmds_rlen + 1), GFP_KERNEL);
	ctrl->act_status_value = kzalloc(sizeof(u8) *
				(ctrl->status_cmds_rlen + 1), GFP_KERNEL);

	if (!ctrl->exp_status_value || !ctrl->act_status_value) {
		pr_err("%s: Error allocating memory for status buffer\n",
						__func__);
		pinfo->esd_check_enabled = false;
		return;
	}

	data = of_find_property(np, "qcom,mdss-spi-panel-status-value", &tmp);
	tmp /= sizeof(u8);
	if (!data || (tmp != ctrl->status_cmds_rlen)) {
		pr_err("%s: Panel status values not found\n", __func__);
		pinfo->esd_check_enabled = false;
		memset(ctrl->exp_status_value, 0, ctrl->status_cmds_rlen);
	} else {
		rc = of_property_read_u8_array(np,
			"qcom,mdss-spi-panel-status-value",
			ctrl->exp_status_value, tmp);
		if (rc) {
			pr_err("%s: Error reading panel status values\n",
				__func__);
			pinfo->esd_check_enabled = false;
			memset(ctrl->exp_status_value, 0,
				ctrl->status_cmds_rlen);
		}
	}
}

static int mdss_spi_panel_parse_dt(struct device_node *np,
		struct spi_panel_data	*ctrl_pdata)
{
	u32 tmp;
	int rc;
	const char *data;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);

	pinfo->cont_splash_enabled = of_property_read_bool(np,
					"qcom,cont-splash-enabled");

	rc = of_property_read_u32(np, "qcom,mdss-spi-panel-width", &tmp);
	if (rc) {
		pr_err("%s: panel width not specified\n", __func__);
		return -EINVAL;
	}
	pinfo->xres = (!rc ? tmp : 240);

	rc = of_property_read_u32(np, "qcom,mdss-spi-panel-height", &tmp);
	if (rc) {
		pr_err("%s:panel height not specified\n", __func__);
		return -EINVAL;
	}
	pinfo->yres = (!rc ? tmp : 320);

	rc = of_property_read_u32(np,
		"qcom,mdss-pan-physical-width-dimension", &tmp);
	pinfo->physical_width = (!rc ? tmp : 0);
	rc = of_property_read_u32(np,
		"qcom,mdss-pan-physical-height-dimension", &tmp);
	pinfo->physical_height = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-spi-panel-framerate", &tmp);
	pinfo->spi.frame_rate = (!rc ? tmp : 30);
	rc = of_property_read_u32(np, "qcom,mdss-spi-h-front-porch", &tmp);
	pinfo->lcdc.h_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-spi-h-back-porch", &tmp);
	pinfo->lcdc.h_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-spi-h-pulse-width", &tmp);
	pinfo->lcdc.h_pulse_width = (!rc ? tmp : 2);
	rc = of_property_read_u32(np, "qcom,mdss-spi-h-sync-skew", &tmp);
	pinfo->lcdc.hsync_skew = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-spi-v-back-porch", &tmp);
	pinfo->lcdc.v_back_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-spi-v-front-porch", &tmp);
	pinfo->lcdc.v_front_porch = (!rc ? tmp : 6);
	rc = of_property_read_u32(np, "qcom,mdss-spi-v-pulse-width", &tmp);
	pinfo->lcdc.v_pulse_width = (!rc ? tmp : 2);


	rc = of_property_read_u32(np, "qcom,mdss-spi-bpp", &tmp);
	if (rc) {
		pr_err("%s: bpp not specified\n", __func__);
		return -EINVAL;
	}
	pinfo->bpp = (!rc ? tmp : 16);

	pinfo->pdest = DISPLAY_1;

	ctrl_pdata->bklt_ctrl = SPI_UNKNOWN_CTRL;
	data = of_get_property(np, "qcom,mdss-spi-bl-pmic-control-type", NULL);
	if (data) {
		if (!strcmp(data, "bl_ctrl_wled")) {
			led_trigger_register_simple("bkl-trigger",
				&bl_led_trigger);
			pr_debug("%s: SUCCESS-> WLED TRIGGER register\n",
				__func__);
			ctrl_pdata->bklt_ctrl = SPI_BL_WLED;
		} else if (!strcmp(data, "bl_gpio_pulse")) {
			led_trigger_register_simple("gpio-bklt-trigger",
				&bl_led_trigger);
			pr_debug("%s: SUCCESS-> GPIO PULSE TRIGGER register\n",
				__func__);
			ctrl_pdata->bklt_ctrl = SPI_BL_WLED;
		} else if (!strcmp(data, "bl_ctrl_pwm")) {
			ctrl_pdata->bklt_ctrl = SPI_BL_PWM;
			ctrl_pdata->pwm_pmi = of_property_read_bool(np,
					"qcom,mdss-spi-bl-pwm-pmi");
			rc = of_property_read_u32(np,
				"qcom,mdss-spi-bl-pmic-pwm-frequency", &tmp);
			if (rc) {
				pr_err("%s: Error, panel pwm_period\n",
					 __func__);
				return -EINVAL;
			}
			ctrl_pdata->pwm_period = tmp;
			if (ctrl_pdata->pwm_pmi) {
				ctrl_pdata->pwm_bl = of_pwm_get(np, NULL);
				if (IS_ERR(ctrl_pdata->pwm_bl)) {
					pr_err("%s: Error, pwm device\n",
							__func__);
					ctrl_pdata->pwm_bl = NULL;
					return -EINVAL;
				}
			} else {
				rc = of_property_read_u32(np,
					"qcom,mdss-spi-bl-pmic-bank-select",
								 &tmp);
				if (rc) {
					pr_err("%s: Error, lpg channel\n",
						__func__);
					return -EINVAL;
				}
				ctrl_pdata->pwm_lpg_chan = tmp;
				tmp = of_get_named_gpio(np,
					"qcom,mdss-spi-pwm-gpio", 0);
				ctrl_pdata->pwm_pmic_gpio = tmp;
				pr_debug("%s: Configured PWM bklt ctrl\n",
								 __func__);
			}
		}
	}
	rc = of_property_read_u32(np, "qcom,mdss-brightness-max-level", &tmp);
	pinfo->brightness_max = (!rc ? tmp : MDSS_MAX_BL_BRIGHTNESS);
	rc = of_property_read_u32(np, "qcom,mdss-spi-bl-min-level", &tmp);
	pinfo->bl_min = (!rc ? tmp : 0);
	rc = of_property_read_u32(np, "qcom,mdss-spi-bl-max-level", &tmp);
	pinfo->bl_max = (!rc ? tmp : 255);
	ctrl_pdata->bklt_max = pinfo->bl_max;


	mdss_spi_panel_parse_reset_seq(np, pinfo->rst_seq,
					&(pinfo->rst_seq_len),
					"qcom,mdss-spi-reset-sequence");

	mdss_spi_panel_parse_cmds(np, &ctrl_pdata->on_cmds,
		"qcom,mdss-spi-on-command");

	mdss_spi_panel_parse_cmds(np, &ctrl_pdata->off_cmds,
		"qcom,mdss-spi-off-command");

	mdss_spi_parse_esd_params(np, ctrl_pdata);

	mdss_spi_panel_parse_cmds(np, &ctrl_pdata->no_esd_cmds,
		"qcom,mdss-spi-no-esd-command");

#ifdef MDSS_INIT_WINDOW_EACH_FRAME
	mdss_spi_panel_parse_cmds(np, &ctrl_pdata->init_window_cmds,
		"qcom,mdss-spi-init-window_command");
#endif

	return 0;
}

#ifndef CONFIG_FIH_LEO
static struct pwm_device *main_pwm_bl = NULL;
#endif
static void mdss_spi_panel_pwm_cfg(struct spi_panel_data *ctrl)
{
	if (ctrl->pwm_pmi)
		return;

	ctrl->pwm_bl = pwm_request(ctrl->pwm_lpg_chan, "lcd-bklt");
	if (ctrl->pwm_bl == NULL || IS_ERR(ctrl->pwm_bl)) {
		pr_err("%s: Error: lpg_chan=%d pwm request failed",
				__func__, ctrl->pwm_lpg_chan);
#ifndef CONFIG_FIH_LEO
		if (strstr(ctrl->panel_data.panel_info.panel_name, "ext panel") && (main_pwm_bl != NULL))
				ctrl->pwm_bl = main_pwm_bl;
	} else {
		if (strstr(ctrl->panel_data.panel_info.panel_name, "spi panel"))
			main_pwm_bl = ctrl->pwm_bl;
#endif
	}
	ctrl->pwm_enabled = 0;
}

static void mdss_spi_panel_bklt_pwm(struct spi_panel_data *ctrl, int level)
{
	int ret;
	u32 duty;
	u32 period_ns;

	if (ctrl->pwm_bl == NULL) {
		pr_err("%s: no PWM\n", __func__);
		return;
	}

	if (level == 0) {
		if (ctrl->pwm_enabled) {
			ret = pwm_config(ctrl->pwm_bl, 0,
				ctrl->pwm_period * NSEC_PER_USEC);
			if (ret)
				pr_err("%s: pwm_config() failed err=%d.\n",
						__func__, ret);
			pwm_disable(ctrl->pwm_bl);
		}
		ctrl->pwm_enabled = 0;
		return;
	}

	duty = level * ctrl->pwm_period;
	duty /= ctrl->bklt_max;

	pr_debug("%s: bklt_ctrl=%d pwm_period=%d pwm_gpio=%d pwm_lpg_chan=%d\n",
			__func__, ctrl->bklt_ctrl, ctrl->pwm_period,
				ctrl->pwm_pmic_gpio, ctrl->pwm_lpg_chan);

	if (ctrl->pwm_period >= USEC_PER_SEC) {
		ret = pwm_config_us(ctrl->pwm_bl, duty, ctrl->pwm_period);
		if (ret) {
			pr_err("%s: pwm_config_us() failed err=%d\n",
					__func__, ret);
			return;
		}
	} else {
		period_ns = ctrl->pwm_period * NSEC_PER_USEC;
		ret = pwm_config(ctrl->pwm_bl,
				82*level * period_ns / (ctrl->bklt_max*100),
				period_ns);
		if (ret) {
			pr_err("%s: pwm_config() failed err=%d\n",
					__func__, ret);
			return;
		}
	}

	if (!ctrl->pwm_enabled) {
		ret = pwm_enable(ctrl->pwm_bl);
		if (ret)
			pr_err("%s: pwm_enable() failed err=%d\n", __func__,
				ret);
		ctrl->pwm_enabled = 1;
	}
}

int bkl_flag = 0;//add by nanasu@t2mobile.com for workaround of bug1549
int count = 0;
static void mdss_spi_panel_bl_ctrl(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	if(strstr(pdata->panel_info.panel_name,"ext panel"))
	{
                printk("[bkl][mdss_spi_ext_panel_bl_ctrl]bl_level=true,bl_level=%d\n",bl_level);
		if (lcd_first_lighton) {
			msleep(50);
		}
		//return;
	}
	/* Allow panel backlight update if secure UI is enabled */
	if (bl_level && !mdp3_res->secure_update_bl) {
		mdp3_res->bklt_level = bl_level;
		pr_debug("[bkl][mdss_spi_panel_bl_ctrl] bl_level = true, bl_level = %d\n", bl_level);
		if (bkl_flag == 1) {
			pr_debug("[bkl] 11 flag = 1\n");
			mdp3_res->bklt_update = true;
			count++;
			if (count >= 2)
				bkl_flag = 0;
		} else {
			pr_debug("[bkl] 22 flag = 0\n");
			mdss_spi_panel_bl_ctrl_update(pdata, bl_level);
		}
	} else {
		pr_debug("[bkl][mdss_spi_panel_bl_ctrl] bl_level = false!!, bl_level = %d\n", bl_level);
		bkl_flag = 1;
		count = 0;
		pr_debug("[bkl] 33 flag = 1\n");
		mdss_spi_panel_bl_ctrl_update(pdata, bl_level);
	}
	lcd_first_lighton = 0;
}

#if defined(CONFIG_FB_MSM_MDSS_SPI_PANEL) && defined(CONFIG_SPI_QUP)
void mdss_spi_panel_bl_ctrl_update(struct mdss_panel_data *pdata,
							u32 bl_level)
{
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	if ((bl_level < pdata->panel_info.bl_min) && (bl_level != 0))
		bl_level = pdata->panel_info.bl_min;

	switch (ctrl_pdata->bklt_ctrl) {
	case SPI_BL_WLED:
		led_trigger_event(bl_led_trigger, bl_level);
		break;
	case SPI_BL_PWM:
		mdss_spi_panel_bklt_pwm(ctrl_pdata, bl_level);
		break;
	default:
		pr_err("%s: Unknown bl_ctrl configuration %d\n",
			__func__, ctrl_pdata->bklt_ctrl);
		break;
	}
}
#endif

static int mdss_spi_panel_init(struct device_node *node,
	struct spi_panel_data	*ctrl_pdata,
	bool cmd_cfg_cont_splash)
{
	int rc = 0;
	static const char *panel_name;
	struct mdss_panel_info *pinfo;

	if (!node || !ctrl_pdata) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	pinfo = &ctrl_pdata->panel_data.panel_info;

	pr_info("%s:%d\n", __func__, __LINE__);
	pinfo->panel_name[0] = '\0';
	panel_name = of_get_property(node, "qcom,mdss-spi-panel-name", NULL);
	if (!panel_name) {
		pr_info("%s:%d, Panel name not specified\n",
						__func__, __LINE__);
	} else {
		pr_info("%s: Panel Name = %s\n", __func__, panel_name);
		strlcpy(&pinfo->panel_name[0], panel_name, MDSS_MAX_PANEL_LEN);
		if (!strncmp(panel_name, "gc9305", strlen("gc9305"))) {
			lcd_vendor_source = 1;
		} else if (!strncmp(panel_name, "gc9307", strlen("gc9307"))) {
			lcd_vendor_source = 2;
		} else if (!strncmp(panel_name, "st7789vx2 qvga", strlen("st7789vx2 qvga"))) {
			lcd_vendor_source = 3;
		} else if (!strncmp(panel_name, "st7789vx2 ctc", strlen("st7789vx2 ctc"))) {
			lcd_vendor_source = 4;
		}
	}
	rc = mdss_spi_panel_parse_dt(node, ctrl_pdata);
	if (rc) {
		pr_err("%s:%d panel dt parse failed\n", __func__, __LINE__);
		return rc;
	}

	ctrl_pdata->byte_pre_frame = pinfo->xres * pinfo->yres * pinfo->bpp/8;

	ctrl_pdata->tx_buf = kmalloc(ctrl_pdata->byte_pre_frame, GFP_KERNEL);
	if (!ctrl_pdata->tx_buf) {
		pr_err("%s: FAILED: cannot alloc tx buf for spi panel\n",
				__func__);
		kfree(ctrl_pdata->tx_buf);
		return -ENOMEM;
	}

	if (!cmd_cfg_cont_splash)
		pinfo->cont_splash_enabled = false;

	pr_info("%s: Continuous splash %s\n", __func__,
		pinfo->cont_splash_enabled ? "enabled" : "disabled");

	pinfo->dynamic_switch_pending = false;
	pinfo->is_lpm_mode = false;
	pinfo->esd_rdy = false;

	ctrl_pdata->on = mdss_spi_panel_on;
	ctrl_pdata->off = mdss_spi_panel_off;
	ctrl_pdata->panel_data.set_backlight = mdss_spi_panel_bl_ctrl;

	return 0;
}

static int mdss_spi_get_panel_cfg(char *panel_cfg,
				struct spi_panel_data	*ctrl_pdata)
{
	int rc;
	struct mdss_panel_cfg *pan_cfg = NULL;

	if (!ctrl_pdata)
		return MDSS_PANEL_INTF_INVALID;

	pan_cfg = ctrl_pdata->mdss_util->panel_intf_type(MDSS_PANEL_INTF_SPI);
	if (IS_ERR(pan_cfg)) {
		return PTR_ERR(pan_cfg);
	} else if (!pan_cfg) {
		panel_cfg[0] = 0;
		return 0;
	}

	pr_debug("%s:%d: cfg:[%s]\n", __func__, __LINE__,
		 pan_cfg->arg_cfg);
	ctrl_pdata->panel_data.panel_info.is_prim_panel = true;
	rc = strlcpy(panel_cfg, pan_cfg->arg_cfg,
		     sizeof(pan_cfg->arg_cfg));
	return rc;
}

static int mdss_spi_panel_regulator_init(struct platform_device *pdev)
{
	int rc = 0;

	struct spi_panel_data *ctrl_pdata = NULL;

	if (!pdev) {
		pr_err("%s: invalid input\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = platform_get_drvdata(pdev);
	if (!ctrl_pdata) {
		pr_err("%s: invalid driver data\n", __func__);
		return -EINVAL;
	}

	rc = msm_mdss_config_vreg(&pdev->dev,
		ctrl_pdata->panel_power_data.vreg_config,
		ctrl_pdata->panel_power_data.num_vreg, 1);
	if (rc)
		pr_err("%s: failed to init vregs for %s\n",
			__func__, "PANEL_PM");

	return rc;

}

static irqreturn_t spi_panel_te_handler(int irq, void *data)
{
	struct spi_panel_data *ctrl_pdata = (struct spi_panel_data *)data;
	static int count = 2;

	if (!ctrl_pdata) {
		pr_err("%s: SPI display not available\n", __func__);
		return IRQ_HANDLED;
	}
	complete(&ctrl_pdata->spi_panel_te);

	if (ctrl_pdata->vsync_client.handler && !(--count)) {
		ctrl_pdata->vsync_client.handler(ctrl_pdata->vsync_client.arg);
		count = 2;
	}

	return IRQ_HANDLED;
}

void mdp3_spi_vsync_enable(struct mdss_panel_data *pdata,
				struct mdp3_notification *vsync_client)
{
	int updated = 0;
	struct spi_panel_data *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct spi_panel_data,
				panel_data);

	if (vsync_client) {
		if (ctrl_pdata->vsync_client.handler != vsync_client->handler) {
			ctrl_pdata->vsync_client = *vsync_client;
			updated = 1;
		}
	} else {
		if (ctrl_pdata->vsync_client.handler) {
			ctrl_pdata->vsync_client.handler = NULL;
			ctrl_pdata->vsync_client.arg = NULL;
			updated = 1;
		}
	}

	if (updated) {
		if (vsync_client && vsync_client->handler)
			enable_spi_panel_te_irq(ctrl_pdata, true);
		else
			enable_spi_panel_te_irq(ctrl_pdata, false);
	}
}

static struct device_node *mdss_spi_pref_prim_panel(
		struct platform_device *pdev)
{
	struct device_node *spi_pan_node = NULL;

	pr_info("%s:%d: Select primary panel from dt\n",
					__func__, __LINE__);
	spi_pan_node = of_parse_phandle(pdev->dev.of_node,
					"qcom,spi-pref-prim-pan", 0);
	if (!spi_pan_node)
		pr_err("%s:can't find panel phandle\n", __func__);

	return spi_pan_node;
}

static int spi_panel_device_register(struct device_node *pan_node,
				struct spi_panel_data *ctrl_pdata)
{
	int rc;
	struct mdss_panel_info *pinfo = &(ctrl_pdata->panel_data.panel_info);
	struct device_node *spi_ctrl_np = NULL;
	struct platform_device *ctrl_pdev = NULL;

	pinfo->type = SPI_PANEL;

	spi_ctrl_np = of_parse_phandle(pan_node,
				"qcom,mdss-spi-panel-controller", 0);
	if (!spi_ctrl_np) {
		pr_err("%s: SPI controller node not initialized\n", __func__);
		return -EPROBE_DEFER;
	}

	ctrl_pdev = of_find_device_by_node(spi_ctrl_np);
	if (!ctrl_pdev) {
		of_node_put(spi_ctrl_np);
		pr_err("%s: SPI controller node not find\n", __func__);
		return -EPROBE_DEFER;
	}

	rc = mdss_spi_panel_regulator_init(ctrl_pdev);
	if (rc) {
		pr_err("%s: failed to init regulator, rc=%d\n",
						__func__, rc);
		return rc;
	}

	pinfo->panel_max_fps = mdss_panel_get_framerate(pinfo,
		FPS_RESOLUTION_HZ);
	pinfo->panel_max_vtotal = mdss_panel_get_vtotal(pinfo);

	ctrl_pdata->disp_te_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
		"qcom,platform-te-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->disp_te_gpio))
		pr_err("%s:%d, TE gpio not specified\n",
						__func__, __LINE__);

	ctrl_pdata->disp_dc_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
		"qcom,platform-spi-dc-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->disp_dc_gpio))
		pr_err("%s:%d, SPI DC gpio not specified\n",
						__func__, __LINE__);

	ctrl_pdata->rst_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
			 "qcom,platform-reset-gpio", 0);
	if (!gpio_is_valid(ctrl_pdata->rst_gpio))
		pr_err("%s:%d, reset gpio not specified\n",
						__func__, __LINE__);


	mdss_spi_dbg("conqueror:spi_panel_device_register,name= %s,  rst is %d,ctrl=0x%x\n",
			ctrl_pdata->panel_data.panel_info.panel_name,
			ctrl_pdata->rst_gpio,(unsigned int)ctrl_pdata );

	ctrl_pdata->panel_data.event_handler = mdss_spi_panel_event_handler;

	if (ctrl_pdata->bklt_ctrl == SPI_BL_PWM)
		mdss_spi_panel_pwm_cfg(ctrl_pdata);

	ctrl_pdata->ctrl_state = CTRL_STATE_UNKNOWN;

	if (pinfo->cont_splash_enabled) {
		rc = mdss_spi_panel_power_ctrl(&(ctrl_pdata->panel_data),
			MDSS_PANEL_POWER_ON);
		if (rc) {
			pr_err("%s: Panel power on failed\n", __func__);
			return rc;
		}
		if (ctrl_pdata->bklt_ctrl == SPI_BL_PWM)
			ctrl_pdata->pwm_enabled = 1;
		pinfo->blank_state = MDSS_PANEL_BLANK_UNBLANK;
		ctrl_pdata->ctrl_state |=
			(CTRL_STATE_PANEL_INIT | CTRL_STATE_MDP_ACTIVE);
	} else {
		pinfo->panel_power_state = MDSS_PANEL_POWER_OFF;
	}

	rc = mdss_register_panel(ctrl_pdev, &(ctrl_pdata->panel_data));
	if (rc) {
		pr_err("%s: unable to register SPI panel\n", __func__);
		return rc;
	}

	pr_info("%s: Panel data initialized\n", __func__);
	return 0;
}


/**
 * mdss_spi_find_panel_of_node(): find device node of spi panel
 * @pdev: platform_device of the spi ctrl node
 * @panel_cfg: string containing intf specific config data
 *
 * Function finds the panel device node using the interface
 * specific configuration data. This configuration data is
 * could be derived from the result of bootloader's GCDB
 * panel detection mechanism. If such config data doesn't
 * exist then this panel returns the default panel configured
 * in the device tree.
 *
 * returns pointer to panel node on success, NULL on error.
 */
static struct device_node *mdss_spi_find_panel_of_node(
		struct platform_device *pdev, char *panel_cfg)
{
	int len, i;
	int ctrl_id = pdev->id - 1;
	char panel_name[MDSS_MAX_PANEL_LEN] = "";
	char ctrl_id_stream[3] =  "0:";
	char *stream = NULL, *pan = NULL;
	struct device_node *spi_pan_node = NULL, *mdss_node = NULL;

	len = strlen(panel_cfg);
	if (!len) {
		/* no panel cfg chg, parse dt */
		pr_err("%s:%d: no cmd line cfg present\n",
			 __func__, __LINE__);
		goto end;
	} else {
		if (ctrl_id == 1)
			strlcpy(ctrl_id_stream, "1:", 3);

		stream = strnstr(panel_cfg, ctrl_id_stream, len);
		if (!stream) {
			pr_err("controller config is not present\n");
			goto end;
		}
		stream += 2;

		pan = strnchr(stream, strlen(stream), ':');
		if (!pan) {
			strlcpy(panel_name, stream, MDSS_MAX_PANEL_LEN);
		} else {
			for (i = 0; (stream + i) < pan; i++)
				panel_name[i] = *(stream + i);
			panel_name[i] = 0;
		}

		pr_debug("%s:%d:%s:%s\n", __func__, __LINE__,
			 panel_cfg, panel_name);

		mdss_node = of_parse_phandle(pdev->dev.of_node,
					     "qcom,mdss-mdp", 0);
		if (!mdss_node) {
			pr_err("%s: %d: mdss_node null\n",
			       __func__, __LINE__);
			return NULL;
		}

		spi_pan_node = of_find_node_by_name(mdss_node,
						    panel_name);
		if (!spi_pan_node) {
			pr_err("%s: invalid pan node, selecting prim panel\n",
			       __func__);
			goto end;
		}
		return spi_pan_node;
	}
end:
	if (strcmp(panel_name, NONE_PANEL))
		spi_pan_node = mdss_spi_pref_prim_panel(pdev);
	of_node_put(mdss_node);
	return spi_pan_node;
}

static ssize_t panel_lcd_id_show(struct class *class,
		struct class_attribute *attr, char *buf)
{
	int primus_result = 0xFFFF;

	switch (lcd_vendor_source) {
	case 1: //first source
		primus_result = 0x0100;
		break;
	case 2: //second source
		primus_result = 0x0140;
		break;
	case 3: //third source
		primus_result = 0x0180;
		break;
	case 4: //forth source
		primus_result = 0x01C0;
		break;
	case 5: //fifth source, for now unuse
	default:
		primus_result = 0xFFFF;
		break;

	}
	return sprintf(buf, "LCD_%04x\n", primus_result);
}

static ssize_t panel_lcd_id_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t count)
{
	printk("not supported!\n ");
	return count;
}

static struct class_attribute panel_lcd_id =
	__ATTR(status, 0664, panel_lcd_id_show, panel_lcd_id_store);

static int mdss_spi_panel_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct spi_panel_data	*ctrl_pdata;
	struct mdss_panel_cfg *pan_cfg = NULL;
	struct device_node *spi_pan_node = NULL;
	bool cmd_cfg_cont_splash = true;
	char panel_cfg[MDSS_MAX_PANEL_LEN];
	struct mdss_util_intf *util;
	const char *ctrl_name;
	struct class *class_lcd_id;

	util = mdss_get_util_intf();
	if (util == NULL) {
		pr_err("Failed to get mdss utility functions\n");
		return -ENODEV;
	}

	if (!util->mdp_probe_done) {
		pr_err("%s: MDP not probed yet\n", __func__);
		return -EPROBE_DEFER;
	}

	if (!pdev->dev.of_node) {
		pr_err("SPI driver only supports device tree probe\n");
		return -ENOTSUPP;
	}

	pan_cfg = util->panel_intf_type(MDSS_PANEL_INTF_DSI);
	if (IS_ERR(pan_cfg)) {
		pr_err("%s: return MDSS_PANEL_INTF_DSI\n", __func__);
		return PTR_ERR(pan_cfg);
	} else if (pan_cfg) {
		pr_err("%s: DSI is primary\n", __func__);
		return -ENODEV;
	}

	ctrl_pdata = platform_get_drvdata(pdev);
	if (!ctrl_pdata) {
		ctrl_pdata = devm_kzalloc(&pdev->dev,
					  sizeof(struct spi_panel_data),
					  GFP_KERNEL);
		if (!ctrl_pdata) {
			pr_err("%s: FAILED: cannot alloc spi panel\n",
			       __func__);
			rc = -ENOMEM;
			goto error_no_mem;
		}
		platform_set_drvdata(pdev, ctrl_pdata);


		mdss_spi_dbg("conqueror:%s, request a ctrl_pdata[0x%x] sucessfully\n",__func__,(unsigned int)ctrl_pdata);
	}

	ctrl_pdata->mdss_util = util;

	ctrl_name = of_get_property(pdev->dev.of_node, "label", NULL);
	if (!ctrl_name)
		pr_info("%s:%d, Ctrl name not specified\n",
			__func__, __LINE__);
	else
		pr_info("%s: Ctrl name = %s\n",
			__func__, ctrl_name);


	rc = of_platform_populate(pdev->dev.of_node,
				  NULL, NULL, &pdev->dev);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: failed to add child nodes, rc=%d\n",
			__func__, rc);
		goto error_no_mem;
	}

	rc = mdss_spi_panel_pinctrl_init(pdev);
	if (rc)
		pr_warn("%s: failed to get pin resources\n", __func__);

	rc = mdss_spi_get_panel_vreg_data(&pdev->dev,
					&ctrl_pdata->panel_power_data);
	if (rc) {
		dev_err(&pdev->dev,
			"%s: failed to get panel vreg data, rc=%d\n",
			__func__, rc);
		goto error_vreg;
	}

	/* SPI panels can be different between controllers */
	rc = mdss_spi_get_panel_cfg(panel_cfg, ctrl_pdata);
	if (!rc)
		/* spi panel cfg not present */
		pr_warn("%s:%d:spi specific cfg not present\n",
			__func__, __LINE__);

	if (strstr(ctrl_name, "ext panel"))
		ctrl_pdata->panel_data.panel_info.is_prim_panel = false;

	/* find panel device node */
	spi_pan_node = mdss_spi_find_panel_of_node(pdev, panel_cfg);
#ifndef CONFIG_FIH_LEO
	spi_pan_node = mdss_spi_pref_prim_panel(pdev);
#endif
	if (!spi_pan_node) {
		pr_err("%s: can't find panel node %s\n", __func__, panel_cfg);
		goto error_pan_node;
	}

	cmd_cfg_cont_splash = true;

	rc = mdss_spi_panel_init(spi_pan_node, ctrl_pdata, cmd_cfg_cont_splash);
	if (rc) {
		pr_err("%s: spi panel init failed\n", __func__);
		goto error_pan_node;
	}

	rc = spi_panel_device_register(spi_pan_node, ctrl_pdata);
	if (rc) {
		pr_err("%s: spi panel dev reg failed\n", __func__);
		goto error_pan_node;
	}

	ctrl_pdata->panel_data.event_handler = mdss_spi_panel_event_handler;


	init_completion(&ctrl_pdata->spi_panel_te);
	mutex_init(&ctrl_pdata->spi_tx_mutex);
	mutex_init(&ctrl_pdata->te_mutex);

#ifndef CONFIG_FIH_LEO
	if (strstr(ctrl_name, "ext panel")) {
		extpan_ctrl_pdata = ctrl_pdata;
	}
#endif
	if (strstr(ctrl_name, "spi panel")) {
		rc = devm_request_irq(&pdev->dev,
			gpio_to_irq(ctrl_pdata->disp_te_gpio),
			spi_panel_te_handler, IRQF_TRIGGER_FALLING,
			"TE_GPIO", ctrl_pdata);
		if (rc) {
			pr_err("TE request_irq failed.\n");
			return rc;
		}
	}

	if (strstr(ctrl_name, "ext panel")) {
		class_lcd_id = class_create(THIS_MODULE, "panel_ext_lcd_id");
	} else {
		class_lcd_id = class_create(THIS_MODULE, "panel_lcd_id");
	}
	if (IS_ERR(class_lcd_id)) {
		rc = PTR_ERR(class_lcd_id);
		pr_err("%s: couldn't create class panel_lcd_id\n", __func__);
	}
	rc = class_create_file(class_lcd_id, &panel_lcd_id);
	if (rc) {
		pr_err("%s: couldn't create file panel_lcd_id status\n", __func__);
	}

	pr_debug("%s: spi panel  initialized\n", __func__);
	return 0;

error_pan_node:
	of_node_put(spi_pan_node);
error_vreg:
	mdss_spi_put_dt_vreg_data(&pdev->dev,
			&ctrl_pdata->panel_power_data);
error_no_mem:
	devm_kfree(&pdev->dev, ctrl_pdata);
	return rc;
}


static const struct of_device_id mdss_spi_panel_match[] = {
	{ .compatible = "qcom,mdss-spi-display" },
	{},
};

static struct platform_driver this_driver = {
	.probe = mdss_spi_panel_probe,
	.driver = {
		.name = "spi_panel",
		.owner  = THIS_MODULE,
		.of_match_table = mdss_spi_panel_match,
	},
};

static int __init mdss_spi_display_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);
	return 0;
}
module_init(mdss_spi_display_init);

MODULE_DEVICE_TABLE(of, mdss_spi_panel_match);
MODULE_LICENSE("GPL v2");
