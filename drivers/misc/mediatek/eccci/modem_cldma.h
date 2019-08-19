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

#ifndef __MODEM_CD_H__
#define __MODEM_CD_H__

#include <linux/wakelock.h>
#include <linux/dmapool.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/skbuff.h>
#include <mt-plat/mt_ccci_common.h>

#include "ccci_config.h"
#include "ccci_bm.h"

#ifdef CONFIG_HTC_FEATURES_RIL_PCN0007_LWA
#include "../ccmni/ccmni.h"
#ifdef CCMNI_MTU
#undef CCMNI_MTU
#endif
#endif
#define CLDMA_TXQ_NUM 8
#define CLDMA_RXQ_NUM 8
#define NET_TXQ_NUM 3
#define NET_RXQ_NUM 3
#define NORMAL_TXQ_NUM 6
#define NORMAL_RXQ_NUM 6
#define MAX_BD_NUM (MAX_SKB_FRAGS + 1)
#define TRAFFIC_MONITOR_INTERVAL 10	
#define SKB_RX_QUEUE_MAX_LEN 200000

#define CHECKSUM_SIZE 0		
#ifndef CLDMA_NO_TX_IRQ
#endif
#define CLDMA_NET_TX_BD

struct cldma_request {
	void *gpd;		
	dma_addr_t gpd_addr;	
	struct sk_buff *skb;
	dma_addr_t data_buffer_ptr_saved;
	struct list_head entry;
	struct list_head bd;

	
	DATA_POLICY policy;
	unsigned char ioc_override;	
};

typedef enum {
	RING_GPD = 0,
	RING_GPD_BD = 1,
	RING_SPD = 2,
} CLDMA_RING_TYPE;

struct md_cd_queue;

struct cldma_ring {
	struct list_head gpd_ring;	
	int length;		
	int pkt_size;		
	CLDMA_RING_TYPE type;

	int (*handle_tx_request)(struct md_cd_queue *queue, struct cldma_request *req,
				  struct sk_buff *skb, DATA_POLICY policy, unsigned int ioc_override);
	int (*handle_rx_done)(struct md_cd_queue *queue, int budget, int blocking, int *result, int *rxbytes);
	int (*handle_tx_done)(struct md_cd_queue *queue, int budget, int blocking, int *result);
	int (*handle_rx_refill)(struct md_cd_queue *queue);
};

static inline struct cldma_request *cldma_ring_step_forward(struct cldma_ring *ring, struct cldma_request *req)
{
	struct cldma_request *next_req;

	if (req->entry.next == &ring->gpd_ring)
		next_req = list_first_entry(&ring->gpd_ring, struct cldma_request, entry);
	else
		next_req = list_entry(req->entry.next, struct cldma_request, entry);
	return next_req;
}

struct md_cd_queue {
	unsigned char index;
	struct ccci_modem *modem;
	struct ccci_port *napi_port;

	struct cldma_ring *tr_ring;
	struct cldma_request *tr_done;
	int budget;		
	struct cldma_request *rx_refill;	
	struct cldma_request *tx_xmit;	
	wait_queue_head_t req_wq;	
	spinlock_t ring_lock;
	struct ccci_skb_queue skb_list; 

	struct workqueue_struct *worker;
	struct work_struct cldma_rx_work;
	struct delayed_work cldma_tx_work;
	struct workqueue_struct *refill_worker; 
	struct work_struct cldma_refill_work; 

	wait_queue_head_t rx_wq;
	struct task_struct *rx_thread;

#ifdef ENABLE_CLDMA_TIMER
	struct timer_list timeout_timer;
	unsigned long long timeout_start;
	unsigned long long timeout_end;
#endif
	u16 debug_id;
	DIRECTION dir;
	unsigned int busy_count;
};

#define QUEUE_LEN(a) (sizeof(a)/sizeof(struct md_cd_queue))

struct md_cd_ctrl {
	struct ccci_modem *modem;
	struct md_cd_queue txq[CLDMA_TXQ_NUM];
	struct md_cd_queue rxq[CLDMA_RXQ_NUM];
	unsigned short txq_active;
	unsigned short rxq_active;
#ifdef NO_START_ON_SUSPEND_RESUME
	unsigned short txq_started;
#endif

	atomic_t reset_on_going;
	atomic_t wdt_enabled;
	atomic_t cldma_irq_enabled;
	atomic_t ccif_irq_enabled;
	char trm_wakelock_name[32];
	struct wake_lock trm_wake_lock;
	char peer_wakelock_name[32];
	struct wake_lock peer_wake_lock;
	struct work_struct ccif_work;
	struct delayed_work ccif_delayed_work;
	struct work_struct ccif_allQreset_work;
	struct timer_list bus_timeout_timer;
	spinlock_t cldma_timeout_lock;	
	struct work_struct cldma_irq_work;
	struct workqueue_struct *cldma_irq_worker;
	int channel_id;		
	struct work_struct wdt_work;

#if TRAFFIC_MONITOR_INTERVAL
	unsigned tx_traffic_monitor[CLDMA_TXQ_NUM];
	unsigned rx_traffic_monitor[CLDMA_RXQ_NUM];
	unsigned tx_pre_traffic_monitor[CLDMA_TXQ_NUM];
	unsigned long long tx_done_last_start_time[CLDMA_TXQ_NUM];
	unsigned int tx_done_last_count[CLDMA_TXQ_NUM];

	struct timer_list traffic_monitor;
	unsigned long traffic_stamp;
#endif

	struct dma_pool *gpd_dmapool;	
	struct cldma_ring net_tx_ring[NET_TXQ_NUM];
	struct cldma_ring net_rx_ring[NET_RXQ_NUM];
	struct cldma_ring normal_tx_ring[NORMAL_TXQ_NUM];
	struct cldma_ring normal_rx_ring[NORMAL_RXQ_NUM];

	void __iomem *cldma_ap_ao_base;
	void __iomem *cldma_md_ao_base;
	void __iomem *cldma_ap_pdn_base;
	void __iomem *cldma_md_pdn_base;
	void __iomem *md_rgu_base;
	void __iomem *l1_rgu_base;
	void __iomem *md_boot_slave_Vector;
	void __iomem *md_boot_slave_Key;
	void __iomem *md_boot_slave_En;
	void __iomem *md_global_con0;
	void __iomem *ap_ccif_base;
	void __iomem *md_ccif_base;
#ifdef MD_PEER_WAKEUP
	void __iomem *md_peer_wakeup;
#endif
	void __iomem *md_bus_status;
	void __iomem *md_pc_monitor;
	void __iomem *md_topsm_status;
	void __iomem *md_ost_status;
	void __iomem *md_pll;
	
	struct md_pll_reg *md_pll_base;

	unsigned int cldma_irq_id;
	unsigned int ap_ccif_irq_id;
	unsigned int md_wdt_irq_id;
	unsigned int ap2md_bus_timeout_irq_id;

	unsigned long cldma_irq_flags;
	unsigned long ap_ccif_irq_flags;
	unsigned long md_wdt_irq_flags;
	unsigned long ap2md_bus_timeout_irq_flags;

	struct md_hw_info *hw_info;
#ifdef CONFIG_HTC_FEATURES_RIL_PCN0007_LWA
	struct timer_list lwa_traffic_monitor;
	struct work_struct lwa_traffic_monitor_work;
	u64 lwa_last_rx_bytes;
	u64 lwa_rx_bytes;
	int  enable_cup_lock;
	unsigned int lwa_enable_threshold_v2;
	unsigned int lwa_pnp_cpu_lock_fail_count;
	unsigned int lwa_rx_threshold_high;
	unsigned int lwa_rx_threshold_low;
	unsigned int lwa_rx_threshold_high2;
	unsigned int lwa_rx_threshold_low2;
#endif
};

struct cldma_tgpd {
	u8 gpd_flags;
	u8 gpd_checksum;
	u16 debug_id;
	u32 next_gpd_ptr;
	u32 data_buff_bd_ptr;
	u16 data_buff_len;
	u8 desc_ext_len;
	u8 non_used;		
} __packed;

struct cldma_rgpd {
	u8 gpd_flags;
	u8 gpd_checksum;
	u16 data_allow_len;
	u32 next_gpd_ptr;
	u32 data_buff_bd_ptr;
	u16 data_buff_len;
	u16 debug_id;
} __packed;

struct cldma_tbd {
	u8 bd_flags;
	u8 bd_checksum;
	u16 reserved;
	u32 next_bd_ptr;
	u32 data_buff_ptr;
	u16 data_buff_len;
	u8 desc_ext_len;
	u8 non_used;
} __packed;

struct cldma_rbd {
	u8 bd_flags;
	u8 bd_checksum;
	u16 data_allow_len;
	u32 next_bd_ptr;
	u32 data_buff_ptr;
	u16 data_buff_len;
	u16 reserved;
} __packed;

struct cldma_rspd {
	u8 spd_flags;
	u8 spd_checksum;
	u16 data_allow_len;
	u32 next_spd_ptr;
	u32 data_buff_ptr;
	u16 data_buff_len;
	u8 reserve_len;
	u8 spd_flags2;
} __packed;

typedef enum {
	UNDER_BUDGET,
	REACH_BUDGET,
	PORT_REFUSE,
	NO_SKB,
	NO_REQ
} RX_COLLECT_RESULT;

enum {
	CCCI_TRACE_TX_IRQ = 0,
	CCCI_TRACE_RX_IRQ = 1,
};

static inline void md_cd_queue_struct_init(struct md_cd_queue *queue, struct ccci_modem *md,
					   DIRECTION dir, unsigned char index)
{
	queue->dir = dir;
	queue->index = index;
	queue->modem = md;
	queue->napi_port = NULL;

	queue->tr_ring = NULL;
	queue->tr_done = NULL;
	queue->tx_xmit = NULL;
	init_waitqueue_head(&queue->req_wq);
	spin_lock_init(&queue->ring_lock);
	queue->debug_id = 0;
	queue->busy_count = 0;
}


static inline void memset_io_raw(void __iomem *dst, int c, size_t count)
{
    
#if defined(__raw_write_logged) && defined(CONFIG_MSM_RTB)
    while (count) {
        count--;
        __raw_writeb_no_log(c, dst);
        dst++;
    }
#else
    memset_io(dst, c, count);
#endif
}

#ifndef CONFIG_MTK_ECCCI_C2K
#ifdef CONFIG_MTK_SVLTE_SUPPORT
extern void c2k_reset_modem(void);
#endif
#endif
extern void mt_irq_dump_status(int irq);
extern unsigned int ccci_get_md_debug_mode(struct ccci_modem *md);

extern u32 mt_irq_get_pending(unsigned int irq);
extern unsigned long ccci_modem_boot_count[];

#define GF_PORT_LIST_MAX 128
extern int gf_port_list_reg[GF_PORT_LIST_MAX];
extern int gf_port_list_unreg[GF_PORT_LIST_MAX];
extern int ccci_ipc_set_garbage_filter(struct ccci_modem *md, int reg);

#ifdef TEST_MESSAGE_FOR_BRINGUP
extern int ccci_sysmsg_echo_test(int, int);
extern int ccci_sysmsg_echo_test_l1core(int, int);
#endif

#ifdef CONFIG_HTC_FEATURES_RIL_PCN0007_LWA
extern ccmni_ctl_block_t *ccmni_ctl_blk[MAX_MD_NUM];
extern unsigned int port_ch_get_lwa_state(void);
extern int port_ch_set_lwa_state(struct ccci_modem *modem, unsigned int new_lwa_state);
extern unsigned int port_ch_get_lwa_monitor_enable(void);
extern int port_ch_set_lwa_monitor_enable(struct ccci_modem *modem, unsigned int lwa_monitor_enable);

typedef enum {
	LWA_GEAR_LEVEL0 = 0,
	LWA_GEAR_LEVEL1 = 1,
	LWA_GEAR_LEVEL2 = 2,
	LWA_GEAR_LEVEL_MAX,
} PNPMGR_LWA_GEAR;

typedef enum {
	LWA_STATE_DISABLE = 0,
	LWA_STATE_ENABLE = 1,
}LWA_STATE_TYPE;

#endif

#endif				
