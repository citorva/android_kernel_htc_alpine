#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include <linux/proc_fs.h>


#include <linux/dma-mapping.h>
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#define PFX "OV16880_pdafotp"
#define LOG_INF(fmt, args...)   pr_debug(PFX "[%s] " fmt, __FUNCTION__, ##args)

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);

#define USHORT             	unsigned short
#define BYTE                unsigned char
#define Sleep(ms)           mdelay(ms)

#define EEPROM              GT24C64A
#define EEPROM_READ_ID    	(0xA0)
#define I2C_SPEED           (400)  

#define MAX_OFFSET          (0xffff)
#define DATA_SIZE           (4096)

static BYTE eeprom_data[DATA_SIZE] = {0};
static bool get_done = false;
static int last_size = 0;
static int last_offset = 0;

static bool selective_read_eeprom(u16 addr, BYTE* data)
{
	char pu_send_cmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
	if(addr > MAX_OFFSET)
		return false;

	if(iReadRegI2C(pu_send_cmd, 2, (u8*)data, 1, EEPROM_READ_ID)<0)
		return false;
	return true;
}

static bool _read_eeprom(u16 addr, BYTE* data, u32 size )
{
	int i = 0;
	int offset = addr;
	for(i = 0; i < size; i++) 
	{
		if(!selective_read_eeprom(offset, &data[i]))
		{
			return false;
		}
		LOG_INF("read_eeprom 0x%x 0x%x\n",offset, data[i]);
		offset++;
	}
	get_done = true;
	last_size = size;
	last_offset = addr;
	return true;
}
 bool read_ov16880_eeprom( u16 addr, BYTE* data, u32 size)
{

	int i =0;
	addr = 0x077A;
	size = 496; 
	LOG_INF("read ov16880 eeprom, addr = %d; size = %d\n", addr, size);
	if(!get_done || last_size != size || last_offset != addr) 
	{
		if(!_read_eeprom(addr, eeprom_data, size))
		{
			get_done = 0;
			last_size = 0;
			last_offset = 0;
			return false;
		}else{
			LOG_INF("Eeprom read proc1 completed \n");
		}
	}else{
		LOG_INF("ERROR: read proc1 failed in entry 1\n");
	}

	if(!get_done || last_size != 876 || last_offset != 0x096C) 
	{
		if(!_read_eeprom(0x096C, eeprom_data+496, 876))
		{
			get_done = 0;
			last_size = 0;
			last_offset = 0;
			return false;
		}else{
			LOG_INF("Eeprom read proc1 completed \n");
		}
	}else{
		LOG_INF("ERROR: read proc1 failed in entry 1\n");
	}	
	memcpy(data, eeprom_data, 1372);
for(i=0;i<1372;i++)
	printk("[PDAF-data] The value of data+%d is: 0x%x\n",i,*(data+i));
  return true;
}





