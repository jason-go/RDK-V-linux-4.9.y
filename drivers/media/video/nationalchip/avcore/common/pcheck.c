
#include "gxav.h"


#define VPU_PARAMETER_CHECK(id,size)                 \
		case  GxVpuPropertyID_##id :                 \
			return ((size == sizeof(GxVpuProperty_##id))?0:-1);

int GxVpu_ParameterCheck(int para, int size) {
	switch(para){
		VPU_PARAMETER_CHECK(LayerViewport     ,size );
		VPU_PARAMETER_CHECK(LayerMixConfig    ,size );
		VPU_PARAMETER_CHECK(LayerMainSurface  ,size );
		VPU_PARAMETER_CHECK(UnsetLayerMainSurface, size );
		VPU_PARAMETER_CHECK(LayerEnable       ,size );
		VPU_PARAMETER_CHECK(LayerAntiFlicker  ,size );
		VPU_PARAMETER_CHECK(LayerOnTop        ,size );
		VPU_PARAMETER_CHECK(LayerVideoMode    ,size );
		VPU_PARAMETER_CHECK(LayerCapture      ,size );
		VPU_PARAMETER_CHECK(CreateSurface     ,size );
		VPU_PARAMETER_CHECK(ReadSurface       ,size );
		VPU_PARAMETER_CHECK(DestroySurface    ,size );
		VPU_PARAMETER_CHECK(Palette           ,size );
		VPU_PARAMETER_CHECK(Alpha             ,size );
		VPU_PARAMETER_CHECK(ColorFormat       ,size );
		VPU_PARAMETER_CHECK(ColorKey          ,size );
		VPU_PARAMETER_CHECK(BackColor         ,size );
		VPU_PARAMETER_CHECK(Point             ,size );
		VPU_PARAMETER_CHECK(DrawLine          ,size );
		VPU_PARAMETER_CHECK(Blit              ,size );
		VPU_PARAMETER_CHECK(FillRect          ,size );
		VPU_PARAMETER_CHECK(FillPolygon       ,size );
		VPU_PARAMETER_CHECK(BeginUpdate       ,size );
		VPU_PARAMETER_CHECK(EndUpdate         ,size );
		VPU_PARAMETER_CHECK(ConvertColor      ,size );
		VPU_PARAMETER_CHECK(VirtualResolution ,size );
		VPU_PARAMETER_CHECK(VbiEnable         ,size );
		VPU_PARAMETER_CHECK(VbiCreateBuffer   ,size );
		VPU_PARAMETER_CHECK(VbiReadAddress    ,size );
		VPU_PARAMETER_CHECK(VbiDestroyBuffer  ,size );
		VPU_PARAMETER_CHECK(CreatePalette     ,size );
		VPU_PARAMETER_CHECK(DestroyPalette    ,size );
		VPU_PARAMETER_CHECK(SurfaceBindPalette,size );
		VPU_PARAMETER_CHECK(RWPalette         ,size );
		VPU_PARAMETER_CHECK(GetEntries        ,size );
		VPU_PARAMETER_CHECK(TurnSurface       ,size );
		VPU_PARAMETER_CHECK(ZoomSurface       ,size );
		VPU_PARAMETER_CHECK(Complet           ,size );
		VPU_PARAMETER_CHECK(LayerClipport     ,size );
		VPU_PARAMETER_CHECK(DfbBlit           ,size );
		VPU_PARAMETER_CHECK(BatchDfbBlit      ,size );
		VPU_PARAMETER_CHECK(SetGAMode         ,size );
		VPU_PARAMETER_CHECK(WaitUpdate        ,size );
		VPU_PARAMETER_CHECK(RegistSurface     ,size );
		VPU_PARAMETER_CHECK(UnregistSurface   ,size );
		VPU_PARAMETER_CHECK(GetIdleSurface    ,size );
		VPU_PARAMETER_CHECK(FlipSurface       ,size );
		VPU_PARAMETER_CHECK(AddRegion         ,size );
		VPU_PARAMETER_CHECK(RemoveRegion      ,size );
		VPU_PARAMETER_CHECK(UpdateRegion      ,size );
		VPU_PARAMETER_CHECK(OsdOverlay        ,size );
		VPU_PARAMETER_CHECK(GAMMAConfig       ,size );
		VPU_PARAMETER_CHECK(GreenBlueConfig   ,size );
		VPU_PARAMETER_CHECK(EnhanceConfig     ,size );
		VPU_PARAMETER_CHECK(MixFilter         ,size );
		VPU_PARAMETER_CHECK(SVPUConfig        ,size );
		VPU_PARAMETER_CHECK(Sync              ,size );
		VPU_PARAMETER_CHECK(ModifySurface     ,size );
		VPU_PARAMETER_CHECK(LayerAutoZoom     ,size );
		VPU_PARAMETER_CHECK(GAMMACorrect      ,size );
		VPU_PARAMETER_CHECK(MixerLayers       ,size );
		VPU_PARAMETER_CHECK(MixerBackcolor    ,size );
		default:
			return -2;
	}

}

