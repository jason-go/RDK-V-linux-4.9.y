#include "gxav.h"
#include "kernelcalls.h"
#include "avcore.h"
#include "include/audio_common.h"
#include "include/audio_module.h"
#include "adec.h"
#include "log_printf.h"

int gx_audiodec_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_audiodec_init(inode->interface);
}

int gx_audiodec_uninit(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_audiodec_uninit();
}

int gx_audiodec_open(struct gxav_module *module)
{
	if (module == NULL)
		return -1;

	return gxav_audiodec_open(module->sub);
}

int gx_audiodec_close(struct gxav_module *module)
{
	if (module == NULL)
		return -1;

	gxav_audiodec_close(module->sub);

	return 0;
}

int gx_audiodec_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if (module == NULL) {
		gxlog_e(LOG_ADEC, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -1;
	}

	switch (property_id) {
		case GxAVGenericPropertyID_ModuleLinkChannel:
			{
				struct fifo_info *fifo = (struct fifo_info *)property;
				struct audiodec_fifo audiodec_fifo;
				struct gxav_channel * channel = fifo->channel;

				if (fifo->channel == NULL) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				gxav_channel_get_phys(fifo->channel,
					&(audiodec_fifo.buffer_start_addr),&(audiodec_fifo.buffer_end_addr), &(audiodec_fifo.buffer_id));

				audiodec_fifo.pts_start_addr = (int)channel->pts_buffer;
				audiodec_fifo.pts_end_addr   = (int)channel->pts_buffer + channel->pts_size - 1;
				audiodec_fifo.pts_buffer_id  = channel->pts_channel_id + 1;

				audiodec_fifo.direction  = fifo->dir;
				audiodec_fifo.pin_id     = fifo->pin_id;
				audiodec_fifo.channel    = fifo->channel;
				audiodec_fifo.channel_id = gxav_channel_id_get(fifo->channel);
				audiodec_fifo.channel_pts_id = gxav_channel_pts_id_get(fifo->channel);
				return gxav_audiodec_link(module->sub,&audiodec_fifo);
			}
			break;

		case GxAVGenericPropertyID_ModuleUnLinkChannel:
			{
				struct fifo_info *fifo = (struct fifo_info *)property;
				struct audiodec_fifo audiodec_fifo;

				if (fifo->channel == NULL) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				audiodec_fifo.pin_id    = fifo->pin_id;
				audiodec_fifo.direction = fifo->dir;
				audiodec_fifo.channel   = fifo->channel;
				return gxav_audiodec_unlink(module->sub,&audiodec_fifo);
			}
			break;
		case GxAudioDecoderPropertyID_Debug:
			{
				GxAudioDecProperty_Debug *debug = (GxAudioDecProperty_Debug*) property;

				if (debug == NULL || size != sizeof(GxAudioDecProperty_Debug)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_set_debug(module->sub, debug);
			}
			break;
		case GxAudioDecoderPropertyID_Dump:
			{
				if (property == NULL || size != sizeof(int)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_set_dump(module->sub, *((int *)property));
			}
			break;
		case GxAudioDecoderPropertyID_Config:
			{
				GxAudioDecProperty_Config *config = (GxAudioDecProperty_Config *) property;

				if (config == NULL || size != sizeof(GxAudioDecProperty_Config)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_config(module->sub, config);
			}
			break;
		case GxAudioDecoderPropertyID_Run:
			{
				return gxav_audiodec_run(module->sub);
			}
			break;
		case GxAudioDecoderPropertyID_Stop:
			{
				return gxav_audiodec_stop(module->sub);
			}
			break;
		case GxAudioDecoderPropertyID_Pause:
			{
				int ret;
				unsigned long flag=0;

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				ret = gxav_audiodec_pause(module->sub);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
			break;
		case GxAudioDecoderPropertyID_Resume:
			{
				int ret;
				unsigned long flag=0;

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				ret = gxav_audiodec_resume(module->sub);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
			break;
		case GxAudioDecoderPropertyID_Update:
			{
				return gxav_audiodec_update(module->sub);
			}
			break;
		case GxAudioDecoderPropertyID_DecodeKey:
			{
				GxAudioDecProperty_DecodeKey *key = (GxAudioDecProperty_DecodeKey *) property;

				if (key == NULL || size != sizeof(GxAudioDecProperty_DecodeKey)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_decode_key(module->sub, key);
			}
			break;
		case GxAudioDecoderPropertyID_BoostVolume:
			{
				GxAudioDecProperty_BoostVolume *boost = (GxAudioDecProperty_BoostVolume *) property;

				if (boost == NULL || size != sizeof(GxAudioDecProperty_BoostVolume)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_boost_volume(module->sub, boost);
			}
			break;
		case GxAudioDecoderPropertyID_PcmInfo:
			{
				GxAudioDecProperty_PcmInfo *info = (GxAudioDecProperty_PcmInfo *) property;

				if (info == NULL || size != sizeof(GxAudioDecProperty_PcmInfo)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_set_pcminfo(module->sub, info);
			}
			break;
		default:
			gxlog_e(LOG_ADEC, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
			return -2;
	}

	return 0;
}

int gx_audiodec_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if (module == NULL) {
		gxlog_e(LOG_ADEC, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -1;
	}

	switch (property_id) {
		case GxAudioDecoderPropertyID_FrameInfo:
			{
				GxAudioDecProperty_FrameInfo *frame_info = (GxAudioDecProperty_FrameInfo *) property;

				if (frame_info == NULL || size != sizeof(GxAudioDecProperty_FrameInfo)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_frameinfo(module->sub, frame_info);
			}
			break;
		case GxAudioDecoderPropertyID_State:
			{
				GxAudioDecProperty_State *state = (GxAudioDecProperty_State *) property;

				if (state == NULL || size != sizeof(GxAudioDecProperty_State)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_state(module->sub, state);
			}
			break;
		case GxAudioDecoderPropertyID_Pts:
			{
				GxAudioDecProperty_Pts *pts = (GxAudioDecProperty_Pts *) property;

				if (pts == NULL || size != sizeof(GxAudioDecProperty_Pts)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_pts(module->sub, pts);
			}
			break;
		case GxAVGenericPropertyID_GetErrno:
			{
				GxAudioDecProperty_Errno *err_no = (GxAudioDecProperty_Errno *) property;

				if (err_no == NULL || size != sizeof(GxAudioDecProperty_Errno)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_err(module->sub, err_no);
			}
			break;
		case GxAudioDecoderPropertyID_Capability:
			{
				GxAudioDecProperty_Capability *cap = (GxAudioDecProperty_Capability *) property;

				if (cap == NULL || size != sizeof(GxAudioDecProperty_Capability)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_cap(module->sub, cap);
			}
			break;
		case GxAudioDecoderPropertyID_OutInfo:
			{
				GxAudioDecProperty_OutInfo *outinfo = (GxAudioDecProperty_OutInfo *) property;

				if (outinfo == NULL || size != sizeof(GxAudioDecProperty_OutInfo)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}

				return gxav_audiodec_outinfo(module->sub, outinfo);
			}
			break;
		case GxAudioDecoderPropertyID_GetKey:
			{
				GxAudioDecProperty_GetKey *key = (GxAudioDecProperty_GetKey *) property;

				if (key == NULL || size != sizeof(GxAudioDecProperty_GetKey)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_get_key(module->sub, key);
			}
			break;
		case GxAudioDecoderPropertyID_Dump:
			{
				GxAvDebugDump *dump = (GxAvDebugDump *) property;

				if (dump == NULL || size != sizeof(GxAvDebugDump)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_get_dump(module->sub, dump);
			}
			break;
		case GxAudioDecoderPropertyID_SupportAD:
			{
				GxAudioDecProperty_SupportAD *sup = (GxAudioDecProperty_SupportAD *) property;

				if (sup == NULL || size != sizeof(GxAudioDecProperty_SupportAD)) {
					gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
					return -1;
				}
				return gxav_audiodec_support_ad(module->sub, sup);
			}
			break;

		default:
			gxlog_e(LOG_ADEC, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
			return -2;
	}

	return 0;
}

int gx_audiodec_write_ctrlinfo(struct gxav_module *module, void *ctrl_info, int ctrl_size)
{
	GxAudioDecProperty_ContrlInfo *ctrlinfo = (GxAudioDecProperty_ContrlInfo *) ctrl_info;

	if (module == NULL) {
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (ctrlinfo == NULL || ctrl_size != sizeof(GxAudioDecProperty_ContrlInfo)) {
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	return gxav_audiodec_write_ctrlinfo(module->sub, ctrlinfo);
}

int gx_audiodec_callback(unsigned int event, void *priv)
{
    struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

	if (inode && inode->interface) {
		event = inode->interface->event_mask&event;
		if (event)
			return gxav_module_inode_set_event(inode, event);
		else
			return 0;
	}

	return -1;
}

struct gxav_module_inode * gx_audiodec_interrupt(struct gxav_module_inode *inode, int irq)
{
	return !gxav_audiodec_irq(irq, gx_audiodec_callback,inode) ? inode : NULL;
}

