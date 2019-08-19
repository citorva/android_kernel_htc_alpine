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

#ifndef _CUST_BATTERY_METER_TABLE_H
#define _CUST_BATTERY_METER_TABLE_H


#define BAT_NTC_10 0
#define BAT_NTC_47 0
#define BAT_NTC_100 1

#if (BAT_NTC_10 == 1)
#define RBAT_PULL_UP_R             24000
#endif

#if (BAT_NTC_47 == 1)
#define RBAT_PULL_UP_R             61900
#endif

#if (BAT_NTC_100 == 1)
#define RBAT_PULL_UP_R             100000
#endif

#define RBAT_PULL_UP_VOLT          1800

#define HTC_AICL_START_VOL 4000
#define HTC_AICL_VBUS_DROP_RATIO 140
#define AC_IBAT_CHARGER_CURRENT				CHARGE_CURRENT_1300_00_MA




typedef struct _BATTERY_PROFILE_STRUCT {
	signed int percentage;
	signed int voltage;
	} BATTERY_PROFILE_STRUCT, *BATTERY_PROFILE_STRUCT_P;

typedef struct _R_PROFILE_STRUCT {
	signed int resistance; 
	signed int voltage;
} R_PROFILE_STRUCT, *R_PROFILE_STRUCT_P;

typedef enum {
	T1_0C,
	T2_25C,
	T3_50C
} PROFILE_TEMPERATURE;



#if (BAT_NTC_10 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 68237},
	{-15, 53650},
	{-10, 42506},
	{ -5, 33892},
	{  0, 27219},
	{  5, 22021},
	{ 10, 17926},
	{ 15, 14674},
	{ 20, 12081},
	{ 25, 10000},
	{ 30, 8315},
	{ 35, 6948},
	{ 40, 5834},
	{ 45, 4917},
	{ 50, 4161},
	{ 55, 3535},
	{ 60, 3014}
};
#endif

#if (BAT_NTC_47 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 483954},
	{-15, 360850},
	{-10, 271697},
	{ -5, 206463},
	{  0, 158214},
	{  5, 122259},
	{ 10, 95227},
	{ 15, 74730},
	{ 20, 59065},
	{ 25, 47000},
	{ 30, 37643},
	{ 35, 30334},
	{ 40, 24591},
	{ 45, 20048},
	{ 50, 16433},
	{ 55, 13539},
	{ 60, 11210}
};
#endif

#if (BAT_NTC_100 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 1282000},
	{-15, 929400},
	{-10, 680900},
	{-5, 504100},
	{0, 376600},
	{5, 283900},
	{10, 216000},
	{15, 165800},
	{20, 128200},
	{25, 100000},
	{30, 78600},
	{35, 62250},
	{40, 49660},
	{45, 39890},
	{50, 32260},
	{55, 26260},
	{60, 21510}
};
#endif

BATTERY_PROFILE_STRUCT battery_profile_t0[] = {
	{0, 4343},
	{2, 4315},
	{3, 4291},
	{5, 4268},
	{6, 4247},
	{8, 4224},
	{10, 4199},
	{11, 4174},
	{13, 4149},
	{14, 4129},
	{16, 4110},
	{18, 4097},
	{19, 4083},
	{21, 4065},
	{22, 4043},
	{24, 4018},
	{26, 3995},
	{27, 3975},
	{29, 3959},
	{31, 3945},
	{32, 3934},
	{34, 3922},
	{35, 3910},
	{37, 3898},
	{39, 3885},
	{40, 3875},
	{42, 3864},
	{43, 3855},
	{45, 3846},
	{47, 3838},
	{48, 3831},
	{50, 3824},
	{51, 3817},
	{53, 3811},
	{55, 3805},
	{56, 3800},
	{58, 3797},
	{59, 3792},
	{61, 3790},
	{63, 3787},
	{64, 3785},
	{66, 3782},
	{67, 3779},
	{69, 3777},
	{71, 3774},
	{72, 3770},
	{74, 3767},
	{76, 3761},
	{77, 3756},
	{79, 3748},
	{80, 3741},
	{82, 3733},
	{84, 3725},
	{85, 3717},
	{87, 3711},
	{88, 3704},
	{90, 3698},
	{91, 3694},
	{92, 3689},
	{93, 3686},
	{94, 3683},
	{95, 3680},
	{95, 3676},
	{96, 3672},
	{97, 3667},
	{97, 3662},
	{97, 3658},
	{98, 3648},
	{98, 3638},
	{99, 3628},
	{99, 3619},
	{99, 3609},
	{100, 3601},
	{100, 3591},
	{100, 3581},
	{100, 3400},
};

BATTERY_PROFILE_STRUCT battery_profile_t1[] = {
	{0, 4343},
	{2, 4315},
	{3, 4291},
	{5, 4268},
	{6, 4247},
	{8, 4224},
	{10, 4199},
	{11, 4174},
	{13, 4149},
	{14, 4129},
	{16, 4110},
	{18, 4097},
	{19, 4083},
	{21, 4065},
	{22, 4043},
	{24, 4018},
	{26, 3995},
	{27, 3975},
	{29, 3959},
	{31, 3945},
	{32, 3934},
	{34, 3922},
	{35, 3910},
	{37, 3898},
	{39, 3885},
	{40, 3875},
	{42, 3864},
	{43, 3855},
	{45, 3846},
	{47, 3838},
	{48, 3831},
	{50, 3824},
	{51, 3817},
	{53, 3811},
	{55, 3805},
	{56, 3800},
	{58, 3797},
	{59, 3792},
	{61, 3790},
	{63, 3787},
	{64, 3785},
	{66, 3782},
	{67, 3779},
	{69, 3777},
	{71, 3774},
	{72, 3770},
	{74, 3767},
	{76, 3761},
	{77, 3756},
	{79, 3748},
	{80, 3741},
	{82, 3733},
	{84, 3725},
	{85, 3717},
	{87, 3711},
	{88, 3704},
	{90, 3698},
	{91, 3694},
	{92, 3689},
	{93, 3686},
	{94, 3683},
	{95, 3680},
	{95, 3676},
	{96, 3672},
	{97, 3667},
	{97, 3662},
	{97, 3658},
	{98, 3648},
	{98, 3638},
	{99, 3628},
	{99, 3619},
	{99, 3609},
	{100, 3601},
	{100, 3591},
	{100, 3581},
	{100, 3400},
};

BATTERY_PROFILE_STRUCT battery_profile_t2[] = {
	{0, 4371},
	{1, 4322},
	{3, 4289},
	{4, 4267},
	{6, 4248},
	{7, 4231},
	{9, 4215},
	{10, 4199},
	{12, 4183},
	{13, 4168},
	{15, 4153},
	{16, 4138},
	{18, 4123},
	{19, 4108},
	{21, 4095},
	{22, 4086},
	{24, 4076},
	{25, 4060},
	{27, 4035},
	{28, 4010},
	{30, 3990},
	{31, 3975},
	{32, 3964},
	{34, 3955},
	{35, 3945},
	{37, 3932},
	{38, 3918},
	{40, 3904},
	{41, 3891},
	{43, 3880},
	{44, 3870},
	{46, 3861},
	{47, 3853},
	{49, 3845},
	{50, 3838},
	{52, 3835},
	{53, 3826},
	{55, 3820},
	{56, 3814},
	{58, 3808},
	{59, 3803},
	{60, 3799},
	{62, 3794},
	{63, 3790},
	{65, 3786},
	{66, 3784},
	{68, 3782},
	{69, 3780},
	{71, 3778},
	{72, 3776},
	{74, 3773},
	{75, 3769},
	{77, 3764},
	{78, 3757},
	{80, 3750},
	{81, 3741},
	{83, 3730},
	{84, 3718},
	{86, 3709},
	{87, 3704},
	{89, 3701},
	{90, 3698},
	{91, 3695},
	{93, 3690},
	{94, 3680},
	{96, 3647},
	{97, 3584},
	{99, 3491},
	{100, 3438},
	{100, 3397},
	{100, 3362},
	{100, 3328},
	{100, 3301},
	{100, 3275},
	{100, 3255},
	{100, 3238},
};

BATTERY_PROFILE_STRUCT battery_profile_t3[] = {
	{0, 4382},
	{1, 4364},
	{3, 4349},
	{4, 4333},
	{5, 4318},
	{7, 4302},
	{8, 4284},
	{10, 4268},
	{11, 4253},
	{12, 4237},
	{14, 4221},
	{15, 4204},
	{16, 4189},
	{18, 4173},
	{19, 4158},
	{21, 4143},
	{22, 4128},
	{23, 4113},
	{25, 4098},
	{26, 4084},
	{27, 4072},
	{29, 4064},
	{30, 4050},
	{31, 4027},
	{33, 4006},
	{34, 3991},
	{36, 3980},
	{37, 3974},
	{38, 3965},
	{40, 3953},
	{41, 3940},
	{42, 3927},
	{44, 3911},
	{45, 3896},
	{46, 3882},
	{48, 3871},
	{49, 3861},
	{51, 3852},
	{52, 3845},
	{53, 3837},
	{55, 3831},
	{56, 3825},
	{57, 3819},
	{59, 3813},
	{60, 3808},
	{62, 3803},
	{63, 3798},
	{64, 3794},
	{66, 3790},
	{67, 3785},
	{68, 3782},
	{70, 3778},
	{71, 3775},
	{72, 3772},
	{74, 3769},
	{75, 3766},
	{77, 3761},
	{78, 3756},
	{79, 3751},
	{81, 3744},
	{82, 3738},
	{83, 3726},
	{85, 3715},
	{86, 3702},
	{88, 3692},
	{89, 3690},
	{90, 3688},
	{92, 3687},
	{93, 3685},
	{94, 3681},
	{96, 3662},
	{97, 3608},
	{98, 3532},
	{100, 3424},
	{100, 3234},
	{100, 3147},
};

BATTERY_PROFILE_STRUCT battery_profile_t4[] = {
	{0, 4392},
	{1, 4375},
	{3, 4360},
	{4, 4345},
	{5, 4330},
	{7, 4315},
	{8, 4299},
	{10, 4283},
	{11, 4267},
	{12, 4251},
	{14, 4235},
	{15, 4219},
	{16, 4202},
	{18, 4187},
	{19, 4171},
	{20, 4156},
	{22, 4140},
	{23, 4125},
	{25, 4110},
	{26, 4095},
	{27, 4081},
	{29, 4067},
	{30, 4053},
	{31, 4039},
	{33, 4026},
	{34, 4013},
	{36, 4000},
	{37, 3988},
	{38, 3977},
	{40, 3965},
	{41, 3954},
	{42, 3942},
	{44, 3930},
	{45, 3915},
	{46, 3897},
	{48, 3882},
	{49, 3871},
	{51, 3862},
	{52, 3854},
	{53, 3846},
	{55, 3839},
	{56, 3832},
	{57, 3826},
	{59, 3820},
	{60, 3814},
	{61, 3809},
	{63, 3803},
	{64, 3799},
	{66, 3794},
	{67, 3789},
	{68, 3785},
	{70, 3781},
	{71, 3776},
	{72, 3767},
	{74, 3759},
	{75, 3754},
	{77, 3749},
	{78, 3743},
	{79, 3737},
	{81, 3731},
	{82, 3725},
	{83, 3716},
	{85, 3705},
	{86, 3693},
	{87, 3680},
	{89, 3678},
	{90, 3677},
	{92, 3677},
	{93, 3675},
	{94, 3671},
	{96, 3653},
	{97, 3605},
	{98, 3537},
	{100, 3435},
	{100, 3266},
	{100, 3102},
};

BATTERY_PROFILE_STRUCT battery_profile_temperature[] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0 }
};

R_PROFILE_STRUCT r_profile_t0[] = {
	{800, 4343},
	{800, 4315},
	{825, 4291},
	{835, 4268},
	{855, 4247},
	{873, 4224},
	{893, 4199},
	{928, 4174},
	{968, 4149},
	{1003, 4129},
	{1025, 4110},
	{1058, 4097},
	{1085, 4083},
	{1095, 4065},
	{1095, 4043},
	{1085, 4018},
	{1075, 3995},
	{1070, 3975},
	{1070, 3959},
	{1075, 3945},
	{1085, 3934},
	{1088, 3922},
	{1088, 3910},
	{1090, 3898},
	{1085, 3885},
	{1090, 3875},
	{1090, 3864},
	{1093, 3855},
	{1095, 3846},
	{1103, 3838},
	{1110, 3831},
	{1113, 3824},
	{1123, 3817},
	{1125, 3811},
	{1135, 3805},
	{1143, 3800},
	{1155, 3797},
	{1163, 3792},
	{1175, 3790},
	{1188, 3787},
	{1203, 3785},
	{1220, 3782},
	{1243, 3779},
	{1263, 3777},
	{1288, 3774},
	{1310, 3770},
	{1343, 3767},
	{1368, 3761},
	{1400, 3756},
	{1433, 3748},
	{1468, 3741},
	{1503, 3733},
	{1540, 3725},
	{1580, 3717},
	{1625, 3711},
	{1678, 3704},
	{1735, 3698},
	{1738, 3694},
	{1728, 3689},
	{1728, 3686},
	{1713, 3683},
	{1703, 3680},
	{1693, 3676},
	{1693, 3672},
	{1675, 3667},
	{1673, 3662},
	{1645, 3658},
	{1620, 3648},
	{1603, 3638},
	{1585, 3628},
	{1558, 3619},
	{1545, 3609},
	{1515, 3601},
	{1500, 3591},
	{1460, 3581},
	{1000, 3400}
};
R_PROFILE_STRUCT r_profile_t1[] = {
	{800, 4343},
	{800, 4315},
	{825, 4291},
	{835, 4268},
	{855, 4247},
	{873, 4224},
	{893, 4199},
	{928, 4174},
	{968, 4149},
	{1003, 4129},
	{1025, 4110},
	{1058, 4097},
	{1085, 4083},
	{1095, 4065},
	{1095, 4043},
	{1085, 4018},
	{1075, 3995},
	{1070, 3975},
	{1070, 3959},
	{1075, 3945},
	{1085, 3934},
	{1088, 3922},
	{1088, 3910},
	{1090, 3898},
	{1085, 3885},
	{1090, 3875},
	{1090, 3864},
	{1093, 3855},
	{1095, 3846},
	{1103, 3838},
	{1110, 3831},
	{1113, 3824},
	{1123, 3817},
	{1125, 3811},
	{1135, 3805},
	{1143, 3800},
	{1155, 3797},
	{1163, 3792},
	{1175, 3790},
	{1188, 3787},
	{1203, 3785},
	{1220, 3782},
	{1243, 3779},
	{1263, 3777},
	{1288, 3774},
	{1310, 3770},
	{1343, 3767},
	{1368, 3761},
	{1400, 3756},
	{1433, 3748},
	{1468, 3741},
	{1503, 3733},
	{1540, 3725},
	{1580, 3717},
	{1625, 3711},
	{1678, 3704},
	{1735, 3698},
	{1738, 3694},
	{1728, 3689},
	{1728, 3686},
	{1713, 3683},
	{1703, 3680},
	{1693, 3676},
	{1693, 3672},
	{1675, 3667},
	{1673, 3662},
	{1645, 3658},
	{1620, 3648},
	{1603, 3638},
	{1585, 3628},
	{1558, 3619},
	{1545, 3609},
	{1515, 3601},
	{1500, 3591},
	{1460, 3581},
	{1000, 3400}
};

R_PROFILE_STRUCT r_profile_t2[] = {
	{350, 4371},
	{350, 4322},
	{430, 4289},
	{450, 4267},
	{455, 4248},
	{460, 4231},
	{468, 4215},
	{470, 4199},
	{473, 4183},
	{475, 4168},
	{480, 4153},
	{485, 4138},
	{485, 4123},
	{485, 4108},
	{485, 4095},
	{505, 4086},
	{530, 4076},
	{530, 4060},
	{513, 4035},
	{498, 4010},
	{473, 3990},
	{488, 3975},
	{490, 3964},
	{495, 3955},
	{493, 3945},
	{483, 3932},
	{475, 3918},
	{468, 3904},
	{460, 3891},
	{458, 3880},
	{455, 3870},
	{458, 3861},
	{460, 3853},
	{465, 3845},
	{468, 3838},
	{480, 3835},
	{470, 3826},
	{480, 3820},
	{483, 3814},
	{488, 3808},
	{490, 3803},
	{495, 3799},
	{500, 3794},
	{503, 3790},
	{508, 3786},
	{513, 3784},
	{518, 3782},
	{528, 3780},
	{538, 3778},
	{548, 3776},
	{563, 3773},
	{575, 3769},
	{588, 3764},
	{603, 3757},
	{620, 3750},
	{640, 3741},
	{658, 3730},
	{678, 3718},
	{700, 3709},
	{730, 3704},
	{723, 3701},
	{818, 3698},
	{878, 3695},
	{938, 3690},
	{1000, 3680},
	{1045, 3647},
	{1105, 3584},
	{1213, 3491},
	{1098, 3438},
	{995, 3397},
	{908, 3362},
	{838, 3328},
	{763, 3301},
	{708, 3275},
	{658, 3255},
	{618, 3238}
};

R_PROFILE_STRUCT r_profile_t3[] = {
	{158, 4382},
	{158, 4364},
	{163, 4349},
	{163, 4333},
	{165, 4318},
	{168, 4302},
	{165, 4284},
	{168, 4268},
	{173, 4253},
	{175, 4237},
	{178, 4221},
	{178, 4204},
	{183, 4189},
	{183, 4173},
	{185, 4158},
	{190, 4143},
	{190, 4128},
	{193, 4113},
	{195, 4098},
	{198, 4084},
	{200, 4072},
	{213, 4064},
	{213, 4050},
	{205, 4027},
	{208, 4006},
	{210, 3991},
	{213, 3980},
	{215, 3974},
	{215, 3965},
	{210, 3953},
	{203, 3940},
	{200, 3927},
	{188, 3911},
	{175, 3896},
	{165, 3882},
	{163, 3871},
	{160, 3861},
	{160, 3852},
	{160, 3845},
	{163, 3837},
	{163, 3831},
	{165, 3825},
	{168, 3819},
	{170, 3813},
	{173, 3808},
	{175, 3803},
	{178, 3798},
	{180, 3794},
	{183, 3790},
	{180, 3785},
	{185, 3782},
	{183, 3778},
	{185, 3775},
	{185, 3772},
	{185, 3769},
	{183, 3766},
	{178, 3761},
	{178, 3756},
	{180, 3751},
	{183, 3744},
	{188, 3738},
	{183, 3726},
	{188, 3715},
	{185, 3702},
	{178, 3692},
	{185, 3690},
	{203, 3688},
	{225, 3687},
	{243, 3685},
	{260, 3681},
	{263, 3662},
	{265, 3608},
	{295, 3532},
	{350, 3424},
	{490, 3234},
	{383, 3147}
};

R_PROFILE_STRUCT r_profile_t4[] = {
	{88, 4392},
	{88, 4375},
	{88, 4360},
	{88, 4345},
	{88, 4330},
	{90, 4315},
	{90, 4299},
	{90, 4283},
	{93, 4267},
	{95, 4251},
	{98, 4235},
	{100, 4219},
	{98, 4202},
	{103, 4187},
	{103, 4171},
	{108, 4156},
	{108, 4140},
	{110, 4125},
	{110, 4110},
	{113, 4095},
	{115, 4081},
	{118, 4067},
	{120, 4053},
	{120, 4039},
	{123, 4026},
	{125, 4013},
	{125, 4000},
	{128, 3988},
	{130, 3977},
	{133, 3965},
	{135, 3954},
	{133, 3942},
	{133, 3930},
	{123, 3915},
	{108, 3897},
	{95, 3882},
	{93, 3871},
	{93, 3862},
	{95, 3854},
	{95, 3846},
	{98, 3839},
	{100, 3832},
	{103, 3826},
	{105, 3820},
	{108, 3814},
	{110, 3809},
	{110, 3803},
	{115, 3799},
	{115, 3794},
	{115, 3789},
	{115, 3785},
	{115, 3781},
	{115, 3776},
	{103, 3767},
	{98, 3759},
	{103, 3754},
	{105, 3749},
	{103, 3743},
	{100, 3737},
	{100, 3731},
	{103, 3725},
	{103, 3716},
	{100, 3705},
	{100, 3693},
	{88, 3680},
	{93, 3678},
	{103, 3677},
	{113, 3677},
	{118, 3675},
	{120, 3671},
	{113, 3653},
	{118, 3605},
	{133, 3537},
	{143, 3435},
	{175, 3266},
	{270, 3102}
};

R_PROFILE_STRUCT r_profile_temperature[] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0}
};

int fgauge_get_saddles(void);
BATTERY_PROFILE_STRUCT_P fgauge_get_profile(unsigned int temperature);

int fgauge_get_saddles_r_table(void);
R_PROFILE_STRUCT_P fgauge_get_profile_r_table(unsigned int temperature);

#endif

