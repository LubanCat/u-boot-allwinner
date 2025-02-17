/* disp_edp.c
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * register edp
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "disp_edp.h"

#if defined(SUPPORT_EDP)

/* 0-1:vcc_power 2:hpd_power*/
#define EDP_POWER_NUM 3

/*gpio: 0:reset_gpio 1-3:normal_gpio 4:sda_gpio 5:sck_gpio*/
#define EDP_GPIO_NUM 6

struct disp_edp_private_data {
	u32 enabled;
	u32 edp_index; /* edp module index */
	bool suspended;
	enum disp_tv_mode mode;
	disp_lcd_flow open_flow;
	disp_lcd_flow close_flow;
	disp_lcd_panel_fun edp_panel_fun;
	struct disp_video_timings *video_info;
	struct disp_tv_func edp_func;
	struct clk *clk;
	u32 irq_no;
	u32 frame_per_sec;
	u32 usec_per_line;
	u32 judge_line;
	u32 edp_pwm_used;
	u32 edp_pwm_ch;
	u32 edp_pwm_freq;
	u32 edp_pwm_pol;
	bool bl_enabled;
	u32 bl_need_enabled;
	u32 backlight_dimming;
	struct {
		uintptr_t dev;
		u32 channel;
		u32 polarity;
		u32 period_ns;
		u32 duty_ns;
		u32 enabled;
	} pwm_info;
	u32 backlight_bright;
	bool edp_bl_en_used;
	disp_gpio_set_t edp_bl_en;
	int edp_bl_en_hdl;
	bool edp_gpio_used[EDP_GPIO_NUM];
	disp_gpio_set_t edp_gpio[EDP_GPIO_NUM];
	u32 gpio_hdl[EDP_GPIO_NUM];
	u32 edp_power_used[EDP_POWER_NUM];
	char panel_driver_name[32];
	char edp_panel_power[EDP_POWER_NUM][32];
	char edp_bl_power[32];
	char *edp_panel_regulator[EDP_POWER_NUM];
	char *edp_bl_regulator;
};

/* global static variable */
static u32 g_edp_used;
static u32 num_devices_support_edp;
static struct disp_device *g_pedp_devices;
static struct disp_edp_private_data *g_pedp_private;
__attribute__((unused)) static spinlock_t g_edp_data_lock;
//static spinlock_t g_edp_data_lock;
static s32 disp_edp_set_bright(struct disp_device *edp, u32 bright);
static s32 disp_edp_get_bright(struct disp_device *edp);
static s32 disp_edp_backlight_enable(struct disp_device *edp);
static s32 disp_edp_backlight_disable(struct disp_device *edp);
static s32 disp_edp_pwm_enable(struct disp_device *edp);
static s32 disp_edp_pwm_disable(struct disp_device *edp);

struct disp_device *disp_get_edp(u32 disp)
{
	u32 num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
	if (disp >= num_screens || !num_devices_support_edp) {
		DE_INF("disp %d not support edp output\n", disp);
		return NULL;
	}

	return &g_pedp_devices[disp];
}

/**
 * @name       disp_edp_get_priv
 * @brief      get disp_edp_private_data of disp_device
 * @param[IN]  p_edp: disp_device var
 * @param[OUT] none
 * @return     0 if success,otherwise fail
 */
static struct disp_edp_private_data *
disp_edp_get_priv(struct disp_device *p_edp)
{
	if (p_edp == NULL) {
		DE_WRN("NULL hdl!\n");
		return NULL;
	}

	return (struct disp_edp_private_data *)p_edp->priv_data;
}

s32 disp_edp_get_fps(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("p_edp set func null  hdl!\n");
		return 0;
	}

	return p_edpp->frame_per_sec;
}

s32 disp_edp_resume(struct disp_device *p_edp)
{
	s32 ret = 0;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("disp set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_edpp->suspended == true) {
		if (p_edpp->edp_func.tv_resume)
			ret = p_edpp->edp_func.tv_resume(p_edpp->edp_index);
		p_edpp->suspended = false;
	}
	return ret;
}

s32 disp_edp_suspend(struct disp_device *p_edp)
{
	s32 ret = 0;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("disp set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_edpp->suspended == false) {
		p_edpp->suspended = true;
		if (p_edpp->edp_func.tv_suspend)
			ret = p_edpp->edp_func.tv_suspend(p_edpp->edp_index);
	}

	return ret;
}

s32 disp_edp_get_input_csc(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("null hdl!\n");
		return 0;
	}

	if (p_edpp->edp_func.tv_get_input_csc == NULL)
		return 0;

	return p_edpp->edp_func.tv_get_input_csc(p_edpp->edp_index);
}



static s32 edp_calc_judge_line(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	int start_delay, usec_start_delay;
	int usec_judge_point;
	int pixel_clk;

	if (!p_edpp || !p_edp) {
		DE_WRN("edp init null hdl!\n");
		return DIS_FAIL;
	}

	pixel_clk = p_edpp->video_info->pixel_clk;

	if (p_edpp->usec_per_line == 0) {

		/*
		 * usec_per_line = 1 / fps / vt * 1000000
		 *               = 1 / (pixel_clk / vt / ht) / vt * 1000000
		 *               = ht / pixel_clk * 1000000
		 */
		p_edpp->frame_per_sec = pixel_clk /
					p_edpp->video_info->hor_total_time /
					p_edpp->video_info->ver_total_time;
		p_edpp->usec_per_line =
		    p_edpp->video_info->hor_total_time * 1000000 / pixel_clk;
	}

	if (p_edpp->judge_line == 0) {
#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
		start_delay = disp_al_device_get_start_delay(p_edp->hwdev_index);
#else
		start_delay =
		    p_edpp->edp_func.tv_get_startdelay(p_edpp->edp_index);
#endif
		usec_start_delay = start_delay * p_edpp->usec_per_line;
		if (usec_start_delay <= 200)
			usec_judge_point = usec_start_delay * 3 / 7;
		else if (usec_start_delay <= 400)
			usec_judge_point = usec_start_delay / 2;
		else
			usec_judge_point = 200;
		p_edpp->judge_line = usec_judge_point / p_edpp->usec_per_line;
	}

	return 0;
}

static s32 disp_edp_power_enable(struct disp_device *edp, u32 power_id)
{
	int ret;
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (edpp->edp_panel_regulator[power_id]) {
		ret = disp_sys_power_enable(edpp->edp_panel_regulator[power_id]);
		if (ret)
			return DIS_FAIL;
	}

	return DIS_SUCCESS;
}

static s32 disp_edp_power_disable(struct disp_device *edp, u32 power_id)
{
	int ret;
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (edpp->edp_panel_regulator[power_id]) {
		ret = disp_sys_power_disable(edpp->edp_panel_regulator[power_id]);
		if (ret)
			return DIS_FAIL;
	}

	return DIS_SUCCESS;
}

static s32 disp_edp_gpio_set_value(struct disp_device *edp, u32 io_index, u32 data)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	char gpio_name[20];

	if ((edp == NULL) || (edpp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (io_index >= EDP_GPIO_NUM) {
		DE_WRN("gpio num out of range\n");
		return DIS_FAIL;
	}

	if (edpp->gpio_hdl[io_index]) {
		sprintf(gpio_name, "edp_gpio_%d", io_index);
		return disp_sys_gpio_set_value(edpp->gpio_hdl[io_index], data, gpio_name);
	} else {
		DE_WRN("gpio:%d(%s) handle is NULL", edpp->gpio_hdl[io_index], gpio_name);
		return DIS_FAIL;
	}
}

/* direction: input(0), output(1) */
static s32 disp_edp_gpio_set_direction(struct disp_device *edp, u32 io_index,
				u32 direction)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	char gpio_name[20];

	if ((edp == NULL) || (edpp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (edpp->gpio_hdl[io_index]) {
		sprintf(gpio_name, "edp_gpio_%d", io_index);
		return disp_sys_gpio_set_direction(edpp->gpio_hdl[io_index], direction, gpio_name);
	} else {
		DE_WRN("gpio:%d(%s) handle is NULL", edpp->gpio_hdl[io_index], gpio_name);
		return DIS_FAIL;
	}

}

extern void sync_event_proc(u32 disp, bool timeout);
#if defined(__LINUX_PLAT__)
static s32 disp_edp_event_proc(int irq, void *parg)
#else
static s32 disp_edp_event_proc(void *parg)
#endif
{
	struct disp_device *p_edp = (struct disp_device *)parg;
	struct disp_manager *mgr = NULL;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	int cur_line;
	int start_delay;

	if (p_edp == NULL || p_edpp == NULL)
		return DISP_IRQ_RETURN;

#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
	if (disp_al_device_query_irq(p_edp->hwdev_index)) {
		cur_line = disp_al_device_get_cur_line(p_edp->hwdev_index);
		start_delay = disp_al_device_get_start_delay(p_edp->hwdev_index);
#else
	if (p_edpp->edp_func.tv_irq_query(p_edpp->edp_index)) {
		cur_line = p_edpp->edp_func.tv_get_cur_line(p_edpp->edp_index);
		start_delay =
		    p_edpp->edp_func.tv_get_startdelay(p_edpp->edp_index);
#endif
		mgr = p_edp->manager;
		if (mgr == NULL) {
			return DISP_IRQ_RETURN;
		}

		if (cur_line <= (start_delay - 4)) {
			sync_event_proc(mgr->disp, false);
		} else {
			sync_event_proc(mgr->disp, true);
		}
	}

	return DISP_IRQ_RETURN;
}


s32 disp_edp_is_enabled(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("edp set func null  hdl!\n");
		return DIS_FAIL;
	}

	return p_edpp->enabled;
}



/**
 * @name       :disp_edp_set_func
 * @brief      :set edp lowlevel function
 * @param[IN]  :p_edp:disp_device
 * @param[OUT] :func:lowlevel function
 * @return     :0 if success else fail
 */
s32 disp_edp_set_func(struct disp_device *p_edp,
		       struct disp_tv_func *func)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL) || (func == NULL)) {
		DE_WRN("edp set func null  hdl!\n");
		DE_WRN("%s,point  p_edp = %p, point  p_edpp = %p\n", __func__,
		       p_edp, p_edpp);
		return DIS_FAIL;
	}
	p_edpp->edp_func.tv_enable = func->tv_enable;
	p_edpp->edp_func.tv_disable = func->tv_disable;
	p_edpp->edp_func.tv_suspend = func->tv_suspend;
	p_edpp->edp_func.tv_resume = func->tv_resume;
	p_edpp->edp_func.tv_set_mode = func->tv_set_mode;
	p_edpp->edp_func.tv_get_mode = func->tv_get_mode;
	p_edpp->edp_func.tv_mode_support = func->tv_mode_support;
	p_edpp->edp_func.tv_get_input_csc = func->tv_get_input_csc;
	p_edpp->edp_func.tv_irq_enable = func->tv_irq_enable;
	p_edpp->edp_func.tv_irq_query = func->tv_irq_query;
	p_edpp->edp_func.tv_get_startdelay = func->tv_get_startdelay;
	p_edpp->edp_func.tv_get_cur_line = func->tv_get_cur_line;
	p_edpp->edp_func.tv_get_video_timing_info =
	    func->tv_get_video_timing_info;
	p_edpp->edp_func.tv_show_builtin_patten = func->tv_show_builtin_patten;

	return 0;
}

s32 edp_regulator_get_wrap(struct disp_edp_private_data *edpp)
{
	int i;
	int node_offset = 0;
	const char *regulator;
	char primary_key[20];

	sprintf(primary_key, "edp%d", edpp->edp_index);
	node_offset = disp_fdt_nodeoffset(primary_key);
	if (edpp->edp_bl_regulator == NULL) {
		if (strlen(edpp->edp_bl_power)) {
			regulator = disp_sys_power_get(node_offset, edpp->edp_bl_power);
			if (!regulator)
				DE_WRN("disp_sys_power_get for %s failed\n", edpp->edp_bl_power);
			else
				edpp->edp_bl_regulator = (char *)regulator;
		}
	}

	for (i = 0; i < EDP_POWER_NUM; i++) {
		if (edpp->edp_panel_regulator[i] == NULL) {
			if (edpp->edp_power_used[i]) {
				regulator = disp_sys_power_get(node_offset, edpp->edp_panel_power[i]);
				if (!regulator) {
					DE_WRN("disp_sys_power_get for %s failed\n", edpp->edp_panel_power[i]);
					continue;
				}
				edpp->edp_panel_regulator[i] = (char *)regulator;
				printf("edpp->edp_bl_regulator:%s\n", edpp->edp_bl_regulator);
				printf("edpp->edp_panel_regulator:%s\n", edpp->edp_panel_regulator[i]);
			}
		}
	}

	return 0;
}

s32 edp_regulator_put_wrap(struct disp_edp_private_data *edpp)
{
	return 0;
}

s32 edp_parse_panel_para(struct disp_edp_private_data *p_edpp,
			 struct disp_video_timings *p_info)
{
	s32 ret = -1;
	s32 fun_ret = 0;
	s32  value = 1;
	char primary_key[20], sub_name[25];
	u32 i = 0;
	disp_gpio_set_t  *gpio_info;

	if (!p_info || !p_edpp)
		goto OUT;

	memset(p_info, 0, sizeof(struct disp_video_timings));

	sprintf(primary_key, "edp%d", p_edpp->edp_index);

	ret = disp_sys_script_get_item(primary_key, "edp_panel_used", &value, 1);
	if (ret != 1 || value == 0) {
		DE_WRN("edp panel unused!\n");
		return fun_ret;
	} else {
		ret = disp_sys_script_get_item(primary_key, "edp_pwm_used", &value, 1);
		if (ret == 1)
			p_edpp->edp_pwm_used = value;

		ret = disp_sys_script_get_item(primary_key, "edp_pwm_ch", &value, 1);
		if (ret == 1)
			p_edpp->edp_pwm_ch = value;

		ret = disp_sys_script_get_item(primary_key, "edp_pwm_freq", &value, 1);
		if (ret == 1)
			p_edpp->edp_pwm_freq = value;

		ret = disp_sys_script_get_item(primary_key, "edp_pwm_pol", &value, 1);
		if (ret == 1)
			p_edpp->edp_pwm_pol = value;

		p_edpp->edp_bl_en_used = 0;

		gpio_info = &(p_edpp->edp_bl_en);
		ret = disp_sys_script_get_item(primary_key, "edp_bl_en",
					       (int *)gpio_info, 3);
		if (ret == 3)
			p_edpp->edp_bl_en_used = 1;

		/* edp_gpio */
		gpio_info = &(p_edpp->edp_gpio[0]);
		ret = disp_sys_script_get_item(primary_key, "edp_gpio_panel_reset", (int *)gpio_info, 3);
		if (!ret)
			p_edpp->edp_gpio_used[0] = 1;

		for (i = 1; i < 4; i++) {
			sprintf(sub_name, "edp_gpio_%d", i);

			gpio_info = &(p_edpp->edp_gpio[i]);
			ret = disp_sys_script_get_item(primary_key, sub_name, (int *)gpio_info, 3);
			if (!ret)
				p_edpp->edp_gpio_used[i] = 1;
		}

		gpio_info = &(p_edpp->edp_gpio[4]);
		ret = disp_sys_script_get_item(primary_key, "edp_gpio_panel_sda", (int *)gpio_info, 3);
		if (!ret)
			p_edpp->edp_gpio_used[4] = 1;

		gpio_info = &(p_edpp->edp_gpio[5]);
		ret = disp_sys_script_get_item(primary_key, "edp_gpio_panel_sck", (int *)gpio_info, 3);
		if (!ret)
			p_edpp->edp_gpio_used[5] = 1;

		for (i = 1; i < EDP_GPIO_NUM; i++) {
			p_edpp->gpio_hdl[i] = 0;
			gpio_info = &(p_edpp->edp_gpio[i]);
			p_edpp->gpio_hdl[i] = disp_sys_gpio_request_simple(gpio_info, 1);
		}

		sprintf(sub_name, "edp_bl_power");
		ret = disp_sys_script_get_item(primary_key, sub_name,
					       (int *)p_edpp->edp_bl_power, 2);

		sprintf(sub_name, "edp_panel_driver");
		ret = disp_sys_script_get_item(primary_key, sub_name,
					       (int *)p_edpp->panel_driver_name, 2);

		sprintf(sub_name, "edp_default_backlight");
		ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (ret == 1) {
			value = (value > 256) ? 256 : value;
			p_edpp->backlight_bright = value;
		} else {
			p_edpp->backlight_bright = 197;
		}

		for (i = 0; i < EDP_POWER_NUM; i++) {
			sprintf(sub_name, "edp_panel_power_%d", i);
			p_edpp->edp_power_used[i] = 0;
			ret = disp_sys_script_get_item(
			    primary_key, sub_name, (int *)(p_edpp->edp_panel_power[i]), 2);
			if (ret == 2)
				p_edpp->edp_power_used[i] = 1;
		}
	}
OUT:
	return fun_ret;
}

static s32 edp_clk_enable(struct disp_device *pedp)
{
	struct disp_edp_private_data *pedpp = disp_edp_get_priv(pedp);

	if (!pedp || !pedpp) {
		DE_WRN("disp_edp device null hdl!\n");
		return DIS_FAIL;
	}

	if (!pedpp->clk) {
		DE_WRN("edp clk is NULL\n");
		return DIS_FAIL;
	}
	return clk_prepare_enable(pedpp->clk);
}

static s32 edp_clk_disable(struct disp_device *pedp)
{
	struct disp_edp_private_data *pedpp = disp_edp_get_priv(pedp);

	if (!pedp || !pedpp) {
		DE_WRN("disp_edp device null hdl!\n");
		return DIS_FAIL;
	}

	if (!pedpp->clk) {
		DE_WRN("edp clk is NULL\n");
		return DIS_FAIL;
	}
	clk_disable(pedpp->clk);

	return 0;
}


/**
 * @name       :disp_edp_init
 * @brief      :get sys_config.
 * @param[IN]  :p_edp:disp_device
 * @param[OUT] :none
 * @return     :0 if success else fail
 */
static s32 disp_edp_init(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	__u64 backlight_bright;
	__u64 period_ns, duty_ns;
	s32 ret = -1;

	if (!p_edp || !p_edpp) {
		DE_WRN("edp init null hdl!\n");
		return DIS_FAIL;
	}
	ret = edp_parse_panel_para(p_edpp, &p_edp->timings);

	ret = edp_regulator_get_wrap(p_edpp);

	if (p_edpp->edp_pwm_used) {
		p_edpp->pwm_info.channel = p_edpp->edp_pwm_ch;
		p_edpp->pwm_info.polarity = p_edpp->edp_pwm_pol;
		p_edpp->pwm_info.dev = disp_sys_pwm_request(p_edpp->edp_pwm_ch);
		if (p_edpp->edp_pwm_freq != 0) {
			period_ns = 1000 * 1000 * 1000 / p_edpp->edp_pwm_freq;
		} else {
			DE_WRN("edp%d.edp_pwm_freq is ZERO\n",
			       p_edpp->edp_index);
			/* default 1khz */
			period_ns = 1000 * 1000 * 1000 / 1000;
		}

		backlight_bright = p_edpp->backlight_bright;
		duty_ns = (backlight_bright * period_ns) / 256;

		p_edpp->pwm_info.duty_ns = duty_ns;
		p_edpp->pwm_info.period_ns = period_ns;

		disp_sys_pwm_config(p_edpp->pwm_info.dev, p_edpp->pwm_info.duty_ns,
				    p_edpp->pwm_info.period_ns);
		disp_sys_pwm_set_polarity(p_edpp->pwm_info.dev,
					  p_edpp->pwm_info.polarity);
	}

	return ret;
}

s32 disp_edp_disable(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	unsigned long flags;
	struct disp_manager *mgr = NULL;
	s32 ret = -1;
	s32 i;

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("edp set func null  hdl!\n");
		return DIS_FAIL;
	}

	mgr = p_edp->manager;
	if (!mgr) {
		DE_WRN("edp%d's mgr is NULL\n", p_edp->disp);
		return DIS_FAIL;
	}

	if (p_edpp->enabled == 0) {
		DE_WRN("edp%d is already closed\n", p_edp->disp);
		return DIS_FAIL;
	}

	if (p_edpp->edp_func.tv_disable == NULL) {
		DE_WRN("tv_func.tv_disable is NULL\n");
		return -1;
	}

	spin_lock_irqsave(&g_edp_data_lock, flags);
	p_edpp->enabled = 0;
	spin_unlock_irqrestore(&g_edp_data_lock, flags);

	p_edpp->close_flow.func_num = 0;
	if (p_edpp->edp_panel_fun.cfg_close_flow) {
		p_edpp->edp_panel_fun.cfg_close_flow(p_edp->disp);

		for (i = 0; i < p_edpp->close_flow.func_num - 1; i++) {
			if (p_edpp->close_flow.func[i].func) {
				p_edpp->close_flow.func[i].func(p_edp->disp);
				DE_INF("close flow:step %d finish, to delay %d\n", i,
				       p_edpp->close_flow.func[i].delay);
				if (p_edpp->close_flow.func[i].delay != 0)
					disp_delay_ms(p_edpp->close_flow.func[i].delay);
			}
		}
	} else
		DE_WRN("edp_panel_fun[%d].cfg_close_flow is NULL\n", p_edp->disp);


	if (p_edpp->edp_func.tv_disable != NULL) {
		p_edpp->edp_func.tv_disable(p_edpp->edp_index);
	} else {
		DE_WRN("edp enable func is NULL\n");
	}

	if (mgr->disable)
		mgr->disable(mgr);

	p_edpp->edp_func.tv_irq_enable(p_edpp->edp_index, 0, 0);

	ret = edp_clk_disable(p_edp);
	if (ret != 0) {
		DE_WRN("fail to enable edp's clock\n");
	}

	disp_al_edp_disable(p_edp->hwdev_index);

	disp_sys_disable_irq(p_edpp->irq_no);
	disp_sys_unregister_irq(p_edpp->irq_no, disp_edp_event_proc,
				(void *)p_edp);
	disp_edp_pwm_disable(p_edp);

	disp_delay_ms(1000);
	return 0;
}

static s32 disp_edp_exit(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if (!p_edp || !p_edpp) {
		DE_WRN("edp init null hdl!\n");
		return DIS_FAIL;
	}

	edp_regulator_put_wrap(p_edpp);

	disp_edp_disable(p_edp);
	kfree(p_edp);
	kfree(p_edpp);
	return 0;
}

s32 disp_edp_get_mode(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	enum disp_tv_mode tv_mode;

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("edp set mode null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_edpp->edp_func.tv_get_mode == NULL) {
		DE_WRN("tv_get_mode is null!\n");
		return DIS_FAIL;
	}

	tv_mode = p_edpp->edp_func.tv_get_mode(p_edpp->edp_index);
	if (tv_mode != p_edpp->mode)
		p_edpp->mode = tv_mode;

	return p_edpp->mode;
}

s32 disp_edp_set_mode(struct disp_device *p_edp, enum disp_tv_mode tv_mode)
{
	s32 ret = 0;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("edp set mode null  hdl!\n");
		return DIS_FAIL;
	}

	if (p_edpp->edp_func.tv_set_mode == NULL) {
		DE_WRN("tv_set_mode is null!\n");
		return DIS_FAIL;
	}

	ret = p_edpp->edp_func.tv_set_mode(p_edpp->edp_index, tv_mode);
	if (ret == 0)
		p_edpp->mode = tv_mode;

	return ret;
}

/*
 * @name       :disp_edp_enable
 * @brief      :enable edp,tcon,register irq,set manager
 * @param[IN]  :p_edp disp_device
 * @return     :0 if success
 */
s32 disp_edp_enable(struct disp_device *p_edp)
{
	unsigned long flags;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	struct disp_manager *mgr = NULL;
	s32 ret = -1;
	u32 bl, i;

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("edp set func null  hdl!\n");
		return DIS_FAIL;
	}
	DE_INF("%s, disp%d\n", __func__, p_edp->disp);

	if (p_edpp->enabled) {
		DE_WRN("edp%d is already enable\n", p_edp->disp);
		return DIS_FAIL;
	}

	if (p_edpp->edp_func.tv_get_video_timing_info == NULL) {
		DE_WRN("tv_get_video_timing_info func is null\n");
		return DIS_FAIL;
	}

	p_edpp->edp_func.tv_get_video_timing_info(p_edpp->edp_index,
						  &(p_edpp->video_info));

	if (p_edpp->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}

	memcpy(&p_edp->timings, p_edpp->video_info,
	       sizeof(struct disp_video_timings));

	mgr = p_edp->manager;
	if (!mgr) {
		DE_WRN("edp%d's mgr is NULL\n", p_edp->disp);
		return DIS_FAIL;
	}

	edp_calc_judge_line(p_edp);

	if (mgr->enable)
		mgr->enable(mgr);

	printf("edp clk_set_rate1!\n");
	clk_set_rate(p_edpp->clk, p_edp->timings.pixel_clk);

	ret = edp_clk_enable(p_edp);
	if (ret != 0) {
		DE_WRN("fail to enable edp's clock\n");
		goto EXIT;
	}

	p_edpp->open_flow.func_num = 0;
	if (p_edpp->edp_panel_fun.cfg_open_flow) {
		p_edpp->edp_panel_fun.cfg_open_flow(p_edp->disp);

		for (i = 0; i < p_edpp->open_flow.func_num - 1; i++) {
			if (p_edpp->open_flow.func[i].func) {
				p_edpp->open_flow.func[i].func(p_edp->disp);
				DE_INF("open flow:step %d finish, to delay %d\n", i,
				       p_edpp->open_flow.func[i].delay);
				if (p_edpp->open_flow.func[i].delay != 0)
					disp_delay_ms(p_edpp->open_flow.func[i].delay);
			}
		}
	} else
		DE_WRN("edp_panel_fun[%d].cfg_open_flow is NULL\n", p_edp->disp);

	if (p_edpp->edp_func.tv_enable != NULL) {
		ret = p_edpp->edp_func.tv_enable(p_edpp->edp_index);
		if (ret) {
			DE_WRN("edp enable func fail\n");
			goto EXIT;
		}
	} else {
		DE_WRN("edp enable func is NULL\n");
		goto EXIT;
	}

	disp_sys_register_irq(p_edpp->irq_no, 0, disp_edp_event_proc,
			      (void *)p_edp, 0, 0);
	disp_sys_enable_irq(p_edpp->irq_no);

	p_edpp->edp_func.tv_irq_enable(p_edpp->edp_index, 0, 1);

	disp_edp_pwm_enable(p_edp);

	disp_edp_backlight_enable(p_edp);

	disp_al_edp_cfg(p_edp->hwdev_index, p_edpp->frame_per_sec,
			p_edpp->edp_index, &p_edp->timings);

	disp_al_edp_enable(p_edp->hwdev_index);

	spin_lock_irqsave(&g_edp_data_lock, flags);
	p_edpp->enabled = 1;
	spin_unlock_irqrestore(&g_edp_data_lock, flags);

	bl = disp_edp_get_bright(p_edp);
	disp_edp_set_bright(p_edp, bl);
EXIT:

	return ret;
}

s32 disp_edp_set_bright(struct disp_device *edp, u32 bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	unsigned long flags;
	bool bright_update = false;
	struct disp_manager *mgr = NULL;
	struct disp_smbl *smbl = NULL;

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	mgr = edp->manager;
	if (!mgr) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}
	smbl = mgr->smbl;

	spin_lock_irqsave(&g_edp_data_lock, flags);
	backlight_bright = (backlight_bright > 255) ? 255 : backlight_bright;
	if (edpp->backlight_bright != backlight_bright) {
		bright_update = true;
		edpp->backlight_bright = backlight_bright;
	}
	spin_unlock_irqrestore(&g_edp_data_lock, flags);
	if (bright_update && smbl)
		smbl->update_backlight(smbl, backlight_bright);

	if (edpp->pwm_info.dev) {
		backlight_bright = edpp->backlight_bright;
		if (backlight_bright != 0)
			backlight_bright += 1;

		edpp->backlight_dimming = (edpp->backlight_dimming == 0)
					      ? 256
					      : edpp->backlight_dimming;
		backlight_dimming = edpp->backlight_dimming;
		period_ns = edpp->pwm_info.period_ns;
		duty_ns =
		    (backlight_bright * backlight_dimming * period_ns / 256 +
		     128) /
		    256;
		edpp->pwm_info.duty_ns = duty_ns;
		disp_sys_pwm_config(edpp->pwm_info.dev, duty_ns, period_ns);
	}

	return DIS_SUCCESS;
}

s32 disp_edp_get_bright(struct disp_device *edp)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	return edpp->backlight_bright;
}

s32 disp_edp_sw_enable(struct disp_device *p_edp)
{
	struct disp_manager *mgr = NULL;
	unsigned long flags;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if (!p_edp || !p_edpp) {
		DE_WRN("p_edp init null hdl!\n");
		return DIS_FAIL;
	}

	mgr = p_edp->manager;
	if (!mgr) {
		DE_WRN("edp%d's mgr is NULL\n", p_edp->disp);
		return DIS_FAIL;
	}

	if (p_edpp->enabled == 1) {
		DE_WRN("edp%d is already open\n", p_edp->disp);
		return DIS_FAIL;
	}


	if (p_edpp->edp_func.tv_irq_enable == NULL) {
		DE_WRN("edp_func.tv_irq_enable is null\n");
		return DIS_FAIL;
	}

	p_edpp->edp_func.tv_get_video_timing_info(p_edpp->edp_index,
						  &(p_edpp->video_info));

	if (p_edpp->video_info == NULL) {
		DE_WRN("video info is null\n");
		return DIS_FAIL;
	}

	memcpy(&p_edp->timings, p_edpp->video_info,
	       sizeof(struct disp_video_timings));

	edp_calc_judge_line(p_edp);

	if (mgr->sw_enable)
		mgr->sw_enable(mgr);

	p_edpp->edp_func.tv_irq_enable(p_edpp->edp_index, 0, 0);
	disp_sys_register_irq(p_edpp->irq_no, 0, disp_edp_event_proc,
			      (void *)p_edp, 0, 0);
	disp_sys_enable_irq(p_edpp->irq_no);
	p_edpp->edp_func.tv_irq_enable(p_edpp->edp_index, 0, 1);

	spin_lock_irqsave(&g_edp_data_lock, flags);
	p_edpp->enabled = 1;
	spin_unlock_irqrestore(&g_edp_data_lock, flags);
	return 0;
}

s32 disp_edp_get_status(struct disp_device *dispdev)
{
	s32 ret = 0;
	/* fifo flow status */
	return ret;
}

#if defined(__LINUX_PLAT__)
s32 disp_edp_deliver_dev(struct disp_device *p_edp,
			 struct device *dev)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if (p_edpp == NULL) {
		DE_WRN("edp private date not found, deliver dev fail!\n");
		return DIS_FAIL;
	}

	p_edpp->dev = dev;

	return 0;
}

/* init callback by each edp driver */
s32 disp_edp_initialize(struct disp_device *p_edp, int sel)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	s32 ret = -1;
	__u64 backlight_bright;
	__u64 period_ns, duty_ns;

	if (p_edpp == NULL) {
		DE_WRN("edp private date not found, deliver dev fail!\n");
		return DIS_FAIL;
	}

	if (p_edpp->edp_index != sel) {
		DE_WRN("[%s]:edp device not match!\n", __func__);
		return DIS_FAIL;
	}

	ret = edp_regulator_get_wrap(p_edpp, sel);

	if (p_edpp->edp_pwm_used) {
		p_edpp->pwm_info.channel = p_edpp->edp_pwm_ch;
		p_edpp->pwm_info.polarity = p_edpp->edp_pwm_pol;
		p_edpp->pwm_info.dev = disp_sys_pwm_request(p_edpp->edp_pwm_ch);
		if (p_edpp->edp_pwm_freq != 0) {
			period_ns = 1000 * 1000 * 1000 / p_edpp->edp_pwm_freq;
		} else {
			DE_WRN("edp%d.edp_pwm_freq is ZERO\n",
			       p_edpp->edp_index);
			/* default 1khz */
			period_ns = 1000 * 1000 * 1000 / 1000;
		}

		backlight_bright = p_edpp->backlight_bright;
		duty_ns = (backlight_bright * period_ns) / 256;

		p_edpp->pwm_info.duty_ns = duty_ns;
		p_edpp->pwm_info.period_ns = period_ns;

		disp_sys_pwm_config(p_edpp->pwm_info.dev, p_edpp->pwm_info.duty_ns,
				    p_edpp->pwm_info.period_ns);
		disp_sys_pwm_set_polarity(p_edpp->pwm_info.dev,
					  p_edpp->pwm_info.polarity);
	}

	return ret;
}

/* de-init callback by each edp driver */
s32 disp_edp_deinitialize(struct disp_device *p_edp, int sel)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	s32 ret = -1;

	if (p_edpp == NULL) {
		DE_WRN("edp private date not found, deliver dev fail!\n");
		return DIS_FAIL;
	}

	if (p_edpp->edp_index != sel) {
		DE_WRN("[%s]:edp device not match!\n", __func__);
		return DIS_FAIL;
	}

	ret = edp_regulator_put_wrap(p_edpp);

	p_edpp->dev = NULL;

	return ret;
}

static s32 cal_real_frame_period(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	unsigned long long temp = 0;

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("disp set func null  hdl!\n");
		return DIS_FAIL;
	}

	if (!p_edpp->edp_func.tv_get_startdelay) {
		DE_WRN("null tv_get_startdelay\n");
		return DIS_FAIL;
	}

	temp = ONE_SEC * p_edp->timings.hor_total_time *
	       p_edp->timings.ver_total_time;

	do_div(temp, p_edp->timings.pixel_clk);

	p_edp->timings.frame_period = temp;

#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
	p_edp->timings.start_delay =
		disp_al_device_get_start_delay(p_edp->hwdev_index);
#else
	p_edp->timings.start_delay =
	    p_edpp->edp_func.tv_get_startdelay(p_edpp->edp_index);
#endif

	DE_INF("frame_period:%lld,start_delay:%d\n",
	       p_edp->timings.frame_period, p_edp->timings.start_delay);

	return 0;
}

u32 disp_edp_usec_before_vblank(struct disp_device *p_edp)
{
	int cur_line;
	int start_delay;
	u32 usec = 0;
	struct disp_video_timings *timings;
	u32 usec_per_line;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if (!p_edp || !p_edpp) {
		DE_WRN("NULL hdl\n");
		goto OUT;
	}

#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
	start_delay = disp_al_device_get_start_delay(p_edp->hwdev_index);
	cur_line = disp_al_device_get_cur_line(p_edp->hwdev_index);
#else
	start_delay = p_edpp->edp_func.tv_get_startdelay(p_edpp->edp_index);
	cur_line = p_edpp->edp_func.tv_get_cur_line(p_edpp->edp_index);
#endif
	if (cur_line > (start_delay - 4)) {
		timings = &p_edp->timings;
		usec_per_line = timings->hor_total_time *
			1000000 / timings->pixel_clk;
		usec = (timings->ver_total_time - cur_line + 1) * usec_per_line;
	}

OUT:
	return usec;
}
#endif

bool disp_edp_is_in_safe_period(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);
	int start_delay, cur_line;
	bool ret = true;

	if (!p_edp || !p_edpp) {
		DE_WRN("NULL hdl\n");
		goto OUT;
	}

#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
	start_delay = disp_al_device_get_start_delay(p_edp->hwdev_index);
	cur_line = disp_al_device_get_cur_line(p_edp->hwdev_index);
#else
	start_delay = p_edpp->edp_func.tv_get_startdelay(p_edpp->edp_index);
	cur_line = p_edpp->edp_func.tv_get_cur_line(p_edpp->edp_index);
#endif

	if (cur_line >= start_delay)
		ret = false;

OUT:
	return ret;
}

static s32 disp_edp_set_static_config(struct disp_device *p_edp,
			       struct disp_device_config *config)
{
	return disp_edp_set_mode(p_edp, config->mode);
}

static s32 disp_edp_get_static_config(struct disp_device *p_edp,
			      struct disp_device_config *config)
{
	int ret = 0;
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL)) {
		DE_WRN("p_edp set func null  hdl!\n");
		goto OUT;
	}

	config->type = p_edp->type;
	config->format = DISP_CSC_TYPE_RGB;
OUT:
	return ret;
}

static s32 disp_edp_set_open_func(struct disp_device *edp, EDP_FUNC func,
				  u32 delay)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if ((edp == NULL) || (edpp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (func) {
		edpp->open_flow.func[edpp->open_flow.func_num].func = func;
		edpp->open_flow.func[edpp->open_flow.func_num].delay = delay;
		edpp->open_flow.func_num++;
	}

	return DIS_SUCCESS;
}

static s32 disp_edp_set_close_func(struct disp_device *edp, EDP_FUNC func,
				   u32 delay)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if ((edp == NULL) || (edpp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}

	if (func) {
		edpp->close_flow.func[edpp->close_flow.func_num].func = func;
		edpp->close_flow.func[edpp->close_flow.func_num].delay = delay;
		edpp->close_flow.func_num++;
	}

	return DIS_SUCCESS;
}

static s32 disp_edp_set_panel_funs(struct disp_device *edp, char *name,
				   disp_lcd_panel_fun *panel_cfg)
{
	s32 ret = -1;
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if ((edp == NULL) || (edpp == NULL)) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}


	DE_INF("edp %d, driver_name %s,  panel_name %s\n", edp->disp, edpp->panel_driver_name,
	       name);
	if (!strcmp(edpp->panel_driver_name, name)) {
		memset(&edpp->edp_panel_fun,
		       0,
		       sizeof(disp_lcd_panel_fun));
		edpp->edp_panel_fun.cfg_panel_info = panel_cfg->cfg_panel_info;
		edpp->edp_panel_fun.cfg_open_flow = panel_cfg->cfg_open_flow;
		edpp->edp_panel_fun.cfg_close_flow = panel_cfg->cfg_close_flow;
		edpp->edp_panel_fun.set_bright = panel_cfg->set_bright;

		ret = 0;
	}

	return ret;
}

static s32 disp_edp_set_bright_dimming(struct disp_device *edp, u32 dimming)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	u32 bl = 0;

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	dimming = dimming > 256?256:dimming;
	edpp->backlight_dimming = dimming;
	bl = disp_edp_get_bright(edp);
	disp_edp_set_bright(edp, bl);

	return DIS_SUCCESS;
}

static s32 disp_edp_backlight_enable(struct disp_device *edp)
{
	disp_gpio_set_t gpio_info;
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	unsigned long flags;
	unsigned int bl;

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&g_edp_data_lock, flags);
	if (edpp->bl_enabled) {
		spin_unlock_irqrestore(&g_edp_data_lock, flags);
		return -EBUSY;
	}

	edpp->bl_need_enabled = 1;
	edpp->bl_enabled = true;
	spin_unlock_irqrestore(&g_edp_data_lock, flags);

	if (edpp->edp_bl_regulator)
		disp_sys_power_enable(edpp->edp_bl_regulator);

	memcpy(&gpio_info, &(edpp->edp_bl_en),
	       sizeof(disp_gpio_set_t));

	edpp->edp_bl_en_hdl = disp_sys_gpio_request_simple(&gpio_info, 1);
	bl = disp_edp_get_bright(edp);
	disp_edp_set_bright(edp, bl);

	return 0;
}

static s32 disp_edp_backlight_disable(struct disp_device *edp)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	unsigned long flags;

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&g_edp_data_lock, flags);
	if (!edpp->bl_enabled) {
		spin_unlock_irqrestore(&g_edp_data_lock, flags);
		return -EBUSY;
	}

	edpp->bl_enabled = false;
	spin_unlock_irqrestore(&g_edp_data_lock, flags);

	if (!edpp->edp_bl_en_used)
		return 0;

	disp_sys_gpio_release(edpp->edp_bl_en_hdl, 2);
	if (edpp->edp_bl_regulator)
		disp_sys_power_disable(edpp->edp_bl_regulator);

	return 0;
}

static s32 disp_edp_pwm_enable(struct disp_device *edp)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	disp_sys_pwm_config(edpp->pwm_info.dev, edpp->pwm_info.duty_ns,
			    edpp->pwm_info.period_ns);
	disp_sys_pwm_set_polarity(edpp->pwm_info.dev,
				  edpp->pwm_info.polarity);

	if (edpp->pwm_info.dev)
		return disp_sys_pwm_enable(edpp->pwm_info.dev);

	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

static s32 disp_edp_pwm_disable(struct disp_device *edp)
{
	struct disp_edp_private_data *edpp = disp_edp_get_priv(edp);
	s32 ret = -1;

	if (!edp || !edpp) {
		DE_WRN("NULL hdl!\n");
		return DIS_FAIL;
	}

	if (edpp->pwm_info.dev) {
		ret = disp_sys_pwm_disable(edpp->pwm_info.dev);
		/* following is for reset pwm state purpose */
		disp_sys_pwm_config(edpp->pwm_info.dev,
				    edpp->pwm_info.duty_ns - 1,
				    edpp->pwm_info.period_ns);
		disp_sys_pwm_set_polarity(edpp->pwm_info.dev,
					  !edpp->pwm_info.polarity);
		return ret;
	}

	DE_WRN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

void disp_edp_show_builtin_patten(struct disp_device *edp, u32 patten)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(edp);

	if (p_edpp->edp_func.tv_show_builtin_patten)
		p_edpp->edp_func.tv_show_builtin_patten(p_edpp->edp_index,
							patten);
}

/**
 * @name       :disp_init_edp
 * @brief      :register edp device
 * @param[IN]  :para init parameter
 * @param[OUT] :none
 * @return     :0 if success otherwise fail
 */
s32 disp_init_edp(disp_bsp_init_para *para)
{
	s32 ret = 0;
	u32 disp = 0, edp_index = 0;
	struct disp_device *p_edp;
	struct disp_edp_private_data *p_edpp;
	u32 num_devices;
	u32 hwdev_index = 0;

	DE_INF("disp_init_edp\n");
	spin_lock_init(&g_edp_data_lock);

	g_edp_used = 1;

	DE_INF("%s\n", __func__);

	num_devices = bsp_disp_feat_get_num_devices();

	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types(
			hwdev_index, DISP_OUTPUT_TYPE_EDP))
			++num_devices_support_edp;
	}

	g_pedp_devices =
	    kmalloc(sizeof(struct disp_device) * num_devices_support_edp,
		    GFP_KERNEL | __GFP_ZERO);

	if (g_pedp_devices == NULL) {
		DE_WRN("malloc memory for g_pedp_devices fail!\n");
		goto malloc_err;
	}

	g_pedp_private = kmalloc_array(num_devices_support_edp, sizeof(*p_edpp),
				       GFP_KERNEL | __GFP_ZERO);
	if (g_pedp_private == NULL) {
		DE_WRN("malloc memory for g_pedp_private fail!\n");
		goto malloc_err;
	}
	DE_INF("disp_init_edp:num:%d\n", num_devices_support_edp);

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types(
			hwdev_index, DISP_OUTPUT_TYPE_EDP)) {
			continue;
		}

		if (para->boot_info.type == DISP_OUTPUT_TYPE_EDP &&
		    edp_index == 0)
			disp = para->boot_info.disp;
		p_edp                     = &g_pedp_devices[edp_index];
		p_edpp                    = &g_pedp_private[edp_index];
		p_edp->priv_data          = (void *)p_edpp;

		p_edp->disp               = disp;
		p_edp->hwdev_index        = hwdev_index;
		sprintf(p_edp->name, "edp%d", edp_index);
		p_edp->type               = DISP_OUTPUT_TYPE_EDP;
		p_edpp->irq_no = para->irq_no[DISP_MOD_LCD0 + hwdev_index];
//		p_edpp->clk = para->clk_tcon[hwdev_index];
		p_edpp->clk = para->mclk[DISP_MOD_LCD0 + hwdev_index];
//		p_edpp->clk_bus = para->clk_bus_tcon[hwdev_index];
//		p_edpp->reset = para->rst_bus_tcon[hwdev_index];
		p_edpp->edp_index = edp_index;
		/* function register */
		p_edp->set_manager        = disp_device_set_manager;
		p_edp->unset_manager      = disp_device_unset_manager;
		p_edp->get_resolution     = disp_device_get_resolution;
		p_edp->get_timings        = disp_device_get_timings;
		p_edp->is_interlace       = disp_device_is_interlace;
		p_edp->init               = disp_edp_init;
		p_edp->exit               = disp_edp_exit;
		p_edp->set_tv_func        = disp_edp_set_func;
#if defined(__LINUX_PLAT__)
		p_edp->deliver_dev        = disp_edp_deliver_dev;
		p_edp->drv_init           = disp_edp_initialize;
		p_edp->drv_deinit         = disp_edp_deinitialize;
#endif
		p_edp->enable             = disp_edp_enable;
		p_edp->disable            = disp_edp_disable;
		p_edp->is_enabled         = disp_edp_is_enabled;
		p_edp->sw_enable          = disp_edp_sw_enable;
		p_edp->get_input_csc      = disp_edp_get_input_csc;
		p_edp->suspend            = disp_edp_suspend;
		p_edp->resume             = disp_edp_resume;
		p_edp->get_fps            = disp_edp_get_fps;
		p_edp->get_status         = disp_edp_get_status;
#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
		p_edp->is_in_safe_period  = disp_device_is_in_safe_period;
//		p_edp->usec_before_vblank = disp_device_usec_before_vblank;
#else
		p_edp->is_in_safe_period  = disp_edp_is_in_safe_period;
//		p_edp->usec_before_vblank = disp_edp_usec_before_vblank;
#endif
		p_edp->set_static_config  = disp_edp_set_static_config;
		p_edp->get_static_config  = disp_edp_get_static_config;
		p_edp->set_panel_func     = disp_edp_set_panel_funs;
		p_edp->set_open_func      = disp_edp_set_open_func;
		p_edp->set_close_func     = disp_edp_set_close_func;
		p_edp->backlight_enable   = disp_edp_backlight_enable;
		p_edp->backlight_disable  = disp_edp_backlight_disable;
		p_edp->set_bright = disp_edp_set_bright;
		p_edp->get_bright = disp_edp_get_bright;
		p_edp->set_bright_dimming = disp_edp_set_bright_dimming;
		p_edp->pwm_enable = disp_edp_pwm_enable;
		p_edp->pwm_disable = disp_edp_pwm_disable;
		p_edp->power_enable = disp_edp_power_enable;
		p_edp->power_disable = disp_edp_power_disable;
		p_edp->gpio_set_value = disp_edp_gpio_set_value;
		p_edp->gpio_set_direction = disp_edp_gpio_set_direction;
		p_edp->show_builtin_patten = disp_edp_show_builtin_patten;
		ret = p_edp->init(p_edp);
		if (ret != 0) {
			DE_INF("edp %d %d is not used\n", disp, hwdev_index);
			++disp;
			if (edp_index <= num_devices_support_edp - 1)
				++edp_index;
			continue;
		}

		disp_device_register(p_edp);
		DE_INF("register edp %d %d\n", disp, hwdev_index);
		++disp;
		if (edp_index <= num_devices_support_edp - 1)
			++edp_index;
	}
	if (ret == 0) {
		goto exit;
	}

malloc_err:
	if (g_pedp_devices != NULL)
		kfree(g_pedp_devices);
	if (g_pedp_private != NULL)
		kfree(g_pedp_private);
	g_pedp_devices = NULL;
	g_pedp_private = NULL;
exit:
	return ret;
}

/**
 * @name       :disp_exit_edp
 * @brief      :exit edp module
 * @param[IN]  :
 * @param[OUT] :
 * @return     :
 */
s32 disp_exit_edp(void)
{
	s32 ret = 0;

	DE_WRN("\n");
	/* TODO */
	return ret;
}

int disp_edp_get_cur_line(struct disp_device *p_edp)
{
	struct disp_edp_private_data *p_edpp = disp_edp_get_priv(p_edp);

	if ((p_edp == NULL) || (p_edpp == NULL))
		return DIS_FAIL;

#if defined(EDP_SUPPORT_TCON_UPDATE_SAFE_TIME)
	return disp_al_device_get_cur_line(p_edp->hwdev_index);
#else
	return p_edpp->edp_func.tv_get_cur_line(p_edpp->edp_index);
#endif
}

#else

struct disp_device *disp_get_edp(u32 disp)
{

	DE_WRN("EDP is not support\n");
	return NULL;
}

#endif /* endif SUPPORT_EDP */
