/*
 * Copyright (C) 2010 MediaTek, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#if defined(CONFIG_MTK_RTC)

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pm_wakeup.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <asm/div64.h>
#include <linux/htc_devices_dtb.h>


#include <mtk_rtc.h>
#include <mtk_rtc_hal_common.h>
#include <mach/mtk_rtc_hal.h>
#include <upmu_common.h>
#include <mt_pmic_wrap.h>
#if defined CONFIG_MTK_KERNEL_POWER_OFF_CHARGING
#include <mt_boot.h>
#include <mt-plat/mt_boot_common.h>
#endif
#include <mt_reboot.h>
#include <mt-plat/charging.h>

#define RTC_NAME	"mt-rtc"
#define RTC_RELPWR_WHEN_XRST	1	


#define RTC_MIN_YEAR		1968
#define RTC_NUM_YEARS		128
#define RTC_OVER_TIME_RESET	1
#define RTC_DEFAULT_YEA		2010
#define RTC_DEFAULT_MTH		1
#define RTC_DEFAULT_DOM		1


#define RTC_MIN_YEAR_OFFSET	(RTC_MIN_YEAR - 1900)
#define AUTOBOOT_ON 0
#define AUTOBOOT_OFF 1










#define rtc_xinfo(fmt, args...)		\
	pr_notice(fmt, ##args)

#define rtc_xerror(fmt, args...)	\
	pr_err(fmt, ##args)

#define rtc_xfatal(fmt, args...)	\
	pr_emerg(fmt, ##args)

static struct rtc_device *rtc;
static DEFINE_SPINLOCK(rtc_lock);

static int rtc_show_time;
static int rtc_show_alarm = 1;

#if 1
unsigned long rtc_read_hw_time(void)
{
	unsigned long time, flags;
	struct rtc_time tm;

	spin_lock_irqsave(&rtc_lock, flags);
	
	
	hal_rtc_get_tick_time(&tm);
	spin_unlock_irqrestore(&rtc_lock, flags);
	tm.tm_year += RTC_MIN_YEAR_OFFSET;
	tm.tm_mon--;
	rtc_tm_to_time(&tm, &time);
	tm.tm_wday = (time / 86400 + 4) % 7;	

	return time;
}
EXPORT_SYMBOL(rtc_read_hw_time);

#endif
int get_rtc_spare_fg_value(void)
{
	
	u16 temp;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	temp = hal_rtc_get_spare_register(RTC_FGSOC);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return temp;
}

int set_rtc_spare_fg_value(int val)
{
	
	unsigned long flags;

	if (val > 100)
		return 1;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_FGSOC, val);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

int get_rtc_spare_coulomb_value(void)
{
	
	u16 temp;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	temp = hal_rtc_get_spare_register(RTC_FGCOUL);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return temp;
}

int set_rtc_spare_coulomb_value(int val)
{
	
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_FGCOUL, val);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;

}

bool crystal_exist_status(void)
{
	unsigned long flags;
	u16 ret;

	spin_lock_irqsave(&rtc_lock, flags);
	ret = hal_rtc_get_spare_register(RTC_32K_LESS);
	spin_unlock_irqrestore(&rtc_lock, flags);

	if (ret)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(crystal_exist_status);

bool rtc_low_power_detected(void)
{
	unsigned long flags;
	u16 ret;

	spin_lock_irqsave(&rtc_lock, flags);
	ret = hal_rtc_get_spare_register(RTC_LP_DET);
	spin_unlock_irqrestore(&rtc_lock, flags);

	if (ret)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(rtc_low_power_detected);

void rtc_gpio_enable_32k(rtc_gpio_user_t user)
{
	unsigned long flags;

	rtc_xinfo("rtc_gpio_enable_32k, user = %d\n", user);

	if (user < RTC_GPIO_USER_WIFI || user > RTC_GPIO_USER_PMIC)
		return;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_gpio_32k_status(user, true);
	spin_unlock_irqrestore(&rtc_lock, flags);
}
EXPORT_SYMBOL(rtc_gpio_enable_32k);

void rtc_gpio_disable_32k(rtc_gpio_user_t user)
{
	unsigned long flags;

	rtc_xinfo("rtc_gpio_disable_32k, user = %d\n", user);

	if (user < RTC_GPIO_USER_WIFI || user > RTC_GPIO_USER_PMIC)
		return;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_gpio_32k_status(user, false);
	spin_unlock_irqrestore(&rtc_lock, flags);
}
EXPORT_SYMBOL(rtc_gpio_disable_32k);

bool rtc_gpio_32k_status(void)
{
	unsigned long flags;
	u16 ret;

	spin_lock_irqsave(&rtc_lock, flags);
	ret = hal_rtc_get_gpio_32k_status();
	spin_unlock_irqrestore(&rtc_lock, flags);

	if (ret)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(rtc_gpio_32k_status);

void rtc_enable_abb_32k(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_abb_32k(1);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

void rtc_disable_abb_32k(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_abb_32k(0);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

void rtc_enable_writeif(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	rtc_set_writeif(true);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

void rtc_disable_writeif(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	rtc_set_writeif(false);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

void rtc_mark_recovery(void)
{
	unsigned long flags;

	rtc_xinfo("rtc_mark_recovery\n");
	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_FAC_RESET, 0x1);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
void rtc_mark_kpoc(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_KPOC, 0x1);
	spin_unlock_irqrestore(&rtc_lock, flags);
}
#endif
void rtc_mark_fast(void)
{
	unsigned long flags;

	rtc_xinfo("rtc_mark_fast\n");
	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_FAST_BOOT, 0x1);
	spin_unlock_irqrestore(&rtc_lock, flags);
}

u16 rtc_rdwr_uart_bits(u16 *val)
{
	u16 ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_spare_register(RTC_UART, *val);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return ret;
}

void rtc_bbpu_power_down(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_bbpu_pwdn();
	spin_unlock_irqrestore(&rtc_lock, flags);
}

void mt_power_off(void)
{
#if !defined(CONFIG_POWER_EXT)
	int count = 0;
#endif
	int kernel_flag;
	rtc_xinfo("mt_power_off\n");
	kernel_flag = get_kernel_flag();

	if(kernel_flag & KERNEL_FLAG_SW_SHUTDOWN_DEBUG)
		{
			printk("[mt_power_off]force panic for debug SW shutdown issue\n");
			BUG_ON(true);
		}

	
	rtc_bbpu_power_down();

	while (1) {
#if defined(CONFIG_POWER_EXT)
		
		rtc_xinfo("EVB without charger\n");
#else
		
		mdelay(100);
		rtc_xinfo("Phone with charger\n");
		if (pmic_chrdet_status() == KAL_TRUE || count > 10)
			arch_reset(0, "charger");
		count++;
#endif
	}
}

void rtc_read_pwron_alarm(struct rtc_wkalrm *alm)
{
	unsigned long flags;
	struct rtc_time *tm;

	if (alm == NULL)
		return;
	tm = &alm->time;
	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_get_pwron_alarm(tm, alm);
	spin_unlock_irqrestore(&rtc_lock, flags);
	tm->tm_year += RTC_MIN_YEAR_OFFSET;
	tm->tm_mon -= 1;
	if (rtc_show_alarm) {
		rtc_xinfo("power-on = %04d/%02d/%02d %02d:%02d:%02d (%d)(%d)\n",
			  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			  tm->tm_hour, tm->tm_min, tm->tm_sec, alm->enabled, alm->pending);
	}
}

static void rtc_handler(void)
{
	bool pwron_alm = false, isLowPowerIrq = false, pwron_alarm = false;
	struct rtc_time nowtm;
	struct rtc_time tm;

	rtc_xinfo("rtc_tasklet_handler start\n");

	spin_lock(&rtc_lock);
	isLowPowerIrq = hal_rtc_is_lp_irq();
	if (isLowPowerIrq) {
		spin_unlock(&rtc_lock);
		return;
	}
#if RTC_RELPWR_WHEN_XRST
	
	hal_rtc_reload_power();
#endif
	pwron_alarm = hal_rtc_is_pwron_alarm(&nowtm, &tm);
	nowtm.tm_year += RTC_MIN_YEAR;
	tm.tm_year += RTC_MIN_YEAR;
	if (pwron_alarm) {
		unsigned long now_time, time;

		now_time =
		    mktime(nowtm.tm_year, nowtm.tm_mon, nowtm.tm_mday, nowtm.tm_hour, nowtm.tm_min,
			   nowtm.tm_sec);
		time = mktime(tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		if (now_time >= time - 1 && now_time <= time + 4) {	
#if defined(CONFIG_MTK_KERNEL_POWER_OFF_CHARGING)
			if (get_boot_mode() == KERNEL_POWER_OFF_CHARGING_BOOT
			    || get_boot_mode() == LOW_POWER_OFF_CHARGING_BOOT) {
				time += 1;
				rtc_time_to_tm(time, &tm);
				tm.tm_year -= RTC_MIN_YEAR_OFFSET;
				tm.tm_mon += 1;
				
				hal_rtc_set_alarm(&tm);
				spin_unlock(&rtc_lock);
				arch_reset(0, "kpoc");
			} else {
				hal_rtc_save_pwron_alarm();
				pwron_alm = true;
			}
#else
			hal_rtc_save_pwron_alarm();
			pwron_alm = true;
#endif
		} else if (now_time < time) {	
			if (tm.tm_sec == 0) {
				tm.tm_sec = 59;
				tm.tm_min -= 1;
			} else {
				tm.tm_sec -= 1;
			}
			hal_rtc_set_alarm(&tm);
		}
	}
	spin_unlock(&rtc_lock);

	if (rtc != NULL)
		rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF);

	if (rtc_show_alarm)
		rtc_xinfo("%s time is up\n", pwron_alm ? "power-on" : "alarm");

}


void rtc_irq_handler(void)
{
	rtc_handler();
}

#if RTC_OVER_TIME_RESET
static void rtc_reset_to_deftime(struct rtc_time *tm)
{
	unsigned long flags;
	struct rtc_time defaulttm;

	tm->tm_year = RTC_DEFAULT_YEA - 1900;
	tm->tm_mon = RTC_DEFAULT_MTH - 1;
	tm->tm_mday = RTC_DEFAULT_DOM;
	tm->tm_wday = 1;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;

	
	defaulttm.tm_year = RTC_DEFAULT_YEA - RTC_MIN_YEAR;
	defaulttm.tm_mon = RTC_DEFAULT_MTH;
	defaulttm.tm_mday = RTC_DEFAULT_DOM;
	defaulttm.tm_wday = 1;
	defaulttm.tm_hour = 0;
	defaulttm.tm_min = 0;
	defaulttm.tm_sec = 0;
	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_alarm(&defaulttm);
	spin_unlock_irqrestore(&rtc_lock, flags);

	rtc_xerror("reset to default date %04d/%02d/%02d\n",
		   RTC_DEFAULT_YEA, RTC_DEFAULT_MTH, RTC_DEFAULT_DOM);
}
#endif

static int rtc_ops_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time, flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_get_tick_time(tm);
	spin_unlock_irqrestore(&rtc_lock, flags);

	tm->tm_year += RTC_MIN_YEAR_OFFSET;
	tm->tm_mon--;
	rtc_tm_to_time(tm, &time);
#if RTC_OVER_TIME_RESET
	if (unlikely(time > (unsigned long)LONG_MAX)) {
		rtc_reset_to_deftime(tm);
		rtc_tm_to_time(tm, &time);
	}
#endif
	tm->tm_wday = (time / 86400 + 4) % 7;	

	if (rtc_show_time) {
		rtc_xinfo("read tc time = %04d/%02d/%02d (%d) %02d:%02d:%02d\n",
			  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			  tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}

	return 0;
}

static int rtc_ops_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time, flags;

	rtc_tm_to_time(tm, &time);
	if (time > (unsigned long)LONG_MAX)
		return -EINVAL;

	tm->tm_year -= RTC_MIN_YEAR_OFFSET;
	tm->tm_mon++;

	rtc_xinfo("set tc time = %04d/%02d/%02d %02d:%02d:%02d\n",
		  tm->tm_year + RTC_MIN_YEAR, tm->tm_mon, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec);

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_tick_time(tm);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

static int rtc_ops_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned long flags;
	struct rtc_time *tm = &alm->time;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_get_alarm(tm, alm);
	spin_unlock_irqrestore(&rtc_lock, flags);

	tm->tm_year += RTC_MIN_YEAR_OFFSET;
	tm->tm_mon--;

	rtc_xinfo("read al time = %04d/%02d/%02d %02d:%02d:%02d (%d)\n",
		  tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec, alm->enabled);

	return 0;
}

static void rtc_save_pwron_time(bool enable, struct rtc_time *tm, bool logo)
{
	hal_rtc_save_pwron_time(enable, tm, logo);
}

static int rtc_ops_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned long time, flags;
	struct rtc_time *tm = &alm->time;

	rtc_tm_to_time(tm, &time);
	if (time > (unsigned long)LONG_MAX)
		return -EINVAL;

	tm->tm_year -= RTC_MIN_YEAR_OFFSET;
	tm->tm_mon++;

	rtc_xinfo("set al time = %04d/%02d/%02d %02d:%02d:%02d (%d)\n",
		  tm->tm_year + RTC_MIN_YEAR, tm->tm_mon, tm->tm_mday,
		  tm->tm_hour, tm->tm_min, tm->tm_sec, alm->enabled);

	spin_lock_irqsave(&rtc_lock, flags);
	if (alm->enabled == 2) {	
		rtc_save_pwron_time(true, tm, false);
	} else if (alm->enabled == 3) {	
		rtc_save_pwron_time(true, tm, true);
	} else if (alm->enabled == 4) {	
		
		rtc_save_pwron_time(false, tm, false);
	}

	
	hal_rtc_clear_alarm(tm);

	if (alm->enabled)
		hal_rtc_set_alarm(tm);
	spin_unlock_irqrestore(&rtc_lock, flags);

	return 0;
}

void rtc_pwm_enable_check(void)
{
#ifdef VRTC_PWM_ENABLE
	U64 time;

	rtc_xinfo("rtc_pwm_enable_check()\n");

	time = sched_clock();
	do_div(time, 1000000000);


	if (time > RTC_PWM_ENABLE_POLLING_TIMER) {
		hal_rtc_pwm_enable();
	} else {
		rtc_xinfo("time=%lld, less than %d, don't enable rtc pwm\n", time,
			  RTC_PWM_ENABLE_POLLING_TIMER);
	}

#endif
}


static int rtc_ops_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	
	rtc_xinfo("rtc_ops_ioctl cmd=%d\n", cmd);
	switch (cmd) {
	case RTC_AUTOBOOT_ON:
		{
			hal_rtc_set_spare_register(RTC_AUTOBOOT, AUTOBOOT_ON);
			rtc_xinfo("rtc_ops_ioctl cmd=RTC_AUTOBOOT_ON\n");
			return 0;
		}
	case RTC_AUTOBOOT_OFF:	
		{
			hal_rtc_set_spare_register(RTC_AUTOBOOT, AUTOBOOT_OFF);
			rtc_xinfo("rtc_ops_ioctl cmd=RTC_AUTOBOOT_OFF\n");
			return 0;
		}
	default:
		break;
	}
	return -ENOIOCTLCMD;
}

static struct rtc_class_ops rtc_ops = {
	.read_time = rtc_ops_read_time,
	.set_time = rtc_ops_set_time,
	.read_alarm = rtc_ops_read_alarm,
	.set_alarm = rtc_ops_set_alarm,
	.ioctl = rtc_ops_ioctl,
};

static int rtc_pdrv_probe(struct platform_device *pdev)
{
	unsigned long flags;

	
	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_set_lp_irq();
	spin_unlock_irqrestore(&rtc_lock, flags);

	device_init_wakeup(&pdev->dev, 1);
	
	rtc = rtc_device_register(RTC_NAME, &pdev->dev, &rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		rtc_xerror("register rtc device failed (%ld)\n", PTR_ERR(rtc));
		return PTR_ERR(rtc);
	}
#ifdef PMIC_REGISTER_INTERRUPT_ENABLE
	pmic_register_interrupt_callback(RTC_INTERRUPT_NUM, rtc_irq_handler);
	pmic_enable_interrupt(RTC_INTERRUPT_NUM, 1, "RTC");
#endif

	return 0;
}

static int rtc_pdrv_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver rtc_pdrv = {
	.probe = rtc_pdrv_probe,
	.remove = rtc_pdrv_remove,
	.driver = {
		   .name = RTC_NAME,
		   .owner = THIS_MODULE,
		   },
};

static struct platform_device rtc_pdev = {
	.name = RTC_NAME,
	.id = -1,
};

static int __init rtc_device_init(void)
{
	int r;

	rtc_xinfo("rtc_init");

	r = platform_device_register(&rtc_pdev);
	if (r) {
		rtc_xerror("register device failed (%d)\n", r);
		return r;
	}

	r = platform_driver_register(&rtc_pdrv);
	if (r) {
		rtc_xerror("register driver failed (%d)\n", r);
		platform_device_unregister(&rtc_pdev);
		return r;
	}
#if (defined(MTK_GPS_MT3332))
	hal_rtc_set_gpio_32k_status(0, true);
#endif


	return 0;
}




static int __init rtc_late_init(void)
{
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);
	hal_rtc_read_rg();
	spin_unlock_irqrestore(&rtc_lock, flags);

	if (crystal_exist_status() == true)
		rtc_xinfo("There is Crystal\n");
	else
		rtc_xinfo("There is no Crystal\n");

	rtc_writeif_unlock();
#if (defined(MTK_GPS_MT3332))
	hal_rtc_set_gpio_32k_status(0, true);
#endif

	return 0;
}


late_initcall(rtc_late_init);
device_initcall(rtc_device_init);

module_param(rtc_show_time, int, 0644);
module_param(rtc_show_alarm, int, 0644);

MODULE_LICENSE("GPL");

#endif 
