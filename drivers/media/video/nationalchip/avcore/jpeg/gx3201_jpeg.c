#include "gxav.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_firmware.h"
#include "clock_hal.h"
#include "sdc_hal.h"

#include "gx3201_jpeg.h"
#include "gx3201_jpeg_reg.h"

#include "vpu_hal.h"
#ifdef NO_OS
extern void *memmove(void *dest, const void *src, size_t n);
#endif

#define JPEG_INTERLACE_THREAD_STACK (8 * 1024)
#define DATA_ALIGN(len, align) ( ((len)+align-1) & ~(align-1) )

static unsigned char *thread_stack;
volatile struct jpeg_regs *gx3201_jpeg_reg = NULL;
static volatile struct gxav_jpegdec gx3201_jpegdec[1] = {{0}};
static volatile int curdec = -1;
struct gxav_module_ops gx3201_jpeg_module;

#define JPEG_PROPERTY_CHECK_PARAM(type) do{  \
	if (property == NULL || size != sizeof(type)){ \
		JPEG_DBG("%s(),jpeg dec state error!\n", __func__); \
		return -1; \
	} \
}while(0)

#define IS_INVAILD_DEC_INDEX(dec_index)     (dec_index == -1)
#define IS_INVAILD_DEC(dec)                 (dec == NULL)

#define gx3201_jpeg_interrupt_mask()        gx_interrupt_mask(gx3201_jpeg_module.irqs[0])
#define gx3201_jpeg_interrupt_unmask()      gx_interrupt_unmask(gx3201_jpeg_module.irqs[0])

extern struct jpeg_ops gx3201_jpeg_ops;

static int jpeg_config_frame_buf(struct gxav_jpegdec *jpegdec, struct fb_addr *fb);
static int jpeg_config_disp_buf(struct gxav_jpegdec *jpegdec, struct disp_addr *disp);
static int jpeg_reg_frame_buf(struct gxav_jpegdec *jpegdec, void *buf_addr, unsigned int buf_size);
static int jpeg_disp_queue_out(struct gxav_jpegdec *jpegdec, struct jpeg_disp_addr *jpeg_disp);
static int jpeg_fb_queue_out(struct gxav_jpegdec *jpegdec, struct jpeg_fb_addr *jpeg_fb);
static int jpeg_fb_queue_in(struct gxav_jpegdec *jpegdec, int id);
static int jpeg_put_frame_info(struct gxav_jpegdec *jpegdec, int id);
static int gx3201_jpeg_create_surface(struct gxav_jpegdec *jpegdec);
static int jpeg_init(struct gxav_jpegdec *jpegdec);
static int jpeg_query_fb_id(struct gxav_jpegdec *jpegdec, unsigned int fb_y_addr);
static int jpeg_uninit(struct gxav_jpegdec *jpegdec);
static void _write_data(int id, char *string);
static int jpeg_bs_reset(struct gxav_jpegdec *jpegdec);

static void gx3201_jpeg_interlace_thread(void* p)
{
	struct gxav_jpegdec *dec;
	dec = (struct gxav_jpegdec *)&gx3201_jpegdec[curdec];

#ifdef NO_OS
	int i = 0;
	while(i < 1)
#else
	while (1)
#endif /*NO_OS*/
	{
		unsigned int area_size = 0;
		int ret = 0, fb_id = 0;
		GxVpuProperty_Complet interlace_info = {0};
		GxVpuSurface temp_surface = {0};
		GxVpuProperty_BeginUpdate begin = {0};
		GxVpuProperty_EndUpdate end = {0};
		struct jpeg_disp_addr jpeg_disp;

#ifndef NO_OS
		if (gx_thread_should_stop())
			break;
		ret = gx_sem_wait_timeout(&(dec->sem_id), 6000);
#endif /*NO_OS*/
		/*
		 * may be time out
		 */
		if (ret != 0)
			continue;

retry:
		gx_mutex_lock(&dec->jpeg_mutex);
		if (dec->dec_state & (JPEGDEC_DESTROYED | JPEGDEC_ERROR)) {
			gx_mutex_unlock(&dec->jpeg_mutex);
			JPEG_DBG("[JPEG] INT...... JPEG interlace abandon \n");
			continue;
		}

		ret = jpeg_disp_queue_out(dec, &jpeg_disp);
		if(ret < 0){
			gx_mutex_unlock(&dec->jpeg_mutex);
			JPEG_DBG("[JPEG] INT...... JPEG interlace get disp error \n");
			gx_msleep(5);
			goto retry;
		}
		ret = jpeg_config_disp_buf(dec, &jpeg_disp.disp);
		fb_id = jpeg_query_fb_id(dec, dec->frame_buffer_y_addr);

		interlace_info.src_buf_Y = (void *)dec->frame_buffer_y_addr;
		interlace_info.src_buf_U = (void *)dec->frame_buffer_cb_addr;
		interlace_info.src_buf_V = (void *)dec->frame_buffer_cr_addr;
		interlace_info.src_base_width = dec->jpeg_frame_buffer_stride;
		interlace_info.src_width = dec->jpeg_disp_width;
		interlace_info.src_height = dec->jpeg_disp_height;
		interlace_info.src_color = dec->src_color;

		temp_surface.width = dec->jpeg_disp_width;
		temp_surface.height = dec->jpeg_disp_height;
		temp_surface.buffer = (void *)dec->frame_buffer_display_addr;
		temp_surface.color_format = dec->dst_color;

		if(!dec->interlace){
			area_size = dec->jpeg_frame_buffer_stride * dec->surface_height;
			memmove(interlace_info.src_buf_Y + area_size, interlace_info.src_buf_U, area_size);
			memmove(interlace_info.src_buf_Y + area_size * 2, interlace_info.src_buf_V, area_size);
		}else{

			ret = gx3201_jpeg_create_surface(dec);
			if(ret < 0){
				JPEG_DBG1("[JPEG] INT...... JPEG create err  \n");
				gx_mutex_unlock(&dec->jpeg_mutex);
				while(1);
			}
			temp_surface.surface_mode = GX_SURFACE_MODE_IMAGE;
			interlace_info.dst_surface = &temp_surface;
			interlace_info.dst_rect.x = interlace_info.dst_rect.y = 0;
			interlace_info.dst_rect.width = dec->jpeg_disp_width;
			interlace_info.dst_rect.height = dec->jpeg_disp_height;
			begin.max_job_num = 1;
			gxav_vpu_SetBeginUpdate(&begin);
			gxav_vpu_Complet(&interlace_info);
			gxav_vpu_SetEndUpdate(&end);
		}

		jpeg_fb_queue_in(dec, fb_id);
		jpeg_put_frame_info(dec, jpeg_disp.id);
		dec->dec_state |= JPEGDEC_ONE_FRAME;
		gx_mutex_unlock(&dec->jpeg_mutex);
#ifdef NO_OS
		i++;
#endif /*NO_OS*/
	}
}
/*---------------START QUEQUE-------------------*/
static void queue_clear(SeqQueue *queue)
{
	queue->front = 0;
	queue->rear = 0;
	queue->count = 0;
	queue->max_size = JPEG_FB_DISP_BUF_COUNT_MAX;
	gx_memset(&queue->elements, -1, sizeof(queue->elements));
}

static int queue_init(SeqQueue *queue, int max_size)
{
	int ret = 0;
	if(max_size <= JPEG_FB_DISP_BUF_COUNT_MAX && max_size > 0){
		queue->front = 0;
		queue->rear = 0;
		queue->count = 0;
		queue->max_size = max_size;
		gx_memset(&queue->elements, -1, sizeof(queue->elements));
	}else{
		ret = -1;
	}

	return ret;
}

static int queue_in(SeqQueue *queue, int value)
{
	int ret = 0;
	if(queue->count == queue->max_size){
		ret = -1;
	}else{
		queue->elements[queue->rear] = value;
		queue->rear = (queue->rear + 1) % queue->max_size;
		queue->count += 1;
	}

	return ret;
}
static int queue_out(SeqQueue *queue)
{
	int ret = 0;
	int value = 0;
	if(queue->count == 0){
		ret = -1;
	}else{
		value = queue->elements[queue->front];
		queue->elements[queue->front] = -1;
		queue->front = (queue->front + 1) % queue->max_size;
		queue->count -= 1;
		return value;
	}

	return ret;
}


static int gx3201_jpeg_create_surface(struct gxav_jpegdec *dec)
{
	GxVpuProperty_CreateSurface CreateSurface;
	int ret;

	if(dec->interlace){
		if(dec->out_surface == NULL){
			CreateSurface.format = dec->dst_color;
			CreateSurface.width  = dec->jpeg_frame_buffer_stride;
			CreateSurface.height = dec->jpeg_frame_buffer_max_height;
			CreateSurface.mode = GX_SURFACE_MODE_IMAGE;
			CreateSurface.buffer = (void*)dec->frame_buffer_display_addr;
			ret = gxav_vpu_GetCreateSurface((GxVpuProperty_CreateSurface *)&CreateSurface);
			if(ret != 0)
				return -1;
			dec->out_surface = CreateSurface.surface;
		}
	}

	return 0;
}

static int gx3201_jpeg_destroy_buffer(void *surface)
{
	GxVpuProperty_DestroySurface DesSurface;
	int ret;

	if(surface)
	{
		DesSurface.surface = surface;
		ret = gxav_vpu_SetDestroySurface((GxVpuProperty_DestroySurface *)&DesSurface);
		if(ret != 0)
			return -1;
	}

	return 0;
}

static void *_align_8_bytes(void *buffer)
{
	unsigned int fb_buffer = (unsigned int)buffer;

	if(fb_buffer % 8)
	{
		fb_buffer = ((fb_buffer / 8) + 1) * 8;
	}

	return ((void *)fb_buffer);
}


static int gx3201_jpeg_run(void)
{
	int ret = 0;
	struct jpeg_fb_addr jpeg_fb;
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	// esv pts  counter clear
	if((gxav_sdc_buffer_reset(jpegdec->channel_id)) != 0)
		return -1;

	memset(&jpeg_fb, 0, sizeof(struct jpeg_fb_addr));
	if(jpegdec->dec_state & JPEGDEC_STOPED){//reconfig
		if(jpegdec->ops->config){
			ret |= jpegdec->ops->config(jpegdec->module, &jpegdec->config);
		}
		ret |= jpeg_bs_reset(jpegdec);
	}
	ret |= jpeg_fb_queue_out(jpegdec, &jpeg_fb);
	ret |= jpeg_config_frame_buf(jpegdec, &jpeg_fb.fb);
	if(ret){
		JPEG_DBG("[JPEG]  JPEG run error \n");
		return ret;
	}

	JPEG_INT_STATUS_DECODE_PIC_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_DECODE_SLICE_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_DECODE_SOF_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_FRAME_BUFFER_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_BS_BUF_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_DECODE_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_BS_BUFFER_ALMOST_AMPTY_CLR(gx3201_jpeg_reg->rINT_STATUS);
	JPEG_INT_STATUS_BS_BUFFER_AMPTY_CLR(gx3201_jpeg_reg->rINT_STATUS);

	JPEG_CPU_INT_SET_SOF_FINISH_SLICE_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, 0x7);
	JPEG_CPU_INT_SET_BUFFER_EMPTY_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, 0x3);
	gx3201_jpeg_reg->rINT_ENABLE_CPU0 |= 0x40;
	gx3201_jpeg_reg->rINT_ENABLE_CPU0 |= 0x10;
	gx3201_jpeg_reg->rINT_ENABLE_CPU0 |= 0x08;

	JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_DEC_SLICE_FINISH);
	JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_LINE_CNT_GATE);

	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_DEC_FRAME_BUFF_ERROR);
	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_CLIP_ERROR);
	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUF_ERROR);
	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_DEC_ERROR);
	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUFFER_AMPTY);
	JPEG_CPU_INT_SET_ENABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUFFER_ALMOST_AMPTY);

	JPEG_CPU0_INT_SET_ENABLE(gx3201_jpeg_reg->rDECODE_CTRL);

	JPEG_RESAMPLE_MODE_SET(gx3201_jpeg_reg->rDECODE_CTRL, jpegdec->resample_mode);
	JPEG_FRAME_BUFFER_ENDIAN_SET_BIG(gx3201_jpeg_reg->rDECODE_CTRL, 0);
	JPEG_BS_BUFFER_ENDIAN_SET_BIG(gx3201_jpeg_reg->rDECODE_CTRL, 0);


	JPEG_CLR_DECODE_END_FILE(gx3201_jpeg_reg->rDECODE_CTRL);
	JPEG_DECODE_RUN_SET_START(gx3201_jpeg_reg->rDECODE_CTRL);

	gx3201_jpeg_reg->rDECODE_DEAD_OVER_TIME_GATE = 0x2CB41780;

	jpegdec->dec_state = JPEGDEC_RUNNING;
	jpegdec->stop_flag = 0;

	return 0;
}

static int gx3201_jpeg_stop(void)
{
	int busy = 0;
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	if(!(jpegdec->dec_state & JPEGDEC_STOPED ||
			jpegdec->dec_state & JPEGDEC_DESTROYED)){
		jpegdec->stop_flag = 1;
		JPEG_DECODE_RUN_SET_STOP(gx3201_jpeg_reg->rDECODE_CTRL);

		busy  = JPEG_DECODE_BUSY_GET(gx3201_jpeg_reg->rINT_STATUS);
		while(busy){
			gx_msleep(5);
			busy  = JPEG_DECODE_BUSY_GET(gx3201_jpeg_reg->rINT_STATUS);
			jpegdec->dec_state |= JPEGDEC_BUSY;
		}

		gx3201_jpeg_reg->rDECODE_CTRL = 0;
		gxav_module_inode_clear_event(jpegdec->module->inode, 0xFFFFFFFF);
		gxav_clock_cold_rst(MODULE_TYPE_JPEG);
		gxav_clock_hot_rst_set(MODULE_TYPE_JPEG);
		gxav_clock_hot_rst_clr(MODULE_TYPE_JPEG);

		jpegdec->dec_state |= JPEGDEC_STOPED;
	}

	return 0;
}

static int gx3201_jpeg_destroy(void)
{
	int ret;
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	if(jpegdec->dec_state & JPEGDEC_DESTROYED)
		return 0;

	gx_mutex_lock(&jpegdec->jpeg_mutex);
	jpegdec->dec_state = JPEGDEC_DESTROYED;
	ret = gx3201_jpeg_destroy_buffer(jpegdec->out_surface);
	if(ret < 0){
		JPEG_DBG("[JPEG] destroy buffer error \n");
		gx_mutex_unlock(&jpegdec->jpeg_mutex);
		return -1;
	}
	jpegdec->out_surface = NULL;
	gx_memset(&jpegdec->config, 0, sizeof(GxJpegProperty_Config));
	jpeg_uninit(jpegdec);

	gxav_clock_module_enable(MODULE_TYPE_JPEG, 0);
	gx_mutex_unlock(&jpegdec->jpeg_mutex);

	return 0;
}

static int _jpeg_channel_put_ext(struct gxav_channel *channel, unsigned char *data, unsigned int len)
{
	int ret = 0;
	unsigned char *buffer_addr_start;
	unsigned int first_len;
	unsigned int rd_addr, wr_addr;
	int id;

	id = channel->channel_id;
	gxav_sdc_rwaddr_get(id, &rd_addr, &wr_addr);
	buffer_addr_start = channel->buffer + wr_addr;
	first_len = GX_MIN(len, channel->size - wr_addr);

	if (data != NULL) {
		memcpy(buffer_addr_start, data, first_len);

		if (first_len < len) {
			memcpy(channel->buffer, data + first_len, len - first_len);
		}
	}

	ret = gxav_sdc_length_set(id, len, GXAV_SDC_WRITE);

	return ret;
}

static int gx3201_jpeg_end(void)
{
#define EXT_LEN (256)
	int ret = 0;
	int n = 0;
	unsigned int len = 0;
	static unsigned int last_len = 0;
	unsigned char ext_buf[EXT_LEN];
	struct gxav_jpegdec *dec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(dec))
		return -1;

	memset(ext_buf, 0, EXT_LEN);
	while (1) {
		gx3201_jpeg_interrupt_mask();
		if (dec->dec_state & (JPEGDEC_ERROR | JPEGDEC_OVER)) {
			gx3201_jpeg_interrupt_unmask();
			break;
		}
		gxav_sdc_free_get(dec->channel_id, &len);
		_jpeg_channel_put_ext(&dec->channel, ext_buf, EXT_LEN);
		if ((!dec->jpeg_empty) || (len < EXT_LEN)) {
			gx3201_jpeg_interrupt_unmask();
			if((n++ <= 10) && ((last_len == 0) || (last_len == len))) {
				last_len = len;
				continue;
			}
			gx3201_jpeg_interrupt_mask();
		}
		_write_data(dec->channel_id, "End");
		gx3201_jpeg_interrupt_unmask();
		last_len = 0;
		break;
	}
	/* jpeg hw bug */
	//JPEG_SET_DECODE_END_FILE(gx3201_jpeg_reg->rDECODE_CTRL);

	return ret;
}

static int gx3201_jpeg_pause(void)
{
	//以后加上，硬件暂时没有实现
	return 0;
}


static int gx3201_jpeg_resume(void)
{
	//以后加上，硬件暂时没有实现
	return 0;
}

static int gx3201_jpeg_update(void)
{
	unsigned int delay_ms = 0;
	struct gxav_jpegdec *dec = jpeg_get_cur_dec();

	if (IS_INVAILD_DEC(dec))
		return -1;

	if (dec->channel_id == -1)
		return -1;

	gxav_sdc_gate_set(dec->channel_id,
			0,
			dec->full_gate);
	for(delay_ms= 0;delay_ms < 150;delay_ms++)
	{
		if((dec->dec_state & JPEGDEC_ONE_FRAME)
#ifdef NO_OS
			|| (dec->dec_state & JPEGDEC_OVER)
#endif /*NO_OS*/
			){
			JPEG_DBG("jpeg decoder over event\n");
			break;
		}
		if(dec->dec_state & JPEGDEC_ERROR ||
				dec->dec_state & JPEGDEC_UNSUPPORT)
		{
			JPEG_DBG("%s(),jpeg stardard error or unsupport\n",__func__);
			gxav_sdc_gate_set(dec->channel_id,
					dec->empty_gate,
					dec->full_gate);
			return -1;
		}
		gx_msleep(10);
	}

	gxav_sdc_gate_set(dec->channel_id,
			dec->empty_gate,
			dec->full_gate);
	if(delay_ms >= 150)
	{
		JPEG_DBG("%s(),###WARNING###,jpeg decoder is not finished\n",__func__);
		return -1;
	}
#ifdef NO_OS
	else
	{
		gx3201_jpeg_interlace_thread((void *)dec);
	}
#endif /*NO_OS*/

	return 0;
}


#define GX3201_JPEG_SDC_RESET() \
{	\
	struct gxav_channel *channel = NULL; \
	channel = gxav_channel_get_channel((struct gxav_device *)inode->dev,\
			gx3201_jpegdec[curdec].channel_id); \
	if(channel == NULL) { \
		JPEG_DBG("%s(),channel NULL.\n",__func__); \
		return -1; \
	} \
	gxav_channel_info_reset((struct gxav_device *)inode->dev,channel); \
}

#ifdef NO_OS
int strcmp(const char *s1, const char *s2);
#endif /*NO_OS*/
static void _write_data(int id, char *string)
{
	unsigned long flag = 0;
	unsigned int buf_size = 0, rd_addr = 0, rd_addr0 = 0, wr_addr = 0, len = 0;
	struct gxav_jpegdec *dec = jpeg_get_cur_dec();

	if (IS_INVAILD_DEC(dec))
		return;

	gx_spin_lock_irqsave(&dec->jpeg_spin_lock,flag);

	buf_size = JPEG_BS_BUFFER_SIZE_GET(gx3201_jpeg_reg->rBS_BUFFER_SIZE);
	gxav_sdc_length_get(id, (unsigned int *)&len);
	gxav_sdc_rwaddr_get(id, &rd_addr, &wr_addr);
	JPEG_DBG("@@@@@@@@@@len =%10u, string =%s@@@@@@@\n", len, string);

	if (len) {
#ifdef NO_OS
		if(0 == strcmp("End", string))
		{
			if((wr_addr != rd_addr) && (wr_addr % 128))
			{
				wr_addr = ((wr_addr / 128) + 1) * 128;
			}
			else
			{
				wr_addr = wr_addr/128*128;
			}
		}
		else
#endif /*NO_OS*/
		{
			wr_addr = wr_addr/128*128;
		}
		JPEG_BS_BUFFER_WR_PTR_SET(gx3201_jpeg_reg->rBS_BUFFER_WR_PTR, wr_addr);

		JPEG_DBG("[JPEG] [%s]ID = %d Write PTR = 0x%x   Read PTR = 0x%x, len = %d buf_size = 0x%x\n",
				string,
				id,
				wr_addr,
				rd_addr,
				len,
				buf_size);

		rd_addr0 = JPEG_BS_BUFFER_RD_PTR_GET(gx3201_jpeg_reg->rBS_BUFFER_RD_PTR);
		gxav_sdc_Rptr_Set(id, rd_addr0);
		dec->jpeg_empty = 0;
	}
	gx_spin_unlock_irqrestore(&dec->jpeg_spin_lock, flag);
}

int gx3201_jpeg_decode_irq(struct gxav_module_inode *inode)
{
	int wr_addr = 0;
	unsigned int jpeg_status = 0;
	int fb_id = 0;
	struct gxav_jpegdec *dec = jpeg_get_cur_dec();

	if (IS_INVAILD_DEC(dec))
		return -1;

	fb_id = jpeg_query_fb_id(dec, dec->frame_buffer_y_addr);

	jpeg_status = (JPEG_INT_STATUS_GET(gx3201_jpeg_reg->rINT_STATUS) & gx3201_jpeg_reg->rINT_ENABLE_CPU0);

	if(jpeg_status & INT_DEC_SOF_FINISH)
	{
		JPEG_DBG("[JPEG] INT SOF FINISH SOF Status = 0x%x\n", gx3201_jpeg_reg->rINT_STATUS);
		dec->jpeg_progressive = JPEG_PIC_PROFILE_GET(gx3201_jpeg_reg->rPIC_INFO);
		switch(JPEG_PIC_FRAME_FORMAT_GET(gx3201_jpeg_reg->rPIC_INFO))
		{
		case 0:
			dec->src_color = GX_COLOR_FMT_YCBCR420_Y_U_V;
			break;
		case 1:
			dec->src_color = GX_COLOR_FMT_YCBCR422v;
			break;
		case 2:
			dec->src_color = GX_COLOR_FMT_YCBCR422h;
			break;
		case 3:
			dec->src_color = GX_COLOR_FMT_YCBCR444;
			break;
		case 4:
			dec->src_color = GX_COLOR_FMT_YCBCR400;
			break;
		default:
			break;
		}


		dec->jpeg_slice_idct  = JPEG_PIC_SLICE_IDCT_GET(gx3201_jpeg_reg->rPIC_INFO);

		if(JPEG_RESAMPLE_MODE_GET(gx3201_jpeg_reg->rDECODE_CTRL) == 0){
			dec->jpeg_h_zoom = JPEG_RESAMPLE_RATIO_MANUAL_H_GET(gx3201_jpeg_reg->rDECODE_CTRL);
			dec->jpeg_v_zoom = JPEG_RESAMPLE_RATIO_MANUAL_V_GET(gx3201_jpeg_reg->rDECODE_CTRL);
		}else{
			dec->jpeg_h_zoom = JPEG_RESAMPLE_RATIO_AUTO_H_GET(gx3201_jpeg_reg->rDECODE_CTRL);
			dec->jpeg_v_zoom = JPEG_RESAMPLE_RATIO_AUTO_V_GET(gx3201_jpeg_reg->rDECODE_CTRL);
		}

		dec->jpeg_origion_width  = JPEG_ORIGION_WIDTH_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
		dec->jpeg_origion_height = JPEG_ORIGION_HEIGHT_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
		dec->jpeg_wb_width       = JPEG_WRITE_BACK_WIDTH_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
		dec->jpeg_wb_height      = JPEG_WRITE_BACK_HEIGHT_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
		dec->jpeg_disp_width     = JPEG_DISPLAY_WIDTH_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
		dec->jpeg_disp_width = DATA_ALIGN(dec->jpeg_disp_width, 2);

		dec->jpeg_disp_height       = JPEG_DISPLAY_HEIGHT_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
		dec->jpeg_disp_coordinate_x = JPEG_DISPLAY_COORDINATE_X_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);
		dec->jpeg_disp_coordinate_y = JPEG_DISPLAY_COORDINATE_Y_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);
		JPEG_INT_STATUS_DECODE_SOF_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);

		JPEG_DBG("[JPEG] Width = %d   Height = %d\n",
				dec->jpeg_disp_width,
				dec->jpeg_wb_height);

		gxav_sdc_rwaddr_get(dec->channel_id,
				NULL,
				(unsigned int *)&wr_addr);
		wr_addr -= 1;

		JPEG_DBG("[JPEG] Write PTR = 0x%x   Read PTR = 0x%x  SDC WR PTR = 0x%x\n",
				gx3201_jpeg_reg->rBS_BUFFER_WR_PTR, gx3201_jpeg_reg->rBS_BUFFER_RD_PTR, wr_addr);

		dec->dec_status |= SOF_OVER;
		dec->stat.cnt_sof++;

		gxav_module_inode_set_event(inode, EVENT_JPEG_FRAMEINFO);
		if(gx3201_jpeg_reg->rDECODE_CTRL & 0x8)
		{
			JPEG_DECODE_GO_ON_CLR(gx3201_jpeg_reg->rDECODE_CTRL);
			JPEG_DECODE_GO_ON_SET(gx3201_jpeg_reg->rDECODE_CTRL);
		}
		if(!dec->stop_flag){
			dec->dec_state |= JPEGDEC_SOF_OVER;
		}
	}

	if(jpeg_status & INT_DEC_PIC_FINISH){
		JPEG_DBG("[JPEG] INT JPEG FINISH \n");

		if((gx3201_jpeg_reg->rDECODE_CTRL >> 1) & 1)
		{
			dec->jpeg_origion_width  = JPEG_ORIGION_WIDTH_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
			dec->jpeg_origion_height = JPEG_ORIGION_HEIGHT_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
			dec->jpeg_wb_width       = JPEG_WRITE_BACK_WIDTH_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
			dec->jpeg_wb_height      = JPEG_WRITE_BACK_HEIGHT_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
			dec->jpeg_disp_width     = JPEG_DISPLAY_WIDTH_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
			dec->jpeg_disp_width = DATA_ALIGN(dec->jpeg_disp_width, 2);

			dec->jpeg_disp_height	= JPEG_DISPLAY_HEIGHT_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
			dec->jpeg_disp_coordinate_x= JPEG_DISPLAY_COORDINATE_X_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);
			dec->jpeg_disp_coordinate_y= JPEG_DISPLAY_COORDINATE_Y_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);
		}

		dec->dec_status |= DECODER_OVER;
		gxav_module_inode_set_event(inode, EVENT_JPEG_DECOVER);

		JPEG_INT_STATUS_DECODE_PIC_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);
		JPEG_INT_STATUS_DECODE_SLICE_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);
		JPEG_INT_STATUS_LINE_CNT_CLR(gx3201_jpeg_reg->rINT_STATUS);
		dec->stat.cnt_pic++;
		if(!dec->stop_flag){
#ifdef NO_OS
			dec->dec_state |= JPEGDEC_OVER;
#else
			gx_sem_post(&(dec->sem_id));
#endif /*NO_OS*/
		}
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUFFER_AMPTY);
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUFFER_ALMOST_AMPTY);
	}else if(jpeg_status & INT_FRAME_BUF_ERROR){

	/*
	if(jpeg_status & INT_DEC_SLICE_FINISH){
		JPEG_DBG("[JPEG] INT SLICE FINISH  \n");
		JPEG_INT_STATUS_DECODE_SLICE_FINISH_CLR(gx3201_jpeg_reg->rINT_STATUS);

		if(gx3201_jpeg_reg->rDECODE_CTRL & 0x10)
		{
			dec->jpeg_origion_width  = JPEG_ORIGION_WIDTH_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
			dec->jpeg_origion_height = JPEG_ORIGION_HEIGHT_GET(gx3201_jpeg_reg->rPIC_ORIGION_SIZE_INFO);
			dec->jpeg_wb_width       = JPEG_WRITE_BACK_WIDTH_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
			dec->jpeg_wb_height      = JPEG_WRITE_BACK_HEIGHT_GET(gx3201_jpeg_reg->rPIC_WRITE_BACK_SIZE_INFO);
			dec->jpeg_disp_width     = JPEG_DISPLAY_WIDTH_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
			if(dec->jpeg_disp_width % 4)
			{
				dec->jpeg_disp_width =
					((dec->jpeg_disp_width / 4) + 1) * 4;
			}
			dec->jpeg_disp_height       = JPEG_DISPLAY_HEIGHT_GET(gx3201_jpeg_reg->rPIC_DISPLAY_SIZE_INFO);
			dec->jpeg_disp_coordinate_x = JPEG_DISPLAY_COORDINATE_X_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);
			dec->jpeg_disp_coordinate_y = JPEG_DISPLAY_COORDINATE_Y_GET(gx3201_jpeg_reg->rPIC_DISPLAY_COORDINATE_INFO);

			ga_jpeg_extend_cbcr_444((void *)dec->frame_buffer_cb_addr,
					(void *)dec->frame_buffer_cr_addr,
					(void *)dec->frame_buffer_444_cb_addr,
					(void *)dec->frame_buffer_444_cr_addr,
					dec);

			ga_jpeg_UYVY((void *)dec->frame_buffer_y_addr,
					(void *)dec->frame_buffer_444_cb_addr,
					(void *)dec->frame_buffer_444_cr_addr,
					(void *)dec->frame_buffer_display_addr,
					dec);
			gxav_module_inode_set_event(inode, EVENT_JPEG_SLICE);
		}
	}
	*/
		JPEG_DBG("[JPEG] INT FRAME BUFFER ERROR \n");
		dec->dec_status |= DECODER_ERROR;
		dec->stat.err_frame++;

		JPEG_INT_STATUS_FRAME_BUFFER_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);

		JPEG_FRAME_BUFFER_MAX_PIC_HEIGHT_SET(gx3201_jpeg_reg->rFRAME_BUFFER_STRIDE,       dec->jpeg_wb_width);
		JPEG_FRAME_BUFFER_MAX_PIC_WIDTH_SET (gx3201_jpeg_reg->rFRAME_BUFFER_MAX_PIC_SIZE, dec->jpeg_wb_height);
		JPEG_FRAME_BUFFER_STRIDE_SET        (gx3201_jpeg_reg->rFRAME_BUFFER_STRIDE,   ((((dec->jpeg_wb_width * 16) + 31) >> 5) << 2) / 2);

		JPEG_INT_STATUS_FRAME_BUFFER_ERROR_CLR(gx3201_jpeg_reg->rDECODE_CTRL);
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_DEC_FRAME_BUFF_ERROR);
		jpeg_fb_queue_in(dec, fb_id);
		if(!dec->stop_flag)
			dec->dec_state |= JPEGDEC_ERROR;
	}else if(jpeg_status & INT_CLIP_ERROR){
		JPEG_DBG("[JPEG] INT CLIP ERROR \n");
		dec->dec_status |= DECODER_ERROR;
		dec->stat.err_clip++;
		JPEG_INT_CLIP_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
		gx3201_jpeg_reg->rCLIP_UPPER_LEFT_COORDINATE = 0;
		gxav_module_inode_set_event(inode, EVENT_JPEG_UNSUPPORT);
		JPEG_DECODE_GO_ON_SET(gx3201_jpeg_reg->rDECODE_CTRL);
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_CLIP_ERROR);
		jpeg_fb_queue_in(dec, fb_id);
		if(!dec->stop_flag)
			dec->dec_state |= JPEGDEC_ERROR;
	}else if(jpeg_status & INT_BS_BUF_ERROR){
		JPEG_DBG("[JPEG] INT BS BUFFER ERROR \n");
		dec->dec_status |= DECODER_ERROR;
		dec->stat.err_bs++;
		gxav_module_inode_set_event(inode, EVENT_JPEG_UNSUPPORT);
		JPEG_INT_STATUS_BS_BUF_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_BS_BUF_ERROR);
		jpeg_fb_queue_in(dec, fb_id);
		if(!dec->stop_flag)
			dec->dec_state |= JPEGDEC_ERROR;
	}else if(jpeg_status & INT_DEC_ERROR){
		JPEG_DBG("[JPEG] INT DECODE ERROR \n");
		dec->dec_status |= DECODER_ERROR;
		dec->stat.err_decode++;
		gxav_module_inode_set_event(inode, EVENT_JPEG_UNSUPPORT);
		JPEG_INT_STATUS_DECODE_ERROR_CLR(gx3201_jpeg_reg->rINT_STATUS);
		JPEG_CPU_INT_SET_DISABLE(gx3201_jpeg_reg->rINT_ENABLE_CPU0, JPEG_DEC_ERROR);
		jpeg_fb_queue_in(dec, fb_id);
		if(!dec->stop_flag)
			dec->dec_state |= JPEGDEC_ERROR;
	}else if(jpeg_status & INT_BS_BUF_ALMOST_EMPTY){
/*
	if(jpeg_status & INT_LINE_CNT_GATE){
		JPEG_DBG("[JPEG] INT LINE COUNT \n");
		JPEG_DBG("[JPEG]########## Line is decoded %d ##########\n", gx3201_jpeg_reg->rPIC_INFO_WB_LAYER & 0x0000FFFF);
		JPEG_INT_STATUS_LINE_CNT_CLR(gx3201_jpeg_reg->rINT_STATUS);
	}
  */
		JPEG_DBG("[JPEG] INT BS BUFFER ALMOST EMPTY \n");

		_write_data(dec->channel_id, "Almost Empty");
		dec->jpeg_empty = 1;

		JPEG_INT_STATUS_BS_BUFFER_ALMOST_AMPTY_CLR(gx3201_jpeg_reg->rINT_STATUS);
	}else if(jpeg_status & INT_BS_BUF_EMPTY){
		JPEG_DBG("[JPEG] INT BS BUFFER EMPTY \n");

		_write_data(dec->channel_id, "Empty");
		dec->jpeg_empty = 1;

		JPEG_INT_STATUS_BS_BUFFER_AMPTY_CLR(gx3201_jpeg_reg->rINT_STATUS);
	}

	return 0;
}

static int jpeg_w_callback(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
	struct gxav_jpegdec *jpegdec = arg;

	if(jpegdec->jpeg_empty)
		_write_data(id, "Write Callback");

	return 0;
}

static int jpeg_bs_reset(struct gxav_jpegdec *jpegdec)
{
	int ret = 0;
	unsigned int start_addr = 0,end_addr = 0,esv_size = 0;
	unsigned char buf_id = 0;
	struct gxav_channel *channel = &jpegdec->channel;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	GXAV_ASSERT(channel != NULL);

	gxav_channel_get_phys(channel, &start_addr, &end_addr, &buf_id);
	gxav_sdc_gate_set(channel->channel_id, jpegdec->empty_gate, jpegdec->full_gate);

	esv_size = ((end_addr - start_addr + 1) & 0xFFFFFFFF);

	JPEG_BS_BUFFER_START_ADDR_SET(gx3201_jpeg_reg->rBS_BUFFER_START_ADDR, start_addr);
	JPEG_BS_BUFFER_SIZE_SET(gx3201_jpeg_reg->rBS_BUFFER_SIZE, esv_size);
	JPEG_BS_BUFFER_ALMOST_EMPTY_TH_SET(gx3201_jpeg_reg->rBS_BUFFER_ALMOST_EMPTY_TH, esv_size / 2);
	JPEG_BS_BUFFER_WR_PTR_SET(gx3201_jpeg_reg->rBS_BUFFER_WR_PTR, 0);

	return ret;
}

static int gx3201_jpeg_link_fifo(struct gxav_module *module,struct fifo_info *fifo)
{
	struct gxav_channel *channel = fifo->channel;
	unsigned int start_addr,end_addr,esv_size;
	unsigned char buf_id;
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	GXAV_ASSERT(channel != NULL);

	gx_memcpy(&jpegdec->channel, channel, sizeof(struct gxav_channel));
	gxav_channel_get_phys(channel, &start_addr, &end_addr, &buf_id);

	if (GXAV_PIN_INPUT == fifo->dir)
	{
		jpegdec->channel_id = channel->channel_id;
		jpegdec->pts_channel_id = channel->pts_channel_id;
		JPEG_DBG("[jpeg or link] jpeg_start_addr : 0x%x ,jpeg_end_addr : 0x%x,buffer id=0x%x\n",
				start_addr, end_addr,buf_id);

		jpegdec->empty_gate = 256;
		jpegdec->full_gate = 0;
		gxav_sdc_gate_set(channel->channel_id,
				jpegdec->empty_gate,
				jpegdec->full_gate);

		esv_size = ((end_addr - start_addr + 1) & 0xFFFFFFFF);

		JPEG_BS_BUFFER_START_ADDR_SET(gx3201_jpeg_reg->rBS_BUFFER_START_ADDR,start_addr);
		JPEG_BS_BUFFER_SIZE_SET(gx3201_jpeg_reg->rBS_BUFFER_SIZE, esv_size);
		JPEG_BS_BUFFER_ALMOST_EMPTY_TH_SET(gx3201_jpeg_reg->rBS_BUFFER_ALMOST_EMPTY_TH,
				esv_size / 2);//channel->almost_empty_gate);
		JPEG_BS_BUFFER_WR_PTR_SET(gx3201_jpeg_reg->rBS_BUFFER_WR_PTR, 0);
		if(0 == fifo->pin_id)
		{
			channel->indata = jpegdec;
			channel->incallback = jpeg_w_callback;
		}
	}
	else
	{
		JPEG_DBG("%s(),link fifo dir=%d,error.\n",__func__,fifo->dir);
		return -1;
	}

	return 0;
}

static int gx3201_jpeg_unlink_fifo(struct gxav_module *module,struct fifo_info *fifo)
{
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	GXAV_ASSERT(fifo != NULL);

	if (GXAV_PIN_INPUT == fifo->dir)
	{
		struct gxav_channel *channel = fifo->channel;
		JPEG_DBG("or (jpeg) unlink.\n");

		JPEG_BS_BUFFER_START_ADDR_SET(gx3201_jpeg_reg->rBS_BUFFER_START_ADDR,0x0);
		JPEG_BS_BUFFER_SIZE_SET(gx3201_jpeg_reg->rBS_BUFFER_SIZE, 0x0);
		JPEG_BS_BUFFER_ALMOST_EMPTY_TH_SET(gx3201_jpeg_reg->rBS_BUFFER_ALMOST_EMPTY_TH, 0x0);
		JPEG_BS_BUFFER_WR_PTR_SET(gx3201_jpeg_reg->rBS_BUFFER_WR_PTR, 0x0);

		jpegdec->empty_gate = 0;
		jpegdec->full_gate = 0;
		jpegdec->channel_id = -1;
		channel->incallback = NULL;
		channel->indata = NULL;
	}
	else
	{
		JPEG_DBG("%s(),unlink fifo dir=%d,error.\n",__func__,fifo->dir);
		return -1;
	}

	return 0;
}

int gx3201_jpeg_config(struct gxav_module *module, GxJpegProperty_Config * property)
{
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();

	gxav_clock_module_enable(MODULE_TYPE_JPEG, 1);
	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	GXAV_ASSERT(property != NULL);

	if (NULL==property)
		return -1;
	if(property->max_width <= 0 || property->max_height <= 0)
		return -1;
	if(property->max_width > 8191 || property->max_height > 8192)
		return -1;

	jpegdec->dst_color = property->dst_color;
	jpegdec->interlace = property->enable_interlace;
	jpegdec->zoom_enable = 0;

	jpegdec->surface_width = property->max_width;
	jpegdec->surface_height = property->max_height;

	jpegdec->mode = property->mode;
	if(property->mode == 0){
		JPEG_CLIP_MODE_SET_NORMAL(gx3201_jpeg_reg->rDECODE_CTRL);
	}else{
		JPEG_CLIP_MODE_SET_CLIP(gx3201_jpeg_reg->rDECODE_CTRL);
		gx3201_jpeg_reg->rCLIP_UPPER_LEFT_COORDINATE = ((property->clip_upper)
				<< 16) | (property->clip_left);
		gx3201_jpeg_reg->rCLIP_LOWER_RIGHT_COORDINATE = ((property->clip_lower)
				<< 16) | (property->clip_right);
	}

	jpegdec->sof_pause = property->sof_pause;
	if(jpegdec->sof_pause){
		JPEG_DECODE_PAUSE_SOF_SET_CAN(gx3201_jpeg_reg->rDECODE_CTRL);
	}else{
		JPEG_DECODE_PAUSE_SOF_SET_NOT(gx3201_jpeg_reg->rDECODE_CTRL);
	}

	jpegdec->slice_pause = property->slice_pause;
	if(jpegdec->slice_pause)
	{
		JPEG_DECODE_PAUSE_PER_SLICE_SET_CAN(gx3201_jpeg_reg->rDECODE_CTRL);
	}else{
		JPEG_DECODE_PAUSE_PER_SLICE_SET_NOT(gx3201_jpeg_reg->rDECODE_CTRL);
	}

	jpegdec->resample_mode = property->resample_mode;
	JPEG_RESAMPLE_MODE_SET(gx3201_jpeg_reg->rDECODE_CTRL, property->resample_mode);

	if(0 == jpegdec->resample_mode)
	{
		jpegdec->resample_manual_h = property->resample_manual_h;
		JPEG_RESAMPLE_RATIO_MANUAL_H_SET(gx3201_jpeg_reg->rDECODE_CTRL,property->resample_manual_h);
		jpegdec->resample_manual_v = property->resample_manual_v;
		JPEG_RESAMPLE_RATIO_MANUAL_V_SET(gx3201_jpeg_reg->rDECODE_CTRL,property->resample_manual_v);
	}

	jpegdec->jpeg_frame_buffer_stride = property->stride;
	JPEG_FRAME_BUFFER_STRIDE_SET(gx3201_jpeg_reg->rFRAME_BUFFER_STRIDE, property->stride);

	jpegdec->jpeg_frame_buffer_max_width = property->max_width;
	JPEG_FRAME_BUFFER_MAX_PIC_WIDTH_SET(gx3201_jpeg_reg->rFRAME_BUFFER_MAX_PIC_SIZE, property->max_width);

	jpegdec->jpeg_frame_buffer_max_height = property->max_height;
	JPEG_FRAME_BUFFER_MAX_PIC_HEIGHT_SET(gx3201_jpeg_reg->rFRAME_BUFFER_MAX_PIC_SIZE, property->max_height);

	return 0;
}

static unsigned int GX3201_JPEG_BASE_ADDR = 0x4400000;

int gx3201_JpegIORemap(void)
{
	unsigned char *jpeg_mapped_addr = NULL;

	if (NULL == gx_request_mem_region(GX3201_JPEG_BASE_ADDR, sizeof(struct jpeg_regs)))
	{
		JPEG_DBG("%s,request_mem_region failed\n",__func__);
		return -1;
	}

	jpeg_mapped_addr = gx_ioremap(GX3201_JPEG_BASE_ADDR, sizeof(struct jpeg_regs));
	if (jpeg_mapped_addr == NULL)
	{
		gx_release_mem_region(GX3201_JPEG_BASE_ADDR, sizeof(struct jpeg_regs));
		JPEG_DBG("%s,ioremap failed.\n",__func__);
		return -1;
	}

	gx3201_jpeg_reg = (struct jpeg_regs *)jpeg_mapped_addr;

	return 0;
}

static int gx3201_JpegIOUnmap(void)
{
	gx_iounmap(gx3201_jpeg_reg);
	gx_release_mem_region(GX3201_JPEG_BASE_ADDR, sizeof(struct jpeg_regs));
	gx3201_jpeg_reg = NULL;

	return 0;
}

static int gx3201_jpeg_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	JPEG_DBG("gx3201_jpeg_init\n");

	gxav_clock_cold_rst(MODULE_TYPE_JPEG);
	if(gx3201_JpegIORemap() < 0)
		return -1;

	return 0;
}

static int gx3201_jpeg_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	JPEG_DBG("gx3201_jpeg_cleanup\n");

	gx3201_JpegIOUnmap();

	return 0;
}

int gx3201_jpeg_open(struct gxav_module *module)
{
	int ret = 0;
	JPEG_DBG("gx3201_jpeg_open\n");

	if (module->sub >= 1){
		JPEG_DBG("%s(),open module->sub error\n", __func__);
		return -1;
	}

	gx3201_jpegdec[module->sub].frame_buffer_444_cb_addr = 0;
	gx3201_jpegdec[module->sub].frame_buffer_444_cr_addr = 0;
	gx3201_jpegdec[module->sub].frame_buffer_display_addr = 0;;
	gx3201_jpegdec[module->sub].frame_buffer_size = 0;
	gx3201_jpegdec[module->sub].channel_id = -1;
	gx3201_jpegdec[module->sub].pts_channel_id = -1;
	gx3201_jpegdec[module->sub].dec_state = JPEGDEC_DESTROYED;
	gx3201_jpegdec[module->sub].dec_status = NOMAL;
	gx3201_jpegdec[module->sub].zoom_inquire = 0;
	gx3201_jpegdec[module->sub].zoom_enable = 0;
	gx3201_jpegdec[module->sub].jpeg_frame_buffer_stride = 2048;
	gx3201_jpegdec[module->sub].jpeg_frame_buffer_max_height = 2048;
	gx3201_jpegdec[module->sub].jpeg_frame_buffer_max_height = 2048;
	gx3201_jpegdec[module->sub].ops = &gx3201_jpeg_ops;
	gx3201_jpegdec[module->sub].module = module;
	curdec = module->sub;
	gx_spin_lock_init(((gx_spin_lock_t *)&(gx3201_jpegdec[curdec].jpeg_spin_lock)));
	gx_spin_lock_init(((gx_spin_lock_t *)&(gx3201_jpegdec[curdec].frame_spin_lock)));
	gx_spin_lock_init(((gx_spin_lock_t *)&(gx3201_jpegdec[curdec].disp_spin_lock)));
	gx_spin_lock_init(((gx_spin_lock_t *)&(gx3201_jpegdec[curdec].fb_spin_lock)));

	gx_mutex_init((gx_mutex_t*)&(gx3201_jpegdec[curdec].jpeg_mutex));
	gx_sem_create((gx_sem_id *)&gx3201_jpegdec[module->sub].sem_id, 0);

	thread_stack = gx_malloc(JPEG_INTERLACE_THREAD_STACK);
	if (thread_stack == NULL) {
		thread_stack = NULL;
		return -1;
	}
	ret = gx_thread_create("gx3201_jpeg_thread", (gx_thread_id *)&gx3201_jpegdec[module->sub].thread_id,
			gx3201_jpeg_interlace_thread,
			(void *)&gx3201_jpegdec[module->sub], thread_stack,
			JPEG_INTERLACE_THREAD_STACK, 9, (gx_thread_info *)&gx3201_jpegdec[module->sub].thread_info);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int gx3201_jpeg_close(struct gxav_module *module)
{
	JPEG_DBG("gx3201_jpeg_close\n");

	//以后加上
	gx3201_jpegdec[module->sub].ops = NULL;
	gx_sem_post((gx_sem_id *)&gx3201_jpegdec[module->sub].sem_id);
	gx_thread_delete(gx3201_jpegdec[module->sub].thread_id);
	gx_sem_delete((gx_sem_id *)&gx3201_jpegdec[module->sub].sem_id);
	gx_mutex_destroy((gx_mutex_t*)&(gx3201_jpegdec[curdec].jpeg_mutex));
	if (thread_stack) {
		gx_free(thread_stack);
		thread_stack = NULL;
	}
	curdec = -1;

	return 0;
}

static int gx3201_jpeg_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	switch (property_id){
	case GxAVGenericPropertyID_ModuleLinkChannel:
		{
			struct fifo_info *fifo = (struct fifo_info *)property;

			JPEG_DBG("[property_id] : 0x%x (ModuleLinkChannel)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(struct fifo_info);

			if(jpegdec->ops->link){
				return jpegdec->ops->link(module,fifo);
			}
		}
		break;
	case GxAVGenericPropertyID_ModuleUnLinkChannel:
		{
			struct fifo_info *fifo = (struct fifo_info *)property;

			JPEG_DBG("[property_id] : 0x%x (ModuleUnLinkChannel)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(struct fifo_info);

			if(jpegdec->ops->unlink){
				return jpegdec->ops->unlink(module,fifo);
			}
		}
		break;
	case GxJPEGPropertyID_Destroy:
		{
			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Destroy)\n", property_id);

			if(jpegdec->ops->destroy){
				return jpegdec->ops->destroy();
			}
		}
		break;
	case GxJPEGPropertyID_Config:
		{
			GxJpegProperty_Config *config = (GxJpegProperty_Config *) property;
			struct disp_addr addrs[1];
			int ret = 0;

			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Config)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_Config);

			jpegdec->dec_status = NOMAL;
			jpeg_init(jpegdec);
			gx_memcpy(&jpegdec->config, config, sizeof(GxJpegProperty_Config));

			if(jpegdec->ops->config){
				ret |= jpegdec->ops->config(module, config);
			}
			ret |= jpeg_reg_frame_buf(jpegdec, config->frame_buffer_address, config->frame_buffer_size);
			addrs[0].addr = (unsigned int)config->display_buffer_address;
			addrs[0].size = config->display_buffer_size;
			if(addrs[0].addr && addrs[0].size)
				ret |= jpeg_reg_disp_buf(jpegdec, addrs, 1);

			if(ret){
				JPEG_DBG("[JPEG]  JPEG config error \n");
			}else{
				jpegdec->dec_state = JPEGDEC_READY;
			}

			return ret;
		}
		break;
	case GxJPEGPropertyID_Run:
		{
			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Run)\n", property_id);

			if(jpegdec->ops->run){
				return jpegdec->ops->run();
			}
		}
		break;
	case GxJPEGPropertyID_Stop:
		{
			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Stop)\n", property_id);

			if(jpegdec->ops->stop){
				return jpegdec->ops->stop();
			}
		}
		break;
	case GxJPEGPropertyID_Update:
		{
			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Update)\n", property_id);

			if(jpegdec->ops->update){
				return jpegdec->ops->update();
			}
		}
		break;
	case GxJPEGPropertyID_End:
		{
			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_End)\n", property_id);
			if(jpegdec->ops->end){
				return jpegdec->ops->end();
			}
		}
		break;
	case GxJPEGPropertyID_Line:
		{
			GxJpegProperty_Line *line = (GxJpegProperty_Line *) property;

			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Line)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_Line);

			if(line->line_num){
				gx3201_jpeg_reg->rINT_ENABLE_CPU0 |= 0x80;
				gx3201_jpeg_reg->rDECODE_PIC_LINE_CNT = line->line_num;
			}
		}
		break;
	case GxJPEGPropertyID_Goon:
		JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Goon)\n", property_id);
		JPEG_DECODE_GO_ON_CLR(gx3201_jpeg_reg->rDECODE_CTRL);
		JPEG_DECODE_GO_ON_SET(gx3201_jpeg_reg->rDECODE_CTRL);
		break;
	case GxJPEGPropertyID_WriteData:
		_write_data(jpegdec->channel_id, "Force Write");
		break;
	default:
		JPEG_DBG("the parameter which jpeg's property_id is wrong, please set the right parameter\n");
		return -2;
	}
	return 0;
}

static int gx3201_jpeg_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	switch (property_id){
	case GxJPEGPropertyID_Create:
		{
			GxJpegProperty_Create *create = (GxJpegProperty_Create *) property;

			JPEG_DBG("[property_id] : 0x%x (GxJpegProperty_Create)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_Create);

			if(!create->enable_interlace){
				create->buffer = (void *)jpegdec->frame_buffer_y_addr;//only for jpeg
			}
			return 0;
		}
		break;
	case GxJPEGPropertyID_State:
		{
			GxJpegProperty_State *state = (GxJpegProperty_State *) property;

			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_State)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_State);

			state->state = jpegdec->dec_state;
		}
		break;
	case GxJPEGPropertyID_Info:
		{
			GxJpegProperty_Info *info = (GxJpegProperty_Info *) property;

			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_Info);

			if(jpegdec->dec_state & JPEGDEC_SOF_OVER){
				// jpeg finally size
				info->width  = jpegdec->jpeg_disp_width;
				info->height = jpegdec->jpeg_disp_height;
				info->original_width = jpegdec->jpeg_origion_width;
				info->original_height = jpegdec->jpeg_origion_height;
				info->source_format = jpegdec->src_color;

				JPEG_DBG("jpeg width=%d,height=%d\n",info->width,info->height);
			}else{
				return -1;
			}
		}
		break;
	case GxJPEGPropertyID_Status:
		{
			GxJpegProperty_Status *status = (GxJpegProperty_Status *) property;

			JPEG_DBG("[property_id] : 0x%x (GxJPEGPropertyID_Status)\n", property_id);
			JPEG_PROPERTY_CHECK_PARAM(GxJpegProperty_Status);

			status->status = jpegdec->dec_status;
		}
		break;
	default:
		JPEG_DBG("the parameter which jpeg's property_id is wrong, please set the right parameter\n");
		return -2;
	}
	return 0;
}

static struct gxav_module_inode * gx3201_jpeg_interrupt(struct gxav_module_inode *inode, int irq)
{
	struct gxav_jpegdec *jpegdec = jpeg_get_cur_dec();
	if (IS_INVAILD_DEC(jpegdec))
		return NULL;

	if(jpegdec->ops->irq){
		return (jpegdec->ops->irq(inode) == 0) ? inode : NULL;
	}

	return NULL;
}


static int jpeg_disp_queue_out(struct gxav_jpegdec *jpegdec, struct jpeg_disp_addr *jpeg_disp)
{
	int ret = 0;
	int seq;
	unsigned long flag = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	ret = gx_sem_trywait(&jpegdec->disp_sem);
	if(ret < 0)return -1;
	gx_spin_lock_irqsave(&jpegdec->disp_spin_lock,flag);
	seq = queue_out(&jpegdec->disp_queue);
	gx_spin_unlock_irqrestore(&jpegdec->disp_spin_lock,flag);
	if(seq < 0)return -1;
	*jpeg_disp = jpegdec->disp_addr_array[seq];

	return ret;
}

int jpeg_disp_queue_in(struct gxav_jpegdec *jpegdec, int id)
{
	int ret = 0;
	unsigned long flag = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	gx_spin_lock_irqsave(&jpegdec->disp_spin_lock,flag);
	if(jpegdec->dec_state & JPEGDEC_DESTROYED)
		return -1;
	ret = queue_in(&jpegdec->disp_queue, id);
	gx_spin_unlock_irqrestore(&jpegdec->disp_spin_lock,flag);
	if(ret < 0)return -1;
	gx_sem_post(&jpegdec->disp_sem);

	return ret;
}

static int jpeg_fb_queue_out(struct gxav_jpegdec *jpegdec, struct jpeg_fb_addr *jpeg_fb)
{
	int ret = 0;
	int seq;
	unsigned long flag = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	gx_spin_lock_irqsave(&jpegdec->fb_spin_lock,flag);
	seq = queue_out(&jpegdec->fb_queue);
	gx_spin_unlock_irqrestore(&jpegdec->fb_spin_lock,flag);
	if(seq < 0)return -1;
	*jpeg_fb = jpegdec->fb_addr_array[seq];

	return ret;
}

static int jpeg_fb_queue_in(struct gxav_jpegdec *jpegdec, int id)
{
	int ret = 0;
	int seq;
	unsigned long flag = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	gx_spin_lock_irqsave(&jpegdec->fb_spin_lock,flag);
	seq = queue_in(&jpegdec->fb_queue, id);
	gx_spin_unlock_irqrestore(&jpegdec->fb_spin_lock,flag);
	if(seq < 0)return -1;

	return ret;
}

int jpeg_get_max_decsize(struct gxav_jpegdec *jpegdec, int *width, int *height)
{
	int ret = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	else {
		if(jpegdec->jpeg_frame_buffer_max_width &&
				jpegdec->jpeg_frame_buffer_max_height) {
			*width = jpegdec->jpeg_frame_buffer_max_width;
			*height = jpegdec->jpeg_frame_buffer_max_height;
		}
		else
			ret = -1;
	}

	return ret;
}

static int jpeg_config_frame_buf(struct gxav_jpegdec *jpegdec, struct fb_addr *fb)
{
	int ret = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	//Y
	jpegdec->frame_buffer_y_addr = (unsigned int)fb->fb_y_addr;
	JPEG_FRAME_BUFFER_Y_BASE_ADDR_SET(gx3201_jpeg_reg->rFRAME_BUFFER_Y_BASE_ADDR, (unsigned int)gx_virt_to_phys(fb->fb_y_addr));
	//CB
	jpegdec->frame_buffer_cb_addr = (unsigned int)fb->fb_cb_addr;
	JPEG_FRAME_BUFFER_CB_BASE_ADDR_SET(gx3201_jpeg_reg->rFRAME_BUFFER_CB_BASE_ADDR, (unsigned int)gx_virt_to_phys(fb->fb_cb_addr));
	//CR
	jpegdec->frame_buffer_cr_addr = (unsigned int)fb->fb_cr_addr;
	JPEG_FRAME_BUFFER_CR_BASE_ADDR_SET(gx3201_jpeg_reg->rFRAME_BUFFER_CR_BASE_ADDR, (unsigned int)gx_virt_to_phys(fb->fb_cr_addr));

	return ret;
}

static int jpeg_queue_init(SeqQueue *queue, int max_size)
{
	int ret = 0, i;

	queue_init(queue, max_size);
	for(i = 0; i < max_size; i++){
		queue_in(queue, i);
	}
	return ret;
}

static int jpeg_reg_frame_buf(struct gxav_jpegdec *jpegdec, void *buf_addr, unsigned int buf_size)
{
	int ret = 0;
	void *framebuffer, *buf_addr_end;
	unsigned int block;
	int size, count = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	framebuffer = buf_addr;
	buf_addr_end = buf_addr + buf_size;

	gx_dcache_inv_range((unsigned int)framebuffer, (unsigned int)buf_addr_end);

	/*framebuffer address and block must be align 8 bytes*/
	framebuffer = _align_8_bytes(buf_addr);
	block = jpegdec->jpeg_frame_buffer_max_width * jpegdec->jpeg_frame_buffer_max_height;
	size = buf_addr_end - framebuffer;

	while(buf_addr_end > framebuffer && size >= block * 3 && count < JPEG_FB_DISP_BUF_COUNT_MAX){
		jpegdec->fb_addr_array[count].id = count;
		jpegdec->fb_addr_array[count].fb.fb_y_addr = (unsigned int)framebuffer;
		jpegdec->fb_addr_array[count].fb.fb_cb_addr = (unsigned int)(framebuffer + block);
		jpegdec->fb_addr_array[count].fb.fb_cr_addr = (unsigned int)(framebuffer + block * 2);
		count++;
		framebuffer = _align_8_bytes(framebuffer + block * 3);
		size = buf_addr_end - framebuffer;
	}

	if(count > 0){
		jpeg_queue_init(&jpegdec->fb_queue, count);

	}
	else
		ret = -1;

	return ret;
}

static int jpeg_config_disp_buf(struct gxav_jpegdec *jpegdec, struct disp_addr *disp)
{
	int ret = 0;
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	jpegdec->frame_buffer_display_addr = disp->addr;
	jpegdec->frame_buffer_size = disp->size;

	gx_dcache_inv_range((unsigned int)disp->addr, (unsigned int)(disp->addr + disp->size));
	return ret;
}

int jpeg_reg_disp_buf(struct gxav_jpegdec *jpegdec, struct disp_addr addrs[], unsigned int len)
{
	int ret = 0;
	int count = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	if(len <= 0)return -1;

	while(count < len){
		jpegdec->disp_addr_array[count].id = count;
		jpegdec->disp_addr_array[count].disp.addr = (unsigned int)addrs[count].addr;
		jpegdec->disp_addr_array[count].disp.size = (unsigned int)addrs[count].size;
		count++;
	}
	jpeg_queue_init(&jpegdec->disp_queue, count);
	gx_sem_create(&jpegdec->disp_sem, count);

	return ret;
}

int jpeg_get_head_info(struct gxav_jpegdec *jpegdec, struct  jpeg_head_info *info)
{
	int ret = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	if(jpegdec->dec_state & JPEGDEC_SOF_OVER ||
			jpegdec->dec_state & JPEGDEC_OVER){
		info->src_base_width = jpegdec->jpeg_frame_buffer_stride;
		info->src_width = jpegdec->jpeg_origion_width;
		info->src_height = jpegdec->jpeg_origion_height;

		info->dst_rect.x = jpegdec->jpeg_disp_coordinate_x;
		info->dst_rect.y = jpegdec->jpeg_disp_coordinate_y;
		info->dst_rect.width = jpegdec->jpeg_disp_width;
		info->dst_rect.height = jpegdec->jpeg_disp_height;
		info->src_color = jpegdec->src_color;
	}

	return ret;
}

int jpeg_get_frame_info(struct gxav_jpegdec *jpegdec, struct jpeg_frame_info *frame)
{
	int ret = 0;
	int seq;
	unsigned long flag = 0;

	if (IS_INVAILD_DEC(jpegdec))
		return -1;
	gx_spin_lock_irqsave(&jpegdec->frame_spin_lock,flag);
	if(jpegdec->dec_state & JPEGDEC_DESTROYED)
		return -1;
	seq = queue_out(&jpegdec->frame_queue);
	if(seq >= 0){
		JPEG_DBG("####frame get=%u, id=%d, front=%d,rear=%d,count=%d####\n",jpegdec->disp_addr_array[seq].disp.addr, seq, jpegdec->frame_queue.front,jpegdec->frame_queue.rear,jpegdec->frame_queue.count);
	}
	gx_spin_unlock_irqrestore(&jpegdec->frame_spin_lock,flag);
	if(seq < 0)return -1;

	gx_memcpy(frame, &jpegdec->frame_info_array[seq], sizeof(struct jpeg_frame_info));

	return ret;
}

static int jpeg_put_frame_info(struct gxav_jpegdec *jpegdec, int id)
{
	int ret = 0;
	struct jpeg_frame_info *frame;
	unsigned int rd_addr, wr_addr;
	unsigned long flag = 0;
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	gx_spin_lock_irqsave(&jpegdec->frame_spin_lock,flag);
	ret = queue_in(&jpegdec->frame_queue, id);
	if(ret >= 0){
		JPEG_DBG("####put frame, id=%d, front=%d,rear=%d,count=%d####\n", id, jpegdec->frame_queue.front,jpegdec->frame_queue.rear,jpegdec->frame_queue.count);
	}
	gx_spin_unlock_irqrestore(&jpegdec->frame_spin_lock,flag);
	if(ret < 0)return -1;

	frame = &jpegdec->frame_info_array[id];
	gx_memset(frame, 0, sizeof(struct jpeg_frame_info));

	frame->pts = -1;
	frame->color = jpegdec->dst_color;
	frame->id = id;
	frame->buf_addr = jpegdec->disp_addr_array[id].disp.addr;
	frame->width = jpegdec->jpeg_disp_width;
	frame->height = jpegdec->jpeg_disp_height;
	if(jpegdec->pts_channel_id >= 0){
		gxav_sdc_rwaddr_get(jpegdec->pts_channel_id, &rd_addr, &wr_addr);
		rd_addr += 4;
		frame->pts = *((int *)(jpegdec->channel.pts_buffer + rd_addr));
		JPEG_DBG("####frame pts=%d####\n",frame->pts);
		gxav_sdc_length_set(jpegdec->pts_channel_id, 8, GXAV_SDC_READ);
	}

	return ret;
}

static int jpeg_init(struct gxav_jpegdec *jpegdec)
{
	int ret = 0;
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	queue_clear(&jpegdec->disp_queue);
	queue_clear(&jpegdec->fb_queue);
	queue_clear(&jpegdec->frame_queue);

	gx_memset(&jpegdec->fb_addr_array, 0, sizeof(jpegdec->fb_addr_array));
	gx_memset(&jpegdec->disp_addr_array, 0, sizeof(jpegdec->disp_addr_array));
	gx_memset(&jpegdec->stat, 0, sizeof(jpegdec->stat));

	return ret;
}

static int jpeg_uninit(struct gxav_jpegdec *jpegdec)
{
	int ret = 0;
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	queue_clear(&jpegdec->disp_queue);
	queue_clear(&jpegdec->fb_queue);
	queue_clear(&jpegdec->frame_queue);
	gx_sem_delete(&jpegdec->disp_sem);

	gx_memset(&jpegdec->fb_addr_array, 0, sizeof(jpegdec->fb_addr_array));
	gx_memset(&jpegdec->disp_addr_array, 0, sizeof(jpegdec->disp_addr_array));
	gx_memset(&jpegdec->stat, 0, sizeof(jpegdec->stat));

	return ret;
}
static int jpeg_query_fb_id(struct gxav_jpegdec *jpegdec, unsigned int fb_y_addr)
{
	int ret = 0;
	int i;
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	for(i = 0;i < JPEG_FB_DISP_BUF_COUNT_MAX; i++){
		if(jpegdec->fb_addr_array[i].fb.fb_y_addr == fb_y_addr)
			return i;
	}

	return ret;
}

int jpeg_get_state(struct gxav_jpegdec *jpegdec, int *state)
{
	if (IS_INVAILD_DEC(jpegdec))
		return -1;

	*state = jpegdec->dec_state;

	return 0;
}

struct gxav_jpegdec *jpeg_get_cur_dec(void)
{
	struct gxav_jpegdec *dec = NULL;

	if (IS_INVAILD_DEC_INDEX(curdec))
		return NULL;
	dec = (struct gxav_jpegdec *)&gx3201_jpegdec[curdec];

	return dec;
}

static int gx3201_jpeg_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	GX3201_JPEG_BASE_ADDR = res->regs[0];

	gx3201_jpeg_module.irqs[0] = res->irqs[0];
	gx3201_jpeg_module.interrupts[res->irqs[0]] = gx3201_jpeg_interrupt;

	return 0;
}

struct jpeg_ops gx3201_jpeg_ops =
{
	.run = gx3201_jpeg_run,
	.stop = gx3201_jpeg_stop,
	.destroy = gx3201_jpeg_destroy,
	.pause = gx3201_jpeg_pause,
	.resume = gx3201_jpeg_resume,
	.update = gx3201_jpeg_update,
	.end = gx3201_jpeg_end,
	.link = gx3201_jpeg_link_fifo,
	.unlink = gx3201_jpeg_unlink_fifo,
	.config = gx3201_jpeg_config,
	.irq = gx3201_jpeg_decode_irq,
	.priv = NULL,
};

struct gxav_module_ops gx3201_jpeg_module =
{
	.module_type = GXAV_MOD_JPEG_DECODER,
	.count = 1,
	.event_mask = (EVENT_JPEG_FRAMEINFO | EVENT_JPEG_DECOVER | EVENT_JPEG_UNSUPPORT | EVENT_JPEG_SLICE),
	.irqs = {36, -1},
	.irq_names = {"jpeg"},
	.irq_flags = {-1},
	.setup = gx3201_jpeg_setup,
	.init = gx3201_jpeg_init,
	.cleanup = gx3201_jpeg_cleanup,
	.open = gx3201_jpeg_open,
	.close = gx3201_jpeg_close,
	.set_property = gx3201_jpeg_set_property,
	.get_property = gx3201_jpeg_get_property,
	.interrupts[36] = gx3201_jpeg_interrupt,
};

