#include "fifo.h"
#include "video_sync.h"
#include "frame_convert.h"
#include "video.h"
#include "frame.h"
#include "video_decode.h"
#include "pp_core.h"
#include "video_display.h"
#include "gxav_vpu_propertytypes.h"
#include "porting.h"
#include "gxav_vout_propertytypes.h"
#include "sdc_hal.h"
#include "vout_hal.h"
#include "vpu_hal.h"
#include "mem_manage.h"

enum {
	FIFO_DEC_OUT = 0,
	FIFO_PP_OUT     ,
	FIFO_PP_BACK    ,
	FIFO_MAX
};

#ifdef DBG_SWITCH_PROGRAM
extern unsigned long long gx_current_tick(void);
unsigned long long head_start, head_end;
unsigned long long decode_first;
unsigned long long display_first;
#endif

static struct   gxav_video        gx_video[MAX_VIDEO] = {{-1,0,NULL}};
static struct   gxfifo            video_fifo[MAX_VIDEO][FIFO_MAX];
static struct   gxav_video_module video_dec_module[MAX_VIDEO];
static struct   gxav_video_module video_pp_module[MAX_VIDEO];
static struct   gxav_video_module video_show_module[MAX_VIDEO];

int gx_video_get_chipid()
{
	return (int)(gxcore_chip_probe());
}

typedef enum {
	PIN_INPUT,
	PIN_OUTPUT,
	PIN_BACK_TO,
	PIN_BACK_FROM,
}VPinID;

static int video_fifo_link(struct gxav_video_module *module, void* fifo, VPinID pin)
{
	if(module==NULL || fifo==NULL)
		return -1;
	if((module->in!=NULL && pin==PIN_INPUT) || (module->out!=NULL && pin==PIN_OUTPUT))
		return -1;

	if(pin == PIN_INPUT)
		module->in = fifo;
	else if(pin == PIN_OUTPUT)
		module->out = fifo;
	else if(pin == PIN_BACK_TO)
		module->back_to = fifo;
	else if(pin == PIN_BACK_FROM)
		module->back_from= fifo;
	else
		return -1;

	return 0;
}

static int video_fifo_unlink(struct gxav_video_module *module, VPinID pin)
{
	if ( module==NULL )
		return -1;

	if(pin == PIN_INPUT)
		module->in = NULL;
	else if(pin == PIN_OUTPUT)
		module->out = NULL;
	else if(pin == PIN_BACK_TO)
		module->back_to = NULL;
	else if(pin == PIN_BACK_FROM)
		module->back_from = NULL;
	else
		return -1;

	return 0;
}

static struct gxav_video *gx_video_find_instance( int sub )
{
	struct gxav_video *video = gx_video;

	int i = 0;

	while(i < MAX_VIDEO) {
		if(sub==video->id && video->used){
			break;
		}
		i++;
		video++;
	}
	return ( MAX_VIDEO==i ) ? NULL : video;
}

static struct gxav_video  *gx_video_creat_instance( int id )
{
	int i = 0;
	static  unsigned int instance_init = 0;
	struct gxav_video  *video = gx_video;

	if( NULL==video )
		return NULL;

	if( !instance_init ) {
		instance_init = !instance_init;
		for( i=0,video=gx_video; i<MAX_VIDEO; i++ ) {
			video->id = -1;
			video->used = 0;
			video++;
		}
	}

	for( i=0,video = gx_video; i<MAX_VIDEO; i++ ) {
		if(video->used && id==video->id){
			return NULL;
		}
		video++;
	}

	for( i=0,video = gx_video; i<MAX_VIDEO; i++ ) {
		if( 0==video->used ){
			gx_memset(video, 0 ,sizeof(struct gxav_video));
			video->id = id;
			video->used = 1;
			break;
		}
		video++;
	}

	return ( MAX_VIDEO==i ) ? NULL : video;
}


static int gx_video_delete_instance( struct gxav_video *video )
{
	gx_memset( video, 0, sizeof(struct gxav_video) );
	video->id = -1;
	video->used = 0;
	return 0;
}


extern int chip_media_reg_ioremap( void );
extern int chip_media_reg_iounmap(void);
static int s_decoder_hw_init = 0;
int gx_video_init(void *arg)
{
	if(!s_decoder_hw_init) {
		s_decoder_hw_init = 1;
		videopp_init((struct pp_ops*)arg);
		chip_media_reg_ioremap();
		videopp_ioremap();
	}

	return 0;
}

int gx_video_cleanup( void )
{
	if(s_decoder_hw_init) {
		s_decoder_hw_init = 0;
		chip_media_reg_iounmap( );
		videopp_iounmap();
	}
	return 0;
}

#if 0
static int gx_video_innerfifo_reset(struct gxav_video *video)
{
	struct  gxfifo *dec_out  = NULL;
	struct  gxfifo *pp_out   = NULL;
	struct  gxfifo *pp_back  = NULL;

	if(video == NULL || video->disp == NULL || video->dec == NULL)  {
		VIDEODEC_PRINTF( "video or disp or dec is null,please check \n" );
		return -1;
	}
	dec_out = &video_fifo[video->id][FIFO_DEC_OUT];
	pp_out  = &video_fifo[video->id][FIFO_PP_OUT];
	pp_back = &video_fifo[video->id][FIFO_PP_BACK];
	gxfifo_reset(dec_out);
	gxfifo_reset(pp_out);
	gxfifo_reset(pp_back);

	return 0;
}
#endif

static int gx_video_innerfifo_link(struct gxav_video *video)
{
	struct  gxfifo *dec_out  = NULL;
	struct  gxfifo *pp_out   = NULL;
	struct  gxfifo *pp_back  = NULL;

	if( video == NULL || video->disp == NULL || video->dec == NULL )  {
		VIDEODEC_PRINTF( "video or disp or dec is null,please check \n" );
		return -1;
	}

	if(video->pp == NULL) {
		VIDEODEC_PRINTF( " video->pp is null, please check \n");
		return -1;
	}

	dec_out = &video_fifo[video->id][FIFO_DEC_OUT];
	pp_out  = &video_fifo[video->id][FIFO_PP_OUT];
	pp_back = &video_fifo[video->id][FIFO_PP_BACK];

	gxfifo_reset(dec_out);
	gxfifo_reset(pp_out);
	gxfifo_reset(pp_back);
	video_fifo_link(video->dec, dec_out, PIN_OUTPUT);
	video_fifo_link(video->pp,  dec_out, PIN_INPUT);
	video_fifo_link(video->pp,  pp_out,  PIN_OUTPUT);
	video_fifo_link(video->dec, pp_back, PIN_BACK_FROM);
	video_fifo_link(video->pp,  pp_back, PIN_BACK_TO);
	return 0;
}

static void gx_video_innerfifo_unlink(struct gxav_video *video )
{
	video_fifo_unlink(video->dec, PIN_OUTPUT);
	video_fifo_unlink(video->dec, PIN_BACK_FROM);
	video_fifo_unlink(video->pp,  PIN_INPUT);
	video_fifo_unlink(video->pp,  PIN_OUTPUT);
	video_fifo_unlink(video->pp,  PIN_BACK_TO);
}

int gx_video_close_ext(int id)
{
	struct gxav_video   *video   = NULL;

	video = gx_video_find_instance( id );

	if( video->dec->ops!=NULL&&((struct video_ops*)video->dec->ops)->close!=NULL ) {
		((struct video_ops*)video->dec->ops)->close( video->dec );
		video->dec->priv = NULL;
		video->dec->pre  = NULL;
	}
	else {
		VIDEODEC_PRINTF( "video->dec->ops or ops->close is NULL " );
	}

	if(videopp_close(video->pp) == 0) {
		video->pp->priv = NULL;
		video->pp->pre  = NULL;
	}
	else {
		VIDEODEC_PRINTF( "video->pp->ops->close is failed!" );
	}

	if( video->disp->ops!=NULL&&((struct disp_ops*)video->disp->ops)->close!=NULL ) {
		((struct disp_ops*)video->disp->ops)->close( video->disp );
		video->disp->priv = NULL;
		video->disp->pre  = NULL;
	}
	else {
		VIDEODEC_PRINTF( "video->disp->ops or ops->close is NULL " );
	}

	return 0;
}

int gx_video_close ( int id )
{
#define _FIFO_RELEASE_(fifo) \
	do {\
		if (fifo)\
			fifo = NULL;\
	}while(0)

	int i = 0;
	struct gxav_video *video = gx_video_find_instance( id );
	if(video == NULL)
		return 0;

	gx_video_stop( id );

	gx_video_close_ext( id );

	video = gx_video_find_instance( id );

	video->dec->ops  = NULL;
	video->pp->ops   = NULL;
	video->disp->ops = NULL;
	_FIFO_RELEASE_(video->dec->out);
	_FIFO_RELEASE_(video->dec->back_from);
	_FIFO_RELEASE_(video->pp->in);
	_FIFO_RELEASE_(video->pp->out);
	_FIFO_RELEASE_(video->pp->back_to);

	video->dec	= NULL;
	video->pp	= NULL;
	video->disp	= NULL;
	gx_video_delete_instance( video );

	for(i=0; i<FIFO_MAX; i++)
		gxfifo_free(&video_fifo[id][i]);

	return 0;
}

int gx_video_open_ext(int id)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec!=NULL&&video->dec->ops!=NULL&&((struct video_ops*)video->dec->ops)->open!=NULL ) {
		video->dec->priv = ((struct video_ops*)video->dec->ops)->open( id );
		if ( video->dec->priv==NULL ) {
			VIDEODEC_PRINTF( " open video dec module error " );
			goto OPEN_ERR;
		}
		video->dec->pre = (void*)video;
	}
	else {
		VIDEODEC_PRINTF( "video->dec->ops or ops->open is NULL " );
		goto OPEN_ERR;
	}

	if(videopp_open(video->pp, &av_sync[id], &fr_con) == 0) {
		video->pp->pre = (void*)video;
	}
	else {
		VIDEODEC_PRINTF( "video->pp->ops->open is failed!" );
	}

	if( video->disp!=NULL&&video->disp->ops!=NULL&&((struct disp_ops*)video->disp->ops)->open!=NULL ) {
		video->disp->priv = ((struct disp_ops*)video->disp->ops)->open(id);
		if ( video->disp->priv==NULL ) {
			VIDEODEC_PRINTF( " open video disp module error " );
			goto OPEN_ERR;
		}
		video->disp->pre  = (void*)video;
	}
	else {
		VIDEODEC_PRINTF( "video->disp->ops or ops->open is NULL " );
		goto OPEN_ERR;
	}

	return 0;

OPEN_ERR:
	gx_video_close_ext(id);
	return -1;
}

int gx_video_open(int id)
{
	unsigned int i=0;
	unsigned int fifo_size = 0;
	struct gxav_video *video  = NULL;

	video = gx_video_creat_instance(id);
	if( NULL==video ) {
		VIDEODEC_PRINTF( " error " );
		return -1;
	}

	video->layer_select   = 0;
	video->ppopen         = 0;
	video->fr_transform  &= ~(FR_ENBALE);
	video->logo           = 0;
	video->redecode       = 0;
	video->vstate         = VSTATE_STOPPED;

	video->dec = &video_dec_module[id];
	gx_memset(video->dec, 0, sizeof(struct gxav_video_module) );
	video->dec->ops = &video_dec_ops;

	if(GET_VIDEO_MAJ_ID(id)==VIDEO_DECODER) {
		video->pp = &video_pp_module[id];
		gx_memset(video->pp, 0, sizeof(struct gxav_video_module));
		video->pp->id = id;
	}
	else {
		video->pp = NULL;
	}

	video->disp = &video_show_module[id];
	gx_memset( video->disp, 0, sizeof(struct gxav_video_module));
	video->disp->ops = &video_disp_ops;

	if(-1==gx_video_open_ext(id)) {
		return -1;
	}
	fifo_size = MAX_DEC_FRAME * sizeof(VideoFrameInfo);
	for(i=0; i<FIFO_MAX; i++)
		 gxfifo_init(&video_fifo[id][i], NULL, fifo_size);

	return 0;
}

static int gx_video_fifo_wcallback(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
	struct gxav_video *video = arg;

	if (video == NULL)
		return -1;

	if(video->dec && video->dec->ops && ((struct video_ops*)video->dec->ops)->in_callback)
		((struct video_ops*)video->dec->ops)->in_callback(video->dec);

	if(overflow) {
		video->state     = VIDEODEC_ERROR;
		video->err_code  = VIDEODEC_ERR_ESV_OVERFLOW;
		video->over_flow = 1;
	}
	return 0;
}

int gx_video_fifo_link(int id, struct h_vd_video_fifo *fifo)
{
	struct gxav_video	*video	= NULL;
	struct gxav_channel *channel= NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->config==NULL ) {
		VIDEODEC_PRINTF("video->dec or dec->ops or dec->ops->config is null\n");
		return -1;
	}

	if(0 != gx_video_innerfifo_link(video)) {
		return -1;
	}

	video->bs_bufid = -1;
	video->bitstream_buf_addr = 0;
	video->bitstream_buf_size = 0;
	video->pts_bufid    = -1;
	video->pts_buf_addr = 0;
	video->pts_buf_size = 0;
	video->channel = fifo->channel;

	if( fifo!=NULL && fifo->bs_id!=-1 &&
		fifo->bitstream_buf_addr>0 && fifo->bitstream_buf_size>0 ) {
		video->bs_bufid           = fifo->bs_id;
		video->bitstream_buf_addr = fifo->bitstream_buf_addr;
		video->bitstream_buf_size = fifo->bitstream_buf_size;

		video->pts_bufid    = fifo->pts_id;
		video->pts_buf_addr = fifo->pts_buf_addr;
		video->pts_buf_size = fifo->pts_buf_size;

		channel = fifo->channel;
		channel->indata = video;
		channel->incallback = gx_video_fifo_wcallback;
		return ((struct video_ops*)video->dec->ops)->config(video->dec, VIDEO_CONFIG_PARAM, fifo, sizeof(struct h_vd_video_fifo));
	}

	return -1;
}

int gx_video_fifo_unlink(int id, struct h_vd_video_fifo *pfifo)
{
	struct gxav_video *video = NULL;
	struct h_vd_video_fifo fifo;
	struct gxav_channel *channel = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->config==NULL ) {
		VIDEODEC_PRINTF("video->dec or dec->ops or dec->ops->config is null\n");
		return -1;
	}

	gx_video_innerfifo_unlink(video);

	fifo.bs_id  = 0;
	fifo.pts_id = 0;

	video->bs_bufid = -1;
	video->bitstream_buf_addr = 0;
	video->bitstream_buf_size = 0;

	video->pts_bufid = -1;
	video->pts_buf_addr = 0;
	video->pts_buf_size = 0;

	if(pfifo!=NULL && pfifo->channel!= NULL) {
		channel = pfifo->channel;
		channel->incallback = NULL;
		channel->indata = NULL;
		return ((struct video_ops*)video->dec->ops)->config(video->dec, VIDEO_CONFIG_PARAM, &fifo,sizeof(struct h_vd_video_fifo));
	}

	return -1;
}

int gx_video_config(int id, unsigned int cmd, void *config, unsigned int size)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->config==NULL ) {
		VIDEODEC_PRINTF("video->dec or dec->ops or dec->ops->config is null\n");
		return -1;
	}

	switch(cmd) {
		case VIDEO_CONFIG_FRAME_IGNORE:
		case VIDEO_CONFIG_DEC_MODE:
			video_tolerance_fresh(&av_sync[id]);
			break;
		case VIDEO_CONFIG_SYNC:
			video_set_sync_mode(&av_sync[id], *(unsigned int*)config);
			break;
		case VIDEO_CONFIG_DEINTERLACE:
			return videopp_config(video->pp, PP_CONFIG_DEINTERLACE, config, size);
		case VIDEO_CONFIG_DEINTERLACE_EN:
			return videopp_config(video->pp, PP_CONFIG_DEINTERLACE_EN, config, size);
		case VIDEO_CONFIG_DROP_FRAME_GATE:
			return videopp_config(video->pp, PP_CONFIG_DROP_GATE, config, size);
		case VIDEO_CONFIG_FREEZE_FRAME:
			return ((struct disp_ops*)video->disp->ops)->config(video->disp, DISP_CONFIG_FREEZE_FRAME, config, size);
		case VIDEO_CONFIG_COLBUF:
			return mm_colbuf_info_set(config);
		case VIDEO_CONFIG_WORKBUF:
			return mm_workbuf_info_set(config);
		case VIDEO_CONFIG_FRAMEBUF:
			return mm_framebuf_info_set(config);
		case VIDEO_CONFIG_PLAYBYPASS:
			videodisp_bypass(id, *(int*)config);
			break;
		default:
			break;
	}

	return ((struct video_ops*)video->dec->ops)->config(video->dec, cmd, config, size);
}

int gx_video_run(int id)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	gx_memset(&video->h_info, 0, sizeof(struct head_info));

	///< 这里不能对如下2个变量进行清0，否则会出错
	video->ppopen &= ~PPOPEN_ENBALE;
	video->fr_transform &= ~(FR_ENBALE);
	video->fr_transform |= FR_REQUIRE;
	video->convert_rate  = 0xFFFFFFFF;

	if(GET_VIDEO_MAJ_ID(id)==VIDEO_DECODER && id==0) {
		video->layer_select = GX_LAYER_VPP;
	}
	else {
		video->layer_select = GX_LAYER_SPP;
	}

	if(videopp_run(video->pp) != 0) {
		VIDEODEC_PRINTF("video->pp->ops->stop is failed\n");
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->run==NULL ){
		VIDEODEC_PRINTF("video->dec or dec->ops or ops->stop is null\n");
		return -1;
	}
	else {
		ret = ((struct video_ops*)video->dec->ops)->run( video->dec );
		if( 0!=ret ) {
			if( video->disp==NULL||video->dec->ops==NULL||((struct disp_ops*)video->disp->ops)->close==NULL ) {
				ret = ((struct disp_ops*)video->disp->ops)->close( video->disp );
			} else {
				VIDEODEC_PRINTF ( " ((struct disp_ops*)video->disp->ops)->close is null,please check " );
			}
			VIDEODEC_PRINTF( " dec->stop fail,please check " );
			return -1;
		}
	}

	if ( video->disp==NULL||video->disp->ops==NULL||((struct disp_ops*)video->disp->ops)->run==NULL ) {
		VIDEODEC_PRINTF("video->disp or disp->ops or ops->stop is null\n");
		return -1;
	}
	else {
		ret = ((struct disp_ops*)video->disp->ops)->run( video->disp );
		if( 0!=ret ) {
			VIDEODEC_PRINTF( " disp->stop fail,please check " );
			return -1;
		}
	}

	video->state     = VIDEODEC_STOPED;
	video->vstate    = VSTATE_RUNNING;
	video->err_code  = VIDEODEC_ERR_NONE;
	video->over_flow = 0;
	return 0;
}

int gx_video_probe(int id)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}
	gx_memset(&video->h_info, 0, sizeof(struct head_info));

	///< 这里不能对如下2个变量进行清0，否则会出错
	video->ppopen &= ~PPOPEN_ENBALE;
	video->fr_transform &= ~(FR_ENBALE);
	video->fr_transform |= FR_REQUIRE;
	video->convert_rate  = 0xFFFFFFFF;

	video->state       = VIDEODEC_STOPED;
	video->vstate      = VSTATE_READY;
	video->over_flow   = 0;

	if(video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->probe==NULL){
		VIDEODEC_PRINTF("video->dec or dec->ops or ops->stop is null\n");
		return -1;
	}
	else {
		ret = ((struct video_ops*)video->dec->ops)->probe(video->dec);
		if( 0!=ret ) {
			if( video->disp==NULL||video->dec->ops==NULL||((struct disp_ops*)video->disp->ops)->close==NULL ) {
				ret = ((struct disp_ops*)video->disp->ops)->close( video->disp );
			} else {
				VIDEODEC_PRINTF ( " ((struct disp_ops*)video->disp->ops)->close is null,please check " );
			}
			VIDEODEC_PRINTF( " dec->stop fail,please check " );
			return -1;
		}
	}

	return 0;
}

int gx_video_stop(int id)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if (videopp_stop(video->pp) != 0) {
		VIDEODEC_PRINTF("video->pp->ops->stop is failed\n");
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->stop==NULL ){
		VIDEODEC_PRINTF("video->dec or dec->ops or ops->stop is null\n");
		//return -1;
	}
	else {
		ret = ((struct video_ops*)video->dec->ops)->stop( video->dec, -1);
		if( 0!=ret ) {
			VIDEODEC_PRINTF( " dec->stop fail,please check " );
		}
	}

	if ( video->disp==NULL||video->disp->ops==NULL||((struct disp_ops*)video->disp->ops)->stop==NULL ) {
		VIDEODEC_PRINTF("video->disp or disp->ops or ops->stop is null\n");
		//return -1;
	}
	else {
		ret = ((struct disp_ops*)video->disp->ops)->stop( video->disp );
		if( 0!=ret ) {
			VIDEODEC_PRINTF( " disp->stop fail,please check " );
		}
	}

	video->ppopen  &= ~PPOPEN_ENBALE;
	video->logo     = 0;
	video->vstate   = VSTATE_STOPPED;
	video->redecode = 0;

	return 0;
}

int gx_video_pause(int id)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	if((video = gx_video_find_instance(id)) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if(video->state == VIDEODEC_PAUSED)
		return -1;
	else {
		if(video->dec==NULL || video->dec->ops==NULL || ((struct video_ops*)video->dec->ops)->pause==NULL) {
			VIDEODEC_PRINTF("video->dec or dec->ops or ops->pause is null\n");
			return -1;
		}
		else {
			ret = ((struct video_ops*)video->dec->ops)->pause(video->dec);
			if(0 != ret) {
				VIDEODEC_PRINTF( " dec->pause fail,please check " );
				return -1;
			}
		}

		if(video->disp==NULL || video->disp->ops==NULL || ((struct disp_ops*)video->disp->ops)->pause==NULL) {
			VIDEODEC_PRINTF("video->disp or disp->ops or ops->pause is null\n");
			return -1;
		}
		else {
			ret = ((struct disp_ops*)video->disp->ops)->pause(video->disp);
			if(0 != ret) {
				VIDEODEC_PRINTF( " disp->pause fail,please check " );
				return -1;
			}
		}
		video->state_shadow = video->state;
		video->state        = VIDEODEC_PAUSED;
	}

	return 0;
}

int gx_video_resume(int id)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	if((video = gx_video_find_instance(id)) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if(video->state == VIDEODEC_PAUSED)
		video->state = video->state_shadow;

	if(video->dec==NULL || video->dec->ops==NULL || ((struct video_ops*)video->dec->ops)->resume==NULL) {
		VIDEODEC_PRINTF("video->dec or dec->ops or ops->resume is null\n");
		return -1;
	}
	else {
		ret = ((struct video_ops*)video->dec->ops)->resume(video->dec);
		if(0 != ret) {
			VIDEODEC_PRINTF( " dec->resume fail,please check " );
			return -1;
		}
	}

	if(video->disp==NULL || video->disp->ops==NULL || ((struct disp_ops*)video->disp->ops)->resume==NULL) {
		VIDEODEC_PRINTF("video->disp or disp->ops or ops->resume is null\n");
		return -1;
	}
	else {
		ret = ((struct disp_ops*)video->disp->ops)->resume(video->disp);
		if(0 != ret) {
			VIDEODEC_PRINTF( " disp->resume fail,please check " );
			return -1;
		}
	}

	return 0;
}

int gx_video_set_decmode(int id, int dec_mode)
{
	unsigned int drop_gate;
	//set dec mode
	gx_video_config(id, VIDEO_CONFIG_DEC_MODE, (void*)&dec_mode, 0);
	//set drop gate
	drop_gate = (dec_mode==DECODE_FAST ? 2 : 0);
	gx_video_config(id, VIDEO_CONFIG_DROP_FRAME_GATE, &drop_gate, 0);
	return 0;
}

int gx_video_get_speed(int id,int *speed)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if(((struct video_ops*)video->dec->ops)->get_decmode) {
		((struct video_ops*)video->dec->ops)->get_decmode(video->dec,speed);
		return 0;
	}

	return -1;
}

int gx_video_set_frame_ignore(int id,unsigned int mode)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}
	gx_video_config(id, VIDEO_CONFIG_FRAME_IGNORE, &mode, 0);
	return 0;
}

static int video_show_log(struct gxav_video *video)
{
	int show_logo = 1;

	if(video && video->pp)
		videopp_config(video->pp, PP_CONFIG_SHOW_LOGO, &show_logo, 0);

	if(video->dec && video->dec->ops && ((struct video_ops*)video->dec->ops)->config)
		return ((struct video_ops*)video->dec->ops)->config( video->dec, VIDEO_CONFIG_SHOW_LOGO ,&show_logo, 0);
	else
		VIDEODEC_PRINTF("video->dec or dec->ops or dec->ops->config is null\n");

	return -1;
}

int gx_video_update(int id)
{
	DecState state;
	unsigned int update = 0;
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->get_state==NULL) {
		VIDEODEC_PRINTF("video->dec or video->disp is null\n");
		return -1;
	}
	((struct video_ops*)video->dec->ops)->get_state(video->dec, &state, NULL);
	if(state == VIDEODEC_STOPED) {
		VIDEODEC_PRINTF("please update data befor decoder stopped\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->config==NULL ) {
		VIDEODEC_PRINTF("video->dec or dec->ops or dec->ops->config is null\n");
		return -1;
	}

	update = 1;
	if(videopp_config(video->pp, PP_CONFIG_UPDATE, &update, 0) != 0) {
		VIDEODEC_PRINTF("config disp update error \n");
	}

	return ((struct video_ops*)video->dec->ops)->config( video->dec, VIDEO_CONFIG_UPDATE ,&update, 0);
}

int gx_video_refresh(int id)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	gx_video_update(id);
	if(video->logo&SHOW_ENABLE) {
		return video_show_log(video);
	}
	else {
		video->logo |= SHOW_REQU;
	}
	return 0;
}

int gx_video_get_frame_info(int id, struct head_info *info)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}
	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->get_frameinfo==NULL ) {
		VIDEODEC_PRINTF("video->dec or video->disp is null\n");
		return -1;
	}

	return ((struct video_ops*)video->dec->ops)->get_frameinfo( video->dec,info);
}

int gx_video_get_frame_stat(int id, struct gxav_frame_stat *stat)
{
	struct gxav_video *video = NULL;

	if ((video = gx_video_find_instance(id)) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	videodec_get_frame_cnt(video->dec, &stat->decode_frame_cnt, &stat->filter_frame_cnt, &stat->error_frame_cnt);
	videodec_get_afd(video->dec, &stat->AFD);
	videopp_get_sync_stat(video->pp, &stat->lose_sync_cnt, &stat->synced_flag);
	videodisp_get_frame_cnt(video->disp, &stat->play_frame_cnt);

	return 0;
}

int gx_video_get_pts(int id,unsigned int *pts)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}
	video_get_pts(&av_sync[id], pts);
	return 0;
}

int gx_video_get_state(int id, DecState *state, DecErrCode *err_code)
{
	struct gxav_video *video = NULL;

	if((video = gx_video_find_instance(id)) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if(video->state == VIDEODEC_ERROR) {
		if(state)
			*state = video->state;
		if(err_code)
			*err_code = video->err_code;
	}
	else
		((struct video_ops*)video->dec->ops)->get_state(video->dec, state, err_code);

	return 0;
}

int gx_video_get_cap( int id,struct h_vd_cap *cap)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if( video->dec==NULL||video->dec->ops==NULL||((struct video_ops*)video->dec->ops)->get_cap==NULL) {
		VIDEODEC_PRINTF("video->dec or video->disp is null\n");
		return -1;
	}

	return ((struct video_ops*)video->dec->ops)->get_cap( video->dec,cap);
}

int gx_video_get_pp_up(int id, unsigned int *up)
{
	struct gxav_video *video = NULL;

	if ( (video = gx_video_find_instance(id) ) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	if ( video->disp==NULL||video->disp->ops==NULL||((struct disp_ops*)video->disp->ops)->get_pp_up==NULL ) {
		VIDEODEC_PRINTF("video->disp or disp->ops or ops->get_pp_up is null\n");
		return -1;
	}
	else {
		return ((struct disp_ops*)video->disp->ops)->get_pp_up( video->disp, up);
	}
}

int gx_video_set_ptsoffset(int id, int offset_ms)
{
	struct gxav_video *video = NULL;

	if((video = gx_video_find_instance(id)) == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		return -1;
	}

	return videosync_set_ptsoffset(&av_sync[id], offset_ms);
}

int gx_video_set_tolerance(int id, int value)
{
	int ret = 0;
	unsigned int tolerance_ms = value;
	struct gxav_video *video = gx_video_find_instance(id);

	if (video == NULL) {
		VIDEODEC_PRINTF("gxav find videodec fail.\n");
		ret = -1;
	} else {
		if (tolerance_ms < MIN_LOW_TOLERANCE_MS) {
			tolerance_ms = MIN_LOW_TOLERANCE_MS;
		}
		video_set_band_tolerance_fresh(&av_sync[0], tolerance_ms);
	}

	return ret;
}

int gx_video_disp_patch(int id)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);

	if( video==NULL || video->disp==NULL||video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}

	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}

	gx_video_frame_rate_transform_require(id);
	return 0;
}

int gx_video_zoom_require(int id)
{
	struct gxav_video *video = NULL;

	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL||video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}
	videopp_zoom_param_update_require(video->pp);

	return 0;
}

int gx_video_ppopen_enable( int id )
{
	static int en_times = 0;
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL || video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}
	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}

	if(video->ppopen&PPOPEN_REQUIRE) {
		gxav_vpu_disp_layer_enable(video->layer_select, 1);
		if(en_times++ >= 20) {
			en_times = 0;
			video->ppopen &= ~PPOPEN_REQUIRE;
			video->ppopen |= PPOPEN_ENBALE;
		}
	}
	else{
		video->ppopen |= PPOPEN_ENBALE;
	}

	return 0;
}

int gx_video_ppopen_require( int id )
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL || video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}

	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}
	VIDEODEC_DBG(" open require : 0x%x \n",video->ppopen);
	video->ppopen |= PPOPEN_REQUIRE;

	return 0;
}

int gx_video_ppclose_require( int id )
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL || video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}

	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}
	VIDEODEC_DBG(" open require : 0x%x \n",video->ppopen);
	video->ppopen &= ~PPOPEN_REQUIRE;

	return 0;
}

int gx_video_frame_rate_transform_require(int id)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL || video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}
	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}
	video->fr_transform|=FR_REQUIRE;

	return 0;
}

int gx_video_frame_rate_transform_enable(int id, int pp_enable)
{
	unsigned dec_frame_num = 0;
	struct gxav_video *video = gx_video_find_instance(id);

	if( video==NULL || video->disp==NULL || video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}
	if(GET_VIDEO_MAJ_ID(id) != VIDEO_DECODER) {
		return 0;
	}

	((struct video_ops*)video->dec->ops)->get_frame_num(video->dec, &dec_frame_num);
	if(video->fr_transform&FR_REQUIRE && dec_frame_num > 0) {
		struct head_info info = {0};
		gx_video_get_frame_info(0, &info);
		video_frame_convert_config(&fr_con, pp_enable, info.fps);
		video->fr_transform &= ~FR_REQUIRE;
	}
	video->fr_transform|=FR_ENBALE;
	return 0;
}

int gx_video_dec_interrupt(int id, int irq, h_vd_int_type *int_type)
{
	int ret = 0;
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if(video==NULL || video->dec==NULL) {
		VIDEODEC_PRINTF( "gxav find videodec fail %p \n",video );
		return -1;
	}

	if(video->vstate == VSTATE_STOPPED) {
		VIDEODEC_PRINTF("%s():%d error\n", __func__, __LINE__);
		return -1;
	}
	ret = videodec_boda_isr(video->dec,irq, int_type);

	switch (*int_type) {
		case VD_SEQ_OVER:
		case VD_SEQ_OVER_EX:
			if(ret != 0)
				return ret;
			break;
		case VD_SEQ_CHANGED:
			if(ret != 0)
				return ret;
			videopp_stop(video->pp);
#if 0
			if(video->disp==NULL || video->disp->ops==NULL || ((struct disp_ops*)video->disp->ops)->stop==NULL) {
				VIDEODEC_PRINTF("video->disp or disp->ops or ops->stop is null\n");
				return -1;
			}
			ret = ((struct disp_ops*)video->disp->ops)->stop(video->disp);
			if(0 != ret) {
				VIDEODEC_PRINTF( " disp->stop fail,please check " );
			}
			gx_video_innerfifo_reset(video);
#endif
			break;
		case VD_DECODE_OVER:
			if(!video->h_info.ready) {
				gx_video_get_frame_info(id,&video->h_info);
				if(video->h_info.ready) {
					video->logo |= SHOW_ENABLE;
					video->h_info.width  = (video->h_info.width + 15) & ~15;
					video->h_info.height = (video->h_info.height + 15) & ~15;
				}
			}
			if(video->logo == (SHOW_ENABLE|SHOW_REQU)) {
				video_show_log(video);
				video->logo = 0;
			}
			break;
		default:
			break;
	}

	return ret;
}

int gx_video_pp_interrupt(int id,int irq)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->pp==NULL )  {
		VIDEODEC_PRINTF( "gxav find %x'd videodec fail %p \n",id,video );
		return -1;
	}

	return videopp_isr( video->pp );
}

void gx_video_patch(int id)
{
	struct gxav_video *video = NULL;
	video = gx_video_find_instance(id);

	if(video->state != VIDEODEC_PAUSED) {
		videopp_callback(video->pp);
		videodec_callback(video->dec);
		if (av_sync[id].sync_mode == SYNC_MODE_SYNC_LATER) {
			unsigned long long disp_frame_cnt = 0;
			videodisp_get_frame_cnt(video->disp, &disp_frame_cnt);
			if(disp_frame_cnt > 0) {
				av_sync[id].sync_mode = SYNC_MODE_NORMAL;
			}
		}
	}
}

int gx_video_vpu_interrupt(int id,int irq)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);

	if(NULL == video) {
		return 0;
	}

	if (video->vstate == VSTATE_STOPPED)
		return -1;

	if (video==NULL || video->disp==NULL || video->dec==NULL)  {
		VIDEODEC_PRINTF( "gxav find %x'd videodec fail %p \n",id,video );
		return -1;
	}

	return videodisp_vpu_isr(video->disp, gx_video_patch);
}

int gx_video_set_stream_ratio(unsigned int ratio)
{
	return gxav_vpu_vpp_set_stream_ratio(ratio);
}

int gx_video_get_viewrect(GxAvRect *rect)
{
	return gxav_vpu_vpp_get_actual_viewrect(rect);
}

int gx_video_get_cliprect(GxAvRect *rect)
{
	return gxav_vpu_vpp_get_actual_cliprect(rect);
}

int gx_video_set_cliprect(GxAvRect *rect)
{
	return gxav_vpu_vpp_set_actual_cliprect(rect);
}

int gx_video_cap_frame(int id, struct cap_frame *frame)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL|| video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find %x'd videodec fail %p \n",id,video );
		return -1;
	}

	return videodisp_get_showing_frame(video->disp, frame);
}

int gx_video_get_userdata(int id, GxVideoDecoderProperty_UserData *user_data)
{
	struct gxav_video *video = NULL;

	video = gx_video_find_instance(id);
	if( video==NULL || video->disp==NULL|| video->dec==NULL )  {
		VIDEODEC_PRINTF( "gxav find %x'd videodec fail %p \n",id,video );
		return -1;
	}

	return videodec_read_userdata(video->dec, user_data->data, user_data->size);
}
