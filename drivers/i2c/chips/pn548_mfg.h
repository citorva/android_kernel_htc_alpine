#define FTM_NFC_CPLC 0

#define MAX_NFC_DATA_SIZE (600)
#define MAX_FW_BLOCK_SIZE (512)
#define MAX_NFC_RETRY (100)
#define MAX_READ_TIME (5)
#define NFC_READ_DELAY (10)
#define MAX_QUP_DATA_LEN (224)
#define MAX_CMD_LEN 257



typedef struct RF_entry_def {
	unsigned int RF_ID;
	unsigned int RF_Protocol;
	unsigned int RF_Technology;
} RF_Entry;

static struct device_info_def {
	unsigned int padding_exist;
	unsigned char padding;
	unsigned long fwVersion;
	unsigned int HW_model;
	unsigned int NCI_version;
	unsigned long NFCC_Features;
	unsigned char manufactor;

	unsigned char FW_Major;
	unsigned char FW_Minor;

	unsigned int protocol_set;
	unsigned int intf_set;
	unsigned int target_rf_id;
	unsigned int activated_INTF;
	unsigned int NTF_count;
	RF_Entry NTF_queue[15];
} gDevice_info;

typedef struct control_msg_pack_def {
	uint8_t cmd[MAX_CMD_LEN];
	uint8_t exp_resp_content[MAX_CMD_LEN];
	uint8_t exp_ntf[MAX_CMD_LEN];
} control_msg_pack;

static control_msg_pack nfc_version_script[] = {
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
};

static control_msg_pack select_rf_target[] = {
	{
		{ 6, 0x21, 0x04, 0x03, 0x01, 0x04, 0x02 },
		{ 1, 0x00 },
		{ 0 },
	},
};

#ifndef NFC_PLL_CLOCK_ENABLE
static control_msg_pack nfc_card_script[] ={
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08 },
		{ 0 },
		{ 0 },
	},
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 15, 0x21, 0x01, 0x0C, 0x00, 0x02, 0x00, 0x03, 0x00, 0x05, 0x01, 0x01, 0x03, 0x00, 0x01, 0x04 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 12, 0x21, 0x03, 0x09, 0x04, 0x81, 0x01, 0x82, 0x01, 0x83, 0x01, 0x85, 0x01 },
		{ 1, 0x00 },
		{ 1, 0x01 }
	}
};
#endif

static control_msg_pack nfc_standby_enble_script[] = {
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		
		{ 4, 0x2F, 0x00, 0x01, 0x01 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x07, 0x01, 0x02 },
		{ 1, 0x00 },
		{ 0 },
	},
};


static control_msg_pack nfc_card_script_plm[] ={
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08 },
		{ 0 },
		{ 0 },
	},
	{   
		{ 0xF6,
    0x20, 0x02, 0xF3, 0x20,
    0xA0, 0x0D, 0x03, 0x00, 0x40, 0x03,
    0xA0, 0x0D, 0x03, 0x04, 0x43, 0xA0,
    0xA0, 0x0D, 0x03, 0x04, 0xFF, 0x05,
    0xA0, 0x0D, 0x06, 0x06, 0x44, 0xA3, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x06, 0x30, 0xBF, 0x00, 0x20, 0x00,
    0xA0, 0x0D, 0x06, 0x06, 0x2F, 0x8F, 0x05, 0x80, 0x11,
    0xA0, 0x0D, 0x04, 0x06, 0x03, 0x00, 0x70,
    0xA0, 0x0D, 0x03, 0x06, 0x48, 0x10,
    0xA0, 0x0D, 0x03, 0x06, 0x43, 0x2C,
    0xA0, 0x0D, 0x06, 0x06, 0x42, 0x01, 0x00, 0xF1, 0xFF,
    0xA0, 0x0D, 0x06, 0x06, 0x41, 0x01, 0x00, 0x00, 0x00,
    0xA0, 0x0D, 0x03, 0x06, 0x37, 0x00,
    0xA0, 0x0D, 0x03, 0x06, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x06, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x06, 0xFF, 0x05, 0x00, 0x06, 0x00,
    0xA0, 0x0D, 0x06, 0x08, 0x44, 0x00, 0x00, 0x00, 0x00,
    0xA0, 0x0D, 0x06, 0x20, 0x4A, 0x00, 0x00, 0x00, 0x00,
    0xA0, 0x0D, 0x06, 0x20, 0x42, 0x88, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x20, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x20, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x22, 0x44, 0x29, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x22, 0x2D, 0x50, 0x44, 0x0C, 0x00,
    0xA0, 0x0D, 0x04, 0x32, 0x03, 0x40, 0x3D,
    0xA0, 0x0D, 0x06, 0x32, 0x42, 0xF8, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x32, 0x16, 0x19,
    0xA0, 0x0D, 0x03, 0x32, 0x15, 0x01,
    0xA0, 0x0D, 0x03, 0x32, 0x0D, 0x26,
    0xA0, 0x0D, 0x03, 0x32, 0x14, 0x26,
    0xA0, 0x0D, 0x06, 0x32, 0x4A, 0x53, 0x07, 0x01, 0x1B,
    0xA0, 0x0D, 0x06, 0x34, 0x2D, 0xDC, 0x50, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x34, 0x34, 0x00, 0x00, 0xE4, 0x03,
    0xA0, 0x0D, 0x06, 0x34, 0x44, 0x21, 0x00, 0x02, 0x00
		},
		{ 0 },
		{ 0 },
	},
	{   
		{ 0xF7,
    0x20, 0x02, 0xF4, 0x1F,
    0xA0, 0x0D, 0x06, 0x35, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x38, 0x4A, 0x56, 0x07, 0x01, 0x1B,
    0xA0, 0x0D, 0x06, 0x38, 0x42, 0x68, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x38, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x38, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x3A, 0x2D, 0x15, 0x57, 0x1F, 0x00,
    0xA0, 0x0D, 0x06, 0x3C, 0x4A, 0x56, 0x07, 0x01, 0x1B,
    0xA0, 0x0D, 0x06, 0x3C, 0x42, 0x68, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x3C, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x3C, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x3E, 0x2D, 0x15, 0x88, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x40, 0x42, 0xF0, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x40, 0x0D, 0x03,
    0xA0, 0x0D, 0x03, 0x40, 0x14, 0x03,
    0xA0, 0x0D, 0x06, 0x40, 0x4A, 0x15, 0x07, 0x00, 0x00,
    0xA0, 0x0D, 0x03, 0x40, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x40, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x42, 0x2D, 0xDD, 0x33, 0x0F, 0x00,
    0xA0, 0x0D, 0x06, 0x46, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x46, 0x2D, 0x05, 0x48, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x44, 0x4A, 0x43, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x06, 0x44, 0x42, 0x90, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x44, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x44, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x4A, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x4A, 0x2D, 0x05, 0x48, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x48, 0x4A, 0x43, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x06, 0x48, 0x42, 0x88, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x48, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x48, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x4E, 0x44, 0x21, 0x00, 0x02, 0x00
		},
		{ 0 },
		{ 0 },
	},
	{   
		{ 0xFA,
    0x20, 0x02, 0xF7, 0x1E,
    0xA0, 0x0D, 0x06, 0x4E, 0x2D, 0x05, 0x48, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x4C, 0x4A, 0x43, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x06, 0x4C, 0x42, 0x88, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x4C, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x4C, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x52, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x52, 0x2D, 0x05, 0x48, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x50, 0x42, 0x90, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x06, 0x50, 0x4A, 0x32, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x03, 0x50, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x50, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x56, 0x2D, 0x05, 0xCC, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x56, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x5C, 0x2D, 0x05, 0xCC, 0x0C, 0x00,
    0xA0, 0x0D, 0x06, 0x5C, 0x44, 0x21, 0x00, 0x02, 0x00,
    0xA0, 0x0D, 0x06, 0x54, 0x42, 0x90, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x06, 0x54, 0x4A, 0x43, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x03, 0x54, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x54, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x5A, 0x42, 0x98, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x06, 0x5A, 0x4A, 0x63, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x03, 0x5A, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x5A, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x98, 0x2F, 0xAF, 0x05, 0x80, 0x17,
    0xA0, 0x0D, 0x06, 0x9A, 0x42, 0x01, 0x00, 0xF1, 0xFF,
    0xA0, 0x0D, 0x06, 0x30, 0x44, 0xA3, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x6C, 0x44, 0xA1, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x6C, 0x30, 0xBF, 0x00, 0x20, 0x00,
    0xA0, 0x0D, 0x06, 0x6C, 0x2F, 0x8F, 0x05, 0x80, 0x11,
    0xA0, 0x0D, 0x06, 0x70, 0x2F, 0x8F, 0x05, 0x80, 0x0D
		},
		{ 0 },
		{ 0 },
	},
	{   
		{ 0xFA,
    0x20, 0x02, 0xF7, 0x1E,
    0xA0, 0x0D, 0x06, 0x70, 0x30, 0x8F, 0x00, 0x04, 0x00,
    0xA0, 0x0D, 0x06, 0x74, 0x2F, 0x6F, 0x05, 0x80, 0x0D,
    0xA0, 0x0D, 0x06, 0x74, 0x30, 0x8F, 0x00, 0x04, 0x00,
    0xA0, 0x0D, 0x06, 0x78, 0x2F, 0x1F, 0x06, 0x80, 0x01,
    0xA0, 0x0D, 0x06, 0x78, 0x30, 0x9F, 0x00, 0x10, 0x00,
    0xA0, 0x0D, 0x06, 0x78, 0x44, 0xA0, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x03, 0x78, 0x47, 0x00,
    0xA0, 0x0D, 0x06, 0x7C, 0x2F, 0xAF, 0x05, 0x80, 0x17,
    0xA0, 0x0D, 0x06, 0x7C, 0x30, 0xBF, 0x00, 0x20, 0x00,
    0xA0, 0x0D, 0x06, 0x7C, 0x44, 0xA3, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x7D, 0x30, 0x8F, 0x00, 0x04, 0x00,
    0xA0, 0x0D, 0x06, 0x80, 0x2F, 0xAF, 0x05, 0x80, 0x0D,
    0xA0, 0x0D, 0x06, 0x80, 0x44, 0xA1, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x84, 0x2F, 0xAF, 0x05, 0x80, 0x0D,
    0xA0, 0x0D, 0x06, 0x84, 0x44, 0xA1, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x06, 0x88, 0x2F, 0x7F, 0x04, 0x80, 0x0D,
    0xA0, 0x0D, 0x06, 0x88, 0x30, 0x8F, 0x00, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x88, 0x47, 0x00,
    0xA0, 0x0D, 0x06, 0x88, 0x44, 0xA3, 0x90, 0x03, 0x00,
    0xA0, 0x0D, 0x03, 0x0C, 0x48, 0x10,
    0xA0, 0x0D, 0x03, 0x10, 0x43, 0xA0,
    0xA0, 0x0D, 0x06, 0x6A, 0x42, 0xF8, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x03, 0x6A, 0x16, 0x19,
    0xA0, 0x0D, 0x03, 0x6A, 0x15, 0x01,
    0xA0, 0x0D, 0x06, 0x6A, 0x4A, 0x53, 0x07, 0x01, 0x1B,
    0xA0, 0x0D, 0x06, 0x8C, 0x42, 0x90, 0x10, 0xFF, 0xFF,
    0xA0, 0x0D, 0x06, 0x8C, 0x4A, 0x43, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x03, 0x8C, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x8C, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x92, 0x42, 0x98, 0x10, 0xFF, 0xFF
		},
		{ 0 },
		{ 0 },
	},
	{   
		{ 0x3A,
    0x20, 0x02, 0x37, 0x07,
    0xA0, 0x0D, 0x06, 0x92, 0x4A, 0x63, 0x07, 0x01, 0x07,
    0xA0, 0x0D, 0x03, 0x92, 0x16, 0x00,
    0xA0, 0x0D, 0x03, 0x92, 0x15, 0x00,
    0xA0, 0x0D, 0x06, 0x0A, 0x30, 0xBF, 0x00, 0x20, 0x00,
    0xA0, 0x0D, 0x06, 0x0A, 0x2F, 0x8F, 0x05, 0x80, 0x11,
    0xA0, 0x0D, 0x03, 0x0A, 0x48, 0x10,
    0xA0, 0x0D, 0x06, 0x0A, 0x44, 0xA3, 0x90, 0x03, 0x00
		},
		{ 0 },
		{ 0 },
	},
	{   
		{ 0x4C,
        0x20, 0x02, 0x49, 0x11,
        0xA0, 0x02, 0x01, 0x00,
        0xA0, 0x03, 0x01, 0x08,
        0xA0, 0x04, 0x01, 0x01,
        0xA0, 0x09, 0x02, 0xE8, 0x03,
        0xA0, 0x0E, 0x01, 0x00,
        0xA0, 0x11, 0x04, 0x14, 0xB8, 0x0B, 0x14,
        0xA0, 0x12, 0x01, 0x00,
        0xA0, 0x13, 0x01, 0x00,
        0xA0, 0x40, 0x01, 0x00,
        0xA0, 0x41, 0x01, 0x00,
        0xA0, 0x42, 0x01, 0x19,
        0xA0, 0x43, 0x01, 0x00,
        0xA0, 0x5E, 0x01, 0x01,
        0xA0, 0x61, 0x01, 0x00,
        0xA0, 0xCD, 0x01, 0x1F,
        0xA0, 0xEC, 0x01, 0x01,
        0xA0, 0xED, 0x01, 0x00
        },
		{ 0 },
		{ 0 },
	},
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 15, 0x21, 0x01, 0x0C, 0x00, 0x02, 0x00, 0x03, 0x00, 0x05, 0x01, 0x01, 0x03, 0x00, 0x01, 0x04 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 12, 0x21, 0x03, 0x09, 0x04, 0x81, 0x01, 0x82, 0x01, 0x83, 0x01, 0x85, 0x01 },
		{ 1, 0x00 },
		{ 1, 0x01 }
	}
};


#ifndef NFC_PLL_CLOCK_ENABLE
static control_msg_pack nfc_reader_script[] ={
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{   
		{ 0x0A,
        0x20, 0x02, 0x07, 0x01, 
        0xA0, 0x0E, 0x03, 0x56, 0x24, 0x0A
        },
		{ 0 },
		{ 0 },
	},
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 7, 0x21, 0x00, 0x04, 0x01, 0x04, 0x01, 0x02 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 8, 0x21, 0x03, 0x05, 0x02, 0x00, 0x01, 0x01, 0x01 },
		{ 1, 0x00 },
		{ 1, 0x01 }
	},
};
#endif
 
#if FTM_NFC_CPLC
#endif 

#ifdef NFC_PLL_CLOCK_ENABLE
static control_msg_pack nfc_card_clk_pll_script[] ={
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x13 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x04, 0x01, 0x01 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x02, 0x01, 0x01 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},

	{
		{ 15, 0x21, 0x01, 0x0C, 0x00, 0x02, 0x00, 0x03, 0x00, 0x05, 0x01, 0x01, 0x03, 0x00, 0x01, 0x04 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 12, 0x21, 0x03, 0x09, 0x04, 0x81, 0x01, 0x82, 0x01, 0x83, 0x01, 0x85, 0x01 },
		{ 1, 0x00 },
		{ 1, 0x01 }
	},
};

static control_msg_pack nfc_reader_clk_pll_script[] ={
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x01 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x13 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x04, 0x01, 0x01 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{
		{ 8, 0x20, 0x02, 0x05, 0x01, 0xA0, 0x02, 0x01, 0x01 },
		{ 2, 0x00, 0x00 },
		{ 0 },
	},
	{	
		{ 0x0A, 0x20, 0x02, 0x07, 0x01, 0xA0, 0x0E, 0x03, 0x56, 0x24, 0x0A },
		{ 0 },
		{ 0 },
	},
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 3, 0x2F, 0x02, 0x00 },
		{ 0 },
		{ 0 },
	},
	{
		{ 7, 0x21, 0x00, 0x04, 0x01, 0x04, 0x01, 0x02 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 8, 0x21, 0x03, 0x05, 0x02, 0x00, 0x01, 0x01, 0x01 },
		{ 1, 0x00 },
		{ 1, 0x01 }
	},
};
#endif

static control_msg_pack nfc_off_mode_charging_enble_script[] = {
	{
		.cmd = { 4, 0x20, 0x00, 0x01, 0x00 },
		.exp_resp_content = { 1, 0x00 },
		.exp_ntf = { 0 },
	},
	{
		{ 3, 0x20, 0x01, 0x00 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		
		{ 5, 0x22, 0x01, 0x02, 0x02, 0x01},
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 4, 0x2F, 0x15, 0x01, 0x01 },
		{ 1, 0x00 },
		{ 0 },
	},
	{
		{ 0x0A, 0x21,0x03,0x07,0x03,0x80,0x01,0x81,0x01,0x82,0x01},
		{ 1, 0x00 },
		{ 0 },
	},
};
