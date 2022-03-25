#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/input.h>

#include <linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <linux/wait.h>
#include <linux/poll.h>

#include <linux/firmware.h>

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
#include <linux/slab.h>
#define INFO_QUEUE (&info->queue)
#else
#define INFO_QUEUE (info->queue)
#endif

#include "gx_irr.h"

static volatile unsigned int g_irr_philip_temp  = 0;
static volatile unsigned int g_irr_simple_ignore_num = 4;
static volatile unsigned int g_irr_open_count = 0;
extern unsigned int gx_chip_id_probe(void);

#define GX_IRR_DEVNAME "goxceed remote control"
#define IRR_CLEAR_INTERVAL (1000)
#define IRR_KFIFO_SIZE (8)
#define MISC_IRR_MINOR (156)
int gx_irr_remove(struct platform_device *pdev);
static char *keymap = NULL;
static const char gx_irr_name[] = "Linux IRR";
struct gx_irr_info *g_info = NULL;

#define MAPPED_KEY_MAX_NUM 10
typedef struct irr_key_t {
	unsigned int real_key;
	unsigned int map_key[MAPPED_KEY_MAX_NUM];
	unsigned int map_key_num;
}irr_key_t;

static irr_key_t keymap_data[KEY_MAX];

static int irr_keymap_init(struct gx_irr_info *info)
{
	unsigned int i = 0,j = 0;
	char *p = keymap;

	if (!p)
		return -1;

	do {
		keymap_data[i].real_key = memparse(p, &p);
		for (j = 0; (*(p++) == '@' && j < MAPPED_KEY_MAX_NUM); j++) {
			keymap_data[i].map_key[j] =  memparse(p, &p);
			//printk("real_key:0x%x map_key:0x%x\n", keymap_data[i].real_key, keymap_data[i].map_key[j]);
		}
		keymap_data[i].map_key_num = j;
		i++;
		if (i >= KEY_MAX) break;
	}while(*p == ' ' || *p == '\n' || *p == '\r' || *p == 0x30);
	info->keymap_num = i;
	return 0;
}

static unsigned int irr_get_key(struct gx_irr_info *info)
{
	unsigned int i;
	for(i = 0;i < info->keymap_num; i++) {
		if (keymap_data[i].real_key == info->key_code)
			return i;
	}
	return KEY_MAX + 1;
}

static int irr_parse_default(struct gx_irr_info* info)
{
	return 0;
}

static int irr_parse_nec(struct gx_irr_info*  info)
{
	unsigned int i, sig_time, mark_time;
	unsigned int num = info->pulse_num;

	mark_time = (info->pulse_val[0] & 0xFFFF);

	if (((num > IRR_MAX_SIMCODE_PULSE_NUM) 
				&&(num < IRR_MAX_FULLCODE_PULSE_NUM)) 
			||(num > IRR_MAX_FULLCODE_PULSE_NUM) ||(num == 0)) {

		irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
		return -1;
	}

	if ((mark_time < (7 * IRR_PROTCOL_TIME))
			|| (mark_time > (20 * IRR_PROTCOL_TIME))) {
		irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
		return -1;
	}

	/* deal with simcode */
	if (num <= IRR_MAX_SIMCODE_PULSE_NUM) {

		info->counter_simple++;    

		irr_debug("%s() line(%d) counter_simple:%d\n", __func__, __LINE__, info->counter_simple);

		if (info->counter_simple <= g_irr_simple_ignore_num) {
			return -1;
		} else {
			return 0;
		}
	} else {
		info->counter_simple = 0;
	}

	/* decode */
	info->key_code = 0;

	for (i = 1; i < IRR_MAX_FULLCODE_PULSE_NUM; i++) {  
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		irr_debug("%s() line(%d) mark_time[%d]:0x%x sig_time[%d]:0x%x\n", 
				__func__, __LINE__, i, mark_time, i, sig_time);
		info->key_code <<= 1;

		if((sig_time) < (IRR_AVERAGE_PULSE_WIDTH)) {//attention inverse err
	
			info->key_code |= 1;
		}
	}

	return 0;
}

#define RC5_DIV   (1)
static int irr_parse_philips(struct gx_irr_info*  info)
{
	unsigned int i;
	unsigned int sig_time;
	unsigned int mark_time;
	unsigned int low_time; unsigned int index = 0;
	unsigned int num   = info->pulse_num;
	unsigned int key_bit   = 0;
	unsigned int key_value = 0;
	static volatile unsigned int g_irr_philip_temp  = 0;
	
	if ( (num < IRR_MIN_PULSE_NUM_PHILIPS) 
			|| (num > IRR_MAX_PULSE_NUM_PHILIPS)) {
		irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
		return -1;
	}

	/* decode */
	info->key_code = 0;
	index++; 		//first, we assume bit 0 = 0 for inverse
	for (i = 0; i < num; i++) { 

		sig_time  = (info->pulse_val[i] >> 16);
		mark_time = (info->pulse_val[i] & 0xFFFF);
		low_time  = sig_time - mark_time;

		irr_debug("%s() line(%d) mark_time[%d]:0x%x sig_time[%d]:0x%x\n", 
				__func__, __LINE__, i, mark_time, i, sig_time);

		if ((2 * mark_time) > (IRR_RC5_PROTCOL_TIME * 3)){
	
			key_bit |= (1 << index++);
			key_bit |= (1 << index++);
		}
		else{
			key_bit |= (1 << index++);
		}

		if ((2 * low_time) > (IRR_RC5_PROTCOL_TIME * 3)) {
		
			index++;
			index++;
		}
		else{
			index++;
		}
	}
	switch(index){
	
		case 25: //110
			key_bit |= (1 << index++);
			key_bit |= (1 << index++);
			index++;

			break;
		case 26: //10
			key_bit |= (1 << index++);
			index++;

			break;
		case 27: //1
			key_bit |= (1 << index++);

			break;
		default: //index == 27,0-27 28bit

			break;
	}

	/*Add 01/10 judgment in order to discard error keycode.*/
	for(i = 0; i < 28; i += 2){
		key_value <<= 1;
		if(0x0 == (unsigned char)((key_bit >> i) & 0x01)
		 && 0x1 == (unsigned char)((key_bit >> (i+1)) & 0x01))
		{
			key_value |= 1;
		}else if(0x01 == (unsigned char)((key_bit >> i) & 0x01)
        		&& 0x00 == (unsigned char)((key_bit >> (i+1)) & 0x01))
		{
			continue;
		}else
		{
			irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
			return -1;
		}
	}

	/* deal with simcode */
	if((g_irr_philip_temp ^ key_value) & 0x800)
	{
		irr_debug("############%s() line(%d)################\n",
				__func__, __LINE__);
		info->key_code   = key_value;
		g_irr_philip_temp = key_value;		
		info->counter_simple = 0;   
	}else
	{
		info->counter_simple++;    
		if(info->counter_simple <= g_irr_simple_ignore_num){
			return -1;
		}else
		{
			info->key_code   = key_value;
		}
	}

	/*Start bit must be 1*/
	if((info->key_code & 0x2000) == 0) {
		irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
		return -1;
	}

	info->key_code &= 0x7ff; //get last 11 bits

	/*Rc5 F bit is used to extend data bits.If Rc5 F bit equal 0, the MSB of datacode is be 
	ignored in transmission, and the value of the datacode's MSB is 1, a total of 7bits.*/
	/*if((info->key_code & 0x1000) == 0)
	{
		info->key_code = ((info->key_code & (0x1f<<6)) << 1) | (info->key_code & 0x3f) | (1<<6);
	}*/
	
	return 0;
}

static int irr_parse_dvb_40bit(struct gx_irr_info*  info)
{
	unsigned int mark_time;
	unsigned int sig_time;
	unsigned int index;
	unsigned int key_code = 0;

	//不处理引导码，默认是合法数据
	//32 bit key code
	key_code = 0;
	for (index = 16; index >8; index--)
	{   
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40))
		{
			key_code |= 1;
		}        
	}

	for (index = 24; index >16; index--)
	{   
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}        
	}

	for (index = 33; index >25; index--)
	{   
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}        
	}

	for (index = 41; index >33; index--)
	{   
		mark_time = (info->pulse_val[index] & 0xFFFF);
		sig_time  = (info->pulse_val[index] >> 16);

		key_code <<= 1;
		if(sig_time > (IRR_AVERAGE_PULSE_WIDTH_STB40)){
			key_code |= 1;
		}        
	}

	info->key_code = key_code;
	return 0;
}

static int irr_parse_bescon(struct gx_irr_info*  info)
{
	unsigned int mark_time;
	unsigned int sig_time;
	unsigned int index;
	unsigned int key_code = 0;
	static volatile unsigned int g_irr_bescon_temp  = 0;

	unsigned long long bit_str;
	unsigned int sig_sub_mark;
	unsigned int pulse_index;

	//D16-D14 :3位引导码
	//D13    :奇偶校验位，用来判断是否是简码，如取反则为全码，反之为简码
	//D12-D8 :5位系统码，恒为0
	//D7 -D0 :8位键码
	key_code = 0;
	bit_str = 0;
	pulse_index = 0;
	index = 0;

	//不处理第1个引导码
	for(pulse_index = 1; pulse_index < info->pulse_num; pulse_index++)
	{
		mark_time = (info->pulse_val[pulse_index] & 0xFFFF);
		sig_time = (info->pulse_val[pulse_index] >> 16);

		sig_sub_mark = sig_time - mark_time;

		if(2 * mark_time > 3 * IRR_PROTCOL_TIME_BESCON){

			bit_str |= (1 << index);
			index++;

			bit_str |= (1 << index);
			index++;
		}
		else{
			bit_str |= (1 << index);
			index++;
		}

		if(2 * sig_sub_mark > 3 * IRR_PROTCOL_TIME_BESCON){

			index += 2;
		}
		else{
			index++;
		}
	}

	//因GX芯片截取红外脉冲的特征，可能会没记录到最后的一些位的数据
	//需要进行修正
	if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 1)
	{
		bit_str |= (1 << (index));
	}
	else if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 2)
	{
		bit_str |= (1 << (index));
	}
	else if((2 * IRR_MAX_PULSE_NUM_BESCON) - index == 3)
	{
		bit_str |= (1 << (index));
		bit_str |= (1 << (index + 1));
	}

	//计算key_code
	for(index = 0; index < (2 * IRR_MAX_PULSE_NUM_BESCON); index += 2)
	{
		key_code <<= 1;
		if(0x00 == (unsigned char)((bit_str >> index) & 0x01)
				&&0x01 == (unsigned char)((bit_str >> (index+1)) & 0x01)){
			key_code |= 1; 
		}
		else if(0x01 == (unsigned char)((bit_str >> index) & 0x01)
				&&0x00 == (unsigned char)((bit_str >> (index+1)) & 0x01)){
			continue;
		}
		else{
			key_code = 0xFFFFFFFF;
			return -1;
		}
	}

	/* deal with simcode */
	if((g_irr_bescon_temp ^ key_code) & (1<<13)) {

		info->key_code   = key_code;
		g_irr_bescon_temp = key_code;
		info->counter_simple = 0;
	}
	else {
		info->counter_simple++;
		if(info->counter_simple <= g_irr_simple_ignore_num){
			return -1;
		}
		else {
			info->key_code = key_code;
		}
	}

	info->key_code &= 0x1fff;
	return 0;
}

static int irr_parse_panasonic(struct gx_irr_info*  info)
{
	static unsigned long long pre_time = 0;
	static unsigned int simple_num_count = 0;
	unsigned long long cur_time;
	unsigned int i, sig_time, mark_time;
	unsigned char data0 = 0, data1 = 0;

	if (info->pulse_num < 49)
		return -1;
	
	// 16 bit key code
	data0 = 0;
	for (i = 33; i < 41; i++){   
		
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		if(sig_time > (IRR_PROTCOL_TIME_PANASONIC * 3))
		{
			data0 |= (1<<(i-33));
		}
	}

	data1 = 0;
	for (i = 41; i < 49; i++)
	{   
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		if(sig_time > (IRR_PROTCOL_TIME_PANASONIC * 3)){

			data1 |= (1<<(i-41));
		}
	}

	cur_time= get_jiffies_64();
	/* long time press key interval 92ms~138ms, most is 112ms. */
	if((cur_time - pre_time) <= 13)	// interval <=130ms
	{
		pre_time = cur_time;
		simple_num_count++;
		if(simple_num_count <= g_irr_simple_ignore_num)
		{
			return -1;
		}else
		{	
			info->key_code = (data0 <<24)|(data1<<8);
			return 0;
		}
	}else
	{
		pre_time = cur_time;
		simple_num_count = 0;	
		info->key_code = (data0 <<24)|(data1<<8);
		return 0;
	}	
	
}

struct irr_algorithm gx_irr_algorithm = {
	.functionality = irr_parse_default,
};

static void gx_irr_inithw(struct gx_irr_info *info,const char *name)
{
	volatile struct irr_regs __iomem *regs = info->regs;
	if(!strcmp(name,"gx_pt2264"))
	{
		regs->irr_cntl = (8000 << 12)|(33 << 6)|(1 << 3)|(1 << 1);
	}else
	{
		regs->irr_cntl = (20000 << 12)|(33 << 6)|(1 << 5)|(1 << 3)|(1 << 1);
	}
	regs->irr_div &= ~(0xFF);
	regs->irr_div |= info->sys_clk/info->irr_clk-1;
}

static void irr_clear_code(unsigned long data)
{
	struct gx_irr_info *info = (struct gx_irr_info *)data;

	info->key_code = 0;
	info->counter_simple = 0;
}

static int gx_irr_pin_mux(void)
{
	int ret = 0;
	int value = 0;
	unsigned int chip_id = gx_chip_id_probe();
	char __iomem *base = NULL;

	switch(chip_id) {
		case 0x6612:
			base = ioremap(0x89400140, 0x4);
			if (!base) {
				ret = -1;
				goto exit;
			}
			value = readl(base);
			value |= 0x00000001;
			writel(value, base);
			iounmap(base);
			break;
		default:
			break;
	}

exit:
	return ret;
}

static int gx_irr_chip_probe(struct gx_irr_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *irr_node = info->dev->of_node;
	if (!irr_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	if (of_irq_count(irr_node) > 0) {
		info->irr_isrvec = platform_get_irq(device, 0);
		irr_debug("irr intc source: %d\n", info->irr_isrvec);
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	info->irr_base = reg->start;
	irr_debug("irr regs base addr: 0x%x\n", info->irr_base);

	info->regs = devm_ioremap_resource(info->dev, reg);
	if (!info->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	irr_debug("irr regs mapped addr: 0x%p\n", info->regs);

	ret = of_property_read_u32(irr_node, "clock-frequency", &info->irr_clk);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	irr_debug("irr clock: %d\n", info->irr_clk);

	ret = of_property_read_u32(irr_node, "system-clock-frequency", &info->sys_clk);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	irr_debug("system clock: %d\n", info->sys_clk);

	ret = gx_irr_pin_mux();
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	ret = 0;
	return ret;
}

#define OSCILLATING_CLOCK_PERIOD		(125) 
#define MAX_PULSE_NUM_PT2264			(26)
#define MIN_PULSE_NUM_PT2264			(21)
#define PROTCOL_PT2264_TIME_UPPER		(1800)
#define PROTCOL_PT2264_TIME_LOWER		(300)
static int irr_parse_pt2264(struct gx_irr_info*  info)
{
	unsigned int sig_time, mark_time;
	unsigned int num = info->pulse_num;
	static unsigned long long pre_time = 0;
	static unsigned int simple_num_count = 0;
	unsigned long long cur_time;
	unsigned int key_value = 0;
	unsigned int key_tmp=0;
	unsigned int val_num;
	int i=0;

	if ((num > MAX_PULSE_NUM_PT2264)||(num < MIN_PULSE_NUM_PT2264)) 
	{

		irr_debug("%s() line(%d) <warn>Receive interfere code.\n", __func__, __LINE__);
		return -1;
	}
	/* decode */
	for (i = 0,val_num=num; i < num;i++ ) 
	{  
		mark_time = (info->pulse_val[i] & 0xFFFF);
		sig_time  = (info->pulse_val[i] >> 16);

		irr_debug("%s() line(%d) mark_time[%d]:0x%x sig_time[%d]:0x%x\n", 
				__func__, __LINE__, i, mark_time, i, sig_time);

		if (mark_time>PROTCOL_PT2264_TIME_UPPER 
				|| mark_time<PROTCOL_PT2264_TIME_LOWER
				|| (sig_time-mark_time)>PROTCOL_PT2264_TIME_UPPER 
				|| (sig_time-mark_time)<PROTCOL_PT2264_TIME_LOWER)
		{
			val_num--;
			continue;
		}
		key_value <<=1;
		if((mark_time) > (8*OSCILLATING_CLOCK_PERIOD )) 
		{
			key_value |= 1;
		}
	}
	if ((val_num<MIN_PULSE_NUM_PT2264))
	{
		return -1;
	}
	key_value =0xff & key_value;
	for(i=3; i>=0; i--)
	{
		if((((key_value>>(2*i))&0x01))==(((key_value>>(2*i+1))&0x01)))
		{
			key_tmp<<=1;
			key_tmp |=((key_value>>(2*i))&0x01);
		}
		else
		{
			return -1;
		}
	}

	cur_time= get_jiffies_64();
	/* long time press key interval 92ms~138ms, most is 112ms. */
	if((cur_time - pre_time) <= 13)	// interval <=130ms
	{
		pre_time = cur_time;
		simple_num_count++;
		if(simple_num_count <= g_irr_simple_ignore_num)
		{
			return -1;
		}else
		{	
			info->key_code =0xf & key_tmp;
			return 0;
		}
	}else
	{
		pre_time = cur_time;
		simple_num_count = 0;	
		info->key_code =0xf & key_tmp;
		return 0;
	}	
}
static int irr_protocol_detect( struct gx_irr_info*  info)
{
	unsigned int pulse_num = info->pulse_num;
	unsigned int mark_time = 0;

	//根据脉冲的数目自适应不同的标准
	if((IRR_MIN_PULSE_NUM_PHILIPS <= pulse_num 
				&& IRR_MAX_PULSE_NUM_PHILIPS >= pulse_num) 
			|| (IRR_MIN_PULSE_NUM_BESCON <= pulse_num 
				&& IRR_MAX_PULSE_NUM_BESCON >= pulse_num))
	{
		mark_time = (info->pulse_val[0] & 0xFFFF);
		//bescon的起始位高电平恒为420us
		if (2 * mark_time < IRR_PROTCOL_TIME_BESCON * 3
				&& (2 * mark_time > IRR_PROTCOL_TIME_BESCON * 1))
		{
			//bescon标准
			info->algo->functionality = irr_parse_bescon;
		}
		else
		{
			//philips标准
			info->algo->functionality = irr_parse_philips;
		}
	}
	else if(IRR_MAX_SIMCODE_PULSE_NUM >= pulse_num
			|| IRR_MAX_FULLCODE_PULSE_NUM <= pulse_num)
	{
		if(pulse_num >=49)
		{
			//松下标准
			info->algo->functionality = irr_parse_panasonic;
		}	
		else if(pulse_num >=42)
		{
			info->algo->functionality = irr_parse_dvb_40bit;
		}		
		else if(pulse_num>=33)
		{
			//NEC标准
			info->algo->functionality = irr_parse_nec;
		}
		else if(pulse_num <= IRR_MAX_SIMCODE_PULSE_NUM)
		{
			;
		}
		else
		{
			//protocol not suported
			return -1;
		}
	}
	else if(pulse_num >=22 && pulse_num<=26)
	{
		//pt2264标准
		info->algo->functionality = irr_parse_pt2264;
	}
	else
	{
		return -1;
	}
	return 0;
}

static int irr_get_fifo_data(struct gx_irr_info* info)
{
	volatile struct irr_regs __iomem *regs = info->regs;
	unsigned int i;

	info->pulse_num = (regs->irr_int >> 3) & 0x3F;;
	if (info->pulse_num == 0)
		return -1;

	irr_debug("Receive data num:%d\n", info->pulse_num);

	if(info->pulse_num > IRR_MAX_PULSE_NUM )
		info->pulse_num = IRR_MAX_PULSE_NUM;

	for(i = 0; i < info->pulse_num; i++)
		info->pulse_val[i] = (u32)regs->irr_fifo;

	return 0;
}

static irqreturn_t irr_isr(int irq, void *dev_id)
{
	struct gx_irr_info *info = dev_id;
	volatile struct irr_regs __iomem *regs = info->regs;
	unsigned int index, i;

	if (irr_get_fifo_data(info) < 0) {
		regs->irr_int |= 1<<1;
		IRR_MODULE_DIS(regs);
		goto exit;
	}

	/* Timeout clear. */
	regs->irr_int |= 1<<1;
	IRR_MODULE_DIS(regs);

	if (irr_protocol_detect(info) < 0)
		goto exit;

	if (info->algo->functionality(info) < 0)
		goto exit;

	irr_debug("key_code:0x%x\n", info->key_code);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	kfifo_in(INFO_QUEUE, (unsigned char *)&info->key_code, sizeof(u32));
#else
	kfifo_put(INFO_QUEUE, (unsigned char *)&info->key_code, sizeof(u32));
#endif
	if((info->irr_mode & O_NONBLOCK) != O_NONBLOCK)
		complete(&info->comp);

	index = irr_get_key(info);
	if (index <= KEY_MAX) {
		for (i = 0; i < keymap_data[index].map_key_num; i++)
			input_report_key(info->input, keymap_data[index].map_key[i], 1); /* PUSHDOWN */
		for (i = 0; i < keymap_data[index].map_key_num; i++)
			input_report_key(info->input, keymap_data[index].map_key[i], 0); /* PUSH  UP */
		input_sync(info->input);
	}

	//mod timer expires
	mod_timer(&info->timer, jiffies + msecs_to_jiffies(IRR_CLEAR_INTERVAL));

	//add by huangjb:select wakeup
	wake_up(&info->irr_read_queue);

exit:
	IRR_MODULE_EN(regs);
	return IRQ_HANDLED;
}

static int irr_open(struct inode *inode, struct file *filp)
{
	filp->private_data = g_info;

	if (g_info == NULL)
		return -1;

	return 0;
}

static int irr_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t irr_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	struct gx_irr_info *info = filp->private_data;
	unsigned int data = 0;
	size_t len = 0;

	if(!info)
		return 0;

	mutex_lock(&info->lock);
retry:
	if (kfifo_len(INFO_QUEUE) == 0) {
		if((info->irr_mode & O_NONBLOCK) == O_NONBLOCK) {
			mutex_unlock(&info->lock);
			return -EAGAIN;
		}
		wait_for_completion(&info->comp);
		goto retry;
	}
	else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
		unsigned int ret;
		ret = kfifo_out(INFO_QUEUE, (unsigned char *)&data, sizeof(u32));
#else
		kfifo_get(INFO_QUEUE, (unsigned char *)&data, sizeof(u32));
#endif
	//	if(data == 0) {
	//		mutex_unlock(&info->lock);
	//		return -EAGAIN;
	//	} //delete it.The data of one of the phillips remote control key 0 is 0.
		len = count > sizeof(u32) ? sizeof(u32):count;
		if (copy_to_user(buf, &data, len))
			return -EIO;
	}

	mutex_unlock(&info->lock);
	return count;
}

/* add by huangjb:select */
unsigned int irr_poll(struct file *filp, struct poll_table_struct *table)
{
	struct gx_irr_info *info = filp->private_data;
	unsigned int mask = 0;

	mutex_lock(&info->lock);
	if (kfifo_len(INFO_QUEUE) == 0)
		poll_wait(filp, &info->irr_read_queue, table);//add to monitor queue
	else
		mask |= POLLIN;		//dev readable
	mutex_unlock(&info->lock);
	return mask;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
static long irr_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int irr_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
#endif
{
	struct gx_irr_info *info = filp->private_data;
	int retval = 0;

	if (!info)
		return 0;

	switch(cmd) {
		case IRR_SET_MODE: /* Set: arg points to the value */
			retval = __get_user(info->irr_mode, (int __user *)arg);
			break;
		case IRR_SIMPLE_IGNORE_CONFIG:
			retval = __get_user(g_irr_simple_ignore_num, (int __user *)arg);
			break;
		case IRR_DISABLE:
			IRR_MODULE_DIS(info->regs);
			break;
		case IRR_ENABLE:
			IRR_MODULE_EN(info->regs);
			break;
		default:  /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}

	return retval;
}

struct file_operations irr_fops = {
	.owner = THIS_MODULE,
	.open = irr_open,
	.read = irr_read,
	.poll = irr_poll,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	.unlocked_ioctl = irr_ioctl,
#else
	.ioctl = irr_ioctl,
#endif
	.release = irr_release,
};

static struct miscdevice irr_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gxirr0",
	.fops = &irr_fops,
};

int gx_irr_probe(struct platform_device *pdev)
{
	struct gx_irr_info *info = NULL;
	struct input_dev *input_dev     = NULL;
	int ret, i, err = 0;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk("irr: no memory for info\n");
		err = -ENOMEM;
		goto err_out;
	}
	memset(info, 0, sizeof(*info));
	g_info = info;
	info->dev = &pdev->dev;

	err = gx_irr_chip_probe(info);
	if (err) {
		err = -ENOTTY;
		goto err_out;
	}

	if (request_irq(info->irr_isrvec, irr_isr, IRQF_SHARED, gx_irr_name, info)) {
		printk("irr failed to install irq (%d)\n", info->irr_isrvec);
		err = -ENOMEM;
		goto err_out;
	}
	irr_debug("open: install irq (%d)\n", info->irr_isrvec);

	mutex_init(&info->lock);
	init_completion(&info->comp);
	info->irr_mode = O_NONBLOCK;

	init_timer(&info->timer);
	info->timer.expires = jiffies + msecs_to_jiffies(IRR_CLEAR_INTERVAL);
	info->timer.data = (unsigned long)info;
	info->timer.function = &irr_clear_code;	/* timer handler */
	add_timer(&info->timer);
	info->timer_added = 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	err = kfifo_alloc(&info->queue, IRR_KFIFO_SIZE, GFP_KERNEL);
	if (err < 0 )
		goto err_out;
#else
	spin_lock_init(&info->spin_lock);
	info->queue = kfifo_alloc(IRR_KFIFO_SIZE, GFP_KERNEL, &info->spin_lock);
#endif

	info->algo = &gx_irr_algorithm;

	input_dev = input_allocate_device();
	if(!input_dev)
		goto err_out;
	info->input = input_dev;
	set_bit(EV_KEY, input_dev->evbit);
	for (i=0; i<BITS_TO_LONGS(KEY_CNT); ++i)
		input_dev->keybit[i] = 0xffffffff;
	input_dev->name = GX_IRR_DEVNAME;
	input_dev->id.bustype = BUS_PCI;
	input_dev->id.version = 2;
	input_dev->id.product = 3;
	input_dev->id.vendor  = 4;
	ret = input_register_device(input_dev);
	if (ret)
		goto err_out;

	irr_keymap_init(info);
	gx_irr_inithw(info,pdev->name);
	platform_set_drvdata(pdev, info);

	/* add by huangjb:select init*/
	init_waitqueue_head(&info->irr_read_queue);

	ret = misc_register(&irr_miscdev);
	if (ret != 0) {
		printk("irr: failed to register misc device\n");
	}

	return 0;
err_out:
	return gx_irr_remove(pdev);
}
EXPORT_SYMBOL_GPL(gx_irr_probe);

int gx_irr_remove(struct platform_device *pdev)
{
	struct gx_irr_info *info = platform_get_drvdata(pdev);
	struct input_dev *input_dev = info->input;

	if(!info)
		return -1;

	platform_set_drvdata(pdev, NULL);

	if (info->timer_added)
		del_timer(&info->timer);

	free_irq(info->irr_isrvec, info);

	if (input_dev)
		input_free_device(input_dev);

	kfree(info);

	misc_deregister(&irr_miscdev);

	return 0;
}
EXPORT_SYMBOL_GPL(gx_irr_remove);
module_param(keymap, charp, S_IRUGO);
