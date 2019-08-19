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

#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

#define STOP_CHARGING_IN_TAKLING
#define TALKING_RECHARGE_VOLTAGE 3800
#define TALKING_SYNC_TIME		   60

#define MTK_TEMPERATURE_RECHARGE_SUPPORT
#define MAX_CHARGE_TEMPERATURE  55
#define MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE	52
#define MIN_CHARGE_TEMPERATURE  0
#define MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE	2
#define ERR_CHARGE_TEMPERATURE  0xFF

#define V_PRE2CC_THRES			3400	
#define V_CC2TOPOFF_THRES		4352
#define RECHARGING_VOLTAGE      4110
#define CHARGING_FULL_CURRENT    150	


#define BATTERY_AVERAGE_DATA_NUMBER	3
#define BATTERY_AVERAGE_SIZE 5

#define V_CHARGER_ENABLE 0				

#define ONEHUNDRED_PERCENT_TRACKING_TIME	10	
#define NPERCENT_TRACKING_TIME 20	
#define SYNC_TO_REAL_TRACKING_TIME 60	
#define V_0PERCENT_TRACKING							3450 


#define BATTERY_NOTIFY_CASE_0001_VCHARGER
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP

#define HIGH_BATTERY_VOLTAGE_SUPPORT

#define CUST_SOC_JEITA_SYNC_TIME 30
#define JEITA_RECHARGE_VOLTAGE  4110	
#ifdef HIGH_BATTERY_VOLTAGE_SUPPORT
#define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE		BATTERY_VOLT_04_400000_V
#define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#else
#define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE		BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE	BATTERY_VOLT_04_200000_V
#define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE	BATTERY_VOLT_03_900000_V
#define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE		BATTERY_VOLT_03_900000_V
#endif
#define JEITA_NEG_10_TO_POS_0_FULL_CURRENT  120
#define JEITA_TEMP_POS_45_TO_POS_60_RECHARGE_VOLTAGE  4000
#define JEITA_TEMP_POS_10_TO_POS_45_RECHARGE_VOLTAGE  4100
#define JEITA_TEMP_POS_0_TO_POS_10_RECHARGE_VOLTAGE   4000
#define JEITA_TEMP_NEG_10_TO_POS_0_RECHARGE_VOLTAGE   3800
#define JEITA_TEMP_POS_45_TO_POS_60_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_10_TO_POS_45_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_0_TO_POS_10_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_NEG_10_TO_POS_0_CC2TOPOFF_THRESHOLD	3850


#define CV_E1_INTERNAL

#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define CONFIG_DIS_CHECK_BATTERY
#endif

#ifdef CONFIG_MTK_FAN5405_SUPPORT
#define FAN5405_BUSNUM 1
#endif

#define SWITCH_CHR_VINDPM_5V 0x13  
#define SWITCH_CHR_VINDPM_7V 0x25  
#define SWITCH_CHR_VINDPM_9V 0x37  
#define SWITCH_CHR_VINDPM_12V 0x54 

#define CONFIG_MTK_THERMAL_TEST_SUPPORT
#define CONFIG_MTK_CHRIND_CLK_PDN_SUPPORT

#define CONFIG_MTK_I2C_CHR_SUPPORT

#define BATTERY_MODULE_INIT

#if defined(CONFIG_MTK_BQ24196_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24296_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24160_SUPPORT)\
	|| defined(CONFIG_MTK_BQ25896_SUPPORT)\
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define SWCHR_POWER_PATH
#endif

#if defined(CONFIG_MTK_FAN5402_SUPPORT) \
	|| defined(CONFIG_MTK_FAN5405_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24158_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24196_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24296_SUPPORT) \
	|| defined(CONFIG_MTK_NCP1851_SUPPORT) \
	|| defined(CONFIG_MTK_NCP1854_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24160_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24157_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24250_SUPPORT) \
	|| defined(CONFIG_MTK_BQ24261_SUPPORT)
#define EXTERNAL_SWCHR_SUPPORT
#endif

#endif 
