/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "accdet.h"
#ifdef CONFIG_ACCDET_EINT
#include <linux/gpio.h>
#endif
#include <upmu_common.h>
#include <linux/timer.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include "accdet_htc.h"
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
#include <sound/htc_acoustic_alsa.h>

void EnableMicbias1(bool enabled);
void htc_accdet_typec_headset_plugout(void);
int isHeadsetAmpOn(bool isL);
static int isEnableUnknownAccessoryTest = 0;
#endif

#define DEBUG_THREAD 1
#define GET_ADC_DIRECTLY


#define REGISTER_VALUE(x)   (x - 1)

static volatile int cur_key_flag = 0;
static volatile int cur_key_report = 0;

#define ACCDET_DEBOUNCE3_PLUG_OUT	(0x20*30) 
static int button_press_debounce = 0x250; 
int cur_key = 0;
struct head_dts_data accdet_dts_data;
s8 accdet_auxadc_offset;
int accdet_irq;
unsigned int gpiopin, headsetdebounce;
unsigned int accdet_eint_type;
struct headset_mode_settings *cust_headset_settings;
#define ACCDET_DEBUG(format, args...) pr_debug(format, ##args)
#define ACCDET_INFO(format, args...) pr_warn(format, ##args)
#define ACCDET_ERROR(format, args...) pr_err(format, ##args)
static struct switch_dev accdet_data;
static struct input_dev *kpd_accdet_dev;
static struct cdev *accdet_cdev;
static struct class *accdet_class;
static struct device *accdet_nor_device;
static dev_t accdet_devno;
static int pre_status;
static int pre_state_swctrl;
static int accdet_status = PLUG_OUT;
static int cable_type;
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
static int cable_pin_recognition;
static int show_icon_delay;
#endif
#if defined(ACCDET_TS3A225E_PIN_SWAP)
#define TS3A225E_CONNECTOR_NONE				0
#define TS3A225E_CONNECTOR_TRS				1
#define TS3A225E_CONNECTOR_TRRS_STANDARD	2
#define TS3A225E_CONNECTOR_TRRS_OMTP		3
unsigned char ts3a225e_reg_value[7] = { 0 };
unsigned char ts3a225e_connector_type = TS3A225E_CONNECTOR_NONE;
#endif
static int eint_accdet_sync_flag;
static int g_accdet_first = 1;
static bool IRQ_CLR_FLAG;
static int call_status; 
static int button_status;
struct wake_lock accdet_suspend_lock;
struct wake_lock accdet_irq_lock;
struct wake_lock accdet_key_lock;
struct wake_lock accdet_timer_lock;
static struct work_struct accdet_work;
static struct workqueue_struct *accdet_workqueue;
static DEFINE_MUTEX(accdet_eint_irq_sync_mutex);
static inline void clear_accdet_interrupt(void);
static inline void clear_accdet_eint_interrupt(void);
static void send_key_event(int keycode, int flag);
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
static struct work_struct accdet_eint_work;
static struct workqueue_struct *accdet_eint_workqueue;
static inline void accdet_init(void);
#define MICBIAS_DISABLE_TIMER   (6 * HZ)	
struct timer_list micbias_timer;
static void disable_micbias(unsigned long a);
#define EINT_PIN_PLUG_IN        (1)
#define EINT_PIN_PLUG_OUT       (0)
int cur_eint_state = EINT_PIN_PLUG_OUT;
static struct work_struct accdet_disable_work;
static struct workqueue_struct *accdet_disable_workqueue;
#else
#endif
#ifndef CONFIG_ACCDET_EINT_IRQ
struct pinctrl *accdet_pinctrl1;
struct pinctrl_state *pins_eint_int;
#endif
#ifdef DEBUG_THREAD
#endif
static u32 pmic_pwrap_read(u32 addr);
static void pmic_pwrap_write(u32 addr, unsigned int wdata);
char *accdet_status_string[5] = {
	"Plug_out",
	"Headset_plug_in",
	
	"Hook_switch",
	
	"Stand_by"
};
char *accdet_report_string[4] = {
	"No_device",
	"Headset_mic",
	"Headset_no_mic",
	
	
};

static struct htc_headset_info *hi;

char *button_string[6]=
{
    "NO_key",
    "UP_key",
    "MD_key",
    "no_define_key",
    "DW_key"
};


void accdet_detect(void)
{
	int ret = 0;

	ACCDET_DEBUG("[Accdet]accdet_detect\n");

	accdet_status = PLUG_OUT;
	ret = queue_work(accdet_workqueue, &accdet_work);
	if (!ret)
		ACCDET_DEBUG("[Accdet]accdet_detect:accdet_work return:%d!\n", ret);
}
EXPORT_SYMBOL(accdet_detect);

void accdet_state_reset(void)
{

	ACCDET_DEBUG("[Accdet]accdet_state_reset\n");

	accdet_status = PLUG_OUT;
	cable_type = NO_DEVICE;
}
EXPORT_SYMBOL(accdet_state_reset);

int accdet_get_cable_type(void)
{
	return cable_type;
}

void accdet_auxadc_switch(int enable)
{
	if (enable) {
		pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) | ACCDET_BF_ON);
		
	} else {
		pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) & ~(ACCDET_BF_ON));
		
	}
}

static u64 accdet_get_current_time(void)
{
	return sched_clock();
}

static bool accdet_timeout_ns(u64 start_time_ns, u64 timeout_time_ns)
{
	u64 cur_time = 0;
	u64 elapse_time = 0;

	
	cur_time = accdet_get_current_time();	
	if (cur_time < start_time_ns) {
		ACCDET_DEBUG("@@@@Timer overflow! start%lld cur timer%lld\n", start_time_ns, cur_time);
		start_time_ns = cur_time;
		timeout_time_ns = 400 * 1000;	
		ACCDET_DEBUG("@@@@reset timer! start%lld setting%lld\n", start_time_ns, timeout_time_ns);
	}
	elapse_time = cur_time - start_time_ns;

	
	if (timeout_time_ns <= elapse_time) {
		
		ACCDET_DEBUG("@@@@ACCDET IRQ clear Timeout\n");
		return false;
	}
	return true;
}

static u32 pmic_pwrap_read(u32 addr)
{
	u32 val = 0;

	pwrap_read(addr, &val);
	return val;
}

static void pmic_pwrap_write(unsigned int addr, unsigned int wdata)
{
	pwrap_write(addr, wdata);
}

#ifdef GET_ADC_DIRECTLY
static int Accdet_PMIC_IMM_GetOneChannelValue(int deCount)
{
	unsigned int vol_val = 0;

	pmic_pwrap_write(ACCDET_AUXADC_CTL_SET, ACCDET_CH_REQ_EN);
	mdelay(3);
	while ((pmic_pwrap_read(ACCDET_AUXADC_REG) & ACCDET_DATA_READY) != ACCDET_DATA_READY)
		;
	
	vol_val = (pmic_pwrap_read(ACCDET_AUXADC_REG) & ACCDET_DATA_MASK);
	vol_val = (vol_val * 1800) / 4096;	
	vol_val -= accdet_auxadc_offset;
	ACCDET_DEBUG("ACCDET accdet_auxadc_offset: %d mv, MIC_Voltage1 = %d mv!\n\r", accdet_auxadc_offset, vol_val);
	return vol_val;
}
#endif

#ifdef CONFIG_ACCDET_PIN_SWAP

static void accdet_FSA8049_enable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ONE);
}

static void accdet_FSA8049_disable(void)
{
	mt_set_gpio_mode(GPIO_FSA8049_PIN, GPIO_FSA8049_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_FSA8049_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_FSA8049_PIN, GPIO_OUT_ZERO);
}

#endif
static inline void headset_plug_out(void)
{
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
	htc_accdet_typec_headset_plugout();
#endif
	accdet_status = PLUG_OUT;
	cable_type = NO_DEVICE;
	
	if (cur_key != 0) {
		send_key_event(cur_key, 0);
		ACCDET_DEBUG(" [accdet] headset_plug_out send key = %d release\n", cur_key);
		cur_key = 0;
	}
	switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	ACCDET_DEBUG(" [accdet] set state in cable_type = NO_DEVICE\n");

}

static inline void enable_accdet(u32 state_swctrl)
{
	
	ACCDET_DEBUG("accdet: enable_accdet\n");
	
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) | state_swctrl);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_ENABLE);

}

static inline void disable_accdet(void)
{
	int irq_temp = 0;

	pmic_pwrap_write(INT_CON_ACCDET_CLR, RG_ACCDET_IRQ_CLR);
	clear_accdet_interrupt();
	udelay(200);
	mutex_lock(&accdet_eint_irq_sync_mutex);
	while (pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) {
		ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going....\n");
		msleep(20);
	}
	irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
	irq_temp = irq_temp & (~IRQ_CLR_BIT);
	pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
	mutex_unlock(&accdet_eint_irq_sync_mutex);
	
	ACCDET_DEBUG("accdet: disable_accdet\n");
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
#ifdef CONFIG_ACCDET_EINT
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
	
	
	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#endif
#ifdef CONFIG_ACCDET_EINT_IRQ
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, ACCDET_EINT_PWM_EN);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) & (~(ACCDET_ENABLE)));
	
#endif

}

#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
static void disable_micbias(unsigned long a)
{
	int ret = 0;

	ret = queue_work(accdet_disable_workqueue, &accdet_disable_work);
	if (!ret)
		ACCDET_DEBUG("[Accdet]disable_micbias:accdet_work return:%d!\n", ret);
}

static void disable_micbias_callback(struct work_struct *work)
{

	if (cable_type == HEADSET_NO_MIC) {
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 0;
		cable_pin_recognition = 0;
		ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
#endif
		
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) & ~ACCDET_SWCTRL_IDLE_EN);
#ifdef CONFIG_ACCDET_PIN_SWAP
		
		
#endif
		disable_accdet();
		ACCDET_DEBUG("[Accdet] more than 5s MICBIAS : Disabled\n");
	}
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	else if (cable_type == HEADSET_MIC) {
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_thresh);
		ACCDET_DEBUG("[Accdet]pin recog after 5s recover micbias polling!\n");
	}
#endif
}

static void accdet_eint_work_callback(struct work_struct *work)
{
#ifdef CONFIG_ACCDET_EINT_IRQ
	int irq_temp = 0;

	if (cur_eint_state == EINT_PIN_PLUG_IN) {
		ACCDET_DEBUG("[Accdet]DCC EINT func :plug-in, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 1;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		wake_lock_timeout(&accdet_timer_lock, 7 * HZ);
#ifdef CONFIG_ACCDET_PIN_SWAP
		
		msleep(800);
		accdet_FSA8049_enable();	
		ACCDET_DEBUG("[Accdet] FSA8049 enable!\n");
		msleep(250);	
#endif

		accdet_init();	

#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 1;
		
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_width);
		ACCDET_DEBUG("[Accdet]pin recog start!  micbias always on!\n");
#endif
		
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_SWCTRL_IDLE_EN));
		
		enable_accdet(ACCDET_SWCTRL_EN);
	} else {
		
		if(hi) hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
		
		ACCDET_DEBUG("[Accdet]DCC EINT func :plug-out, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 0;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		del_timer_sync(&micbias_timer);
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 0;
		cable_pin_recognition = 0;
#endif
#ifdef CONFIG_ACCDET_PIN_SWAP
		
		accdet_FSA8049_disable();	
		ACCDET_DEBUG("[Accdet] FSA8049 disable!\n");
#endif
		
		disable_accdet();
		headset_plug_out();
		
		
		irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
		irq_temp = irq_temp & (~IRQ_EINT_CLR_BIT);
		pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
	}
#else
	
	if (cur_eint_state == EINT_PIN_PLUG_IN) {
		ACCDET_DEBUG("[Accdet]ACC EINT func :plug-in, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 1;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		wake_lock_timeout(&accdet_timer_lock, 7 * HZ);
#ifdef CONFIG_ACCDET_PIN_SWAP
		
		msleep(800);
		accdet_FSA8049_enable();	
		ACCDET_DEBUG("[Accdet] FSA8049 enable!\n");
		msleep(250);	
#endif
#if defined(ACCDET_TS3A225E_PIN_SWAP)
		ACCDET_DEBUG("[Accdet] TS3A225E enable!\n");
		ts3a225e_write_byte(0x04, 0x01);
		msleep(500);
		ts3a225e_read_byte(0x02, &ts3a225e_reg_value[1]);
		ts3a225e_read_byte(0x03, &ts3a225e_reg_value[2]);
		ts3a225e_read_byte(0x05, &ts3a225e_reg_value[4]);
		ts3a225e_read_byte(0x06, &ts3a225e_reg_value[5]);
		ACCDET_DEBUG("[Accdet] TS3A225E CTRL1=%x!\n", ts3a225e_reg_value[1]);
		ACCDET_DEBUG("[Accdet] TS3A225E CTRL2=%x!\n", ts3a225e_reg_value[2]);
		ACCDET_DEBUG("[Accdet] TS3A225E DAT1=%x!\n", ts3a225e_reg_value[4]);
		ACCDET_DEBUG("[Accdet] TS3A225E INT=%x!\n", ts3a225e_reg_value[5]);
		if (ts3a225e_reg_value[5] == 0x01) {
			ACCDET_DEBUG("[Accdet] TS3A225E A standard TSR headset detected, RING2 and SLEEVE shorted!\n");
			ts3a225e_connector_type = TS3A225E_CONNECTOR_TRS;
			ts3a225e_write_byte(0x02, 0x07);
			ts3a225e_write_byte(0x03, 0xf3);
			msleep(20);
		} else if (ts3a225e_reg_value[5] == 0x02) {
			ACCDET_DEBUG("[Accdet] TS3A225E A microphone detected on either RING2 or SLEEVE!\n");
			if ((ts3a225e_reg_value[4] & 0x40) == 0x00)
				ts3a225e_connector_type = TS3A225E_CONNECTOR_TRRS_STANDARD;
			else
				ts3a225e_connector_type = TS3A225E_CONNECTOR_TRRS_OMTP;
		} else {
			ACCDET_DEBUG("[Accdet] TS3A225E Detection sequence completed without successful!\n");
		}
#endif

		accdet_init();	

#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 1;
		
		pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
		pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_width);
		ACCDET_DEBUG("[Accdet]pin recog start!  micbias always on!\n");
#endif
		
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_SWCTRL_IDLE_EN));
		
		enable_accdet(ACCDET_SWCTRL_EN);
	} else {
		
		if(hi) hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
		
		ACCDET_DEBUG("[Accdet]DCC EINT func :plug-out, cur_eint_state = %d\n", cur_eint_state);
		mutex_lock(&accdet_eint_irq_sync_mutex);
		eint_accdet_sync_flag = 0;
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		del_timer_sync(&micbias_timer);
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		show_icon_delay = 0;
		cable_pin_recognition = 0;
#endif
#ifdef CONFIG_ACCDET_PIN_SWAP
		
		accdet_FSA8049_disable();	
		ACCDET_DEBUG("[Accdet] FSA8049 disable!\n");
#endif
#if defined(ACCDET_TS3A225E_PIN_SWAP)
		ACCDET_DEBUG("[Accdet] TS3A225E disable!\n");
		ts3a225e_connector_type = TS3A225E_CONNECTOR_NONE;
#endif
		
		disable_accdet();
		headset_plug_out();
	}
	enable_irq(accdet_irq);
	ACCDET_DEBUG("[Accdet]enable_irq  !!!!!!\n");
#endif
}

static irqreturn_t accdet_eint_func(int irq, void *data)
{
	int ret = 0;

	ACCDET_DEBUG("[Accdet]Enter accdet_eint_func !!!!!!\n");
	if (cur_eint_state == EINT_PIN_PLUG_IN) {
#ifndef CONFIG_ACCDET_EINT_IRQ
		if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_HIGH);
		else
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_LOW);
#endif
#ifdef CONFIG_ACCDET_EINT_IRQ
		pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) & (~(7 << 4)));
		
		pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) | EINT_IRQ_DE_IN);
		pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);

#else
		gpio_set_debounce(gpiopin, headsetdebounce);
#endif

		
		cur_eint_state = EINT_PIN_PLUG_OUT;
	} else {
#ifndef CONFIG_ACCDET_EINT_IRQ
		if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_LOW);
		else
			irq_set_irq_type(accdet_irq, IRQ_TYPE_LEVEL_HIGH);
#endif

#ifdef CONFIG_ACCDET_EINT_IRQ
		pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) & (~(7 << 4)));
		
		pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) | EINT_IRQ_DE_OUT);
#else
		gpio_set_debounce(gpiopin, accdet_dts_data.accdet_plugout_debounce * 1000);
#endif
		
		cur_eint_state = EINT_PIN_PLUG_IN;

		mod_timer(&micbias_timer, jiffies + MICBIAS_DISABLE_TIMER);
	}
#ifndef CONFIG_ACCDET_EINT_IRQ
	disable_irq_nosync(accdet_irq);
#endif
	ACCDET_DEBUG("[Accdet]accdet_eint_func after cur_eint_state=%d\n", cur_eint_state);

	ret = queue_work(accdet_eint_workqueue, &accdet_eint_work);
	return IRQ_HANDLED;
}
#ifndef CONFIG_ACCDET_EINT_IRQ
static inline int accdet_setup_eint(struct platform_device *accdet_device)
{
	int ret;
	u32 ints[2] = { 0, 0 };
	u32 ints1[2] = { 0, 0 };
	struct device_node *node = NULL;
	struct pinctrl_state *pins_default;

	
	ACCDET_INFO("[Accdet]accdet_setup_eint\n");
	accdet_pinctrl1 = devm_pinctrl_get(&accdet_device->dev);
	if (IS_ERR(accdet_pinctrl1)) {
		ret = PTR_ERR(accdet_pinctrl1);
		dev_err(&accdet_device->dev, "fwq Cannot find accdet accdet_pinctrl1!\n");
		return ret;
	}

	pins_default = pinctrl_lookup_state(accdet_pinctrl1, "default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		dev_err(&accdet_device->dev, "fwq Cannot find accdet pinctrl default!\n");
	}

	pins_eint_int = pinctrl_lookup_state(accdet_pinctrl1, "state_eint_as_int");
	if (IS_ERR(pins_eint_int)) {
		ret = PTR_ERR(pins_eint_int);
		dev_err(&accdet_device->dev, "fwq Cannot find accdet pinctrl state_eint_int!\n");
		return ret;
	}
	pinctrl_select_state(accdet_pinctrl1, pins_eint_int);

	node = of_find_matching_node(node, accdet_of_match);
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		of_property_read_u32_array(node, "interrupts", ints1, ARRAY_SIZE(ints1));
		gpiopin = ints[0];
		headsetdebounce = ints[1];
		accdet_eint_type = ints1[1];
		gpio_set_debounce(gpiopin, headsetdebounce);
		accdet_irq = irq_of_parse_and_map(node, 0);
		ret = request_irq(accdet_irq, accdet_eint_func, IRQF_TRIGGER_NONE, "accdet-eint", NULL);
		if (ret != 0) {
			ACCDET_ERROR("[Accdet]EINT IRQ LINE NOT AVAILABLE\n");
		} else {
			ACCDET_ERROR("[Accdet]accdet set EINT finished, accdet_irq=%d, headsetdebounce=%d\n",
				     accdet_irq, headsetdebounce);
		}
	} else {
		ACCDET_ERROR("[Accdet]%s can't find compatible node\n", __func__);
	}
	return 0;
}
#endif				
#endif				

#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ

#define KEY_SAMPLE_PERIOD        (60)	
#define MULTIKEY_ADC_CHANNEL	 (8)

static DEFINE_MUTEX(accdet_multikey_mutex);
#define NO_KEY			 (0x0)
#define UP_KEY			 (0x01)
#define MD_KEY		  	 (0x02)
#define DW_KEY			 (0x04)
#define AS_KEY			 (0x08)

#ifndef CONFIG_FOUR_KEY_HEADSET
static int key_check(int b)
{
	ACCDET_DEBUG("adc_data: %d v\n",b); 

	
	
	if ((b < accdet_dts_data.three_key.down_key) && (b >= accdet_dts_data.three_key.up_key))
		return DW_KEY;
	else if ((b < accdet_dts_data.three_key.up_key) && (b >= accdet_dts_data.three_key.mid_key))
		return UP_KEY;
	else if ((b < accdet_dts_data.three_key.mid_key) && (b >= 0))
		return MD_KEY;
	ACCDET_DEBUG("[accdet] leave key_check!!\n");
	return NO_KEY;
}
#else
static int key_check(int b)
{
	
	
	if ((b < accdet_dts_data.four_key.down_key_four) && (b >= accdet_dts_data.four_key.up_key_four))
		return DW_KEY;
	else if ((b < accdet_dts_data.four_key.up_key_four) && (b >= accdet_dts_data.four_key.voice_key_four))
		return UP_KEY;
	else if ((b < accdet_dts_data.four_key.voice_key_four) && (b >= accdet_dts_data.four_key.mid_key_four))
		return AS_KEY;
	else if ((b < accdet_dts_data.four_key.mid_key_four) && (b >= 0))
		return MD_KEY;
	ACCDET_DEBUG("[accdet] leave key_check!!\n");
	return NO_KEY;
}

#endif
static void send_key_event(int keycode, int flag)
{
	if (!hi) {
		ACCDET_DEBUG("[accdet] %s: hi is NULL\n", __func__);
		return;
	} else {
		ACCDET_DEBUG("[accdet] %s: hs_mfg_mode %d\n", __func__, hi->hs_mfg_mode);
	}

	switch (keycode)
	{
		case DW_KEY:
			if (hi->hs_mfg_mode) {
				input_report_key(kpd_accdet_dev, KEY_NEXTSONG, flag);
				input_sync(kpd_accdet_dev);
				ACCDET_DEBUG("[accdet]KEY_NEXTSONG %d\n",flag);
			} else {
				input_report_key(kpd_accdet_dev, KEY_VOLUMEDOWN, flag);
				input_sync(kpd_accdet_dev);
				ACCDET_DEBUG("[accdet]KEY_VOLUMEDOWN %d\n",flag);
			}
			break;
		case UP_KEY:
			if (hi->hs_mfg_mode) {
				input_report_key(kpd_accdet_dev, KEY_PREVIOUSSONG, flag);
				input_sync(kpd_accdet_dev);
				ACCDET_DEBUG("[accdet]KEY_PREVIOUSSONG %d\n",flag);
			} else {
				input_report_key(kpd_accdet_dev, KEY_VOLUMEUP, flag);
				input_sync(kpd_accdet_dev);
				ACCDET_DEBUG("[accdet]KEY_VOLUMEUP %d\n",flag);
			}
			break;
		case MD_KEY:
			input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, flag);
			input_sync(kpd_accdet_dev);
			ACCDET_DEBUG("[accdet]KEY_PLAYPAUSE %d\n",flag);
		break;
	}
	
	cur_key_flag = flag;
	cur_key_report = keycode;

#if 0 
	switch (keycode) {
	case DW_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOLUMEDOWN, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOLUMEDOWN %d\n", flag);
		break;
	case UP_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOLUMEUP, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOLUMEUP %d\n", flag);
		break;
	case MD_KEY:
		input_report_key(kpd_accdet_dev, KEY_PLAYPAUSE, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_PLAYPAUSE %d\n", flag);
		break;
	case AS_KEY:
		input_report_key(kpd_accdet_dev, KEY_VOICECOMMAND, flag);
		input_sync(kpd_accdet_dev);
		ACCDET_DEBUG("[accdet]KEY_VOICECOMMAND %d\n", flag);
		break;
	}
#endif 
}

static void multi_key_detection(int current_status)
{
	int m_key = 0;
	int cali_voltage = 0;

	if (0 == current_status) {
#ifdef GET_ADC_DIRECTLY
		cali_voltage = Accdet_PMIC_IMM_GetOneChannelValue(1);
#else
		cali_voltage = PMIC_IMM_GetOneChannelValue(PMIC_AUX_VACCDET_AP, 1, 0);
		cali_voltage -= accdet_auxadc_offset;
		ACCDET_DEBUG("[Accdet]adc cali_voltage1 = %d mv\n", cali_voltage);
#endif
		m_key = cur_key = key_check(cali_voltage);
	}
	mdelay(30);
#ifdef CONFIG_ACCDET_EINT_IRQ
	if (((pmic_pwrap_read(ACCDET_IRQ_STS) & EINT_IRQ_STATUS_BIT) != EINT_IRQ_STATUS_BIT) || eint_accdet_sync_flag) {
#else	
	if (((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) != IRQ_STATUS_BIT) || eint_accdet_sync_flag) {
#endif
		send_key_event(cur_key, !current_status);
	} else {
		ACCDET_DEBUG("[Accdet]plug out side effect key press, do not report key = %d\n", cur_key);
		cur_key = NO_KEY;
	}
	if (current_status)
		cur_key = NO_KEY;
}
#endif
static void accdet_workqueue_func(void)
{
	int ret;

	ret = queue_work(accdet_workqueue, &accdet_work);
	if (!ret)
		ACCDET_DEBUG("[Accdet]accdet_work return:%d!\n", ret);
}

#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
static int htc_usb_headset_type = UNSUPPORTED_TYPE;

int htc_usb_headset_type_get(void)
{
	return htc_usb_headset_type;
}

static const char* get_htc_usb_headset_type_string(void)
{
	if (htc_usb_headset_type == UNSUPPORTED_TYPE)
		return "UNSUPPORTED TYPE";
	if (htc_usb_headset_type == HTC_HEADSET_TYPE)
		return "HTC USB headset";
	if (htc_usb_headset_type == HTC_ADAPTOR_TYPE)
		return "HTC USB adaptor";

	if (htc_usb_headset_type == UNKNOWN_ACCESSORY_TYPE)
		return "UNKNOWN_ACCESSORY_TYPE";

	return "Error";
}

void htc_accdet_typec_headset_plugout(void)
{
	if (htc_typec_usb_accessory_detection_get() == 0)
		return;

	pr_info("[ACCDET] %s: enter\n", __func__);
	htc_usb_headset_type = UNSUPPORTED_TYPE;

	htc_aud_hsmic_2v85_en(0);
	htc_miclr_flip_en_set(MICLR_NON_FLIP);
	htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
	htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);
	htc_aud_miclr_dgnd_sw_en(MICLR_DGND_TO_GND);
	htc_as2_en(AS2_TO_MAIN_MIC);
	pr_info("[ACCDET] %s: exit\n", __func__);
}

int htc_accdet_typec_usb_accessory_detect(void)
{
	int typec_id1;
	int typec_id2;
	int usb_position;
	
	int headset_type = UNSUPPORTED_TYPE;
	int retry = 3;

	pr_info("[ACCDET] %s: enter\n", __func__);

	
	htc_aud_hsmic_2v85_en(1);
	htc_miclr_flip_en_set(MICLR_NON_FLIP);
	htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
	htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);
	htc_aud_miclr_dgnd_sw_en(MICLR_DGND_TO_MICLR);

	EnableMicbias1(true);

	htc_aud_hs_switch(HEADSET_L,true);
	htc_aud_hs_switch(HEADSET_R,true);

	
	msleep(100);

	typec_id1 = htc_typec_id1_get();
	typec_id2 = htc_typec_id2_get();

	if (typec_id1 * typec_id2 == 1)
		htc_as2_en(AS2_TO_MICLR_MIC);
	else
		htc_as2_en(AS2_TO_MAIN_MIC);

	pr_info("[ACCDET] %s: detect usb position\n", __func__);

	if (typec_id1 + typec_id2 == 1) 
	{
		if (typec_id1 == 0)
		{
			htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_FLIP);
			htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_FLIP);
			typec_id1 = htc_typec_id1_get();
			if (typec_id1 == 0)
			{
				pr_info("[ACCDET] %s: incorrect typec id1 value !!\n", __func__);
			}
		}
		headset_type = HTC_ADAPTOR_TYPE;
	}
	else if (typec_id1 * typec_id2 == 1) 
	{
		do
		{
			usb_position = htc_usb_position_get();
			if (usb_position == 0)
			{
				
				htc_miclr_flip_en_set(MICLR_FLIP);
				htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_FLIP);
				htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_FLIP);

				usb_position = htc_usb_position_get();
				if (usb_position == 0)
				{
					
					htc_miclr_flip_en_set(MICLR_NON_FLIP);
					htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
					htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);

					pr_warn("[ACCDET] %s: usb position incorrect...retry %d\n", __func__, retry);
				}
			}

			if (usb_position == 1)
			{
				headset_type = HTC_HEADSET_TYPE;
				break;
			}
		}
		while(--retry >= 0);
	}
	
	else if ( (typec_id1 + typec_id1 == 0) 		
		&& (isEnableUnknownAccessoryTest > 0 ) )
	{
		do
		{
			usb_position = htc_usb_position_get();
			if (usb_position == 0)
			{
				
				htc_miclr_flip_en_set(MICLR_FLIP);
				htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_FLIP);
				htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_FLIP);

				usb_position = htc_usb_position_get();
				if (usb_position == 0)
				{
					
					htc_miclr_flip_en_set(MICLR_NON_FLIP);
					htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
					htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);

					pr_warn("[ACCDET] %s: usb position incorrect...retry %d\n", __func__, retry);
				}
			}

			if (usb_position == 1)
			{
				headset_type = UNKNOWN_ACCESSORY_TYPE;
				break;
			}
		}
		while(--retry >= 0);

	}
	

	if (headset_type == UNSUPPORTED_TYPE)
	{
		htc_aud_hsmic_2v85_en(0);
	}

	if (!isHeadsetAmpOn(HEADSET_L))
		htc_aud_hs_switch(HEADSET_L,false);
	if (!isHeadsetAmpOn(HEADSET_R))
		htc_aud_hs_switch(HEADSET_R,false);

	EnableMicbias1(false);

	pr_info("[ACCDET] %s: exit type = %d\n", __func__, headset_type);
	return headset_type;
}

#endif

int accdet_irq_handler(void)
{
	u64 cur_time = 0;

	cur_time = accdet_get_current_time();

#ifdef CONFIG_ACCDET_EINT_IRQ
	ACCDET_DEBUG("[Accdet accdet_irq_handler]clear_accdet_eint_interrupt: ACCDET_IRQ_STS = 0x%x\n",
		     pmic_pwrap_read(ACCDET_IRQ_STS));
	if ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
	    && ((pmic_pwrap_read(ACCDET_IRQ_STS) & EINT_IRQ_STATUS_BIT) != EINT_IRQ_STATUS_BIT)) {
		clear_accdet_interrupt();
		if (accdet_status == MIC_BIAS) {
			
			pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
			pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_width));
		}
		accdet_workqueue_func();
		while (((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
			&& (accdet_timeout_ns(cur_time, ACCDET_TIME_OUT))))
			;
	} else if ((pmic_pwrap_read(ACCDET_IRQ_STS) & EINT_IRQ_STATUS_BIT) == EINT_IRQ_STATUS_BIT) {
		if (cur_eint_state == EINT_PIN_PLUG_IN) {
			if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
				pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) | EINT_IRQ_POL_HIGH);
			else
				pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & ~EINT_IRQ_POL_LOW);
		} else {
			if (accdet_eint_type == IRQ_TYPE_LEVEL_HIGH)
				pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & ~EINT_IRQ_POL_LOW);
			else
				pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) | EINT_IRQ_POL_HIGH);
		}
		clear_accdet_eint_interrupt();
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
		pr_info("[ACCDET] EINT trigger cur_eint_state = %d\n", cur_eint_state);
		if (htc_typec_usb_accessory_detection_get())
		{
			if (cur_eint_state == EINT_PIN_PLUG_OUT)
			{
				
				if ((htc_usb_headset_type = htc_accdet_typec_usb_accessory_detect()) == UNSUPPORTED_TYPE)
				{
					cur_eint_state = EINT_PIN_PLUG_IN;
					pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & (~IRQ_EINT_CLR_BIT));
					switch_set_state(&hi->unsupported_type, 1);
					pr_info("[ACCDET] unsupport usb headset type...return\n");
					return 1;
				}
			} else if (cur_eint_state == EINT_PIN_PLUG_IN) {
				switch_set_state(&hi->unsupported_type, 0);
			}
		}
#endif
		while (((pmic_pwrap_read(ACCDET_IRQ_STS) & EINT_IRQ_STATUS_BIT)
			&& (accdet_timeout_ns(cur_time, ACCDET_TIME_OUT))))
			;
		accdet_eint_func(accdet_irq, NULL);
	} else {
		ACCDET_DEBUG("ACCDET IRQ and EINT IRQ don't be triggerred!!\n");
	}
#else
	if ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT))
		clear_accdet_interrupt();
	if (accdet_status == MIC_BIAS) {
		
		pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
		pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_width));
	}
	accdet_workqueue_func();
	while (((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
		&& (accdet_timeout_ns(cur_time, ACCDET_TIME_OUT))))
		;
#endif
#ifdef ACCDET_NEGV_IRQ
	cur_time = accdet_get_current_time();
	if ((pmic_pwrap_read(ACCDET_IRQ_STS) & NEGV_IRQ_STATUS_BIT) == NEGV_IRQ_STATUS_BIT) {
		ACCDET_DEBUG("[ACCDET NEGV detect]plug in a error Headset\n\r");
		pmic_pwrap_write(ACCDET_IRQ_STS, (IRQ_NEGV_CLR_BIT));
		while (((pmic_pwrap_read(ACCDET_IRQ_STS) & NEGV_IRQ_STATUS_BIT)
			&& (accdet_timeout_ns(cur_time, ACCDET_TIME_OUT))))
			;
		pmic_pwrap_write(ACCDET_IRQ_STS, (pmic_pwrap_read(ACCDET_IRQ_STS) & (~IRQ_NEGV_CLR_BIT)));
	}
#endif

	return 1;
}

static inline void clear_accdet_interrupt(void)
{
	
	pmic_pwrap_write(ACCDET_IRQ_STS, ((pmic_pwrap_read(ACCDET_IRQ_STS)) & 0x8000) | (IRQ_CLR_BIT));
	ACCDET_DEBUG("[Accdet]clear_accdet_interrupt: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
}

static inline void clear_accdet_eint_interrupt(void)
{
	pmic_pwrap_write(ACCDET_IRQ_STS, (((pmic_pwrap_read(ACCDET_IRQ_STS)) & 0x8000) | IRQ_EINT_CLR_BIT));
	ACCDET_DEBUG("[Accdet]clear_accdet_eint_interrupt: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
}

static inline void check_cable_type(void)
{
	int current_status = 0;
	int irq_temp = 0;	
	int wait_clear_irq_times = 0;
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	int pin_adc_value = 0;
#define PIN_ADC_CHANNEL 5
#endif

	current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0) >> 6);	
	ACCDET_DEBUG("[Accdet]accdet interrupt happen:[%s]current AB = %d\n",
		     accdet_status_string[accdet_status], current_status);

	button_status = 0;
	pre_status = accdet_status;

	
	IRQ_CLR_FLAG = false;
	switch (accdet_status) {
	case PLUG_OUT:
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		pmic_pwrap_write(ACCDET_DEBOUNCE1, cust_headset_settings->debounce1);
#endif
		if (current_status == 0) {
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			
			pmic_pwrap_write(ACCDET_PWM_WIDTH, cust_headset_settings->pwm_width);
			pmic_pwrap_write(ACCDET_PWM_THRESH, cust_headset_settings->pwm_width);
			ACCDET_DEBUG("[Accdet]PIN recognition micbias always on!\n");
			ACCDET_DEBUG("[Accdet]before adc read, pin_adc_value = %d mv!\n", pin_adc_value);
			msleep(500);
			current_status = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0) >> 6);	
			if (current_status == 0 && show_icon_delay != 0) {
				
#ifdef GET_ADC_DIRECTLY
				pin_adc_value = Accdet_PMIC_IMM_GetOneChannelValue(1);
#else
				pin_adc_value = PMIC_IMM_GetOneChannelValue(PMIC_AUX_VACCDET_AP, 1, 0);
				pin_adc_value -= accdet_auxadc_offset;
#endif
				ACCDET_DEBUG("[Accdet]pin_adc_value = %d mv!\n", pin_adc_value);
				
				if (180 > pin_adc_value && pin_adc_value > 90) {	
					
					
					mutex_lock(&accdet_eint_irq_sync_mutex);
					if (1 == eint_accdet_sync_flag) {
						cable_type = HEADSET_NO_MIC;
						accdet_status = HOOK_SWITCH;
						cable_pin_recognition = 1;
						ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n",
							     cable_pin_recognition);
					} else {
						ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
					}
					mutex_unlock(&accdet_eint_irq_sync_mutex);
				} else {
					mutex_lock(&accdet_eint_irq_sync_mutex);
					if (1 == eint_accdet_sync_flag) {
						cable_type = HEADSET_NO_MIC;
						accdet_status = HOOK_SWITCH;
					} else {
						ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
					}
					mutex_unlock(&accdet_eint_irq_sync_mutex);
				}
			}
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				cable_type = HEADSET_NO_MIC;
				accdet_status = HOOK_SWITCH;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
				
				pmic_pwrap_write(ACCDET_DEBOUNCE3, ACCDET_DEBOUNCE3_PLUG_OUT);
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
			
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
			pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
#endif
		} else if (current_status == 3) {
			ACCDET_DEBUG("[Accdet]PLUG_OUT state not change!\n");
#ifdef CONFIG_ACCDET_EINT
			ACCDET_DEBUG("[Accdet] do not send plug out event in plug out\n");
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = PLUG_OUT;
				cable_type = NO_DEVICE;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]PLUG_OUT can't change to this state!\n");
		}
		break;

	case MIC_BIAS:
		
		pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);

		if (current_status == 0) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				while ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT)
					&& (wait_clear_irq_times < 3)) {
					ACCDET_DEBUG("[Accdet]check_cable_type: MIC BIAS clear IRQ on-going1....\n");
					wait_clear_irq_times++;
					msleep(20);
				}
				irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
				irq_temp = irq_temp & (~IRQ_CLR_BIT);
				pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
				IRQ_CLR_FLAG = true;
				accdet_status = HOOK_SWITCH;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			button_status = 1;
			if (button_status) {
				mutex_lock(&accdet_eint_irq_sync_mutex);
				if (1 == eint_accdet_sync_flag)
					multi_key_detection(current_status);
				else
					ACCDET_DEBUG("[Accdet] multi_key_detection: Headset has plugged out\n");
				mutex_unlock(&accdet_eint_irq_sync_mutex);
				
				
				pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
				pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
			}
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
				ACCDET_DEBUG("[Accdet]MIC_BIAS state not change!\n");
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else if (current_status == 3) {
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
			ACCDET_DEBUG("[Accdet]do not send plug ou in micbiast\n");
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = PLUG_OUT;
				cable_type = NO_DEVICE;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]MIC_BIAS can't change to this state!\n");
		}
		break;

	case HOOK_SWITCH:
		if (current_status == 0) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				
				
				
				ACCDET_DEBUG("[Accdet]HOOK_SWITCH state not change!\n");
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
		} else if (current_status == 1) {
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				multi_key_detection(current_status);
				accdet_status = MIC_BIAS;
				cable_type = HEADSET_MIC;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
			
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			cable_pin_recognition = 0;
			ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
			pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
			pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
#endif
			
			pmic_pwrap_write(ACCDET_DEBOUNCE0, button_press_debounce);
		} else if (current_status == 3) {

#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
			cable_pin_recognition = 0;
			ACCDET_DEBUG("[Accdet] cable_pin_recognition = %d\n", cable_pin_recognition);
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
			ACCDET_DEBUG("[Accdet] do not send plug out event in hook switch\n");
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag)
				accdet_status = PLUG_OUT;
			else
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = PLUG_OUT;
				cable_type = NO_DEVICE;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]HOOK_SWITCH can't change to this state!\n");
		}
		break;
	case STAND_BY:
		if (current_status == 3) {
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
			ACCDET_DEBUG("[Accdet]accdet do not send plug out event in stand by!\n");
#else
			mutex_lock(&accdet_eint_irq_sync_mutex);
			if (1 == eint_accdet_sync_flag) {
				accdet_status = PLUG_OUT;
				cable_type = NO_DEVICE;
			} else {
				ACCDET_DEBUG("[Accdet] Headset has plugged out\n");
			}
			mutex_unlock(&accdet_eint_irq_sync_mutex);
#endif
		} else {
			ACCDET_DEBUG("[Accdet]STAND_BY can't change to this state!\n");
		}
		break;

	default:
		ACCDET_DEBUG("[Accdet]check_cable_type: accdet current status error!\n");
		break;

	}

	if (!IRQ_CLR_FLAG) {
		mutex_lock(&accdet_eint_irq_sync_mutex);
		if (1 == eint_accdet_sync_flag) {
			while ((pmic_pwrap_read(ACCDET_IRQ_STS) & IRQ_STATUS_BIT) && (wait_clear_irq_times < 3)) {
				ACCDET_DEBUG("[Accdet]check_cable_type: Clear interrupt on-going2....\n");
				wait_clear_irq_times++;
				msleep(20);
			}
		}
		irq_temp = pmic_pwrap_read(ACCDET_IRQ_STS);
		irq_temp = irq_temp & (~IRQ_CLR_BIT);
		pmic_pwrap_write(ACCDET_IRQ_STS, irq_temp);
		mutex_unlock(&accdet_eint_irq_sync_mutex);
		IRQ_CLR_FLAG = true;
		ACCDET_DEBUG("[Accdet]check_cable_type:Clear interrupt:Done[0x%x]!\n", pmic_pwrap_read(ACCDET_IRQ_STS));

	} else {
		IRQ_CLR_FLAG = false;
	}

	ACCDET_DEBUG("[Accdet]cable type:[%s], status switch:[%s]->[%s]\n",
		     accdet_report_string[cable_type], accdet_status_string[pre_status],
		     accdet_status_string[accdet_status]);
}

static void accdet_work_callback(struct work_struct *work)
{

	wake_lock(&accdet_irq_lock);
	check_cable_type();

#ifdef CONFIG_ACCDET_PIN_SWAP
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	if (cable_pin_recognition == 1) {
		cable_pin_recognition = 0;
		accdet_FSA8049_disable();
		cable_type = HEADSET_NO_MIC;
		accdet_status = PLUG_OUT;
	}
#endif
#endif
	mutex_lock(&accdet_eint_irq_sync_mutex);
	if (1 == eint_accdet_sync_flag) {
		switch_set_state((struct switch_dev *)&accdet_data, cable_type);
		
		if(hi) hi->hs_35mm_type = cable_type;
		
	} else {
		ACCDET_DEBUG("[Accdet] Headset has plugged out don't set accdet state\n");
		
		if(hi) hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
		
	}
	mutex_unlock(&accdet_eint_irq_sync_mutex);
	ACCDET_DEBUG(" [accdet] set state in cable_type  status\n");

	wake_unlock(&accdet_irq_lock);
}

void accdet_get_dts_data(void)
{
	struct device_node *node = NULL;
	int debounce[7];
	#ifdef CONFIG_FOUR_KEY_HEADSET
	int four_key[5];
	#else
	int three_key[4];
	#endif

	ACCDET_INFO("[ACCDET]Start accdet_get_dts_data");
	node = of_find_matching_node(node, accdet_of_match);
	if (node) {
		of_property_read_u32_array(node, "headset-mode-setting", debounce, ARRAY_SIZE(debounce));
		of_property_read_u32(node, "accdet-mic-vol", &accdet_dts_data.mic_mode_vol);
		of_property_read_u32(node, "accdet-plugout-debounce", &accdet_dts_data.accdet_plugout_debounce);
		of_property_read_u32(node, "accdet-mic-mode", &accdet_dts_data.accdet_mic_mode);
		#ifdef CONFIG_FOUR_KEY_HEADSET
		of_property_read_u32_array(node, "headset-four-key-threshold", four_key, ARRAY_SIZE(four_key));
		memcpy(&accdet_dts_data.four_key, four_key+1, ((sizeof(four_key)/sizeof(four_key[0])-1)*sizeof(four_key[0]))); 
		ACCDET_INFO("[Accdet]mid-Key = %d, voice = %d, up_key = %d, down_key = %d\n",
		     accdet_dts_data.four_key.mid_key_four, accdet_dts_data.four_key.voice_key_four,
		     accdet_dts_data.four_key.up_key_four, accdet_dts_data.four_key.down_key_four);
		#else
		of_property_read_u32_array(node, "headset-three-key-threshold", three_key, ARRAY_SIZE(three_key));
		memcpy(&accdet_dts_data.three_key, three_key+1, ((sizeof(three_key)/sizeof(three_key[0])-1)*sizeof(three_key[0])) ); 
		ACCDET_INFO("[Accdet]mid-Key = %d, up_key = %d, down_key = %d\n",
		     accdet_dts_data.three_key.mid_key, accdet_dts_data.three_key.up_key,
		     accdet_dts_data.three_key.down_key);
		#endif

		memcpy(&accdet_dts_data.headset_debounce, debounce, sizeof(debounce));
		cust_headset_settings = &accdet_dts_data.headset_debounce;
		ACCDET_INFO("[Accdet]pwm_width = %x, pwm_thresh = %x\n deb0 = %x, deb1 = %x, mic_mode = %d\n",
		     cust_headset_settings->pwm_width, cust_headset_settings->pwm_thresh,
		     cust_headset_settings->debounce0, cust_headset_settings->debounce1,
		     accdet_dts_data.accdet_mic_mode);
	} else {
		ACCDET_ERROR("[Accdet]%s can't find compatible dts node\n", __func__);
	}
}
void accdet_pmic_Read_Efuse_HPOffset(void)
{
	s16 efusevalue;

	efusevalue = (s16) pmic_Read_Efuse_HPOffset(RG_OTP_PA_ADDR_WORD_INDEX);
	accdet_auxadc_offset = (efusevalue >> RG_OTP_PA_ACCDET_BIT_SHIFT) & 0xFF;
	accdet_auxadc_offset = (accdet_auxadc_offset / 2);
	ACCDET_INFO(" efusevalue = 0x%x, accdet_auxadc_offset = %d\n", efusevalue, accdet_auxadc_offset);
}

static inline void accdet_init(void)
{
	ACCDET_DEBUG("[Accdet]accdet hardware init\n");
	
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
	
	
	
	pmic_pwrap_write(TOP_RST_ACCDET_SET, ACCDET_RESET_SET);
	
	pmic_pwrap_write(TOP_RST_ACCDET_CLR, ACCDET_RESET_CLR);
	
	pmic_pwrap_write(ACCDET_PWM_WIDTH, REGISTER_VALUE(cust_headset_settings->pwm_width));
	pmic_pwrap_write(ACCDET_PWM_THRESH, REGISTER_VALUE(cust_headset_settings->pwm_thresh));
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0x07);

	
	pmic_pwrap_write(ACCDET_EN_DELAY_NUM,
			 (cust_headset_settings->fall_delay << 15 | cust_headset_settings->rise_delay));
	
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
	pmic_pwrap_write(ACCDET_DEBOUNCE1, 0xFFFF);	
	pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);
	pmic_pwrap_write(ACCDET_DEBOUNCE4, ACCDET_DE4);
#else
	pmic_pwrap_write(ACCDET_DEBOUNCE0, cust_headset_settings->debounce0);
	pmic_pwrap_write(ACCDET_DEBOUNCE1, cust_headset_settings->debounce1);
	pmic_pwrap_write(ACCDET_DEBOUNCE3, cust_headset_settings->debounce3);
	pmic_pwrap_write(ACCDET_DEBOUNCE4, ACCDET_DE4);
#endif
	
#ifdef CONFIG_ACCDET_EINT_IRQ
	pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & (~IRQ_EINT_CLR_BIT));
#endif
#ifdef CONFIG_ACCDET_EINT
	pmic_pwrap_write(ACCDET_IRQ_STS, pmic_pwrap_read(ACCDET_IRQ_STS) & (~IRQ_CLR_BIT));
#endif
	pmic_pwrap_write(INT_CON_ACCDET_SET, RG_ACCDET_IRQ_SET);
#ifdef CONFIG_ACCDET_EINT_IRQ
	pmic_pwrap_write(INT_CON_ACCDET_SET, RG_ACCDET_EINT_IRQ_SET);
#endif
#ifdef ACCDET_NEGV_IRQ
	pmic_pwrap_write(INT_CON_ACCDET_SET, RG_ACCDET_NEGV_IRQ_SET);
#endif
   
	pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) | 0xF);
	pmic_pwrap_write(ACCDET_MICBIAS_REG, pmic_pwrap_read(ACCDET_MICBIAS_REG)
		| (accdet_dts_data.mic_mode_vol<<4) | 0x80);
	pmic_pwrap_write(ACCDET_RSV, 0x0010);
#ifdef CONFIG_ACCDET_EINT_IRQ
	pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG)
		| ACCDET_EINT_CON_EN); 
#endif
#ifdef ACCDET_NEGV_IRQ
	pmic_pwrap_write(ACCDET_EINT_NV, pmic_pwrap_read(ACCDET_EINT_NV) | ACCDET_NEGV_DT_EN);
#endif
	if (accdet_dts_data.accdet_mic_mode == 1)	
		;
	else if (accdet_dts_data.accdet_mic_mode == 2)	
		pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) | 0x08C0);
	else if (accdet_dts_data.accdet_mic_mode == 6) {	
		pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) | 0x08C0);
		pmic_pwrap_write(ACCDET_MICBIAS_REG, pmic_pwrap_read(ACCDET_MICBIAS_REG) | 0x0104);
	}
    
#if defined CONFIG_ACCDET_EINT
	
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0x0);
	pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#elif defined CONFIG_ACCDET_EINT_IRQ
	if (cur_eint_state == EINT_PIN_PLUG_OUT)
		pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) | EINT_IRQ_DE_IN);
	pmic_pwrap_write(ACCDET_EINT_CTL, pmic_pwrap_read(ACCDET_EINT_CTL) | EINT_PWM_THRESH);
	
	pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_DISABLE);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) & (~ACCDET_SWCTRL_EN));
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_EINT_PWM_EN);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_EINT_EN);
#else
	
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_ENABLE);
#endif
#ifdef ACCDET_NEGV_IRQ
	pmic_pwrap_write(ACCDET_EINT_PWM_DELAY, pmic_pwrap_read(ACCDET_EINT_PWM_DELAY) & (~0x1F));
	pmic_pwrap_write(ACCDET_EINT_PWM_DELAY, pmic_pwrap_read(ACCDET_EINT_PWM_DELAY) | 0x0F);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_NEGV_EN);
#endif
   
	pmic_pwrap_write(ACCDET_AUXADC_AUTO_SPL, (pmic_pwrap_read(ACCDET_AUXADC_AUTO_SPL) | ACCDET_AUXADC_AUTO_SET));
}

#if DEBUG_THREAD
static int dump_register(void)
{
	int i = 0;

	for (i = ACCDET_RSV; i <= ACCDET_RSV_CON1; i += 2)
		ACCDET_DEBUG(" ACCDET_BASE + %x=%x\n", i, pmic_pwrap_read(ACCDET_BASE + i));

	ACCDET_DEBUG(" TOP_RST_ACCDET(0x%x) =%x\n", TOP_RST_ACCDET, pmic_pwrap_read(TOP_RST_ACCDET));
	ACCDET_DEBUG(" INT_CON_ACCDET(0x%x) =%x\n", INT_CON_ACCDET, pmic_pwrap_read(INT_CON_ACCDET));
	ACCDET_DEBUG(" TOP_CKPDN(0x%x) =%x\n", TOP_CKPDN, pmic_pwrap_read(TOP_CKPDN));
	ACCDET_DEBUG(" ACCDET_MICBIAS_REG(0x%x) =%x\n", ACCDET_MICBIAS_REG, pmic_pwrap_read(ACCDET_MICBIAS_REG));
	ACCDET_DEBUG(" ACCDET_ADC_REG(0x%x) =%x\n", ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG));
#ifdef CONFIG_ACCDET_PIN_SWAP
	
	
#endif
	return 0;
}

#if defined(ACCDET_TS3A225E_PIN_SWAP)
static ssize_t show_TS3A225EConnectorType(struct device_driver *ddri, char *buf)
{
	ACCDET_DEBUG("[Accdet] TS3A225E ts3a225e_connector_type=%d\n", ts3a225e_connector_type);
	return sprintf(buf, "%u\n", ts3a225e_connector_type);
}

static DRIVER_ATTR(TS3A225EConnectorType, 0664, show_TS3A225EConnectorType, NULL);
#endif
static ssize_t accdet_store_call_state(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%d", &call_status);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	switch (call_status) {
	case CALL_IDLE:
		ACCDET_DEBUG("[Accdet]accdet call: Idle state!\n");
		break;

	case CALL_RINGING:

		ACCDET_DEBUG("[Accdet]accdet call: ringing state!\n");
		break;

	case CALL_ACTIVE:
		ACCDET_DEBUG("[Accdet]accdet call: active or hold state!\n");
		ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);
		
		break;

	default:
		ACCDET_DEBUG("[Accdet]accdet call : Invalid values\n");
		break;
	}
	return count;
}

static ssize_t show_pin_recognition_state(struct device_driver *ddri, char *buf)
{
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
	ACCDET_DEBUG("ACCDET show_pin_recognition_state = %d\n", cable_pin_recognition);
	return sprintf(buf, "%u\n", cable_pin_recognition);
#else
	return scnprintf(buf, PAGE_SIZE, "%u\n",0);
#endif
}

static DRIVER_ATTR(accdet_pin_recognition, 0664, show_pin_recognition_state, NULL);
static DRIVER_ATTR(accdet_call_state, 0664, NULL, accdet_store_call_state);

static int g_start_debug_thread;
static struct task_struct *thread;
static int g_dump_register;
static int dbug_thread(void *unused)
{
	while (g_start_debug_thread) {
		if (g_dump_register) {
			dump_register();
			
		}

		msleep(500);

	}
	return 0;
}

static ssize_t store_accdet_start_debug_thread(struct device_driver *ddri, const char *buf, size_t count)
{

	int start_flag;
	int error;
	int ret;

	ret = sscanf(buf, "%d", &start_flag);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet] start flag =%d\n", start_flag);

	g_start_debug_thread = start_flag;

	if (1 == start_flag) {
		thread = kthread_run(dbug_thread, 0, "ACCDET");
		if (IS_ERR(thread)) {
			error = PTR_ERR(thread);
			ACCDET_DEBUG(" failed to create kernel thread: %d\n", error);
		}
	}

	return count;
}

static ssize_t store_accdet_set_headset_mode(struct device_driver *ddri, const char *buf, size_t count)
{

	int value;
	int ret;

	ret = sscanf(buf, "%d", &value);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	ACCDET_DEBUG("[Accdet]store_accdet_set_headset_mode value =%d\n", value);

	return count;
}

static ssize_t store_accdet_dump_register(struct device_driver *ddri, const char *buf, size_t count)
{
	int value;
	int ret;
	ret = sscanf(buf, "%d", &value);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	g_dump_register = value;

	ACCDET_DEBUG("[Accdet]store_accdet_dump_register value =%d\n", value);

	return count;
}

static DRIVER_ATTR(dump_register, S_IWUSR | S_IRUGO, NULL, store_accdet_dump_register);

static DRIVER_ATTR(set_headset_mode, S_IWUSR | S_IRUGO, NULL, store_accdet_set_headset_mode);

static DRIVER_ATTR(start_debug, S_IWUSR | S_IRUGO, NULL, store_accdet_start_debug_thread);

static struct driver_attribute *accdet_attr_list[] = {
	&driver_attr_start_debug,
	&driver_attr_set_headset_mode,
	&driver_attr_dump_register,
	&driver_attr_accdet_call_state,
	
	&driver_attr_accdet_pin_recognition,
	
#if defined(ACCDET_TS3A225E_PIN_SWAP)
	&driver_attr_TS3A225EConnectorType,
#endif
};

static int accdet_create_attr(struct device_driver *driver)
{
	int idx, err = 0;
	int num = (int)(sizeof(accdet_attr_list) / sizeof(accdet_attr_list[0]));

	if (driver == NULL)
		return -EINVAL;
	for (idx = 0; idx < num; idx++) {
		err = driver_create_file(driver, accdet_attr_list[idx]);
		if (err) {
			ACCDET_DEBUG("driver_create_file (%s) = %d\n", accdet_attr_list[idx]->attr.name, err);
			break;
		}
	}
	return err;
}

#endif

int switch_send_event(unsigned int bit, int on)
{
	unsigned int state;

	mutex_lock(&hi->mutex_lock);
	state = switch_get_state(&hi->sdev_h2w);
	state &= ~(bit);

	if (on)
		state |= bit;

	switch_set_state(&hi->sdev_h2w, state);
	mutex_unlock(&hi->mutex_lock);
	return 0;
}

static void set_35mm_hw_state(int state)
{
	ACCDET_DEBUG();
}

static ssize_t headset_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int length = 0;
	char *state = NULL;

	ACCDET_DEBUG("%s \n", __func__);
	if (!hi) {
		ACCDET_DEBUG("%s: hi is NULL\n", __func__);
		state = "error";
		length = snprintf(buf, PAGE_SIZE, "%s\n", state);
		return length;
	}

	switch (hi->hs_35mm_type) {
	case HTC_HEADSET_UNPLUG:
		state = "headset_unplug";
		break;
	case HTC_HEADSET_NO_MIC:
		state = "headset_no_mic";
		break;
	case HTC_HEADSET_MIC:
		state = "headset_mic";
		break;
	case HTC_HEADSET_METRICO:
		state = "headset_metrico";
		break;
	case HTC_HEADSET_UNKNOWN_MIC:
		state = "headset_unknown_mic";
		break;
	case HTC_HEADSET_TV_OUT:
		state = "headset_tv_out";
		break;
	case HTC_HEADSET_UNSTABLE:
		state = "headset_unstable";
		break;
	case HTC_HEADSET_INDICATOR:
		state = "headset_indicator";
		break;
	case HTC_HEADSET_UART:
		state = "headset_uart";
		break;
	default:
		state = "error_state";
	}

	length = snprintf(buf, PAGE_SIZE, "%s\n", state);

	return length;
}

static ssize_t headset_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ACCDET_DEBUG("%s \n", __func__);
	return 0;
}

static DEVICE_HEADSET_ATTR(state, 0644, headset_state_show,
			   headset_state_store);

static ssize_t headset_simulate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ACCDET_DEBUG("%s \n", __func__);
	return snprintf(buf, PAGE_SIZE, "Command is not supported\n");
}

static ssize_t headset_simulate_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int type = NO_DEVICE;

	ACCDET_DEBUG("%s \n", __func__);
	if (!hi) {
		ACCDET_DEBUG("%s: hi is NULL\n", __func__);
		return -1;
	}

	switch_set_state((struct switch_dev *)&accdet_data, type);
	if (strncmp(buf, "headset_unplug", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_unplug\n");
		hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
		set_35mm_hw_state(0);
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
		if (htc_typec_usb_accessory_detection_get())
		{
			htc_aud_hsmic_2v85_en(0);
			htc_miclr_flip_en_set(MICLR_NON_FLIP);
			htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
			htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);
			htc_aud_miclr_dgnd_sw_en(MICLR_DGND_TO_GND);
			htc_as2_en(AS2_TO_MAIN_MIC);
		}
#endif
		return count;
	}
	set_35mm_hw_state(1);

	if (strncmp(buf, "headset_no_mic", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_no_mic\n");
		hi->hs_35mm_type = HTC_HEADSET_NO_MIC;
		type= HEADSET_NO_MIC;
	} else if (strncmp(buf, "headset_mic", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_mic\n");
		hi->hs_35mm_type = HTC_HEADSET_MIC;
		type= HEADSET_MIC;
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
		if (htc_typec_usb_accessory_detection_get())
		{
			htc_aud_hsmic_2v85_en(1);
			htc_miclr_flip_en_set(MICLR_NON_FLIP);
			htc_aud_hpmic_agnd_flip_en_s0_set(HSMIC_AGND_NON_FLIP);
			htc_aud_hpmic_agnd_flip_en_s1_set(HSMIC_AGND_NON_FLIP);
			htc_aud_miclr_dgnd_sw_en(MICLR_DGND_TO_MICLR);
			htc_as2_en(AS2_TO_MICLR_MIC);
		}
#endif
	} else if (strncmp(buf, "headset_metrico", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_metrico\n");
		
		hi->hs_35mm_type = HTC_HEADSET_METRICO;
		type= HEADSET_NO_MIC;
	} else if (strncmp(buf, "headset_unknown_mic", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_unknown_mic\n");
		hi->hs_35mm_type = HTC_HEADSET_UNKNOWN_MIC;
		type= HEADSET_NO_MIC;
	} else if (strncmp(buf, "headset_tv_out", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_tv_out\n");
		
		hi->hs_35mm_type = HTC_HEADSET_TV_OUT;
		type = HEADSET_NO_MIC;
#if defined(CONFIG_FB_MSM_TVOUT) && defined(CONFIG_ARCH_MSM8X60)
		
#endif
	} else if (strncmp(buf, "headset_indicator", count - 1) == 0) {
		ACCDET_DEBUG("Headset simulation: headset_indicator\n");
		
		hi->hs_35mm_type = HTC_HEADSET_INDICATOR;
	} else {
		ACCDET_DEBUG("Invalid parameter\n");
		hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
		return count;
	}
	switch_set_state((struct switch_dev *)&accdet_data, type);
	return count;
}

static DEVICE_HEADSET_ATTR(simulate, 0644, headset_simulate_show,
			   headset_simulate_store);

static ssize_t headset_mfg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int length = 0;
	char *state = NULL;

	ACCDET_DEBUG("[accdet] %s\n", __func__);
	if (!hi) {
		ACCDET_DEBUG("[accdet] %s: hi is NULL\n", __func__);
		state = "hi is NULL";
		length = snprintf(buf, PAGE_SIZE, "%s\n", state);
		return length;
	}

	if (hi->hs_mfg_mode) {
		state = "it is MFG rom";
	} else {
		state = "it is not MFG rom";
	}

	length = snprintf(buf, PAGE_SIZE, "%s\n", state);

	return length;
}

static ssize_t headset_mfg_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ACCDET_DEBUG("[accdet] %s: buf %s\n", __func__, buf);
	if (!hi) {
		ACCDET_DEBUG("[accdet] %s: hi is NULL\n", __func__);
		return -1;
	}

	if (strncmp(buf, "headset_mfg_mode", count - 1) == 0) {
		ACCDET_DEBUG("[accdet] %s: headset_mfg_mode = 1\n", __func__);
		hi->hs_mfg_mode = true;
	} else {
		hi->hs_mfg_mode = false;
		ACCDET_DEBUG("Invalid parameter\n");
	}
	return count;
}

static DEVICE_HEADSET_ATTR(mfg, 0644, headset_mfg_show,
			   headset_mfg_store);

#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
static ssize_t headset_typec_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int length = 0;
	ACCDET_DEBUG("%s\n", __func__);

	if (htc_typec_usb_accessory_detection_get())
	{
		length = snprintf(buf, PAGE_SIZE, "TypeC USB audio accessory states dump:\n");
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_aud_hsmic_2v85_en = %d\n", htc_aud_hsmic_2v85_en_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_aud_hpmic_agnd_flip_en_s0 = %d\n", htc_aud_hpmic_agnd_flip_en_s0_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_aud_hpmic_agnd_flip_en_s1 = %d\n", htc_aud_hpmic_agnd_flip_en_s1_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_miclr_flip_en = %d\n", htc_miclr_flip_en_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_aud_miclr_dgnd_sw_en = %d\n", htc_aud_miclr_dgnd_sw_en_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_as2_en = %d\n", htc_as2_en_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_usb_position_get() = %d\n", htc_usb_position_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_adaptor_mic_det_get() = %d\n", htc_adaptor_mic_det_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_typec_id1_get() = %d\n", htc_typec_id1_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_typec_id2_get() = %d\n", htc_typec_id2_get());
		length += snprintf(buf+length, PAGE_SIZE-length, "  htc_usb_headset_type = %s\n", get_htc_usb_headset_type_string());
	}
	else
	{
		length = snprintf(buf, PAGE_SIZE, "no typec usb accessory support!\n");
	}

	return length;
}

static ssize_t headset_typec_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_HEADSET_ATTR(typec, 0644, headset_typec_show, headset_typec_store);

static ssize_t headset_typec_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int length = 0;
	pr_info("%s\n", __func__);

	if (htc_typec_usb_accessory_detection_get())
	{
		int id1, id2;
		id1 = htc_mfg_id1_get();
		id2 = htc_mfg_id2_get();

		length = snprintf(buf, PAGE_SIZE, "AUD_TYPEC_ID1 = %d    AUD_TYPEC_ID2 = %d\n", id1, id2);
	}
	else
	{
		length = snprintf(buf, PAGE_SIZE, "no typec usb accessory support!\n");
	}

	return length;
}

static ssize_t headset_typec_id_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_HEADSET_ATTR(typec_id_test, 0644, headset_typec_id_show, headset_typec_id_store);

static ssize_t headset_typec_position_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int length = 0;
	pr_info("%s\n", __func__);

	if (htc_typec_usb_accessory_detection_get())
	{
		int usb_position, adaptor_mic_det;
		htc_aud_hsmic_2v85_en(1);
		EnableMicbias1(1);
		msleep(100);

		usb_position = htc_usb_position_get();
		adaptor_mic_det = htc_adaptor_mic_det_get();

		EnableMicbias1(0);
		htc_aud_hsmic_2v85_en(0);

		length = snprintf(buf, PAGE_SIZE, "AUD_USB_POSITION = %d    AUD_ADAPTOR_MIC_DET = %d\n", usb_position, adaptor_mic_det);
	}
	else
	{
		length = snprintf(buf, PAGE_SIZE, "no typec usb accessory support!\n");
	}

	return length;
}

static ssize_t headset_typec_position_store(struct device *dev,	struct device_attribute *attr, const char *buf, size_t count)
{
	return count;
}

static DEVICE_HEADSET_ATTR(typec_position_test, 0644, headset_typec_position_show, headset_typec_position_store);

static ssize_t headset_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "Unsupported_device\n");
}

static ssize_t store_unknown_accessory_test_state(struct device *dev,	struct device_attribute *attr, const char *buf, size_t count)
{
	int value;
	int ret;
	ret = sscanf(buf, "%d", &value);
	if (ret != 1) {
		ACCDET_DEBUG("accdet: Invalid values\n");
		return -EINVAL;
	}

	isEnableUnknownAccessoryTest = value;

	ACCDET_DEBUG("ACCDET: set isEnableUnknownAccessoryTest=%d\n", isEnableUnknownAccessoryTest);

	return count;
}

static ssize_t show_unknown_accessory_test_state(struct device *dev, struct device_attribute *attr, char *buf)
{
	ACCDET_DEBUG("ACCDET isEnableUnknownAccessoryTest = %d\n", isEnableUnknownAccessoryTest);

	return snprintf(buf, PAGE_SIZE, "%u\n", isEnableUnknownAccessoryTest);
}

static DEVICE_HEADSET_ATTR(unknown_acc_test, S_IWUSR | S_IRUGO, show_unknown_accessory_test_state, store_unknown_accessory_test_state);

#endif


static int register_attributes(void)
{
	int ret = 0;

	ACCDET_DEBUG("[accdet] %s ++\n", __func__);
	if (!hi) {
		ACCDET_DEBUG("[accdet] %s: hi is NULL\n", __func__);
		return -1;
	}

	hi->htc_accessory_class = class_create(THIS_MODULE, "htc_accessory");
	if (IS_ERR(hi->htc_accessory_class)) {
		ret = PTR_ERR(hi->htc_accessory_class);
		hi->htc_accessory_class = NULL;
		printk(KERN_ERR "[accdet] %s: err_create_class\n", __func__);
		goto err_create_class;
	}

	
	hi->headset_dev = device_create(hi->htc_accessory_class,
					NULL, 0, "%s", "headset");
	if (unlikely(IS_ERR(hi->headset_dev))) {
		ret = PTR_ERR(hi->headset_dev);
		hi->headset_dev = NULL;
		printk(KERN_ERR "[accdet] %s: err_create_headset_device\n", __func__);
		goto err_create_headset_device;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_state);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_state_device_file\n", __func__);
		goto err_create_headset_state_device_file;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_simulate);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_simulate_device_file\n", __func__);
		goto err_create_headset_simulate_device_file;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_mfg);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_simulate_device_mfg\n", __func__);
		goto err_create_headset_mfg_device_file;
	}

#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
	ret = device_create_file(hi->headset_dev, &dev_attr_headset_typec);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_typec_device_file\n", __func__);
		goto err_create_headset_typec_device_file;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_typec_id_test);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_typec_id_test_device_file\n", __func__);
		goto err_create_headset_typec_id_test_device_file;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_typec_position_test);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_typec_position_test_device_file\n", __func__);
		goto err_create_headset_typec_position_test_device_file;
	}

	ret = device_create_file(hi->headset_dev, &dev_attr_headset_unknown_acc_test);
	if (ret) {
		printk(KERN_ERR "[accdet] %s: err_create_headset_unknown_acc_test_device_file\n", __func__);
		goto err_create_headset_unknown_acc_test_device_file;
	}

	hi->unsupported_type.name = "Unsupported_device";
	hi->unsupported_type.print_name = headset_print_name;

	ret = switch_dev_register(&hi->unsupported_type);
	if (ret < 0) {
		pr_err("failed to register headset switch device!\n");
		return ret;
	}
#endif
#if 0
	
	hi->tty_dev = device_create(hi->htc_accessory_class,
				    NULL, 0, "%s", "tty");
	if (unlikely(IS_ERR(hi->tty_dev))) {
		ret = PTR_ERR(hi->tty_dev);
		hi->tty_dev = NULL;
		printk(KERN_ERR "%s: err_create_tty_device\n", __func__);
		goto err_create_tty_device;
	}

	ret = device_create_file(hi->tty_dev, &dev_attr_tty);
	if (ret) {
		printk(KERN_ERR "%s: err_create_tty_device_file\n", __func__);
		goto err_create_tty_device_file;
	}

	
	hi->fm_dev = device_create(hi->htc_accessory_class,
				   NULL, 0, "%s", "fm");
	if (unlikely(IS_ERR(hi->fm_dev))) {
		ret = PTR_ERR(hi->fm_dev);
		hi->fm_dev = NULL;
		printk(KERN_ERR "%s: err_create_fm_device\n", __func__);
		goto err_create_fm_device;
	}

	ret = device_create_file(hi->fm_dev, &dev_attr_fm);
	if (ret) {
		printk(KERN_ERR "%s: err_create_fm_device_file\n", __func__);
		goto err_create_fm_device_file;
	}

	
	hi->debug_dev = device_create(hi->htc_accessory_class,
				      NULL, 0, "%s", "debug");
	if (unlikely(IS_ERR(hi->debug_dev))) {
		ret = PTR_ERR(hi->debug_dev);
		hi->debug_dev = NULL;
		printk(KERN_ERR "%s: err_create_debug_device\n", __func__);
		goto err_create_debug_device;
	}

	
	ret = device_create_file(hi->debug_dev, &dev_attr_debug);
	if (ret) {
		printk(KERN_ERR "%s: err_create_debug_device_file\n", __func__);
		goto err_create_debug_device_file;
	}

#endif
	ACCDET_DEBUG("[accdet] %s --\n", __func__);
	return 0;
#if 0
err_create_debug_device_file:
	device_unregister(hi->debug_dev);

err_create_debug_device:
	device_remove_file(hi->fm_dev, &dev_attr_fm);

err_create_fm_device_file:
	device_unregister(hi->fm_dev);

err_create_fm_device:
	device_remove_file(hi->tty_dev, &dev_attr_tty);

err_create_tty_device_file:
	device_unregister(hi->tty_dev);
#endif
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT

err_create_headset_unknown_acc_test_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_unknown_acc_test);

err_create_headset_typec_position_test_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_typec_id_test);

err_create_headset_typec_id_test_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_typec);

err_create_headset_typec_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_mfg);
#endif

err_create_headset_mfg_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_simulate);

err_create_headset_simulate_device_file:
	device_remove_file(hi->headset_dev, &dev_attr_headset_state);

err_create_headset_state_device_file:
	device_unregister(hi->headset_dev);

err_create_headset_device:
	class_destroy(hi->htc_accessory_class);

err_create_class:

	printk(KERN_ERR "[accdet] %s: create error %d \n", __func__, ret);
	return ret;
}

static void unregister_attributes(void)
{
	ACCDET_DEBUG("[accdet] %s ++\n", __func__);
	if (hi) {
#if 0
		device_remove_file(hi->debug_dev, &dev_attr_debug);
		device_unregister(hi->debug_dev);
		device_remove_file(hi->fm_dev, &dev_attr_fm);
		device_unregister(hi->fm_dev);
		device_remove_file(hi->tty_dev, &dev_attr_tty);
		device_unregister(hi->tty_dev);
#endif
#ifdef CONFIG_HTC_TYPEC_HEADSET_DETECT
		switch_dev_unregister(&hi->unsupported_type);
		device_remove_file(hi->headset_dev, &dev_attr_headset_typec_position_test);
		device_remove_file(hi->headset_dev, &dev_attr_headset_typec_id_test);
		device_remove_file(hi->headset_dev, &dev_attr_headset_typec);
		device_remove_file(hi->headset_dev, &dev_attr_headset_unknown_acc_test);
#endif
		device_remove_file(hi->headset_dev, &dev_attr_headset_mfg);
		device_remove_file(hi->headset_dev, &dev_attr_headset_simulate);
		device_remove_file(hi->headset_dev, &dev_attr_headset_state);
		device_unregister(hi->headset_dev);
		class_destroy(hi->htc_accessory_class);
	} else {
		ACCDET_DEBUG("[accdet] %s: hi is NULL\n", __func__);
	}
	ACCDET_DEBUG("[accdet] %s --\n", __func__);
}

void accdet_int_handler(void)
{
	int ret = 0;

	ACCDET_DEBUG("[accdet_int_handler]....\n");
	ret = accdet_irq_handler();
	if (0 == ret)
		ACCDET_DEBUG("[accdet_int_handler] don't finished\n");
}

void accdet_eint_int_handler(void)
{
	int ret = 0;

	ACCDET_DEBUG("[accdet_eint_int_handler]....\n");
	ret = accdet_irq_handler();
	if (0 == ret)
		ACCDET_DEBUG("[accdet_int_handler] don't finished\n");
}

int mt_accdet_probe(struct platform_device *dev)
{
	int ret = 0;

#if DEBUG_THREAD
	struct platform_driver accdet_driver_hal = accdet_driver_func();
#endif

	ACCDET_INFO("[Accdet]accdet_probe begin!\n");

	accdet_data.name = "h2w";
	accdet_data.index = 0;
	accdet_data.state = NO_DEVICE;
	ret = switch_dev_register(&accdet_data);
	if (ret) {
		ACCDET_ERROR("[Accdet]switch_dev_register returned:%d!\n", ret);
		return 1;
	}
	ret = alloc_chrdev_region(&accdet_devno, 0, 1, ACCDET_DEVNAME);
	if (ret)
		ACCDET_ERROR("[Accdet]alloc_chrdev_region: Get Major number error!\n");

	accdet_cdev = cdev_alloc();
	accdet_cdev->owner = THIS_MODULE;
	accdet_cdev->ops = accdet_get_fops();
	ret = cdev_add(accdet_cdev, accdet_devno, 1);
	if (ret)
		ACCDET_ERROR("[Accdet]accdet error: cdev_add\n");

	accdet_class = class_create(THIS_MODULE, ACCDET_DEVNAME);

	
	accdet_nor_device = device_create(accdet_class, NULL, accdet_devno, NULL, ACCDET_DEVNAME);

	kpd_accdet_dev = input_allocate_device();
	if (!kpd_accdet_dev) {
		ACCDET_ERROR("[Accdet]kpd_accdet_dev : fail!\n");
		return -ENOMEM;
	}
	
	init_timer(&micbias_timer);
	micbias_timer.expires = jiffies + MICBIAS_DISABLE_TIMER;
	micbias_timer.function = &disable_micbias;
	micbias_timer.data = ((unsigned long)0);

	
	__set_bit(EV_KEY, kpd_accdet_dev->evbit);
	__set_bit(KEY_PLAYPAUSE, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOLUMEDOWN, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOLUMEUP, kpd_accdet_dev->keybit);
	__set_bit(KEY_VOICECOMMAND, kpd_accdet_dev->keybit);

	kpd_accdet_dev->id.bustype = BUS_HOST;
	kpd_accdet_dev->name = "ACCDET";
	if (input_register_device(kpd_accdet_dev))
		ACCDET_ERROR("[Accdet]kpd_accdet_dev register : fail!\n");
	accdet_workqueue = create_singlethread_workqueue("accdet");
	INIT_WORK(&accdet_work, accdet_work_callback);

	wake_lock_init(&accdet_suspend_lock, WAKE_LOCK_SUSPEND, "accdet wakelock");
	wake_lock_init(&accdet_irq_lock, WAKE_LOCK_SUSPEND, "accdet irq wakelock");
	wake_lock_init(&accdet_key_lock, WAKE_LOCK_SUSPEND, "accdet key wakelock");
	wake_lock_init(&accdet_timer_lock, WAKE_LOCK_SUSPEND, "accdet timer wakelock");
#if DEBUG_THREAD
	ret = accdet_create_attr(&accdet_driver_hal.driver);
	if (ret != 0)
		ACCDET_ERROR("create attribute err = %d\n", ret);
#endif
	pmic_register_interrupt_callback(12, accdet_int_handler);
	pmic_register_interrupt_callback(13, accdet_eint_int_handler);
	ACCDET_INFO("[Accdet]accdet_probe : ACCDET_INIT\n");
	if (g_accdet_first == 1) {
		eint_accdet_sync_flag = 1;
#ifdef CONFIG_ACCDET_EINT_IRQ
		accdet_eint_workqueue = create_singlethread_workqueue("accdet_eint");
		INIT_WORK(&accdet_eint_work, accdet_eint_work_callback);
		accdet_disable_workqueue = create_singlethread_workqueue("accdet_disable");
		INIT_WORK(&accdet_disable_work, disable_micbias_callback);
#endif
		
		accdet_get_dts_data();
		accdet_init();
		accdet_pmic_Read_Efuse_HPOffset();
		
		queue_work(accdet_workqueue, &accdet_work);
#ifdef CONFIG_ACCDET_EINT
		accdet_disable_workqueue = create_singlethread_workqueue("accdet_disable");
		INIT_WORK(&accdet_disable_work, disable_micbias_callback);
		accdet_eint_workqueue = create_singlethread_workqueue("accdet_eint");
		INIT_WORK(&accdet_eint_work, accdet_eint_work_callback);
		accdet_setup_eint(dev);
#endif
		g_accdet_first = 0;
	}

	if (!hi) {
		hi = kzalloc(sizeof(struct htc_headset_info), GFP_KERNEL);
		if (!hi) {
			printk(KERN_ERR "[Accdet] %s: alloc hi failed\n", __func__);
		} else {
			
			hi->hs_35mm_type = HTC_HEADSET_UNPLUG;
			hi->hs_mfg_mode = false;
			mutex_init(&hi->mutex_lock);
		}
	}
	ret = register_attributes();
	if (ret)
		printk(KERN_ERR "[Accdet] %s: register_attributes error\n", __func__);

	ACCDET_INFO("[Accdet]accdet_probe done!\n");
	return 0;
}

void mt_accdet_remove(void)
{
	ACCDET_DEBUG("[Accdet]accdet_remove begin!\n");

	
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
	destroy_workqueue(accdet_eint_workqueue);
#endif
	destroy_workqueue(accdet_workqueue);
	switch_dev_unregister(&accdet_data);
	device_del(accdet_nor_device);
	class_destroy(accdet_class);
	cdev_del(accdet_cdev);
	unregister_chrdev_region(accdet_devno, 1);
	input_unregister_device(kpd_accdet_dev);
	unregister_attributes();
	if (hi) {
		kfree(hi);
		hi = NULL;
	}
	ACCDET_DEBUG("[Accdet]accdet_remove Done!\n");
}

void mt_accdet_suspend(void)	
{

#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
	ACCDET_DEBUG("[Accdet] in suspend1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else
	ACCDET_DEBUG("[Accdet]accdet_suspend: ACCDET_CTRL=[0x%x], STATE=[0x%x]->[0x%x]\n",
	       pmic_pwrap_read(ACCDET_CTRL), pre_state_swctrl, pmic_pwrap_read(ACCDET_STATE_SWCTRL));
#endif
}

void mt_accdet_resume(void)	
{
#if defined CONFIG_ACCDET_EINT || defined CONFIG_ACCDET_EINT_IRQ
	ACCDET_DEBUG("[Accdet] in resume1: ACCDET_IRQ_STS = 0x%x\n", pmic_pwrap_read(ACCDET_IRQ_STS));
#else
	ACCDET_DEBUG("[Accdet]accdet_resume: ACCDET_CTRL=[0x%x], STATE_SWCTRL=[0x%x]\n",
	       pmic_pwrap_read(ACCDET_CTRL), pmic_pwrap_read(ACCDET_STATE_SWCTRL));

#endif

}

#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
struct timer_list accdet_disable_ipoh_timer;
static void mt_accdet_pm_disable(unsigned long a)
{
	if (cable_type == NO_DEVICE && eint_accdet_sync_flag == 0) {
		
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
#ifdef CONFIG_ACCDET_EINT
		pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
		
		pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#endif
#ifdef CONFIG_ACCDET_EINT_IRQ
		pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) & (~(ACCDET_ENABLE)));
#endif
		ACCDET_DEBUG("[Accdet]daccdet_pm_disable: disable!\n");
	} else {
		ACCDET_DEBUG("[Accdet]daccdet_pm_disable: enable!\n");
	}
}
#endif
void mt_accdet_pm_restore_noirq(void)
{
	int current_status_restore = 0;

	ACCDET_DEBUG("[Accdet]accdet_pm_restore_noirq start!\n");
	
	ACCDET_DEBUG("accdet: enable_accdet\n");
	
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_CLK_CLR);
#ifdef CONFIG_ACCDET_EINT_IRQ
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_EINT_IRQ_CLR);
	pmic_pwrap_write(ACCDET_RSV, pmic_pwrap_read(ACCDET_RSV) | ACCDET_INPUT_MICP);
	pmic_pwrap_write(ACCDET_ADC_REG, pmic_pwrap_read(ACCDET_ADC_REG) | ACCDET_EINT_CON_EN);
	pmic_pwrap_write(ACCDET_CTRL, ACCDET_EINT_EN);
#endif
#ifdef ACCDET_NEGV_IRQ
	pmic_pwrap_write(TOP_CKPDN_CLR, RG_ACCDET_NEGV_IRQ_CLR);
	pmic_pwrap_write(ACCDET_EINT_NV, pmic_pwrap_read(ACCDET_EINT_NV) | ACCDET_NEGV_DT_EN);
	pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) | ACCDET_NEGV_EN);
#endif
	enable_accdet(ACCDET_SWCTRL_EN);
	pmic_pwrap_write(ACCDET_STATE_SWCTRL, (pmic_pwrap_read(ACCDET_STATE_SWCTRL) | ACCDET_SWCTRL_IDLE_EN));

	eint_accdet_sync_flag = 1;
	current_status_restore = ((pmic_pwrap_read(ACCDET_STATE_RG) & 0xc0) >> 6);	

	switch (current_status_restore) {
	case 0:		
		cable_type = HEADSET_NO_MIC;
		accdet_status = HOOK_SWITCH;
		break;
	case 1:		
		cable_type = HEADSET_MIC;
		accdet_status = MIC_BIAS;
		break;
	case 3:		
		cable_type = NO_DEVICE;
		accdet_status = PLUG_OUT;
		break;
	default:
		ACCDET_DEBUG("[Accdet]accdet_pm_restore_noirq: accdet current status error!\n");
		break;
	}
	switch_set_state((struct switch_dev *)&accdet_data, cable_type);
	if (cable_type == NO_DEVICE) {
#ifdef CONFIG_ACCDET_PIN_RECOGNIZATION
		init_timer(&accdet_disable_ipoh_timer);
		accdet_disable_ipoh_timer.expires = jiffies + 3 * HZ;
		accdet_disable_ipoh_timer.function = &mt_accdet_pm_disable;
		accdet_disable_ipoh_timer.data = ((unsigned long)0);
		add_timer(&accdet_disable_ipoh_timer);
		ACCDET_DEBUG("[Accdet]enable! pm timer\n");

#else
		
		pre_state_swctrl = pmic_pwrap_read(ACCDET_STATE_SWCTRL);
#ifdef CONFIG_ACCDET_EINT
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, 0);
		pmic_pwrap_write(ACCDET_CTRL, ACCDET_DISABLE);
		
		pmic_pwrap_write(TOP_CKPDN_SET, RG_ACCDET_CLK_SET);
#endif
#ifdef CONFIG_ACCDET_EINT_IRQ
		pmic_pwrap_write(ACCDET_STATE_SWCTRL, ACCDET_EINT_PWM_EN);
		pmic_pwrap_write(ACCDET_CTRL, pmic_pwrap_read(ACCDET_CTRL) & (~(ACCDET_ENABLE)));
#endif
#endif
	}
}

long mt_accdet_unlocked_ioctl(unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case ACCDET_INIT:
		break;
	case SET_CALL_STATE:
		call_status = (int)arg;
		ACCDET_DEBUG("[Accdet]accdet_ioctl : CALL_STATE=%d\n", call_status);
		break;
	case GET_BUTTON_STATUS:
		ACCDET_DEBUG("[Accdet]accdet_ioctl : Button_Status=%d (state:%d)\n", button_status, accdet_data.state);
		return button_status;
	default:
		ACCDET_DEBUG("[Accdet]accdet_ioctl : default\n");
		break;
	}
	return 0;
}