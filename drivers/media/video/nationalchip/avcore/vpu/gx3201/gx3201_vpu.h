#ifndef __GX3201_VPU_INTERTNAL_H__
#define __GX3201_VPU_INTERTNAL_H__

#include "gxav_vpu_propertytypes.h"

#include "gx3201_vpu_reg.h"
#include "gx3201_vpu_internel.h"

#define VPU_GA_INC_SRC      (62)

#define VPU_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

extern int Gx3201_Vpu_WPalette              ( GxVpuProperty_RWPalette            *property);
extern int Gx3201_Vpu_DestroyPalette        ( GxVpuProperty_DestroyPalette       *property);
extern int Gx3201_Vpu_SurfaceBindPalette    ( GxVpuProperty_SurfaceBindPalette   *property);
extern int Gx3201_Vpu_SetLayerViewport      ( GxVpuProperty_LayerViewport        *property);
extern int Gx3201_Vpu_SetLayerMainSurface   ( GxVpuProperty_LayerMainSurface     *property);
extern int Gx3201_Vpu_SetLayerEnable        ( GxVpuProperty_LayerEnable          *property);
extern int Gx3201_Vpu_SetLayerAntiFlicker   ( GxVpuProperty_LayerAntiFlicker     *property);
extern int Gx3201_Vpu_SetLayerOnTop         ( GxVpuProperty_LayerOnTop           *property);
extern int Gx3201_Vpu_SetLayerVideoMode     ( GxVpuProperty_LayerVideoMode       *property);
extern int Gx3201_Vpu_SetDestroySurface     ( GxVpuProperty_DestroySurface       *property);
extern int Gx3201_Vpu_SetPalette            ( GxVpuProperty_Palette              *property);
extern int Gx3201_Vpu_SetAlpha              ( GxVpuProperty_Alpha                *property);
extern int Gx3201_Vpu_SetColorFormat        ( GxVpuProperty_ColorFormat          *property);
extern int Gx3201_Vpu_SetColorKey           ( GxVpuProperty_ColorKey             *property);
extern int Gx3201_Vpu_SetBackColor          ( GxVpuProperty_BackColor            *property);
extern int Gx3201_Vpu_SetPoint              ( GxVpuProperty_Point                *property);
extern int Gx3201_Vpu_SetDrawLine           ( GxVpuProperty_DrawLine             *property);
extern int Gx3201_Vpu_SetConvertColor       ( GxVpuProperty_ConvertColor         *property);
extern int Gx3201_Vpu_SetVirtualResolution  ( GxVpuProperty_Resolution           *property);
extern int Gx3201_Vpu_SetVbiEnable          ( GxVpuProperty_VbiEnable            *property);
extern int Gx3201_Vpu_SetAspectRatio        ( GxVpuProperty_AspectRatio          *property);
extern int Gx3201_Vpu_SetTvScreen           ( GxVpuProperty_TvScreen             *property);
extern int Gx3201_Vpu_SetLayerEnablePatch   ( GxVpuProperty_LayerEnable          *property);
extern int Gx3201_Vpu_GetEntries            ( GxVpuProperty_GetEntries           *property);
extern int Gx3201_Vpu_RPalette              ( GxVpuProperty_RWPalette            *property);
extern int Gx3201_Vpu_GetCreatePalette      ( GxVpuProperty_CreatePalette        *property);
extern int Gx3201_Vpu_GetLayerViewport      ( GxVpuProperty_LayerViewport        *property);
extern int Gx3201_Vpu_GetLayerClipport      ( GxVpuProperty_LayerClipport        *property);
extern int Gx3201_Vpu_GetLayerMainSurface   ( GxVpuProperty_LayerMainSurface     *property);
extern int Gx3201_Vpu_GetLayerEnable        ( GxVpuProperty_LayerEnable          *property);
extern int Gx3201_Vpu_GetLayerAntiFlicker   ( GxVpuProperty_LayerAntiFlicker     *property);
extern int Gx3201_Vpu_GetLayerOnTop         ( GxVpuProperty_LayerOnTop           *property);
extern int Gx3201_Vpu_GetLayerVideoMode     ( GxVpuProperty_LayerVideoMode       *property);
extern int Gx3201_Vpu_GetLayerCapture       ( GxVpuProperty_LayerCapture         *property);
extern int Gx3201_Vpu_GetCreateSurface      ( GxVpuProperty_CreateSurface        *property);
extern int Gx3201_Vpu_GetReadSurface        ( GxVpuProperty_ReadSurface          *property);
extern int Gx3201_Vpu_GetPalette            ( GxVpuProperty_Palette              *property);
extern int Gx3201_Vpu_GetAlpha              ( GxVpuProperty_Alpha                *property);
extern int Gx3201_Vpu_GetColorFormat        ( GxVpuProperty_ColorFormat          *property);
extern int Gx3201_Vpu_GetColorKey           ( GxVpuProperty_ColorKey             *property);
extern int Gx3201_Vpu_GetBackColor          ( GxVpuProperty_BackColor            *property);
extern int Gx3201_Vpu_GetPoint              ( GxVpuProperty_Point                *property);
extern int Gx3201_Vpu_GetConvertColor       ( GxVpuProperty_ConvertColor         *property);
extern int Gx3201_Vpu_GetVirtualResolution  ( GxVpuProperty_Resolution           *property);
extern int Gx3201_Vpu_GetVbiEnable          ( GxVpuProperty_VbiEnable            *property);
extern int Gx3201_Vpu_GetVbiCreateBuffer    ( GxVpuProperty_VbiCreateBuffer      *property);
extern int Gx3201_Vpu_GetVbiReadAddress     ( GxVpuProperty_VbiReadAddress       *property);
extern int Gx3201_Vpu_GetFrameBuffer		( GxVpuProperty_GetFrameBuffer		 *property);
extern int Gx3201_Vpu_SetLayerClipport      ( GxVpuProperty_LayerClipport	 	 *property);

extern int GX3201_Vpu_Ga_Interrupt          (int irq);
extern int Gx3201_Vpu_SetBlit               ( GxVpuProperty_Blit         *property);
extern int Gx3201_Vpu_SetDfbBlit			( GxVpuProperty_DfbBlit *property);
extern int Gx3201_Vpu_SetFillRect           ( GxVpuProperty_FillRect     *property);
extern int Gx3201_Vpu_SetFillPolygon        ( GxVpuProperty_FillPolygon  *property);
extern int Gx3201_Vpu_SetBeginUpdate        ( GxVpuProperty_BeginUpdate  *property);
extern int Gx3201_Vpu_SetEndUpdate          ( GxVpuProperty_EndUpdate    *property);
extern int Gx3201_Vpu_ZoomSurface           ( GxVpuProperty_ZoomSurface  *property);
extern int Gx3201_Vpu_Complet				( GxVpuProperty_Complet		 *property);
extern int Gx3201_Vpu_TurnSurface           ( GxVpuProperty_TurnSurface  *property);

extern int gx3201_vpp_get_actual_viewrect(GxAvRect *rect);
extern int gx3201_vpp_get_actual_cliprect(GxAvRect *rect);
extern int gx3201_vpp_set_stream_ratio(unsigned int ratio);
extern int gx3201_vpp_set_framesize(unsigned int width, unsigned int height);
extern int gx3201_vpp_set_playmode(GxVpuLayerPlayMode mode);
extern int gx3201_vpu_set_video_dac(int enable);
extern int gx3201_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect);
#endif

