/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Ft_gesture_lib.c
*
* Author:
*
* Created: 2015-01-01
*
* Abstract: function for hand recognition
*
************************************************************************/
#ifndef __LINUX_FT_GESTURE_LIB_H__
#define __LINUX_FT_GESTURE_LIB_H__


int fetch_object_sample(unsigned char *buf,short pointnum);

void init_para(int x_pixel,int y_pixel,int time_slot,int cut_x_pixel,int cut_y_pixel);


#endif
