#include "gxav.h"
#include "gxav_parameter_check.h"
#include "kernelcalls.h"

#include "vpu_hal.h"

int gx_vpu_open(struct gxav_module *module)
{
	return gxav_vpu_open();
}

int gx_vpu_close(struct gxav_module *module)
{
	return gxav_vpu_close();
}

int gx_vpu_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	return gxav_vpu_init(inode->interface);
}

int gx_vpu_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	return gxav_vpu_cleanup();
}

int gx_vpu_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int retVal ;

	retVal = GxVpu_ParameterCheck(property_id,size);

	if(retVal) {
		VPU_PRINTF(" set property error ID = %d size = %d \n",property_id,size);
		return retVal;
	}
	if (property == NULL) {

		return -1;
	}

	switch (property_id) {

	case GxVpuPropertyID_RWPalette:{
			retVal = gxav_vpu_WPalette((GxVpuProperty_RWPalette *) property);
			break;
		}
	case GxVpuPropertyID_DestroyPalette:{
			retVal = gxav_vpu_DestroyPalette((GxVpuProperty_DestroyPalette *) property);
			break;
		}
	case GxVpuPropertyID_SurfaceBindPalette:{
			retVal = gxav_vpu_SurfaceBindPalette((GxVpuProperty_SurfaceBindPalette *) property);
			break;
		}

	case GxVpuPropertyID_LayerViewport:{
			retVal = gxav_vpu_SetLayerViewport((GxVpuProperty_LayerViewport *) property);
			break;
		}
	case GxVpuPropertyID_LayerMainSurface:{
			retVal = gxav_vpu_SetLayerMainSurface((GxVpuProperty_LayerMainSurface *) property);
			break;
		}
	case GxVpuPropertyID_UnsetLayerMainSurface:{
			retVal = gxav_vpu_UnsetLayerMainSurface((GxVpuProperty_LayerMainSurface *) property);
			break;
		}
	case GxVpuPropertyID_LayerEnable:{
			unsigned long flag = gxav_device_spin_lock_irqsave(module->inode->dev);
			retVal = gxav_vpu_SetLayerEnable((GxVpuProperty_LayerEnable *) property);
			gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);
			break;
		}
	case GxVpuPropertyID_LayerAntiFlicker:{
			retVal = gxav_vpu_SetLayerAntiFlicker((GxVpuProperty_LayerAntiFlicker *) property);
			break;
		}
	case GxVpuPropertyID_LayerOnTop:{
			retVal = gxav_vpu_SetLayerOnTop((GxVpuProperty_LayerOnTop *) property);
			break;
		}
	case GxVpuPropertyID_LayerVideoMode:{
			retVal = gxav_vpu_SetLayerVideoMode((GxVpuProperty_LayerVideoMode *) property);
			break;
		}
	case GxVpuPropertyID_LayerMixConfig:{
			retVal = gxav_vpu_SetLayerMixConfig((GxVpuProperty_LayerMixConfig*) property);
			break;
		}
	case GxVpuPropertyID_DestroySurface:{
			retVal = gxav_vpu_SetDestroySurface((GxVpuProperty_DestroySurface *) property);
			break;
		}
	case GxVpuPropertyID_Palette:{
			retVal = gxav_vpu_SetPalette((GxVpuProperty_Palette *) property);
			break;
		}
	case GxVpuPropertyID_Alpha:{
			retVal = gxav_vpu_SetAlpha((GxVpuProperty_Alpha *) property);
			break;
		}
	case GxVpuPropertyID_ColorFormat:{
			retVal = gxav_vpu_SetColorFormat((GxVpuProperty_ColorFormat *) property);
			break;
		}
	case GxVpuPropertyID_ColorKey:{
			retVal = gxav_vpu_SetColorKey((GxVpuProperty_ColorKey *) property);
			break;
		}
	case GxVpuPropertyID_BackColor:{
			retVal = gxav_vpu_SetBackColor((GxVpuProperty_BackColor *) property);
			break;
		}
	case GxVpuPropertyID_Point:{
			retVal = gxav_vpu_SetPoint((GxVpuProperty_Point *) property);
			break;
		}
	case GxVpuPropertyID_DrawLine:{
			retVal = gxav_vpu_SetDrawLine((GxVpuProperty_DrawLine *) property);
			break;
		}
	case GxVpuPropertyID_Blit:{
			retVal = gxav_vpu_SetBlit((GxVpuProperty_Blit *) property);
			break;
		}

	case GxVpuPropertyID_DfbBlit:{
			retVal = gxav_vpu_SetDfbBlit((GxVpuProperty_DfbBlit *) property);
			break;
		}

	case GxVpuPropertyID_BatchDfbBlit:{
			retVal = gxav_vpu_SetBatchDfbBlit((GxVpuProperty_BatchDfbBlit *) property);
			break;
		}

	case GxVpuPropertyID_FillRect:{
			retVal = gxav_vpu_SetFillRect((GxVpuProperty_FillRect *) property);
			break;
		}
	case GxVpuPropertyID_FillPolygon:{
			retVal = gxav_vpu_SetFillPolygon((GxVpuProperty_FillPolygon *) property);
			break;
		}
	case GxVpuPropertyID_SetGAMode:{
			retVal = gxav_vpu_SetGAMode((GxVpuProperty_SetGAMode *) property);
			break;
		}
	case GxVpuPropertyID_WaitUpdate:{
			retVal = gxav_vpu_SetWaitUpdate((GxVpuProperty_WaitUpdate *) property);
			break;
		}
	case GxVpuPropertyID_BeginUpdate:{
			retVal = gxav_vpu_SetBeginUpdate((GxVpuProperty_BeginUpdate *) property);
			break;
		}
	case GxVpuPropertyID_EndUpdate:{
			retVal = gxav_vpu_SetEndUpdate((GxVpuProperty_EndUpdate *) property);
			break;
		}
	case GxVpuPropertyID_ConvertColor:{
			retVal = gxav_vpu_SetConvertColor((GxVpuProperty_ConvertColor *) property);
			break;
		}
	case GxVpuPropertyID_VirtualResolution:{
			retVal = gxav_vpu_SetVirtualResolution((GxVpuProperty_Resolution *) property);
			break;
		}
	case GxVpuPropertyID_VbiEnable:{
			retVal = gxav_vpu_SetVbiEnable((GxVpuProperty_VbiEnable *) property);
			break;
		}
	case GxVpuPropertyID_ZoomSurface:{
			retVal = gxav_vpu_ZoomSurface((GxVpuProperty_ZoomSurface*) property);
			break;
		}
	case GxVpuPropertyID_Complet:{
			retVal = gxav_vpu_Complet((GxVpuProperty_Complet*) property);
			break;
		}
	case GxVpuPropertyID_TurnSurface:{
			retVal = gxav_vpu_TurnSurface((GxVpuProperty_TurnSurface*) property);
			break;
		}
	case GxVpuPropertyID_LayerClipport: {
		return gxav_vpu_SetLayerClipport((GxVpuProperty_LayerClipport*) property);
    }

	case GxVpuPropertyID_RegistSurface: {
		return gxav_vpu_RegistSurface((GxVpuProperty_RegistSurface*) property);
	}
	case GxVpuPropertyID_UnregistSurface: {
		return gxav_vpu_UnregistSurface((GxVpuProperty_UnregistSurface*) property);
	}
	case GxVpuPropertyID_FlipSurface: {
		return gxav_vpu_FlipSurface((GxVpuProperty_FlipSurface*) property);
	}
	case GxVpuPropertyID_AddRegion: {
		return gxav_vpu_AddRegion((GxVpuProperty_AddRegion*)property);
	}
	case GxVpuPropertyID_RemoveRegion: {
		return gxav_vpu_RemoveRegion((GxVpuProperty_RemoveRegion*)property);
	}
	case GxVpuPropertyID_UpdateRegion: {
		return gxav_vpu_UpdateRegion((GxVpuProperty_UpdateRegion*)property);
	}
	case GxVpuPropertyID_OsdOverlay: {
		return gxav_vpu_SetOsdOverlay((GxVpuProperty_OsdOverlay*)property);
	}
	case GxVpuPropertyID_GAMMAConfig:
		retVal = gxav_vpu_SetGAMMA((GxVpuProperty_GAMMAConfig *) property);
		break;
	case GxVpuPropertyID_GreenBlueConfig:
		retVal = gxav_vpu_SetGreenBlue((GxVpuProperty_GreenBlueConfig *)property);
		break;
	case GxVpuPropertyID_EnhanceConfig:
		retVal = gxav_vpu_SetEnhance((GxVpuProperty_EnhanceConfig *)property);
		break;
	case GxVpuPropertyID_MixFilter:
		retVal = gxav_vpu_SetMixFilter((GxVpuProperty_MixFilter *)property);
		break;
	case GxVpuPropertyID_SVPUConfig:
		retVal = gxav_vpu_SetSVPUConfig((GxVpuProperty_SVPUConfig *)property);
		break;
	case GxVpuPropertyID_Sync:
		retVal = gxav_vpu_Sync((GxVpuProperty_Sync *)property);
		break;
	case GxVpuPropertyID_ModifySurface:
		retVal = gxav_vpu_ModifySurface((GxVpuProperty_ModifySurface *)property);
		break;
	case GxVpuPropertyID_LayerAutoZoom:
		retVal = gxav_vpu_SetLayerAutoZoom((GxVpuProperty_LayerAutoZoom *)property);
		break;
	case GxVpuPropertyID_GAMMACorrect:
		retVal = gxav_vpu_SetGAMMACorrect((GxVpuProperty_GAMMACorrect *)property);
		break;
	case GxVpuPropertyID_MixerLayers:
		retVal = gxav_vpu_SetMixerLayers((GxVpuProperty_MixerLayers*)property);
		break;
	case GxVpuPropertyID_MixerBackcolor:
		retVal = gxav_vpu_SetMixerBackcolor((GxVpuProperty_MixerBackcolor*)property);
		break;
	default: {
		VPU_PRINTF("the parameter which vpu's property_id is wrong or not support! ID = %d\n",property_id);
		return -2;
	}
	}
	return retVal;

}

int gx_vpu_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int retVal ;

	retVal = GxVpu_ParameterCheck(property_id,size);

	switch (property_id) {
	case GxVpuPropertyID_GetEntries:
			retVal = gxav_vpu_GetEntries((GxVpuProperty_GetEntries *) property);
			break;

	case GxVpuPropertyID_RWPalette:{
			retVal = gxav_vpu_RPalette((GxVpuProperty_RWPalette *) property);
			break;
		}
	case GxVpuPropertyID_CreatePalette:{
			retVal = gxav_vpu_GetCreatePalette((GxVpuProperty_CreatePalette *) property);
			break;
		}
	case GxVpuPropertyID_LayerViewport:{
			retVal = gxav_vpu_GetLayerViewport((GxVpuProperty_LayerViewport *) property);
			break;
		}
	case GxVpuPropertyID_LayerClipport:{
			retVal = gxav_vpu_GetLayerClipport((GxVpuProperty_LayerClipport *) property);
			break;
		}
	case GxVpuPropertyID_LayerMainSurface:{
			retVal = gxav_vpu_GetLayerMainSurface((GxVpuProperty_LayerMainSurface *) property);
			break;
		}
	case GxVpuPropertyID_LayerEnable:{
			retVal = gxav_vpu_GetLayerEnable((GxVpuProperty_LayerEnable *) property);
			break;
		}
	case GxVpuPropertyID_LayerAntiFlicker:{
			retVal = gxav_vpu_GetLayerAntiFlicker((GxVpuProperty_LayerAntiFlicker *) property);
			break;
		}
	case GxVpuPropertyID_LayerOnTop:{
			retVal = gxav_vpu_GetLayerOnTop((GxVpuProperty_LayerOnTop *) property);
			break;
		}
	case GxVpuPropertyID_LayerVideoMode:{
			retVal = gxav_vpu_GetLayerVideoMode((GxVpuProperty_LayerVideoMode *) property);
			break;
		}
	case GxVpuPropertyID_LayerCapture:{
			retVal = gxav_vpu_GetLayerCapture((GxVpuProperty_LayerCapture *) property);
			break;
		}
	case GxVpuPropertyID_CreateSurface:{
			retVal = gxav_vpu_GetCreateSurface((GxVpuProperty_CreateSurface *) property);
			break;
		}
	case GxVpuPropertyID_ReadSurface:{
			retVal = gxav_vpu_GetReadSurface((GxVpuProperty_ReadSurface *) property);
			break;
		}
	case GxVpuPropertyID_Palette:{
			retVal = gxav_vpu_GetPalette((GxVpuProperty_Palette *) property);
			break;
		}
	case GxVpuPropertyID_Alpha:{
			retVal = gxav_vpu_GetAlpha((GxVpuProperty_Alpha *) property);
			break;
		}
	case GxVpuPropertyID_ColorFormat:{
			retVal = gxav_vpu_GetColorFormat((GxVpuProperty_ColorFormat *) property);
			break;
		}
	case GxVpuPropertyID_ColorKey:{
			retVal = gxav_vpu_GetColorKey((GxVpuProperty_ColorKey *) property);
			break;
		}
	case GxVpuPropertyID_BackColor:{
			retVal = gxav_vpu_GetBackColor((GxVpuProperty_BackColor *) property);
			break;
		}
	case GxVpuPropertyID_Point:{
			retVal = gxav_vpu_GetPoint((GxVpuProperty_Point *) property);
			break;
		}
	case GxVpuPropertyID_ConvertColor:{
			retVal = gxav_vpu_GetConvertColor((GxVpuProperty_ConvertColor *) property);
			break;
		}
	case GxVpuPropertyID_VirtualResolution:{
			retVal = gxav_vpu_GetVirtualResolution((GxVpuProperty_Resolution *) property);
			break;
		}
	case GxVpuPropertyID_VbiEnable:{
			retVal = gxav_vpu_GetVbiEnable((GxVpuProperty_VbiEnable *) property);
			break;
		}
	case GxVpuPropertyID_VbiCreateBuffer:{
			retVal = gxav_vpu_GetVbiCreateBuffer((GxVpuProperty_VbiCreateBuffer *) property);
			break;
		}
	case GxVpuPropertyID_VbiReadAddress:{
			retVal = gxav_vpu_GetVbiReadAddress((GxVpuProperty_VbiReadAddress *) property);
			break;
		}

	case GxVpuPropertyID_GetIdleSurface: {
		return gxav_vpu_GetIdleSurface((GxVpuProperty_GetIdleSurface*) property);
	}
	case GxVpuPropertyID_GAMMAConfig:
		retVal = gxav_vpu_GetGAMMA((GxVpuProperty_GAMMAConfig *) property);
		break;
	case GxVpuPropertyID_GreenBlueConfig:
		retVal = gxav_vpu_GetGreenBlue((GxVpuProperty_GreenBlueConfig *)property);
		break;
	case GxVpuPropertyID_EnhanceConfig:
		retVal = gxav_vpu_GetEnhance((GxVpuProperty_EnhanceConfig *)property);
		break;
	case GxVpuPropertyID_MixFilter:
		retVal = gxav_vpu_GetMixFilter((GxVpuProperty_MixFilter *)property);
		break;
	case GxVpuPropertyID_LayerAutoZoom:
		retVal = gxav_vpu_GetLayerAutoZoom((GxVpuProperty_LayerAutoZoom *)property);
		break;
	default:
		VPU_PRINTF("the parameter which vpu's property_id is wrong or not support! ID =%d\n",property_id);
		return -2;

	}

	return retVal;
}

struct gxav_module_inode *gx_vpu_ga_interrupt(struct gxav_module_inode *inode, int irq)
{
	int ret = 0;

	if ( inode == NULL )
	{
		VPU_PRINTF("vpu ga interrup inode error! inode=%p \t irq_num=%x\n",inode,irq);
		return NULL;
	}

	ret = gxav_vpu_Ga_Interrupt(irq);

	if ( ret!=0 ) {
		VPU_PRINTF( " vpu ga interrupt error " );
	}
	return inode;
}

