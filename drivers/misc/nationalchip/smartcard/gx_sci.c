#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/slab.h>
#include <linux/signal.h>

#include <linux/of_irq.h>

#include "gx_sci.h"
#include "smartcard_params.h"

static int if_output_debug_info = 0;
#define SCI_DBG(fmt, args...)	do{if(if_output_debug_info)printk(fmt, ##args);}while(0)

#define MISC_SCI_MINOR (MISC_DYNAMIC_MINOR)

static struct platform_device *sci_device;
static struct miscdevice sci_miscdev;

static struct work_struct sci_card_in_wq;
static struct work_struct sci_card_out_wq;

static int card_in_isr = 0;

static void sci_printf_regs(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;

	SCI_DBG("\n----------sci printf regs start-----------\n");
	SCI_DBG("rSCI_CTRL1:        %08x\n", regs->rSCI_CNTL1);
	SCI_DBG("rSCI_CTRL2:        %08x\n", regs->rSCI_CNTL2);
	SCI_DBG("rSCI_CTRL3:        %08x\n", regs->rSCI_CNTL3);
	SCI_DBG("rSCI_STATUS:       %08x\n", regs->rSCI_STATUS);
	SCI_DBG("rSCI_INTEN:        %08x\n", regs->rSCI_INTEN);
	SCI_DBG("rSCI_EGT:          %08x\n", regs->rSCI_EGT);
	SCI_DBG("rSCI_TGT:          %08x\n", regs->rSCI_TGT);
	SCI_DBG("rSCI_WDT:          %08x\n", regs->rSCI_WDT);
	SCI_DBG("rSCI_TWDT:         %08x\n", regs->rSCI_TWDT);
	SCI_DBG("rSCI_CFG_ACK2:     %08x\n", regs->rSCI_CFG_ACK2);
	SCI_DBG("rSCI_CFG_CLDRST:   %08x\n", regs->rSCI_CFG_CLDRST);
	SCI_DBG("rSCI_CFG_HOTRST:   %08x\n", regs->rSCI_CFG_HOTRST);
	SCI_DBG("rSCI_CFG_PARITY:   %08x\n", regs->rSCI_CFG_PARITY);
	SCI_DBG("\n----------sci printf regs end-----------\n");
}

#define WATI_FIFO_RESET_POLLED(_ctx_, _bit_, _us_) do {      \
	unsigned int sta,i = _us_;                     \
	do {                                            \
		sta = (_ctx_->rSCI_STATUS>>_bit_)&0x7F;     \
	}while((sta) && ((i--) > 0));                   \
} while(0);

static void inline sci_read_buffer_init(struct gxsci_info *info)
{
	info->sci_rbpos = 0;
	info->sci_rbleft = 0;
	info->sci_rdata_num = 0;
	info->sci_read_fifo_ready = 0;
	info->sci_write_fifo_empty = 1;
	return;
}

static void sci_poweron(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	regs->rSCI_CNTL2 |= SCI_CTL2_VCCEN;
}

static void sci_poweroff(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	regs->rSCI_CNTL2 &= ~SCI_CTL2_VCCEN;
}

static void sci_kill_async(struct gxsci_info *info)
{
	if (info->async_queue)
		kill_fasync(&info->async_queue, SIGIO, POLL_IN);
}
//-----------------------------------
static void sci_get_card_status(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;

	if ((regs->rSCI_STATUS & 1<<10) && card_in_isr == 1){
		//SCI_DBG("%s card in\n", __func__);
		info->card_status = GXSMART_CARD_IN;
	}else{
		//SCI_DBG("%s card out\n", __func__);
		info->card_status = GXSMART_CARD_OUT;
	}
}

static int sci_select_card(struct gxsci_info *info, void *buf)
{
	GXSMART_SCI_t sci_select;
	sci_select = *(GXSMART_SCI_t *)buf;
	if (sci_select > GXSMART_SCI2)
		return -EINVAL;

	return 0;
}

static void sci_card_in_out_process(struct gxsci_info *info, u32 status)
{
	struct sci_regs __iomem *regs = info->regs;

	/* NOTE: here's one hardware bug (when card inserted/plugged out, rSCI_STATUS[0]
	 * & rSCI_STATUS[1] MAY be set simultaneously, and two interrupts triggered).
	 * One interrupt should be ignored by checking rSCI_STATUS[10] */
	if ((status & SCI_STATUS_SCOUT_FOR_INVERTED) /* rSCI_STATUS[0] */
			|| (status & SCI_STATUS_SCIN_FOR_INVERTED)) /* rSCI_STATUS[1] */
	{
		regs->rSCI_STATUS &= (SCI_STATUS_SCIN_FOR_INVERTED|SCI_STATUS_SCOUT_FOR_INVERTED);
		/* call SCOUT callback in DSR if card plugged out */
		if (!(regs->rSCI_STATUS&1<<10))
		{
			card_in_isr = 0;
			info->interrupt_status = SCI_INTSTATUS_SCIOUT;
			info->card_status = GXSMART_CARD_OUT;
			sci_kill_async(info);
		        // SCI MODULE RESET
			regs->rSCI_CNTL1 |= SCI_CTL1_RST;
                        // EXIT SELECT FROM APP
                        info->sci_read_fifo_ready = 1;
                        wake_up_interruptible(&info->r_wait);
                        info->sci_rdata_num =0;

                        sci_poweroff(info);
			schedule_work(&sci_card_out_wq);
			SCI_DBG("%s: card out\n", __func__);
		}
		else if ((regs->rSCI_STATUS&1<<10))
		{
			sci_poweron(info);
			info->interrupt_status = SCI_INTSTATUS_SCIIN;
			info->card_status = GXSMART_CARD_IN;
			sci_kill_async(info);
			card_in_isr = 1;
			schedule_work(&sci_card_in_wq);
			SCI_DBG("%s: card in\n", __func__);
		}
	}
}

static struct gxsci_extra_ops gxsci_ops_v2 = {
	.get_card_status = sci_get_card_status,
	.select_card = sci_select_card,
	.card_in_out_process = sci_card_in_out_process,
};
//notify udev/mdev to road modual
static void sci_card_in_wq_func(struct work_struct* pdata)
{
	kobject_uevent(&sci_device->dev.kobj, KOBJ_ADD);
}
static void sci_card_out_wq_func(struct work_struct* pdata)
{
	/*使用常节点模式，智能卡拔出时不再通知，删除节点 */
   //kobject_uevent(&sci_device.dev.kobj, KOBJ_REMOVE);
}

/*
 * TX FIFO level intr
 * */
static void sci_tx_intr(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	u32 len = 0, i = 0;
	u8 *wbbuf = info->sci_wbuffer;

	wbbuf += info->sci_wbpos;
	len = ((regs->rSCI_STATUS & 0x7F0000) >> 16);
	SCI_DBG("%s:reg len=%d\n", __func__, len);
	len = (len >= 64) ? 0: (64-len);
	len = (info->sci_wdata_num >= len) ? len:info->sci_wdata_num;

	info->sci_write_fifo_empty = 0;
	for (i = 0; i < len; i++)
		regs->rSCI_DATA = *wbbuf++;
	info->sci_wbpos += len;
	info->sci_wdata_num -= len;
	if (info->sci_wdata_num == 0)
	{
		regs->rSCI_INTEN &= ~(TX_FIFO_LEVEL_STATUS_INTEN);
		regs->rSCI_INTEN |= TX_FIFO_EMPTY_INTEN;
	}
	SCI_DBG("%s:sci_wdata_num=%d\n", __func__, info->sci_wdata_num);
}
/*
 * TX FIFO empty intr,so tx finish
 * */
static void sci_tx_intr_finish(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;

	info->sci_wdata_num = 0;
	info->sci_wbpos = 0;
	regs->rSCI_INTEN &= ~(TX_FIFO_EMPTY_INTEN);

	//complete(&info->write_comp);
	info->sci_write_fifo_empty = 1;
	wake_up_interruptible(&info->w_wait);
}

/*
 * RX FIFO level
 * */
static void sci_rx_intr(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	u8 *rbbuf = info->sci_rbuffer;
	u32 len = 0;
    u8 tmp=0;

    len = (regs->rSCI_STATUS & 0x7F0000) >> 16;
	if (((info->sci_rdata_num)+len) <= BUFFER_MAX_SIZE)
	{
		rbbuf += info->sci_rbpos;
		while (len > 0)
		{
			*rbbuf++ = (u8)regs->rSCI_DATA;
			info->sci_rbpos++;
			len--;
			SCI_DBG("0x%x ", *(rbbuf-1));
		}
		SCI_DBG("\n");
		write_lock(&info->read_rwlock);
		info->sci_rdata_num = info->sci_rbpos;
		//SCI_DBG("[sci module] 1 sci_rdata_num = %d\n",info->sci_rdata_num);
		write_unlock(&info->read_rwlock);
	}
    else
    {
        sci_poweroff(info);
		while (len > 0)
		{
			tmp = (u8)regs->rSCI_DATA;
			len--;
		}
    }
}

/*
 * RX finish
 * */
static void sci_rx_intr_finish(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	u8 *rbbuf = info->sci_rbuffer;
	u32 len = 0;

	rbbuf += info->sci_rbpos;
	len = ((regs->rSCI_STATUS & 0x7F0000) >> 16);
	SCI_DBG("%s:reg read len=%d\n", __func__, len);
	write_lock(&info->read_rwlock);
	info->sci_rdata_num += len;
	SCI_DBG("[sci module] 1 sci_rdata_num = %d\n",info->sci_rdata_num);
	write_unlock(&info->read_rwlock);
	while (len > 0)
	{
		if (info->sci_rdata_num < BUFFER_MAX_SIZE) {
			*rbbuf++ = (u8)regs->rSCI_DATA;
			SCI_DBG("0x%x ", *(rbbuf-1));
		}
		len--;
	}
	info->sci_rbpos = 0;
	SCI_DBG("\n");

	/* Some card would ack 0x60 when command is long enouth, in this case, it
	 * should setup a read opearation actively at once. */
	if ((info->sci_rdata_num == 1) && (info->sci_rbuffer[0] == 0x60)) {
		info->interrupt_status = SCI_INTSTATUS_INVALID;

		sci_read_buffer_init(info);

		//改版后的智能卡控制器会一直处于接收模式，不需要重新打开接收模式
		if(!(regs->rSCI_CNTL1 & SCI_CTL1_RCV))
			regs->rSCI_CNTL1 |= SCI_CTL1_RE;

		SCI_DBG("rec 0x60.\n");
	} else {
		/* ONLY when all data received, send signal to wake up pended rx task */
		info->sci_read_fifo_ready = 1;
		wake_up_interruptible(&info->r_wait);
		complete(&info->read_comp);
	}
}

/*
 * ERROR intr
 * */
static void sci_card_intr_error(struct gxsci_info *info, sci_interrupt_status_t status)
{
	struct sci_regs __iomem *regs = info->regs;


	switch (status)
	{
		case SCI_INTSTATUS_CARD_NOACK:
			regs->rSCI_INTEN &= ~(CARD_NACK_INTEN);
			break;
		case SCI_INTSTATUS_REPEAT_ERROR:
			regs->rSCI_INTEN &= ~(REPEAT_ERROR_INTEN);
			break;
		case SCI_INTSTATUS_RX_FIFO_OVERFLOW:
			regs->rSCI_INTEN &= ~(RX_FIFO_OVERFLOW_INTEN);
			break;
		default:
			break;
	}
	info->interrupt_status = status;
	info->sci_write_fifo_empty = 1;

}

static int card_no_present(struct gxsci_info *info)
{
	return info->card_status != GXSMART_CARD_IN;
}

static void fifo_reset(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;

	regs->rSCI_CNTL1 |= SCI_CTL1_DATAFIFO_RST;
	WATI_FIFO_RESET_POLLED(regs, 16, 100000);
	return;
}

/*
 * Check error state
 * */
static int sci_interrupt_status_check(struct gxsci_info *info)
{
	struct sci_regs __iomem *regs = info->regs;
	int res = 0;
	switch (info->interrupt_status)
	{
		case SCI_INTSTATUS_CARD_NOACK:
			regs->rSCI_INTEN |= CARD_NACK_INTEN;
			SCI_DBG("sci error: card no ack!\n");
			res = -ENODEV;
			break;
		case SCI_INTSTATUS_REPEAT_ERROR:
			regs->rSCI_INTEN |= REPEAT_ERROR_INTEN;
			SCI_DBG("sci error: repeat error!\n");
			res = -EINVAL;
			break;
		case SCI_INTSTATUS_RX_FIFO_OVERFLOW:
			regs->rSCI_INTEN |= RX_FIFO_OVERFLOW_INTEN;
			SCI_DBG("sci error: receive fifo overflow!\n");
			res = -ENOMEM;
			break;
		case SCI_INTSTATUS_SCIOUT:
			SCI_DBG("sci error: plugged out while operating!\n");
			res = -ENODEV;
			break;
			/* nothing for mis-triggered status */
		default:
			break;
	}
	return res;
}

ssize_t sci_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	struct gxsci_info *info = filp->private_data;
	size_t size = count;
	u32 len = 0, rev_len = 0;
	u32 res = 0;
	int ret;
	DECLARE_WAITQUEUE(wait,current);

	SCI_DBG("call %s\n", __func__);
	if (!info)
		return 0;

	mutex_lock(&info->lock);
	add_wait_queue(&info->r_wait,&wait);
	res = card_no_present(info);
	if (res) {
		SCI_DBG("sci: no card inserted\n");
		ret = -1;
		goto out;
	}
	SCI_DBG(KERN_INFO "[sci module]sci_read_fifo_ready = %d\n",info->sci_read_fifo_ready);
#if 1
	while(info->sci_read_fifo_ready == 0){
		if(filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			SCI_DBG("error: %s(%d)\n", __func__, __LINE__);
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&info->lock);
		schedule();
		if(signal_pending(current)){
			ret = -ERESTARTSYS;
			SCI_DBG("error: %s(%d)\n", __func__, __LINE__);
			goto out1;
		}
		mutex_lock(&info->lock);
	}
#endif
	/* wait until all data received */
	/* NOTE: if any error occurs, the data will be incorrect */

	/* handling any possible errors */
	ret = sci_interrupt_status_check(info);
	if (ret < 0){
		SCI_DBG("%s:%d status check error.\n", __func__, __LINE__);
		goto out;
	}

	/* correct read size if user not specified */
	if (!size)
		size = BUFFER_MAX_SIZE;

	/* reading out all correct data */
	rev_len = info->sci_rdata_num;
	len = (size >= rev_len) ? rev_len : size;
	if (len == 0) {
		ret = -1;
		SCI_DBG("sci_read: no data received\n");
		goto out;
	}

	//SCI_DBG(KERN_INFO "[sci module] len = %d\n",len);
	if (copy_to_user(buf, info->sci_rbuffer + info->sci_rbleft, len)) {
		ret = -EFAULT;
		SCI_DBG("error: copy_to_user failed.\n");
		goto out;
	}
	ret = len;
out:
	info->interrupt_status = SCI_INTSTATUS_INVALID;
	//sci_read_buffer_init(info);
	//info->sci_rbpos = 0;
	info->sci_rbleft += len;
	write_lock(&info->read_rwlock);
	info->sci_rdata_num -= len;
	write_unlock(&info->read_rwlock);
	if(info->sci_rdata_num == 0){
		info->sci_read_fifo_ready = 0;
		info->sci_rbleft = 0;
	}
	SCI_DBG("%s:rbleft=%d len=%d sci_rdata_num=%d\n", __func__, info->sci_rbleft, len, info->sci_rdata_num);
	mutex_unlock(&info->lock);
out1:
	remove_wait_queue(&info->r_wait,&wait);
	set_current_state(TASK_RUNNING);

	return ret;
}

ssize_t sci_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
	struct gxsci_info *info = filp->private_data;
	struct sci_regs __iomem *regs = info->regs;
	u32 size = count;
	u32 length = 0, i = 0, res = 0;
	u8 *cmd_buf = NULL;
	u8 *bbuf = NULL;
	int ret = -1;
	DECLARE_WAITQUEUE(wait, current);

	SCI_DBG("call %s: count = %d\n", __func__, count);
	if (!info)
		return 0;

	mutex_lock(&info->lock);
	add_wait_queue(&info->w_wait, &wait);
	res = card_no_present(info);
	if (res) {
		SCI_DBG("sci write: no card inserted\n");
		ret = -1;
		mutex_unlock(&info->lock);
		goto out;
	}
	while(info->sci_write_fifo_empty == 0){
		if(filp->f_flags & O_NONBLOCK){
			ret = -EAGAIN;
			mutex_unlock(&info->lock);
			SCI_DBG("error: %s(%d)\n", __func__, __LINE__);
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&info->lock);
		schedule();
		if(signal_pending(current)){
			ret = -ERESTARTSYS;
			SCI_DBG("error: %s(%d)\n", __func__, __LINE__);
			goto out1;
		}
		mutex_lock(&info->lock);
	}

	cmd_buf = kmalloc(size+2,GFP_KERNEL);
	if (!cmd_buf) {
		SCI_DBG("sci write:alloc mem error\n");
		ret = -ENOMEM;
		goto out;
	}

	if(copy_from_user(cmd_buf, buf, size)) {
		SCI_DBG("sci write: copy_from_user occur error\n");
		res = -EFAULT;
		goto out;
	}
	bbuf = cmd_buf;

	SCI_DBG("write data: ");
	for(i = 0; i < count; i++){
		SCI_DBG("0x%02x ", bbuf[i]);
	}
	SCI_DBG("\n");
	fifo_reset(info);

	//改版后智能卡控制器会接收到自己写入的数据，需要发送数据前关闭接收中断
	if(regs->rSCI_CNTL1 & SCI_CTL1_RCV)
		regs->rSCI_INTEN &= ~RX_FIFO_LEVEL_STATUS_INTEN;

	/* Send data */
	length = (size > 64) ? 64 : size;
	ret = size;
	for (i = 0; i < length; i++){
		regs->rSCI_DATA = *bbuf++;
	}
	/* open interrupt */
	if (size <= 64) { /* just enable intr TX_FIFI_EMPTY */
		regs->rSCI_INTEN &= ~TX_FIFO_LEVEL_STATUS_INTEN;
		regs->rSCI_INTEN |= TX_FIFO_EMPTY_INTEN;
	}else{
		regs->rSCI_INTEN |= TX_FIFO_LEVEL_STATUS_INTEN;
		regs->rSCI_INTEN |= TX_FIFO_EMPTY_INTEN;
	}

	/* set offset */
	size -= length;
	if (size > 0) {
		memset(info->sci_wbuffer, 0, BUFFER_MAX_SIZE);
		info->sci_wbpos = 0;
		info->sci_wdata_num = size;
		SCI_DBG("%s sci_wdata_num=%d\n", __func__, info->sci_wdata_num);
		memcpy(info->sci_wbuffer, bbuf, size);
	} else {
		info->sci_wbpos = 0;
		info->sci_wdata_num = 0;
	}
	info->sci_write_fifo_empty = 0;
	regs->rSCI_CNTL1 |= SCI_CTL1_TE;

	if(sci_interrupt_status_check(info) != 0)
		ret = -1;
out:
	info->interrupt_status = SCI_INTSTATUS_INVALID;

	if (cmd_buf) {
		kfree(cmd_buf);
		cmd_buf = NULL;
	}

	mutex_unlock(&info->lock);

out1:
	remove_wait_queue(&info->w_wait,&wait);
	set_current_state(TASK_RUNNING);
	return ret;
}
static int sci_get_config(struct gxsci_info *info,
		u32 key,
		const void *buffer,
		u32 len)
{
	struct sci_regs __iomem *regs = info->regs;
	int val;
	GXSMART_OpenParams_t  *smartopen;
	if(!info)
		return -EBADF;
	switch(key)
	{
		case SCI_GETCONFIG_OPEN:
			smartopen = (GXSMART_OpenParams_t *)buffer;
			smartopen->Protocol = ((regs->rSCI_CNTL2 & (1<<4)) >> 4);
			smartopen->ParityType = ((regs->rSCI_CNTL2 & (1<<7)) >> 7);
			smartopen->IoConv = ((regs->rSCI_CNTL2 & (1<<5)) >> 5);
			val = (regs->rSCI_CNTL3 & 0xff);
			smartopen->BaudRate = info->sys_clk/2/(val+1);
			smartopen->Etu = (regs->rSCI_CNTL3 >> 20) + 1;
			smartopen->TimeParams.Twdt = regs->rSCI_TWDT;
			smartopen->TimeParams.Wdt = regs->rSCI_WDT;
			smartopen->TimeParams.Egt = (regs->rSCI_EGT & 0xfffff);
			smartopen->TimeParams.Tgt = (regs->rSCI_TGT & 0xfffff);
			smartopen->AutoEtu = !((regs->rSCI_CNTL2&(1<<27))>>27);
			smartopen->AutoIoConvAndParity = !((regs->rSCI_CNTL2&(1<<28))>>28);
			break;

		default:
			break;
	}
	return 0;
}
static int sci_set_config(struct gxsci_info *info,
		u32 key,
		const void *buffer,
		u32 len)
{
	struct sci_regs __iomem *regs = info->regs;
	GXSMART_ConfigFifoLevel_t	 *smartfifolever;
	GXSMART_ConfigIntPart_t		 *smartintpart;
	GXSMART_OpenParams_t		 *smartopen;
	GXSMART_DetectPol_t			 cd_polarity;
	unsigned char				 val;
	unsigned int clk = 0;

	if (!info)
		return -EBADF;
	switch (key)
	{
		case SCI_SETCONFIG_FIFOLEVEL:
			smartfifolever	=	(GXSMART_ConfigFifoLevel_t *)buffer;
			if(len	!=	sizeof(GXSMART_ConfigFifoLevel_t))
				return -EDOM;

			regs->rSCI_CNTL2 &= ~(SCI_CTL2_RX_FIFO_LEVEL(0x3F) | SCI_CTL2_TX_FIFO_LEVEL(0x3F));
			regs->rSCI_CNTL2 |= (SCI_CTL2_RX_FIFO_LEVEL(smartfifolever->rx_fifolevel)
					| SCI_CTL2_TX_FIFO_LEVEL(smartfifolever->tx_fifolevel));

			break;
		case SCI_SETCONFIG_INTPART:
			smartintpart			=	(GXSMART_ConfigIntPart_t *)buffer;
			if(len	!=	sizeof(GXSMART_ConfigIntPart_t))
				return -EDOM;

			/* clear INT status at the beginning */
			regs->rSCI_STATUS = regs->rSCI_STATUS;
			regs->rSCI_INTEN	=	smartintpart->intpart;
			break;

		case SCI_SETCONFIG_OPEN:
			smartopen			=	(GXSMART_OpenParams_t *)buffer;
			if (len!=sizeof(GXSMART_OpenParams_t))
				return -EDOM;

			regs->rSCI_CNTL2 &= ~(SCI_CTL2_PARITY(1)|SCI_CTL2_IO_CONV(1)|SCI_CTL2_PT(1));
			/* Set to the way which the mode of parity is controlled by software as default.
			 * When etu in applications is not set, it would be assigned to 372. */
			regs->rSCI_CNTL2 |= (SCI_CTL2_PARITY(smartopen->ParityType)
					|SCI_CTL2_IO_CONV(smartopen->IoConv)
					|SCI_CTL2_PT(smartopen->Protocol));

			regs->rSCI_CNTL3 &= ~(SCI_CTL3_SCI_ETU(0xFFF)|SCI_CTL3_REPEAT_TIME(0xF));
			if (smartopen->Etu)
				regs->rSCI_CNTL3 |= SCI_CTL3_SCI_ETU(smartopen->Etu - 1);
			else
				regs->rSCI_CNTL3 |= SCI_CTL3_SCI_ETU(371);

			regs->rSCI_CNTL3 |= SCI_CTL3_REPEAT_TIME(1);
			clk = regs->rSCI_CNTL3;
			clk &= 0xffffff00;
			clk |= (info->sys_clk/2/(smartopen->BaudRate)- 1);
			regs->rSCI_CNTL3 = clk;
			/* auto etu and if set, etu,ioconv,parity seted before will ignore*/
			regs->rSCI_CNTL2 |= (SCI_CTL2_AUTO_ETU(1)|SCI_CTL2_AUTO_PARITY_IOCONV(1));
			if (smartopen->AutoEtu)
				regs->rSCI_CNTL2 &= ~SCI_CTL2_AUTO_ETU(1);
			if (smartopen->AutoIoConvAndParity)
				regs->rSCI_CNTL2 &= ~SCI_CTL2_AUTO_PARITY_IOCONV(1);


			regs->rSCI_EGT = smartopen->TimeParams.Egt;
			regs->rSCI_TGT = smartopen->TimeParams.Tgt;		//TGT
			regs->rSCI_WDT = smartopen->TimeParams.Wdt;
			regs->rSCI_TWDT = smartopen->TimeParams.Twdt;
			break;
		case SCI_SETCONFIG_CD_POLARITY:
			cd_polarity = *(GXSMART_DetectPol_t*)buffer;

			regs->rSCI_CNTL2 |= (SCI_CTL2_DETECT_POL(1));
			if(cd_polarity)
				regs->rSCI_CNTL2 &= ~(SCI_CTL2_DETECT_POL(1));

			if (cd_polarity) /* none-inverted: one hardware resistor pull up -> pull down */
				info->sci_cd_inverse = 0;
			else
				info->sci_cd_inverse = 1;
			break;

		case SCI_SETCONFIG_EN:
			val			=	*(unsigned char*)buffer;
			regs->rSCI_CNTL2	|= val<<10;
			break;

		case SCI_SETCONFIG_AUTOREC:
			val			=	*(unsigned char*)buffer;
			regs->rSCI_CNTL2	|= val<<9;
			break;

		case SCI_SETCONFIG_DATAFIFORESET:
			val = *(unsigned char*)buffer;
			regs->rSCI_CNTL1	&= ~(1<<5);
			regs->rSCI_CNTL1	|= (val)<<5;

			break;
		case SCI_SETCONFIG_MODULERESET:
			val = *(unsigned char*)buffer;
			regs->rSCI_CNTL1 &= ~(1<<6);
			regs->rSCI_CNTL1 |= (val)<<6;
			break;
		case SCI_SETCONFIG_NO_PARITY:
			val = *(unsigned char*)buffer;
			regs->rSCI_CNTL2 &= ~(1<<8);
			regs->rSCI_CNTL2 |= (val<<8);
			break;
		case SCI_SETCONFIG_STOPLEN:
			val = *(unsigned char*)buffer;
			regs->rSCI_CNTL2 &= ~(0x3<<11);
			regs->rSCI_CNTL2 |= (val<<11);
			break;
		case SCI_SETCONFIG_AUTOETU:
			regs->rSCI_CNTL2 &= ~(1<<27);
			break;
		case SCI_SETCONFIG_AUTOCONV:
			regs->rSCI_CNTL2 &= ~(1<<28);
			break;
		default:
			break;
	}
	return 0;
}

static int sci_reset(struct gxsci_info *info, int cmd, unsigned long arg)
{
	struct sci_regs __iomem *regs = info->regs;
	u32 res = 0;

	SCI_DBG("call %s\n", __func__);
	if (!info)
		return 0;

	res = card_no_present(info);
	if (res) {
		SCI_DBG("sci: no card inserted\n");
		return -1;
	}

	/* power on card now */
	sci_poweron(info);

	mutex_lock(&info->lock);

	sci_read_buffer_init(info);

	/* reset the module first, always */
	/* NOTE: maybe some card-in interrupts will be triggered for verilog bugs */
	fifo_reset(info);

	SCI_DBG("Smartcard %s-reset...\n", (cmd == IOCTL_SET_RESET)?"cold":"hot");
	regs->rSCI_CNTL1 |= (cmd == IOCTL_SET_RESET)?SCI_CTL1_CLDRST:SCI_CTL1_HOTRST;
	//regs->rSCI_CNTL1 |= (cmd == IOCTL_SET_RESET)?SCI_CTL1_HOTRST:SCI_CTL1_CLDRST;

	info->interrupt_status = SCI_INTSTATUS_INVALID;
	mutex_unlock(&info->lock);
	return res;
}

static long sci_ioctl(struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct gxsci_info *info = filp->private_data;
	struct sci_regs __iomem *regs = info->regs;
	u8 buf[128];
	unsigned int val;
	int err = 0;
	int retval = 0;

	if (!info)
		return 0;
	switch(cmd) {
		case IOCTL_SHOW_REGS:
			sci_printf_regs(info);
			break;
		case IOCTL_SET_DEBUG:
			if (copy_from_user(&val, (void __user *)arg, sizeof(int)))
				return -EFAULT;
			if_output_debug_info = val;
			SCI_DBG("Smartcard: enable to output debug info!\n");
			break;

		case IOCTL_SET_RESET:
			retval = sci_reset(info, cmd, arg);
			if (retval < 0) {
				SCI_DBG("sci reset: failed\n");
				return retval;
			}
			break;
		case IOCTL_SET_PARAMETERS:
			if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_OpenParams_t)))
				return -EFAULT;

			sci_set_config(info,SCI_SETCONFIG_OPEN,buf,sizeof(GXSMART_OpenParams_t));
			break;
		case IOCTL_GET_PARAMETERS:
			memset(buf,0,sizeof(buf));
			sci_get_config(info,SCI_GETCONFIG_OPEN,buf,sizeof(GXSMART_OpenParams_t));
			if(copy_to_user((void __user*)arg,buf,sizeof(GXSMART_OpenParams_t)))
				return -EFAULT;
			break;
		case IOCTL_SET_STOPLEN:
			if (copy_from_user(buf, (void __user *)arg, sizeof(unsigned char)))
				return -EFAULT;
			sci_set_config(info,SCI_SETCONFIG_STOPLEN,buf,sizeof(unsigned char));
			break;
		case IOCTL_SET_IOCONV:
			{
				GXSMART_DataConv_t v;
				if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_DataConv_t)))
					return -EFAULT;
				v  = *(GXSMART_DataConv_t *)buf;
				if (v == GXSMART_DATA_CONV_DIRECT)
					regs->rSCI_CNTL2 &=	~SCI_CTL2_IO_CONV(1);
				else
					regs->rSCI_CNTL2 |=	SCI_CTL2_IO_CONV(1);
			}
			break;
		case IOCTL_SET_PARITY_TYPE:
			{
				GXSMART_ParityType_t v;
				if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_ParityType_t)))
					return -EFAULT;
				v = *(GXSMART_ParityType_t *)buf;
				if (v == GXSMART_PARITY_ODD)
					regs->rSCI_CNTL2 &=	~SCI_CTL2_PARITY(1);
				else
					regs->rSCI_CNTL2 |=	SCI_CTL2_PARITY(1);

			}
			break;

		case IOCTL_SET_PROTOCOL:
			{
				GXSMART_Protocol_t v;
				if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_ParityType_t)))
					return -EFAULT;
				v = *(GXSMART_Protocol_t *)buf;
				if (v == GXSMART_T0_PROTOCOL)
					regs->rSCI_CNTL2 &=	~SCI_CTL2_PT(1);
				else
					regs->rSCI_CNTL2 |=	SCI_CTL2_PARITY(1);

			}
			break;
		case IOCTL_SET_FRE:
			{
				unsigned int tmp_v;
				unsigned int v;
				if (copy_from_user(buf, (void __user *)arg, sizeof(unsigned int)))
					return -EFAULT;
				v = *(unsigned int *)buf;
				tmp_v = regs->rSCI_CNTL3;
				tmp_v &= 0xffffff00;
				tmp_v |= (info->sys_clk/2/(v) - 1);
				regs->rSCI_CNTL3 = tmp_v;

			}
			break;


		case IOCTL_SET_NOPARITY:
			if (copy_from_user(buf, (void __user *)arg, sizeof(unsigned char)))
				return -EFAULT;
			sci_set_config(info,SCI_SETCONFIG_NO_PARITY,buf,sizeof(unsigned char));
			break;
		case IOCTL_GET_IS_CARD_PRESENT:
			info->ops->get_card_status(info);
			if(put_user(info->card_status,(unsigned int*)arg))
				return -EFAULT;
			break;
		case IOCTL_GET_IS_CARD_ACTIVATED:
			if(put_user(1,(unsigned int*)arg))
				return -EFAULT;
			break;
		case IOCTL_SET_DEACTIVATE:
			regs->rSCI_CNTL1 |= SCI_CTL1_DEACT;
			break;
		case IOCTL_SET_AUTOETU:
			sci_set_config(info,SCI_SETCONFIG_AUTOETU,NULL,0);
			break;
		case IOCTL_SET_AUTOCONV:
			sci_set_config(info,SCI_SETCONFIG_AUTOCONV,NULL,0);
			break;
		case IOCTL_SET_ETU:
			if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			regs->rSCI_CNTL3 &= ~(0xfff << 20);
			regs->rSCI_CNTL3 |= ((val-1) << 20);
			break;
		case IOCTL_SET_CWT:
			if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			regs->rSCI_WDT = val;
			break;
		case IOCTL_SET_BWT:
			if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			regs->rSCI_TWDT = val;
			break;
		case IOCTL_SET_EGT:
			if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			regs->rSCI_EGT = val;
			break;
		case IOCTL_SET_TGT:
			if (copy_from_user(&val, (void __user *)arg, sizeof(unsigned int)))
				return -EFAULT;
			regs->rSCI_TGT = val;
			break;
		case IOCTL_SET_ATR_READY:
			break;
		case IOCTL_SET_SELECT_SCI:
			if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_SCI_t)))
				return -EFAULT;
			err = info->ops->select_card(info, (void *)buf);
			if (err < 0)
				return -EFAULT;
			break;
		case IOCTL_SET_VCCEN_POLARITY:
			if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_VccPol_t)))
				return -EFAULT;
			if (*(GXSMART_VccPol_t*)buf == GXSMART_VCC_HIGH_LEVEL)
				regs->rSCI_CNTL2 &= ~(SCI_CTL2_VCCEN_POL(1));
			else
				regs->rSCI_CNTL2 |= (SCI_CTL2_VCCEN_POL(1));
			break;
		case IOCTL_SET_CD_POLARITY:
			if (copy_from_user(buf, (void __user *)arg, sizeof(GXSMART_DetectPol_t)))
				return -EFAULT;
			sci_set_config(info,SCI_SETCONFIG_CD_POLARITY,buf,sizeof(GXSMART_DetectPol_t));
			break;
		case IOCTL_SET_MODES:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_SET_MODES cmd \n");
			break;
		case IOCTL_GET_MODES:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_GET_MODES cmd \n");
			break;
		case IOCTL_SET_CLOCK_START:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_SET_CLOCK_START cmd \n");
			break;
		case IOCTL_SET_CLOCK_STOP:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_SET_CLOCK_STOP cmd \n");
			break;
		case IOCTL_GET_ATR_STATUS:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_GET_ATR_STATUS cmd \n");
			break;
		case IOCTL_DUMP_REGS:
			SCI_DBG(KERN_INFO "[sci module] IOCTL_DUMP_REGS cmd \n");
			break;
		case IOCTL_SET_POWER_OFF:
			sci_poweroff(info);
			break;
		default:
			SCI_DBG(KERN_INFO "[sci module] unknow ioctl cmd \n");
			return -ENOTTY;
			break;
	}

	return 0;
}

static int sci_open(struct inode *inode, struct file *filp)
{
	GXSMART_ConfigFifoLevel_t fifolevel;
	GXSMART_ConfigIntPart_t smartinttype;
	GXSMART_ConfigAutoRec_t	smartautorec;
	GXSMART_ConfigEn_t en_params;
	int err = 0;

	struct gxsci_info *info = platform_get_drvdata(sci_device);
	struct sci_regs __iomem *regs = info->regs;
	if (!info)
		return 0;

	regs->rSCI_CNTL1 |= SCI_CTL1_RST;


	fifolevel.rx_fifolevel				=	16;
	fifolevel.tx_fifolevel				=	48;
	smartinttype.intpart				=	(RECEIVE_OVER_INTEN
			|RX_FIFO_LEVEL_STATUS_INTEN
			|SCIN_INTEN
			|SCOUT_INTEN
			|CARD_NACK_INTEN
			|REPEAT_ERROR_INTEN
			|RX_FIFO_OVERFLOW_INTEN);

	smartautorec.module_auto			=	SCI_AUTOREC;
	en_params.module_en					=	ENABLE;

	err  = sci_set_config(info,SCI_SETCONFIG_FIFOLEVEL,&fifolevel,sizeof(GXSMART_ConfigFifoLevel_t));
	err += sci_set_config(info,SCI_SETCONFIG_INTPART,&smartinttype,sizeof(GXSMART_ConfigIntPart_t));
	err += sci_set_config(info,SCI_SETCONFIG_AUTOREC,&smartautorec,sizeof(GXSMART_ConfigAutoRec_t));
	err += sci_set_config(info,SCI_SETCONFIG_EN,&en_params,sizeof(GXSMART_ConfigEn_t));

	sci_printf_regs(info);
	/* poweroff card at first, always */
	/* NOTE: power on/off MUST be supported by hardware, or rSCI_CNTL2[13] won't work */
	/* NOTE: power on/off is controlled by rSCI_CNTL2[13](in driver) and rSCI_CNTL2[3](in app) */
	sci_poweroff(info);
	filp->private_data = info;

	return err;
}
static unsigned int sci_poll(struct file *filp,struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct gxsci_info *info = filp->private_data;
	poll_wait(filp,&info->r_wait,wait);
	poll_wait(filp,&info->w_wait,wait);
	if(info->sci_read_fifo_ready == 1){
		mask |= (POLLIN | POLLRDNORM);
	}
	if(info->sci_write_fifo_empty == 1){
		mask |= (POLLOUT | POLLWRNORM);
	}
	//SCI_DBG(KERN_INFO "sci_read_fifo_ready = %d \n",info->sci_read_fifo_ready);
	//SCI_DBG(KERN_INFO "sci_write_fifo_empty= %d \n",info->sci_write_fifo_empty);
	return mask;
}

static int sci_fasync(int fd, struct file *filp, int mode)
{
	struct gxsci_info *info = filp->private_data;

	return fasync_helper(fd, filp, mode, &info->async_queue);
}

static int sci_release(struct inode *inode, struct file *filp)
{   struct gxsci_info *info = filp->private_data;
	struct sci_regs __iomem *regs = info->regs;

	SCI_DBG("close sci...\n");
	if (!info)
		return 0;

	sci_fasync(-1, filp, 0);

	regs->rSCI_CNTL2 &= ~(1<<10);

	return 0;
}

static irqreturn_t sci_isr(int irq, void *dev_id)
{
	u32 status = 0;
	u32 int_en = 0;
	struct gxsci_info *info = dev_id;
	struct sci_regs __iomem *regs = info->regs;
	status = regs->rSCI_STATUS;
	int_en = regs->rSCI_INTEN;
	SCI_DBG("%s:status=%#x int_en=%#x\n",__FUNCTION__,status, int_en);

	/* indicating TX finished process FIFO_EMPTY first or it maybe err*/
	if (status & int_en & SCI_STATUS_TX_FIFO_LEVEL_STATUS) {
		SCI_DBG("---- SCI_STATUS_TX_FIFO_LEVEL_STATUS [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_TX_FIFO_LEVEL_STATUS;
		sci_tx_intr(info);
	}
	if (status & int_en& SCI_STATUS_TX_FIFO_EMPTY) {
		SCI_DBG("---- SCI_STATUS_TX_FIFO_EMPTY [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_TX_FIFO_EMPTY;//clear intr state step by setp

		//改版后智能卡控制器会接收到自己写入的数据，需要发送数据前关闭接收中断
		if(regs->rSCI_CNTL1 & SCI_CTL1_RCV)
			regs->rSCI_INTEN |= RX_FIFO_LEVEL_STATUS_INTEN;

		sci_tx_intr_finish(info);
	}

	/* check any error first */
	if (status & int_en & SCI_STATUS_CARD_NACK) {
		SCI_DBG("---- SCI_STATUS_CARD_NACK [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_CARD_NACK;
		sci_card_intr_error(info, SCI_INTSTATUS_CARD_NOACK);
	}
	if (status & int_en & SCI_STATUS_REPEAT_ERROR) {
		SCI_DBG("---- SCI_STATUS_REPEAT_ERROR [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_REPEAT_ERROR;
		sci_card_intr_error(info, SCI_STATUS_REPEAT_ERROR);
	}

	/* check rx overflow first */
	if (status & SCI_STATUS_RX_FIFO_OVERFLOW) {
		SCI_DBG("---- SCI_STATUS_RX_FIFO_OVERFLOW [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_RX_FIFO_OVERFLOW;
		sci_card_intr_error(info, SCI_STATUS_RX_FIFO_OVERFLOW);
	}

	/* receiving the data even if some error occured */
	/* NOTE: if status[9] & status[2] set simultaneously (32~64 bytes),check status[9] first */
	if (status & SCI_STATUS_RECEIVE_OVER) {
		/* for COLD/HOT RESET, status[SCI_STATUS_RSTFINISH] will be set meanwhile */
		/* copy data into temp buffer */
		SCI_DBG("---- SCI_STATUS_RECEIVE_OVER [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_RECEIVE_OVER;
		sci_rx_intr_finish(info);
	} else if (status & SCI_STATUS_RX_FIFO_LEVEL_STATUS) {
		SCI_DBG("---- SCI_STATUS_RX_FIFO_LEVEL_STATUS [%08x] ----\n", status);
		regs->rSCI_STATUS &= SCI_STATUS_RX_FIFO_LEVEL_STATUS;
		sci_rx_intr(info);
	}

	info->ops->card_in_out_process(info, status);

	return IRQ_HANDLED;
}

static int sci_remove(struct platform_device *pdev)
{
	struct gxsci_info *info = platform_get_drvdata(pdev);

	if (!info)
		return 0;

	platform_set_drvdata(pdev, NULL);

	/* free the common resources */
	if (info->isRegistered) {
		free_irq(info->irq_num , info);
		misc_deregister(&sci_miscdev);
		info->isRegistered = 0;
	}

	kfree(info);

	return 0;
}

struct file_operations sci_fops =
{
	.owner = THIS_MODULE,
	.open = sci_open,
	.read = sci_read,
	.write = sci_write,
	.unlocked_ioctl = sci_ioctl,
	.poll = sci_poll,
	.fasync = sci_fasync,
	.release = sci_release,
};
static struct miscdevice sci_miscdev =
{
	.minor = MISC_SCI_MINOR,
	.name = "gxsmartcard0",
	.fops = &sci_fops,
};

static int sci_get_info(struct gxsci_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->device);

	struct device_node *sci_node = info->device->of_node;
	if (!sci_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	if (of_irq_count(sci_node) > 0) {
		info->irq_num = platform_get_irq(device, 0);
		SCI_DBG("sci intc source: %d\n", info->irq_num);
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	info->sci_base = reg->start;
	SCI_DBG("sci regs base addr: 0x%x\n", info->sci_base);

	info->regs = devm_ioremap_resource(info->device, reg);
	if (!info->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	SCI_DBG("sci regs mapped addr: 0x%p\n", info->regs);

	ret = of_property_read_u32(sci_node, "system-clock-frequency", &info->sys_clk);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	SCI_DBG("sci system clock: %d\n", info->sys_clk);

	ret = 0;
	return ret;
}

static int sci_probe(struct platform_device *pdev)
{
	struct gxsci_info *info = NULL;
	int err = 0;
	GXSMART_ConfigIntPart_t     smartinttype;
	GXSMART_ConfigEn_t          en_params;
	en_params.module_en		=	ENABLE;
	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		SCI_DBG("sci probe: no memory for sci info\n");
		err = -ENOMEM;
		goto err_out;
	}

	memset(info, 0, sizeof(*info));

	platform_set_drvdata(pdev, info);

	sci_device = pdev;

	mutex_init(&info->lock);
	rwlock_init(&info->read_rwlock);
	init_completion(&info->write_comp);
	init_completion(&info->read_comp);
	init_waitqueue_head(&info->wait);
	init_waitqueue_head(&info->r_wait);
	init_waitqueue_head(&info->w_wait);
	info->card_status = GXSMART_CARD_INIT;
	info->interrupt_status = SCI_INTSTATUS_INVALID;
	info->sci_read_fifo_ready = 0;
	info->sci_write_fifo_empty = 1;
	info->sci_cd_inverse = 0;
	info->ops = &gxsci_ops_v2;
	info->device = &pdev->dev;

	err = sci_get_info(info);
	if (err)
		goto err_out;

	SCI_DBG("sci probe: %x mapped registers at %p\n", info->sci_base, info->regs);

	INIT_WORK(&sci_card_in_wq, sci_card_in_wq_func);
	INIT_WORK(&sci_card_out_wq, sci_card_out_wq_func);

	//install irq
	err = request_irq(info->irq_num, (void *)sci_isr, IRQF_SHARED, "Linux SmartCard",info);//pdev->name, info);
	if (err) {
		SCI_DBG("sci probe: failed to install irq (%d)\n", err);
		goto err_out;
	}
	SCI_DBG("sci probe: install irq (%d)\n", info->irq_num);

	//改版后的智能卡控制器会一直处于接收模式，配置FIFO时需要关闭中断，不然会一直进入接收中断
	if(info->regs->rSCI_CNTL1 & SCI_CTL1_RCV)
		info->regs->rSCI_INTEN &= ~RX_FIFO_LEVEL_STATUS_INTEN;

	info->regs->rSCI_IO_CFG |= (1<<4);
	smartinttype.intpart = (SCIN_INTEN | SCOUT_INTEN);
	err  = sci_set_config(info,SCI_SETCONFIG_INTPART,&smartinttype,sizeof(GXSMART_ConfigIntPart_t));
	err += sci_set_config(info,SCI_SETCONFIG_EN,&en_params,sizeof(GXSMART_ConfigEn_t));
	if (err) {
		SCI_DBG("sci probe: failed to open isr\n");
		goto err_out;
	}

	err = misc_register(&sci_miscdev);
	if (err) {
		SCI_DBG("sci probe: failed to register misc device\n");
		goto err_out;
	}
	info->isRegistered = 1;

	return 0;
err_out:
	err = sci_remove(pdev);
	if (!err)
		err = -EINVAL;
	return err;
}

#ifdef CONFIG_PM
static int sci_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int sci_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define sci_suspend    NULL
#define sci_resume     NULL
#endif

static struct of_device_id gxsci_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-sci1",
		.data = NULL,
	},
};

static struct platform_driver gxsci_driver = {
	.probe		= sci_probe,
	.remove		= sci_remove,
#ifdef	CONFIG_PM
	.suspend	= sci_suspend,
	.resume		= sci_resume,
#endif
	.driver = {
		.name = "gxsmartcard0",
		.of_match_table = gxsci_device_match,
	},
};

module_platform_driver(gxsci_driver);

MODULE_DESCRIPTION("support for NationalChilp SmartCard modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
