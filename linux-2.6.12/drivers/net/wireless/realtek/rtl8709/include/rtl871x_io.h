

#ifndef _IO_H_
#define _IO_H_


#include <drv_conf.h>
#include <osdep_service.h>
#include <rtl8711_spec.h>
#include <osdep_intf.h>

#ifdef PLATFORM_LINUX

//#include <linux/config.h>
#include <asm/byteorder.h>
#include <asm/semaphore.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>

#ifdef CONFIG_USB_HCI
#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
#include <linux/usb_ch9.h>
#else
#include <linux/usb/ch9.h>
#endif
#endif

#endif


//#define RND4(x)	(((x) >> 2 + ((x) & 3) ? 1: 0) << 2 )

#define NUM_IOREQ		16
#ifdef PLATFORM_WINDOWS
#define MAX_PROT_SZ	64
#endif
#ifdef PLATFORM_LINUX
#define MAX_PROT_SZ	(64-16)
#endif
#define _IOREADY			0
#define _IO_WAIT_COMPLETE   1
#define _IO_WAIT_RSP        2

// IO COMMAND TYPE
#define _IOSZ_MASK_		(0x7F)
#define _IO_WRITE_		BIT(7)
#define _IO_FIXED_		BIT(8)
#define _IO_BURST_		BIT(9)
#define _IO_BYTE_		BIT(10)
#define _IO_HW_			BIT(11)
#define _IO_WORD_		BIT(12)
#define _IO_SYNC_		BIT(13)
#define _IO_CMDMASK_	(0x1F80)


/* 
	For prompt mode accessing, caller shall free io_req
	Otherwise, io_handler will free io_req
*/



// IO STATUS TYPE
#define _IO_ERR_		BIT(2)
#define _IO_SUCCESS_	BIT(1)
#define _IO_DONE_		BIT(0)


#define IO_RD32			(_IO_SYNC_ | _IO_WORD_)
#define IO_RD16			(_IO_SYNC_ | _IO_HW_)
#define IO_RD8			(_IO_SYNC_ | _IO_BYTE_)

#define IO_RD32_ASYNC	(_IO_WORD_)
#define IO_RD16_ASYNC	(_IO_HW_)
#define IO_RD8_ASYNC	(_IO_BYTE_)

#define IO_WR32			(_IO_WRITE_ | _IO_SYNC_ | _IO_WORD_)
#define IO_WR16			(_IO_WRITE_ | _IO_SYNC_ | _IO_HW_)
#define IO_WR8			(_IO_WRITE_ | _IO_SYNC_ | _IO_BYTE_)

#define IO_WR32_ASYNC	(_IO_WRITE_ | _IO_WORD_)
#define IO_WR16_ASYNC	(_IO_WRITE_ | _IO_HW_)
#define IO_WR8_ASYNC	(_IO_WRITE_ | _IO_BYTE_)

/*

	Only Sync. burst accessing is provided.

*/

#define IO_WR_BURST(x)		(_IO_WRITE_ | _IO_SYNC_ | _IO_BURST_ | ( (x) & _IOSZ_MASK_))
#define IO_RD_BURST(x)		(_IO_SYNC_ | _IO_BURST_ | ( (x) & _IOSZ_MASK_))



//below is for the intf_option bit defition...

#define _INTF_ASYNC_	BIT(0)	//support async io

struct intf_priv;
struct	intf_hdl;
struct io_queue;

struct	_io_ops {

#ifdef USE_SYNC_IRP


		uint (*_sdbus_read)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

		uint (*_sdbus_read_bytes_to_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);
		uint (*_sdbus_read_blocks_to_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

		void (*_attrib_read)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);

		u8 (*_read8)(struct intf_hdl *pintfhdl, u32 addr);
		
		u16 (*_read16)(struct intf_hdl *pintfhdl, u32 addr);
		
		u32 (*_read32)(struct intf_hdl *pintfhdl, u32 addr);

		uint (*_sdbus_write)(struct intf_priv *pintfpriv, u32 addr, u32 cnt);

		uint (*_sdbus_write_blocks_from_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

		uint (*_sdbus_write_bytes_from_membuf)(struct intf_priv *pintfpriv, u32 addr, u32 cnt, u8 *pbuf);

		void (*_attrib_write)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);

		void (*_write8)(struct intf_hdl *pintfhdl, u32 addr, u8 val);
		
		void (*_write16)(struct intf_hdl *pintfhdl, u32 addr, u16 val);
		
		void (*_write32)(struct intf_hdl *pintfhdl, u32 addr, u32 val);

		void (*_read_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
		
		void (*_write_mem)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
		
		void (*_sync_irp_protocol_rw)(struct io_queue *pio_q);

#endif
		
		u8 (*_cmd52r)(struct intf_priv *pintfpriv, u32 addr);

		void (*_cmd52w)(struct intf_priv *pintfpriv, u32 addr, u8 val8);

		void (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
		
		u32 (*_write_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);

		u32 (*_write_scsi)(struct intf_hdl *pintfhdl,u32 cnt, u8 *pmem);
#ifdef USE_ASYNC_IRP
#if 1
		u8 (*_async_read8)(struct intf_hdl *pintfhdl, u32 addr);
		
		u16 (*_async_read16)(struct intf_hdl *pintfhdl, u32 addr);
		
		u32 (*_async_read32)(struct intf_hdl *pintfhdl, u32 addr);
				
		void (*_async_write8)(struct intf_hdl *pintfhdl, u32 addr, u8 val);
		
		void (*_async_write16)(struct intf_hdl *pintfhdl, u32 addr, u16 val);
		
		void (*_async_write32)(struct intf_hdl *pintfhdl, u32 addr, u32 val);
					
#endif 
		void (*_async_irp_protocol_write)(struct io_queue *pio_q);  //use async irp to perform protocol write

		void (*_async_irp_protocol_read)(struct io_queue *pio_q);  //use async irp to perform protocol read

#endif

};

struct io_req {	
	_list	list;
	u32	addr;	
	volatile u32	val;
	u32	command;
	u32	status;
	u8	*pbuf;
#ifdef	USE_ASYNC_IRP
	_sema	sema;
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt);
	u8 *cnxt;
#endif	
};

struct	intf_hdl {
	
	u32	intf_option;
	u32	bus_status;
	u32	do_flush;
	u8	*adapter;
	u8	*intf_dev;	
	struct intf_priv	*pintfpriv;
	u8	cnt;
	void (*intf_hdl_init)(u8 *priv);
	void (*intf_hdl_unload)(u8 *priv);
	void (*intf_hdl_open)(u8 *priv);
	void (*intf_hdl_close)(u8 *priv);
	struct	_io_ops	io_ops;
	//u8 intf_status;
	u16 len;
	u16 done_len;
	
};

struct reg_protocol_rd {

#ifdef CONFIG_LITTLE_ENDIAN	

	//DW1
	u32		NumOfTrans:4;
	u32		Reserved1:4;
	u32		Reserved2:24;
	//DW2
	u32		ByteCount:7;
	u32		WriteEnable:1;		//0:read, 1:write
	u32		FixOrContinuous:1;	//0:continuous, 1: Fix
	u32		BurstMode:1;
	u32		Byte1Access:1;
	u32		Byte2Access:1;
	u32		Byte4Access:1;
	u32		Reserved3:3;
	u32		Reserved4:16;
	//DW3
	u32		BusAddress;
	//DW4
	//u32		Value;
#else


//DW1
	u32 Reserved1  :4;
	u32 NumOfTrans :4;	

	u32 Reserved2  :24;	

	//DW2
	u32 WriteEnable : 1;
	u32 ByteCount :7;	


	u32 Reserved3 : 3;
	u32 Byte4Access : 1;	

	u32 Byte2Access : 1;
	u32 Byte1Access : 1;	
	u32 BurstMode :1 ;	
	u32 FixOrContinuous : 1;	

	u32 Reserved4 : 16;

	//DW3
	u32		BusAddress;

	//DW4
	//u32		Value;

#endif
	
};


struct reg_protocol_wt {
	

#ifdef CONFIG_LITTLE_ENDIAN

	//DW1
	u32		NumOfTrans:4;
	u32		Reserved1:4;
	u32		Reserved2:24;
	//DW2
	u32		ByteCount:7;
	u32		WriteEnable:1;		//0:read, 1:write
	u32		FixOrContinuous:1;	//0:continuous, 1: Fix
	u32		BurstMode:1;
	u32		Byte1Access:1;
	u32		Byte2Access:1;
	u32		Byte4Access:1;
	u32		Reserved3:3;
	u32		Reserved4:16;
	//DW3
	u32		BusAddress;
	//DW4
	u32		Value;

#else
	//DW1
	u32 Reserved1  :4;
	u32 NumOfTrans :4;	

	u32 Reserved2  :24;	

	//DW2
	u32 WriteEnable : 1;
	u32 ByteCount :7;	
		
	u32 Reserved3 : 3;
	u32 Byte4Access : 1;	

	u32 Byte2Access : 1;
	u32 Byte1Access : 1;	
	u32 BurstMode :1 ;	
	u32 FixOrContinuous : 1;	

	u32 Reserved4 : 16;

	//DW3
	u32		BusAddress;

	//DW4
	u32		Value;

#endif

};



/*
Below is the data structure used by _io_handler

*/

struct io_queue {	
	_lock	lock;	
	_list  	free_ioreqs;	
	_list		pending;		//The io_req list that will be served in the single protocol read/write.	
	_list		processing;
	u8	*free_ioreqs_buf; // 4-byte aligned
	u8	*pallocated_free_ioreqs_buf;
	struct	intf_hdl	intf;
};

static __inline u32 _RND4(u32 sz)
{

	u32	val;

	val = ((sz >> 2) + ((sz & 3) ? 1: 0)) << 2;
	
	return val;

}

///*int ioreq_enqueue(struct io_req *preq, _queue	*pending);

uint ioreq_flush(_adapter *adapter, struct io_queue *ioqueue);

//struct io_req *alloc_ioreq(struct io_queue *pio_q);

//int free_ioreq(struct io_req *preq, _queue *free_queue);
//*/

extern void sync_ioreq_enqueue(struct io_req *preq,struct io_queue *ioqueue);

extern uint sync_ioreq_flush(_adapter *adapter, struct io_queue *ioqueue);

extern uint free_ioreq(struct io_req *preq, struct io_queue *pio_queue);

extern struct io_req *alloc_ioreq(struct io_queue *pio_q);


extern uint register_intf_hdl(u8 *dev, struct intf_hdl *pintfhdl);

extern void unregister_intf_hdl(struct intf_hdl *pintfhdl);

extern void attrib_read(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void attrib_write(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);

extern u8 read8(_adapter *adapter, u32 addr);
extern u16 read16(_adapter *adapter, u32 addr);
extern u32 read32(_adapter *adapter, u32 addr);
extern void read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void write8(_adapter *adapter, u32 addr, u8 val);
extern void write16(_adapter *adapter, u32 addr, u16 val);
extern void write32(_adapter *adapter, u32 addr, u32 val);
extern void write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void write_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void write_scsi(_adapter *adapter, u32 cnt, u8 *pmem);
//ioreq 
extern void ioreq_read8(_adapter *adapter, u32 addr, u8 *pval);
extern void ioreq_read16(_adapter *adapter, u32 addr, u16 *pval);	
extern void ioreq_read32(_adapter *adapter, u32 addr, u32 *pval);
extern void ioreq_write8(_adapter *adapter, u32 addr, u8 val);
extern void ioreq_write16(_adapter *adapter, u32 addr, u16 val);
extern void ioreq_write32(_adapter *adapter, u32 addr, u32 val);

#ifdef USE_ASYNC_IRP

extern uint async_read8(_adapter *adapter, u32 addr, u8 *pbuff,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt); 
extern uint async_read16(_adapter *adapter, u32 addr,  u8 *pbuff,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt); 
extern uint async_read32(_adapter *adapter, u32 addr,  u8 *pbuff,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt); 

extern void async_read_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void async_read_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);

extern void async_write8(_adapter *adapter, u32 addr, u8 val,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt);
extern void async_write16(_adapter *adapter, u32 addr, u16 val,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt);
extern void async_write32(_adapter *adapter, u32 addr, u32 val,
	void (*_async_io_callback)(_adapter *padater, struct io_req *pio_req, u8 *cnxt), u8 *cnxt);

extern void async_write_mem(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);
extern void async_write_port(_adapter *adapter, u32 addr, u32 cnt, u8 *pmem);

#endif

extern uint	alloc_io_queue(_adapter *adapter);
extern void free_io_queue(_adapter *adapter);
extern void async_bus_io(struct io_queue *pio_q);
extern void bus_sync_io(struct io_queue *pio_q);
u32 _ioreq2rwmem(struct io_queue *pio_q);


#endif	//_RTL8711_IO_H_
