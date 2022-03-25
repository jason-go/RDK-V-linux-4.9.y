#include "vpu_hal.h"
#include "avcore.h"

static struct vpu_ops* _vpu_ops;

int gxav_vpu_init(struct gxav_module_ops* ops)
{
	_vpu_ops = ops->priv;

	if (_vpu_ops && _vpu_ops->init)
		return _vpu_ops->init();

	return 0;
}

int gxav_vpu_cleanup(void)
{
	if (_vpu_ops && _vpu_ops->cleanup)
		return _vpu_ops->cleanup();
	_vpu_ops = NULL;

	return 0;
}

int gxav_vpu_open(void)
{
	if (_vpu_ops && _vpu_ops->open){
		return _vpu_ops->open();
	}
	return 0;
}

int gxav_vpu_close(void)
{
	if (_vpu_ops && _vpu_ops->close){
		return _vpu_ops->close();
	}
	return 0;
}

int gxav_vpu_reset(void)
{
	if (_vpu_ops && _vpu_ops->reset){
		return _vpu_ops->reset();
	}
	return 0;
}

int gxav_vpu_WPalette(GxVpuProperty_RWPalette* property)
{
	if (_vpu_ops && _vpu_ops->WPalette){
		return _vpu_ops->WPalette(property);
	}
	return 0;
}

int gxav_vpu_DestroyPalette(GxVpuProperty_DestroyPalette* property)
{
	if (_vpu_ops && _vpu_ops->DestroyPalette){
		return _vpu_ops->DestroyPalette(property);
	}
	return 0;
}

int gxav_vpu_SurfaceBindPalette(GxVpuProperty_SurfaceBindPalette* property)
{
	if (_vpu_ops && _vpu_ops->SurfaceBindPalette){
		return _vpu_ops->SurfaceBindPalette(property);
	}
	return 0;
}

int gxav_vpu_SetLayerViewport(GxVpuProperty_LayerViewport* property)
{
	int ret = -1;

	if (_vpu_ops && _vpu_ops->SetLayerViewport) {
		ret = _vpu_ops->SetLayerViewport(property);
		if(property && property->layer == GX_LAYER_VPP)
			gx_mdelay(40);
	}

	return ret;
}

int gxav_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface* property)
{
	if (_vpu_ops && _vpu_ops->UnsetLayerMainSurface){
		return _vpu_ops->UnsetLayerMainSurface(property);
	}
	return 0;
}

int gxav_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerMainSurface){
		return _vpu_ops->SetLayerMainSurface(property);
	}
	return 0;
}

int gxav_vpu_SetLayerEnable(GxVpuProperty_LayerEnable* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerEnable){
		return _vpu_ops->SetLayerEnable(property);
	}
	return 0;
}

int gxav_vpu_SetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerAntiFlicker){
		return _vpu_ops->SetLayerAntiFlicker(property);
	}
	return 0;
}

int gxav_vpu_SetLayerOnTop(GxVpuProperty_LayerOnTop* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerOnTop){
		return _vpu_ops->SetLayerOnTop(property);
	}
	return 0;
}

int gxav_vpu_SetLayerVideoMode(GxVpuProperty_LayerVideoMode* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerVideoMode){
		return _vpu_ops->SetLayerVideoMode(property);
	}
	return 0;
}

int gxav_vpu_SetLayerMixConfig(GxVpuProperty_LayerMixConfig* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerMixConfig){
		return _vpu_ops->SetLayerMixConfig(property);
	}
	return 0;
}

int gxav_vpu_ModifySurface(GxVpuProperty_ModifySurface* property)
{
	if (_vpu_ops && _vpu_ops->ModifySurface){
		return _vpu_ops->ModifySurface(property);
	}
	return 0;
}

int gxav_vpu_SetDestroySurface(GxVpuProperty_DestroySurface* property)
{
	if (_vpu_ops && _vpu_ops->SetDestroySurface){
		return _vpu_ops->SetDestroySurface(property);
	}
	return 0;
}

int gxav_vpu_SetPalette(GxVpuProperty_Palette* property)
{
	if (_vpu_ops && _vpu_ops->SetPalette){
		return _vpu_ops->SetPalette(property);
	}
	return 0;
}

int gxav_vpu_SetAlpha(GxVpuProperty_Alpha* property)
{
	if (_vpu_ops && _vpu_ops->SetAlpha){
		return _vpu_ops->SetAlpha(property);
	}
	return 0;
}

int gxav_vpu_SetColorFormat(GxVpuProperty_ColorFormat* property)
{
	if (_vpu_ops && _vpu_ops->SetColorFormat){
		return _vpu_ops->SetColorFormat(property);
	}
	return 0;
}

int gxav_vpu_SetColorKey(GxVpuProperty_ColorKey* property)
{
	if (_vpu_ops && _vpu_ops->SetColorKey){
		return _vpu_ops->SetColorKey(property);
	}
	return 0;
}

int gxav_vpu_SetBackColor(GxVpuProperty_BackColor* property)
{
	if (_vpu_ops && _vpu_ops->SetBackColor){
		return _vpu_ops->SetBackColor(property);
	}
	return 0;
}

int gxav_vpu_SetPoint(GxVpuProperty_Point* property)
{
	if (_vpu_ops && _vpu_ops->SetPoint){
		return _vpu_ops->SetPoint(property);
	}
	return 0;
}

int gxav_vpu_SetDrawLine(GxVpuProperty_DrawLine* property)
{
	if (_vpu_ops && _vpu_ops->SetDrawLine){
		return _vpu_ops->SetDrawLine(property);
	}
	return 0;
}

int gxav_vpu_SetConvertColor(GxVpuProperty_ConvertColor* property)
{
	if (_vpu_ops && _vpu_ops->SetConvertColor){
		return _vpu_ops->SetConvertColor(property);
	}
	return 0;
}

int gxav_vpu_SetVirtualResolution(GxVpuProperty_Resolution* property)
{
	if (_vpu_ops && _vpu_ops->SetVirtualResolution){
		return _vpu_ops->SetVirtualResolution(property);
	}
	return 0;
}

int gxav_vpu_SetVbiEnable(GxVpuProperty_VbiEnable* property)
{
	if (_vpu_ops && _vpu_ops->SetVbiEnable){
		return _vpu_ops->SetVbiEnable(property);
	}
	return 0;
}

int gxav_vpu_SetAspectRatio(GxVpuProperty_AspectRatio* property)
{
	int ret = -1;

	if (_vpu_ops && _vpu_ops->SetAspectRatio) {
		ret = _vpu_ops->SetAspectRatio(property);
		gx_mdelay(40);
	}

	return ret;
}

int gxav_vpu_SetTvScreen(GxVpuProperty_TvScreen* property)
{
	if (_vpu_ops && _vpu_ops->SetTvScreen){
		return _vpu_ops->SetTvScreen(property);
	}
	return 0;
}

int gxav_vpu_SetLayerEnablePatch(GxVpuProperty_LayerEnable* property)
{
	if (_vpu_ops && _vpu_ops->SetLayerEnablePatch){
		return _vpu_ops->SetLayerEnablePatch(property);
	}
	return 0;
}

int gxav_vpu_GetEntries(GxVpuProperty_GetEntries* property)
{
	if (_vpu_ops && _vpu_ops->GetEntries){
		return _vpu_ops->GetEntries(property);
	}
	return 0;
}

int gxav_vpu_RPalette(GxVpuProperty_RWPalette* property)
{
	if (_vpu_ops && _vpu_ops->RPalette){
		return _vpu_ops->RPalette(property);
	}
	return 0;
}

int gxav_vpu_GetCreatePalette(GxVpuProperty_CreatePalette* property)
{
	if (_vpu_ops && _vpu_ops->GetCreatePalette){
		return _vpu_ops->GetCreatePalette(property);
	}
	return 0;
}

int gxav_vpu_GetLayerViewport(GxVpuProperty_LayerViewport* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerViewport) {
		return _vpu_ops->GetLayerViewport(property);
	}
	return 0;
}

int gxav_vpu_GetLayerClipport(GxVpuProperty_LayerClipport* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerClipport){
		return _vpu_ops->GetLayerClipport(property);
	}
	return 0;
}

int gxav_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerMainSurface){
		return _vpu_ops->GetLayerMainSurface(property);
	}
	return 0;
}

int gxav_vpu_GetLayerEnable(GxVpuProperty_LayerEnable* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerEnable){
		return _vpu_ops->GetLayerEnable(property);
	}
	return 0;
}

int gxav_vpu_GetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerAntiFlicker){
		return _vpu_ops->GetLayerAntiFlicker(property);
	}
	return 0;
}

int gxav_vpu_GetLayerOnTop(GxVpuProperty_LayerOnTop* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerOnTop){
		return _vpu_ops->GetLayerOnTop(property);
	}
	return 0;
}

int gxav_vpu_GetLayerVideoMode(GxVpuProperty_LayerVideoMode* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerVideoMode){
		return _vpu_ops->GetLayerVideoMode(property);
	}
	return 0;
}

int gxav_vpu_GetLayerCapture(GxVpuProperty_LayerCapture* property)
{
	if (_vpu_ops && _vpu_ops->GetLayerCapture){
		return _vpu_ops->GetLayerCapture(property);
	}
	return 0;
}

int gxav_vpu_GetCreateSurface(GxVpuProperty_CreateSurface* property)
{
	if (_vpu_ops && _vpu_ops->GetCreateSurface){
		return _vpu_ops->GetCreateSurface(property);
	}
	return 0;
}

int gxav_vpu_GetReadSurface(GxVpuProperty_ReadSurface* property)
{
	if (_vpu_ops && _vpu_ops->GetReadSurface){
		return _vpu_ops->GetReadSurface(property);
	}
	return 0;
}

int gxav_vpu_GetPalette(GxVpuProperty_Palette* property)
{
	if (_vpu_ops && _vpu_ops->GetPalette){
		return _vpu_ops->GetPalette(property);
	}
	return 0;
}

int gxav_vpu_GetAlpha(GxVpuProperty_Alpha* property)
{
	if (_vpu_ops && _vpu_ops->GetAlpha){
		return _vpu_ops->GetAlpha(property);
	}
	return 0;
}

int gxav_vpu_GetColorFormat(GxVpuProperty_ColorFormat* property)
{
	if (_vpu_ops && _vpu_ops->GetColorFormat){
		return _vpu_ops->GetColorFormat(property);
	}
	return 0;
}

int gxav_vpu_GetColorKey(GxVpuProperty_ColorKey* property)
{
	if (_vpu_ops && _vpu_ops->GetColorKey){
		return _vpu_ops->GetColorKey(property);
	}
	return 0;
}

int gxav_vpu_GetBackColor(GxVpuProperty_BackColor* property)
{
	if (_vpu_ops && _vpu_ops->GetBackColor){
		return _vpu_ops->GetBackColor(property);
	}
	return 0;
}

int gxav_vpu_GetPoint(GxVpuProperty_Point* property)
{
	if (_vpu_ops && _vpu_ops->GetPoint){
		return _vpu_ops->GetPoint(property);
	}
	return 0;
}

int gxav_vpu_GetConvertColor(GxVpuProperty_ConvertColor* property)
{
	if (_vpu_ops && _vpu_ops->GetConvertColor){
		return _vpu_ops->GetConvertColor(property);
	}
	return 0;
}

int gxav_vpu_GetVirtualResolution(GxVpuProperty_Resolution* property)
{
	if (_vpu_ops && _vpu_ops->GetVirtualResolution){
		return _vpu_ops->GetVirtualResolution(property);
	}
	return 0;
}

int gxav_vpu_GetVbiEnable(GxVpuProperty_VbiEnable* property)
{
	if (_vpu_ops && _vpu_ops->GetVbiEnable){
		return _vpu_ops->GetVbiEnable(property);
	}
	return 0;
}

int gxav_vpu_GetVbiCreateBuffer(GxVpuProperty_VbiCreateBuffer* property)
{
	if (_vpu_ops && _vpu_ops->GetVbiCreateBuffer){
		return _vpu_ops->GetVbiCreateBuffer(property);
	}
	return 0;
}

int gxav_vpu_GetVbiReadAddress(GxVpuProperty_VbiReadAddress* property)
{
	if (_vpu_ops && _vpu_ops->GetVbiReadAddress){
		return _vpu_ops->GetVbiReadAddress(property);
	}
	return 0;
}

int gxav_vpu_SetLayerClipport(GxVpuProperty_LayerClipport* property)
{
	int ret = -1;

	if (_vpu_ops && _vpu_ops->SetLayerClipport) {
		ret = _vpu_ops->SetLayerClipport(property);
		if(property && property->layer == GX_LAYER_VPP)
			gx_mdelay(40);
	}

	return ret;
}


int gxav_vpu_SetBlit(GxVpuProperty_Blit* property)
{
	if (_vpu_ops && _vpu_ops->SetBlit){
		return _vpu_ops->SetBlit(property);
	}
	return 0;
}

int gxav_vpu_SetDfbBlit(GxVpuProperty_DfbBlit* property)
{
	if (_vpu_ops && _vpu_ops->SetDfbBlit){
		return _vpu_ops->SetDfbBlit(property);
	}
	return 0;
}

int gxav_vpu_SetBatchDfbBlit(GxVpuProperty_BatchDfbBlit* property)
{
	if (_vpu_ops && _vpu_ops->SetBatchDfbBlit){
		return _vpu_ops->SetBatchDfbBlit(property);
	}
	return 0;
}

int gxav_vpu_SetFillRect(GxVpuProperty_FillRect* property)
{
	if (_vpu_ops && _vpu_ops->SetFillRect){
		return _vpu_ops->SetFillRect(property);
	}
	return 0;
}

int gxav_vpu_SetFillPolygon(GxVpuProperty_FillPolygon* property)
{
	if (_vpu_ops && _vpu_ops->SetFillPolygon){
		return _vpu_ops->SetFillPolygon(property);
	}
	return 0;
}

int gxav_vpu_SetGAMode(GxVpuProperty_SetGAMode* property)
{
	if (_vpu_ops && _vpu_ops->SetGAMode){
		return _vpu_ops->SetGAMode(property);
	}
	return 0;
}

int gxav_vpu_SetWaitUpdate(GxVpuProperty_WaitUpdate* property)
{
	if (_vpu_ops && _vpu_ops->SetWaitUpdate){
		return _vpu_ops->SetWaitUpdate(property);
	}
	return 0;
}

int gxav_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate* property)
{
	if (_vpu_ops && _vpu_ops->SetBeginUpdate){
		return _vpu_ops->SetBeginUpdate(property);
	}
	return 0;
}

int gxav_vpu_SetEndUpdate(GxVpuProperty_EndUpdate* property)
{
	if (_vpu_ops && _vpu_ops->SetEndUpdate){
		return _vpu_ops->SetEndUpdate(property);
	}
	return 0;
}

int gxav_vpu_ZoomSurface(GxVpuProperty_ZoomSurface* property)
{
	if (_vpu_ops && _vpu_ops->ZoomSurface){
		return _vpu_ops->ZoomSurface(property);
	}
	return 0;
}

int gxav_vpu_Complet(GxVpuProperty_Complet* property)
{
	if (_vpu_ops && _vpu_ops->Complet){
		return _vpu_ops->Complet(property);
	}
	return 0;
}

int gxav_vpu_TurnSurface(GxVpuProperty_TurnSurface* property)
{
	if (_vpu_ops && _vpu_ops->TurnSurface){
		return _vpu_ops->TurnSurface(property);
	}
	return 0;
}

int gxav_vpu_Ga_Interrupt(int irq)
{
	if (_vpu_ops && _vpu_ops->Ga_Interrupt){
		return _vpu_ops->Ga_Interrupt(irq);
	}
	return 0;
}


int gxav_vpu_SetVoutIsready(int ready)
{
	if (_vpu_ops && _vpu_ops->SetVoutIsready){

		return _vpu_ops->SetVoutIsready(ready);
	}
	return 0;
}

int gxav_vpu_DACEnable(GxVpuDacID id, int enable)
{
	if (_vpu_ops && _vpu_ops->DACEnable){
		return _vpu_ops->DACEnable(id, enable);
	}
	return 0;
}

int gxav_vpu_DACPower(GxVpuDacID id)
{
	if (_vpu_ops && _vpu_ops->DACPower){
		return _vpu_ops->DACPower(id);
	}
	return 0;
}

int gxav_vpu_DACSource(GxVpuDacID id, GxVpuDacSrc source)
{
	if (_vpu_ops && _vpu_ops->DACSource){
		return _vpu_ops->DACSource(id, source);
	}
	return 0;
}

int gxav_vpu_vpp_get_base_line(void)
{
	if (_vpu_ops && _vpu_ops->vpp_get_base_line){
		return _vpu_ops->vpp_get_base_line();
	}
	return 0;
}

int gxav_vpu_vpp_set_base_line(unsigned int value)
{
	if (_vpu_ops && _vpu_ops->vpp_set_base_line){
		return _vpu_ops->vpp_set_base_line(value);
	}
	return 0;
}

int gxav_vpu_disp_get_buff_id(void)
{
	if (_vpu_ops && _vpu_ops->disp_get_buff_id){
		return _vpu_ops->disp_get_buff_id();
	}
	return 0;
}

int gxav_vpu_disp_set_rst(void)
{
	if (_vpu_ops && _vpu_ops->disp_set_rst){
		return _vpu_ops->disp_set_rst();
	}
	return 0;
}

int gxav_vpu_disp_clr_rst(void)
{
	if (_vpu_ops && _vpu_ops->disp_clr_rst){
		return _vpu_ops->disp_clr_rst();
	}
	return 0;
}

int gxav_vpu_disp_get_view_active_cnt(void)
{
	if (_vpu_ops && _vpu_ops->disp_get_view_active_cnt){
		return _vpu_ops->disp_get_view_active_cnt();
	}
	return 0;
}

int gxav_vpu_disp_clr_field_error_int(void)
{
	if (_vpu_ops && _vpu_ops->disp_clr_field_error_int){
		return _vpu_ops->disp_clr_field_error_int();
	}
	return 0;
}

int gxav_vpu_disp_clr_field_start_int(void)
{
	if (_vpu_ops && _vpu_ops->disp_clr_field_start_int){
		return _vpu_ops->disp_clr_field_start_int();
	}
	return 0;
}

int gxav_vpu_disp_field_error_int_en(void)
{
	if (_vpu_ops && _vpu_ops->disp_field_error_int_en){
		return _vpu_ops->disp_field_error_int_en();
	}
	return 0;
}

int gxav_vpu_disp_field_start_int_en(void)
{
	if (_vpu_ops && _vpu_ops->disp_field_start_int_en){
		return _vpu_ops->disp_field_start_int_en();
	}
	return 0;
}

int gxav_vpu_disp_field_error_int_dis(void)
{
	if (_vpu_ops && _vpu_ops->disp_field_error_int_dis){
		return _vpu_ops->disp_field_error_int_dis();
	}
	return 0;
}

int gxav_vpu_disp_field_start_int_dis(void)
{
	if (_vpu_ops && _vpu_ops->disp_field_start_int_dis){
		return _vpu_ops->disp_field_start_int_dis();
	}
	return 0;
}

int gxav_vpu_disp_get_buff_cnt(void)
{
	if (_vpu_ops && _vpu_ops->disp_get_buff_cnt){
		return _vpu_ops->disp_get_buff_cnt();
	}
	return 0;
}

int gxav_vpu_disp_get_view_field(void)
{
	if (_vpu_ops && _vpu_ops->disp_get_view_field){
		return _vpu_ops->disp_get_view_field();
	}
	return 0;
}

int gxav_vpu_disp_get_display_buf(int i)
{
	if (_vpu_ops && _vpu_ops->disp_get_display_buf){
		return _vpu_ops->disp_get_display_buf(i);
	}
	return 0;
}

int gxav_vpu_disp_layer_enable(GxLayerID layer, int enable)
{
	if (_vpu_ops && _vpu_ops->disp_layer_enable){
		return _vpu_ops->disp_layer_enable(layer, enable);
	}
	return 0;
}


int gxav_vpu_vpp_set_stream_ratio(unsigned int ratio)
{
	if (_vpu_ops && _vpu_ops->vpp_set_stream_ratio){
		return _vpu_ops->vpp_set_stream_ratio(ratio);
	}
	return 0;
}

int gxav_vpu_vpp_get_actual_viewrect(GxAvRect*view_rect)
{
	if (_vpu_ops && _vpu_ops->vpp_get_actual_viewrect){
		return _vpu_ops->vpp_get_actual_viewrect(view_rect);
	}
	return 0;
}

int gxav_vpu_vpp_get_actual_cliprect(GxAvRect*clip_rect)
{
	if (_vpu_ops && _vpu_ops->vpp_get_actual_cliprect){
		return _vpu_ops->vpp_get_actual_cliprect(clip_rect);
	}
	return 0;
}

int gxav_vpu_vpp_set_actual_cliprect(GxAvRect*clip_rect)
{
	if (_vpu_ops && _vpu_ops->vpp_set_actual_cliprect){
		return _vpu_ops->vpp_set_actual_cliprect(clip_rect);
	}
	return 0;
}

int gxav_vpu_vpp_play_frame(struct frame *frame)
{
	if (_vpu_ops && _vpu_ops->vpp_play_frame) {
		return _vpu_ops->vpp_play_frame(frame);
	}

	return 0;
}

int gxav_vpu_RegistSurface(GxVpuProperty_RegistSurface *property)
{
	if (_vpu_ops && _vpu_ops->RegistSurface){
		return _vpu_ops->RegistSurface(property);
	}
	return 0;
}

int gxav_vpu_UnregistSurface(GxVpuProperty_UnregistSurface *property)
{
	if (_vpu_ops && _vpu_ops->UnregistSurface){
		return _vpu_ops->UnregistSurface(property);
	}
	return 0;
}

int gxav_vpu_GetIdleSurface(GxVpuProperty_GetIdleSurface *property)
{
	if (_vpu_ops && _vpu_ops->GetIdleSurface){
		return _vpu_ops->GetIdleSurface(property);
	}
	return 0;
}

int gxav_vpu_FlipSurface(GxVpuProperty_FlipSurface *property)
{
	if (_vpu_ops && _vpu_ops->FlipSurface){
		return _vpu_ops->FlipSurface(property);
	}
	return 0;
}

int gxav_vpu_PanDisplay(GxLayerID layer, void *buffer)
{
	if (_vpu_ops && _vpu_ops->PanDisplay){
		return _vpu_ops->PanDisplay(layer, buffer);
	}
	return 0;
}

int gxav_vpu_VpuIsrCallback(void *arg)
{
	if (_vpu_ops && _vpu_ops->VpuIsrCallback){
		return _vpu_ops->VpuIsrCallback(arg);
	}
	return 0;
}

int gxav_vpu_AddRegion(GxVpuProperty_AddRegion *property)
{
	if (_vpu_ops && _vpu_ops->AddRegion) {
		return _vpu_ops->AddRegion(property);
	}
	return 0;
}

int gxav_vpu_RemoveRegion(GxVpuProperty_RemoveRegion *property)
{
	if (_vpu_ops && _vpu_ops->RemoveRegion) {
		return _vpu_ops->RemoveRegion(property);
	}
	return 0;
}

int gxav_vpu_UpdateRegion(GxVpuProperty_UpdateRegion *property)
{
	if (_vpu_ops && _vpu_ops->UpdateRegion) {
		return _vpu_ops->UpdateRegion(property);
	}
	return 0;
}

int gxav_vpu_GetOsdOverlay(GxVpuProperty_OsdOverlay *property)
{
	if (_vpu_ops && _vpu_ops->GetOsdOverlay) {
		return _vpu_ops->GetOsdOverlay(property);
	}
	return 0;
}

int gxav_vpu_SetOsdOverlay(GxVpuProperty_OsdOverlay *property)
{
	if (_vpu_ops && _vpu_ops->GetOsdOverlay) {
		return _vpu_ops->SetOsdOverlay(property);
	}
	return 0;
}

int gxav_vpu_SetGAMMA(GxVpuProperty_GAMMAConfig *property) {
	if( _vpu_ops && _vpu_ops->SetGAMMAConfig ) {
		return _vpu_ops->SetGAMMAConfig(property);
	}

	return (0);
}

int gxav_vpu_GetGAMMA(GxVpuProperty_GAMMAConfig *property) {
	if( _vpu_ops && _vpu_ops->GetGAMMAConfig ) {
		return _vpu_ops->GetGAMMAConfig(property);
	}

	return (0);
}

int gxav_vpu_SetGreenBlue(GxVpuProperty_GreenBlueConfig *property) {
	if( _vpu_ops && _vpu_ops->SetGreenBlueConfig ) {
		return _vpu_ops->SetGreenBlueConfig(property);
	}

	return (0);
}

int gxav_vpu_GetGreenBlue(GxVpuProperty_GreenBlueConfig *property) {
	if( _vpu_ops && _vpu_ops->GetGreenBlueConfig ) {
		return _vpu_ops->GetGreenBlueConfig(property);
	}

	return (0);
}

int gxav_vpu_SetEnhance(GxVpuProperty_EnhanceConfig *property) {
	if( _vpu_ops && _vpu_ops->SetEnhanceConfig ) {
		return _vpu_ops->SetEnhanceConfig(property);
	}

	return (0);
}

int gxav_vpu_GetEnhance(GxVpuProperty_EnhanceConfig *property) {
	if( _vpu_ops && _vpu_ops->GetEnhanceConfig ) {
		return _vpu_ops->GetEnhanceConfig(property);
	}

	return (0);
}

int gxav_vpu_SetMixFilter(GxVpuProperty_MixFilter *property) {
	if( _vpu_ops && _vpu_ops->SetMixFilter ) {
		return _vpu_ops->SetMixFilter(property);
	}

	return (0);
}

int gxav_vpu_SetSVPUConfig(GxVpuProperty_SVPUConfig *property) {
	if( _vpu_ops && _vpu_ops->SetSVPUConfig ) {
		return _vpu_ops->SetSVPUConfig(property);
	}

	return (0);
}

int gxav_vpu_Sync(GxVpuProperty_Sync *property)
{
	gx_dcache_flush_range(property->start_addr, property->end_addr);
	return (0);
}

int gxav_vpu_SetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	if( _vpu_ops && _vpu_ops->SetLayerAutoZoom ) {
		return (_vpu_ops->SetLayerAutoZoom(property));
	}

	return (0);
}

int gxav_vpu_SetGAMMACorrect(GxVpuProperty_GAMMACorrect *property)
{
	if( _vpu_ops && _vpu_ops->SetGAMMACorrect ) {
		return (_vpu_ops->SetGAMMACorrect(property));
	}

	return (0);
}

int gxav_vpu_GetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	if( _vpu_ops && _vpu_ops->GetLayerAutoZoom ) {
		return (_vpu_ops->GetLayerAutoZoom(property));
	}

	return (0);
}

int gxav_vpu_GetMixFilter(GxVpuProperty_MixFilter *property)
{
	if( _vpu_ops && _vpu_ops->GetMixFilter ) {
		return _vpu_ops->GetMixFilter(property);
	}

	return (0);
}

int gxav_vpu_SetMixerLayers(GxVpuProperty_MixerLayers *property)
{
	if( _vpu_ops && _vpu_ops->SetMixerLayers) {
		return _vpu_ops->SetMixerLayers(property);
	}

	return (0);
}

int gxav_vpu_SetMixerBackcolor(GxVpuProperty_MixerBackcolor *property)
{
	if( _vpu_ops && _vpu_ops->SetMixerBackcolor) {
		return _vpu_ops->SetMixerBackcolor(property);
	}

	return (0);
}

unsigned int gxav_vpu_get_scan_line(void)
{
	if (_vpu_ops && _vpu_ops->vpu_get_scan_line) {
		return _vpu_ops->vpu_get_scan_line();
	}
	return 0;
}

int gxav_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info)
{
	if (_vpu_ops && _vpu_ops->svpu_config) {
		return _vpu_ops->svpu_config(mode_vout0, mode_vout1, buf_info);
	}
	return 0;
}

int gxav_svpu_config_buf(SvpuSurfaceInfo *buf_info)
{
	if (_vpu_ops && _vpu_ops->svpu_config_buf) {
		return _vpu_ops->svpu_config_buf(buf_info);
	}
	return 0;
}

int gxav_svpu_get_buf(SvpuSurfaceInfo *buf_info)
{
	if (_vpu_ops && _vpu_ops->svpu_get_buf) {
		return _vpu_ops->svpu_get_buf(buf_info);
	}
	return 0;
}

int gxav_svpu_run(void)
{
	if (_vpu_ops && _vpu_ops->svpu_run) {
		return _vpu_ops->svpu_run();
	}
	return 0;
}

int gxav_svpu_stop(void)
{
	if (_vpu_ops && _vpu_ops->svpu_stop) {
		return _vpu_ops->svpu_stop();
	}
	return 0;
}

void gxav_vpu_set_triming_offset(GxVpuDacID dac_id, char offset)
{
	if (_vpu_ops && _vpu_ops->set_triming_offset) {
		return _vpu_ops->set_triming_offset(dac_id, offset);
	}
}

EXPORT_SYMBOL(gxav_vpu_WPalette);
EXPORT_SYMBOL(gxav_vpu_DestroyPalette);
EXPORT_SYMBOL(gxav_vpu_SurfaceBindPalette);
EXPORT_SYMBOL(gxav_vpu_SetLayerViewport);
EXPORT_SYMBOL(gxav_vpu_SetLayerMainSurface);
EXPORT_SYMBOL(gxav_vpu_UnsetLayerMainSurface);
EXPORT_SYMBOL(gxav_vpu_SetLayerEnable);
EXPORT_SYMBOL(gxav_vpu_SetLayerAntiFlicker);
EXPORT_SYMBOL(gxav_vpu_SetLayerOnTop);
EXPORT_SYMBOL(gxav_vpu_SetLayerVideoMode);
EXPORT_SYMBOL(gxav_vpu_SetLayerMixConfig);
EXPORT_SYMBOL(gxav_vpu_ModifySurface);
EXPORT_SYMBOL(gxav_vpu_SetDestroySurface);
EXPORT_SYMBOL(gxav_vpu_SetPalette);
EXPORT_SYMBOL(gxav_vpu_SetAlpha);
EXPORT_SYMBOL(gxav_vpu_SetColorFormat);
EXPORT_SYMBOL(gxav_vpu_SetColorKey);
EXPORT_SYMBOL(gxav_vpu_SetBackColor);
EXPORT_SYMBOL(gxav_vpu_SetPoint);
EXPORT_SYMBOL(gxav_vpu_SetDrawLine);
EXPORT_SYMBOL(gxav_vpu_SetConvertColor);
EXPORT_SYMBOL(gxav_vpu_SetVirtualResolution);
EXPORT_SYMBOL(gxav_vpu_SetVbiEnable);
EXPORT_SYMBOL(gxav_vpu_SetAspectRatio);
EXPORT_SYMBOL(gxav_vpu_SetTvScreen);
EXPORT_SYMBOL(gxav_vpu_SetLayerEnablePatch);
EXPORT_SYMBOL(gxav_vpu_GetEntries);
EXPORT_SYMBOL(gxav_vpu_RPalette);
EXPORT_SYMBOL(gxav_vpu_GetCreatePalette);
EXPORT_SYMBOL(gxav_vpu_GetLayerViewport);
EXPORT_SYMBOL(gxav_vpu_GetLayerClipport);
EXPORT_SYMBOL(gxav_vpu_GetLayerMainSurface);
EXPORT_SYMBOL(gxav_vpu_GetLayerEnable);
EXPORT_SYMBOL(gxav_vpu_GetLayerAntiFlicker);
EXPORT_SYMBOL(gxav_vpu_GetLayerOnTop);
EXPORT_SYMBOL(gxav_vpu_GetLayerVideoMode);
EXPORT_SYMBOL(gxav_vpu_GetLayerCapture);
EXPORT_SYMBOL(gxav_vpu_GetCreateSurface);
EXPORT_SYMBOL(gxav_vpu_GetReadSurface);
EXPORT_SYMBOL(gxav_vpu_GetPalette);
EXPORT_SYMBOL(gxav_vpu_GetAlpha);
EXPORT_SYMBOL(gxav_vpu_GetColorFormat);
EXPORT_SYMBOL(gxav_vpu_GetColorKey);
EXPORT_SYMBOL(gxav_vpu_GetBackColor);
EXPORT_SYMBOL(gxav_vpu_GetPoint);
EXPORT_SYMBOL(gxav_vpu_GetConvertColor);
EXPORT_SYMBOL(gxav_vpu_GetVirtualResolution);
EXPORT_SYMBOL(gxav_vpu_GetVbiEnable);
EXPORT_SYMBOL(gxav_vpu_GetVbiCreateBuffer);
EXPORT_SYMBOL(gxav_vpu_GetVbiReadAddress);
EXPORT_SYMBOL(gxav_vpu_SetLayerClipport);
EXPORT_SYMBOL(gxav_vpu_SetBlit);
EXPORT_SYMBOL(gxav_vpu_SetDfbBlit);
EXPORT_SYMBOL(gxav_vpu_SetBatchDfbBlit);
EXPORT_SYMBOL(gxav_vpu_SetFillRect);
EXPORT_SYMBOL(gxav_vpu_SetFillPolygon);
EXPORT_SYMBOL(gxav_vpu_SetGAMode);
EXPORT_SYMBOL(gxav_vpu_SetWaitUpdate);
EXPORT_SYMBOL(gxav_vpu_SetBeginUpdate);
EXPORT_SYMBOL(gxav_vpu_SetEndUpdate);
EXPORT_SYMBOL(gxav_vpu_ZoomSurface);
EXPORT_SYMBOL(gxav_vpu_Complet);
EXPORT_SYMBOL(gxav_vpu_TurnSurface);
EXPORT_SYMBOL(gxav_vpu_Sync);
EXPORT_SYMBOL(gxav_vpu_Ga_Interrupt);
EXPORT_SYMBOL(gxav_vpu_RegistSurface);
EXPORT_SYMBOL(gxav_vpu_UnregistSurface);
EXPORT_SYMBOL(gxav_vpu_GetIdleSurface);
EXPORT_SYMBOL(gxav_vpu_FlipSurface);
EXPORT_SYMBOL(gxav_vpu_AddRegion);
EXPORT_SYMBOL(gxav_vpu_RemoveRegion);
EXPORT_SYMBOL(gxav_vpu_UpdateRegion);
EXPORT_SYMBOL(gxav_vpu_PanDisplay);
EXPORT_SYMBOL(gxav_vpu_SetLayerAutoZoom);
EXPORT_SYMBOL(gxav_vpu_GetLayerAutoZoom);
EXPORT_SYMBOL(gxav_vpu_SetGAMMACorrect);
EXPORT_SYMBOL(gxav_vpu_SetMixerLayers);
EXPORT_SYMBOL(gxav_vpu_SetMixerBackcolor);
EXPORT_SYMBOL(gxav_vpu_set_triming_offset);
