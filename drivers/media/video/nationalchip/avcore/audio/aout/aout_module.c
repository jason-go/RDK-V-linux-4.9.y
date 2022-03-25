#include "gxav.h"
#include "kernelcalls.h"
#include "avcore.h"
#include "include/audio_common.h"
#include "include/audio_module.h"
#include "aout.h"
#include "log_printf.h"

int gx_audioout_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_audioout_init(inode->interface);
}

int gx_audioout_uninit(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_audioout_uninit();
}

int gx_audioout_open(struct gxav_module *module)
{
	if (module == NULL)
		return -1;

	return gxav_audioout_open(module->sub);
}

int gx_audioout_close(struct gxav_module *module)
{
	if (module == NULL)
		return -1;

	return gxav_audioout_close(module->sub);
}

int gx_audioout_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if (module == NULL) {
		gxlog_e(LOG_AOUT, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -1;
	}

	switch (property_id) {
	case GxAVGenericPropertyID_ModuleLinkChannel:
		{
			struct fifo_info *fifo = (struct fifo_info *)property;
			struct audioout_fifo audioout_fifo;

			if (fifo == NULL || size != sizeof(struct fifo_info)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			gxav_channel_get_phys(fifo->channel,
					&(audioout_fifo.buffer_start_addr),&(audioout_fifo.buffer_end_addr), &(audioout_fifo.buffer_id));

			audioout_fifo.channel_id = gxav_channel_id_get(fifo->channel);

			if(fifo->pin_id == 1)
			{
				gxav_channel_get_ptsbuffer(fifo->channel,
						&(audioout_fifo.pts_start_addr), &(audioout_fifo.pts_end_addr), &(audioout_fifo.pts_buffer_id));

				audioout_fifo.channel_pts_id = gxav_channel_pts_id_get(fifo->channel);
			}

			audioout_fifo.direction = fifo->dir;
			audioout_fifo.pin_id = fifo->pin_id;
			audioout_fifo.channel = fifo->channel;
			return gxav_audioout_link(module->sub,&audioout_fifo);
		}
		break;

	case GxAVGenericPropertyID_ModuleUnLinkChannel:
		{
			struct fifo_info *fifo = (struct fifo_info *)property;
			struct audioout_fifo audioout_fifo;

			if (fifo == NULL || size != sizeof(struct fifo_info)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			audioout_fifo.direction = fifo->dir;
			audioout_fifo.pin_id = fifo->pin_id;
			audioout_fifo.channel = fifo->channel;
			return gxav_audioout_unlink(module->sub,&audioout_fifo);
		}
		break;
	case GxAudioOutPropertyID_ConfigPort:
		{
			GxAudioOutProperty_ConfigPort *config = (GxAudioOutProperty_ConfigPort *) property;

			if (config == NULL || size != sizeof(GxAudioOutProperty_ConfigPort)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_config_port(module->sub,config);
		}
		break;
	case GxAudioOutPropertyID_ConfigSource:
		{
			GxAudioOutProperty_ConfigSource *source = (GxAudioOutProperty_ConfigSource *) property;

			if (source == NULL || size != sizeof(GxAudioOutProperty_ConfigSource)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_config_source(module->sub, source);
		}
		break;
	case GxAudioOutPropertyID_ConfigSync:
		{
			GxAudioOutProperty_ConfigSync *sync = (GxAudioOutProperty_ConfigSync*) property;

			if (sync == NULL || size != sizeof(GxAudioOutProperty_ConfigSync)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_config_sync(module->sub, sync);
		}
		break;
	case GxAudioOutPropertyID_Run:
		{
			return gxav_audioout_run(module->sub);
		}

	case GxAudioOutPropertyID_Stop:
		{
			return gxav_audioout_stop(module->sub);
		}

	case GxAudioOutPropertyID_Pause:
		{
			int ret;
			unsigned long flag=0;

			flag = gxav_device_spin_lock_irqsave(module->inode->dev);
			ret = gxav_audioout_pause(module->sub);
			gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

			return ret;
		}

	case GxAudioOutPropertyID_Resume:
		{
			int ret;
			unsigned long flag=0;

			flag = gxav_device_spin_lock_irqsave(module->inode->dev);
			ret = gxav_audioout_resume(module->sub);
			gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

			return ret;
		}

	case GxAudioOutPropertyID_Mute:
		{
			GxAudioOutProperty_Mute *mute = (GxAudioOutProperty_Mute *) property;

			if (mute == NULL || size != sizeof(GxAudioOutProperty_Mute)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_mute(module->sub,mute);
		}
		break;

	case GxAudioOutPropertyID_Channel:
		{
			GxAudioOutProperty_Channel *channel = (GxAudioOutProperty_Channel *) property;

			if (channel == NULL || size != sizeof(GxAudioOutProperty_Channel)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_channel(module->sub,channel);
		}
		break;

	case GxAudioOutPropertyID_VolumeTable:
		{
			GxAudioOutProperty_VolumeTable *table = (GxAudioOutProperty_VolumeTable *) property;

			if (table == NULL || size != sizeof(GxAudioOutProperty_VolumeTable)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_set_voltable(module->sub, table);
		}
		break;


	case GxAudioOutPropertyID_Volume:
		{
			GxAudioOutProperty_Volume *volume = (GxAudioOutProperty_Volume *) property;

			if (volume == NULL || size != sizeof(GxAudioOutProperty_Volume)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_set_volume(module->sub,volume);
		}
		break;

	case GxAudioOutPropertyID_Update:
		{
			return gxav_audioout_update(module->sub);
		}
		break;

	case GxAudioOutPropertyID_PcmMix:
		{
			GxAudioOutProperty_PcmMix *mix = (GxAudioOutProperty_PcmMix *) property;

			if (mix == NULL || size != sizeof(GxAudioOutProperty_PcmMix)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_mixpcm(module->sub, mix);
		}
		break;

	case GxAudioOutPropertyID_Speed:
		{
			GxAudioOutProperty_Speed *speed = (GxAudioOutProperty_Speed *) property;

			if (speed == NULL || size != sizeof(GxAudioOutProperty_Speed)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_speed(module->sub, speed);
		}
		break;

	case GxAudioOutPropertyID_PtsOffset:
		{
			GxAudioOutProperty_PtsOffset *pts = (GxAudioOutProperty_PtsOffset *) property;

			if (pts == NULL || size != sizeof(GxAudioOutProperty_PtsOffset)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_ptsoffset(module->sub, pts);
		}
		break;
	case GxAudioOutPropertyID_PowerMute:
		{
			GxAudioOutProperty_PowerMute *power_mute = (GxAudioOutProperty_PowerMute *) property;

			if (power_mute == NULL || size != sizeof(GxAudioOutProperty_PowerMute)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_power_mute(module->sub, power_mute);
		}
		break;
	case GxAudioOutPropertyID_SetPort:
		{
			GxAudioOutProperty_SetPort *port = (GxAudioOutProperty_SetPort *) property;

			if (port == NULL || size != sizeof(GxAudioOutProperty_SetPort)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_set_port(module->sub, port);
		}
		break;

	case GxAudioOutPropertyID_TurnPort:
		{
			GxAudioOutProperty_TurnPort *port = (GxAudioOutProperty_TurnPort *) property;

			if (port == NULL || size != sizeof(GxAudioOutProperty_TurnPort)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_turn_port(module->sub, port);
		}
		break;

	case GxAudioOutPropertyID_Dump:
		{
			if (property == NULL || size != sizeof(int)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_set_dump(module->sub, *((int *)property));
		}
		break;
	case GxAudioOutPropertyID_Debug:
		{
			if (property == NULL || size != sizeof(GxAudioOutProperty_Debug)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_set_debug(module->sub, (GxAudioOutProperty_Debug *)property);
		}
		break;
	default:
		gxlog_e(LOG_AOUT, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -2;

	}

	return 0;
}

int gx_audioout_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if (module == NULL) {
		gxlog_e(LOG_AOUT, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -1;
	}

	switch (property_id) {
	case GxAudioOutPropertyID_State:
		{
			GxAudioOutProperty_State *state = (GxAudioOutProperty_State *) property;

			if (state == NULL || size != sizeof(GxAudioOutProperty_State)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_state(module->sub, state);
		}
		break;
	case GxAudioOutPropertyID_Pts:
		{
			GxAudioOutProperty_Pts *pts = (GxAudioOutProperty_Pts *) property;

			if (pts == NULL || size != sizeof(GxAudioOutProperty_Pts)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_pts(module->sub,pts);
		}
		break;
	case GxAudioOutPropertyID_Mute:
		{
			GxAudioOutProperty_Mute *mute = (GxAudioOutProperty_Mute *) property;

			if (mute == NULL || size != sizeof(GxAudioOutProperty_Mute)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_get_mute(module->sub,mute);
		}
		break;
	case GxAudioOutPropertyID_Volume:
		{
			GxAudioOutProperty_Volume *volume = (GxAudioOutProperty_Volume *) property;

			if (volume == NULL || size != sizeof(GxAudioOutProperty_Volume)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return 0;
		}
		break;
	case GxAudioOutPropertyID_Dump:
		{
			GxAvDebugDump *dump = (GxAvDebugDump *) property;

			if (property == NULL || size != sizeof(GxAvDebugDump)) {
				gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
				return -1;
			}

			return gxav_audioout_get_dump(module->sub, dump);
		}
		break;
	default:
		gxlog_e(LOG_AOUT, "%s %d [property_id: %d]\n", __func__, __LINE__, property_id);
		return -2;
	}

	return 0;
}

int gx_audioout_callback(unsigned int event,void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

	return gxav_module_inode_set_event(inode, event);
}

struct gxav_module_inode * gx_audioout_i2s_interrupt(struct gxav_module_inode *inode, int irq)
{
	return !gxav_audioout_i2s_irq(irq,gx_audioout_callback,inode) ? inode : NULL;
}

struct gxav_module_inode * gx_audioout_spdif_interrupt(struct gxav_module_inode *inode, int irq)
{
	return !gxav_audioout_spdif_irq(irq,gx_audioout_callback,inode) ? inode : NULL;
}

