#ifndef __GX_VPU_HAL_H__
#define __GX_VPU_HAL_H__

#include "avcore.h"
#include "gxav_vpu_propertytypes.h"
#include "gxav_vout_propertytypes.h"

#define VPU_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

#define VPU_LAYER_ENABLE_ACTIVE (1<<0)
#define VPU_OSD_ENABLE_REQUIRE  (1<<1)
#define VPU_SPP_ENABLE_REQUIRE  (1<<2)

typedef struct gx_vpu_layer     GxVpuLayer;
typedef struct gx_vpu_surface   GxVpuSurface;


typedef struct gx_vpu_surface {
	unsigned int    width;
	unsigned int    height;

	void            *buffer;
	int             prealloced;

	volatile int    dirty;
	GxAlpha         alpha;
	GxColor         bg_color;
	GxColor         color_key;
	int             color_key_en;
	GxColorFormat   color_format;
	GxSurfaceMode   surface_mode;
	GxPalette       *palette;

	GxVpuLayer  *layer;
	struct gx_vpu_surface   *next;
} VpuSurface;

typedef struct  gx_vpu_layer_ops {
	int (*set_view_port)(GxAvRect* clip_rect,GxAvRect* view_rect);
	int (*set_pan_display)(void *buffer);
	int (*set_main_surface)(GxVpuSurface* surface);
	int (*set_main_surface_multregion)(void);
	int (*set_enable)(int enable);
	int (*set_anti_flicker)(int enable);
	int (*set_on_top)(int enable);
	int (*set_video_mode)(GxLayerVideoMode mode);
	int (*set_palette)(GxPalette* palette);
	int (*set_alpha)(GxAlpha alpha);
	int (*set_color_format)(GxColorFormat format);
	int (*set_color_key_mode)(GxColorkeyMode mode, unsigned char ext_alpha);
	int (*set_color_key)(GxColor* color,int enable);
	int (*set_bg_color)(GxColor color);

	int (*add_region)   (GxVpuSurface *surface, unsigned pos_x, unsigned pos_y);
	int (*remove_region)(GxVpuSurface *surface);
	int (*update_region)(GxVpuSurface *surface, unsigned pos_x, unsigned pos_y);
}GxVpuLayerOps;

struct gx_vpu_layer{
	GxLayerID           id;
	GxAvRect            view_rect;
	GxAvRect            clip_rect;
	GxAvRect            clip_full_rect;

	int                 on_top;
	int                 enable;
	int                 enable_require;
	int                 anti_flicker_en;
	int                 auto_zoom;
	volatile int        flip_require;

	GxLayerVideoMode    video_mode;
	GxVpuSurface        *surface;
	GxVpuLayerOps       *ops;
	GxVpSpec            spec;
	GxTvScreen          screen;
	GxColor             bg_color;
	void                *priv;
};

typedef enum play_mode {
	PLAY_MODE_FIELD            = (0<<1),
	PLAY_MODE_FRAME            = (1<<1),
	PLAY_MODE_Y_FRAME_UV_FIELD = (1<<1)|(1<<2),
	PLAY_MODE_FRAME_REPEAT     = (1<<3),//only for 3113c
} PlayMode;

typedef PlayMode GxVpuLayerPlayMode;

typedef struct GxMasteringDisplayMetadata {
	/**
	* CIE 1931 xy chromaticity coords of color primaries (r, g, b order).
	*/
	unsigned display_primaries[3][2];

	/**
	* CIE 1931 xy chromaticity coords of white point.
	*/
	unsigned white_point[2];

	/**
	* Min luminance of mastering display (cd/m^2).
	*/
	unsigned min_luminance;

	/**
	* Max luminance of mastering display (cd/m^2).
	*/
	unsigned max_luminance;

	/**
	* Flag indicating whether the display primaries (and white point) are set.
	*/
	int has_primaries;

	/**
	* Flag indicating whether the luminance (min_ and max_) have been set.
	*/
	int has_luminance;
} GxMasteringDisplayMetadata;

typedef struct GxContentLightMetadata {
	/**
	* Max content light level (cd/m^2).
	*/
	unsigned MaxCLL;

	/**
	* Max average light level per frame (cd/m^2).
	*/
	unsigned MaxFALL;
} GxContentLightMetadata;

/**
* Chromaticity coordinates of the source primaries.
* These values match the ones defined by ISO/IEC 23001-8_2013 § 7.1.
430   */
enum GxColorPrimaries {
	GXCOL_PRI_RESERVED0    = 0,
	GXCOL_PRI_BT709        = 1,  ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP177 Annex B
	GXCOL_PRI_UNSPECIFIED  = 2,
	GXCOL_PRI_RESERVED     = 3,
	GXCOL_PRI_BT470M       = 4,  ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	GXCOL_PRI_BT470BG      = 5,  ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
	GXCOL_PRI_SMPTE170M    = 6,  ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	GXCOL_PRI_SMPTE240M    = 7,  ///< functionally identical to above
	GXCOL_PRI_FILM         = 8,  ///< colour filters using Illuminant C
	GXCOL_PRI_BT2020       = 9,  ///< ITU-R BT2020
	GXCOL_PRI_SMPTE428     = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
	GXCOL_PRI_SMPTEST428_1 = GXCOL_PRI_SMPTE428,
	GXCOL_PRI_SMPTE431     = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
	GXCOL_PRI_SMPTE432     = 12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
	GXCOL_PRI_JEDEC_P22    = 22, ///< JEDEC P22 phosphors
	GXCOL_PRI_NB                ///< Not part of ABI
};

/**
 * Color Transfer Characteristic.
 * These values match the ones defined by ISO/IEC 23001-8_2013 § 7.2.
 */
enum GxColorTransferCharacteristic {
	GXCOL_TRC_RESERVED0    = 0,
	GXCOL_TRC_BT709        = 1,  ///< also ITU-R BT1361
	GXCOL_TRC_UNSPECIFIED  = 2,
	GXCOL_TRC_RESERVED     = 3,
	GXCOL_TRC_GAMMA22      = 4,  ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
	GXCOL_TRC_GAMMA28      = 5,  ///< also ITU-R BT470BG
	GXCOL_TRC_SMPTE170M    = 6,  ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
	GXCOL_TRC_SMPTE240M    = 7,
	GXCOL_TRC_LINEAR       = 8,  ///< "Linear transfer characteristics"
	GXCOL_TRC_LOG          = 9,  ///< "Logarithmic transfer characteristic (100:1 range)"
	GXCOL_TRC_LOG_SQRT     = 10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
	GXCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
	GXCOL_TRC_BT1361_ECG   = 12, ///< ITU-R BT1361 Extended Colour Gamut
	GXCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
	GXCOL_TRC_BT2020_10    = 14, ///< ITU-R BT2020 for 10-bit system
	GXCOL_TRC_BT2020_12    = 15, ///< ITU-R BT2020 for 12-bit system
	GXCOL_TRC_SMPTE2084    = 16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
	GXCOL_TRC_SMPTEST2084  = GXCOL_TRC_SMPTE2084,
	GXCOL_TRC_SMPTE428     = 17, ///< SMPTE ST 428-1
	GXCOL_TRC_SMPTEST428_1 = GXCOL_TRC_SMPTE428,
	GXCOL_TRC_ARIB_STD_B67 = 18, ///< ARIB STD-B67, known as "Hybrid log-gamma"
	GXCOL_TRC_NB                 ///< Not part of ABI
};

struct frame {
	unsigned w, h;
	unsigned baseline;
	unsigned baseheight;
	unsigned bpp;
	unsigned stream_id;
	unsigned disp_num;
	int topfirst;
	int is_image;
	int is_freeze;
	PlayMode playmode;

	struct field {
		unsigned addry;
		unsigned addrcbcr;
		unsigned addry_2bit;
		unsigned addrcbcr_2bit;
	} top, bot;

	int colorimetry;
	int transfer_characteristic;
	GxMasteringDisplayMetadata mdmd;
	GxContentLightMetadata     clmd;
};

typedef struct disp_ctrl {
#define MAX_DISPBUF_NUM (8)
	int buf_to_wr;
	struct disp_unit {
		volatile unsigned int word0;
		volatile unsigned int word1;
		volatile unsigned int word2;
		volatile unsigned int word3;
		volatile unsigned int word4;
		volatile unsigned int word5;
		volatile unsigned int word6;
		volatile unsigned int word7;
		volatile unsigned int word8;
		volatile unsigned int word9;
		volatile unsigned int word10;
		volatile unsigned int word11;
	} *unit[MAX_DISPBUF_NUM];
	struct frame last_frame;
	int    zoom_require;
	GxAvRect clip_rect;
	GxAvRect view_rect;
} DispControler;

#define IS_NULL(pointer)                       \
	(pointer == NULL)

#define IS_INVAILD_LAYER(layer)                \
	(((layer) < GX_LAYER_OSD) || ((layer) > GX_LAYER_BKG))

#define IS_MAIN_SURFACE(sur)                   \
	((sur)->layer != NULL && (sur)->layer->surface == sur)

#define IS_VIDEO_SURFACE(sur)                  \
	((sur)->surface_mode == GX_SURFACE_MODE_VIDEO)

#define IS_IMAGE_SURFACE(sur)                  \
	((sur)->surface_mode == GX_SURFACE_MODE_IMAGE)

#define IS_INVAILD_IMAGE_COLOR(color)          \
	(color == GX_COLOR_FMT_YCBCR420)

#define IS_INVAILD_GA_COLOR(color)             \
	(((color) == GX_COLOR_FMT_RGB888) || ((color) == GX_COLOR_FMT_BGR888))

#define IS_INVAILD_POINT(point,xres,yres)      \
	(((point)->x >= xres) || ((point)->y >= yres))

#define IS_INVAILD_RECT(rect,xres,yres)        \
	(((rect)->x + (rect)->width  > xres)   \
	 ||((rect)->y + (rect)->height > yres) \
	 ||((rect)->x   >= xres)               \
	 ||((rect)->y   >= yres)               \
	 ||((rect)->width   == 0)              \
	 ||((rect)->height  == 0)              \
	 ||((rect)->width   > xres)            \
	 ||((rect)->height   > yres))

#define IS_REFERENCE_COLOR(color)              \
	(((color) >= GX_COLOR_FMT_CLUT1) && ((color) <= GX_COLOR_FMT_CLUT8))

#define IS_INVAILD_COLOR(color)                \
	(((color) < GX_COLOR_FMT_CLUT1) || ((color) > GX_COLOR_FMT_YCBCR444_UV))

#define IS_INVAILD_ZOOM_COLOR(color)    \
	(    (color != GX_COLOR_FMT_RGB565)\
		 &&    (color != GX_COLOR_FMT_RGBA4444)\
		 &&    (color != GX_COLOR_FMT_RGBA5551)\
		 &&    (color != GX_COLOR_FMT_RGBA8888)\
		 &&    (color != GX_COLOR_FMT_ARGB8888)\
		 &&    (color != GX_COLOR_FMT_ARGB4444)\
		 &&    (color != GX_COLOR_FMT_ABGR4444)\
		 &&    (color != GX_COLOR_FMT_ARGB1555)\
		 &&    (color != GX_COLOR_FMT_ABGR1555)\
		 &&    (color != GX_COLOR_FMT_A8)\
		 &&    (color != GX_COLOR_FMT_Y8)\
		 &&    (color != GX_COLOR_FMT_YCBCRA6442))

#define IS_INVAILD_TURN_COLOR(color) \
	(    (color != GX_COLOR_FMT_RGB565)\
		 &&    (color != GX_COLOR_FMT_RGBA4444)\
		 &&    (color != GX_COLOR_FMT_RGBA5551)\
		 &&    (color != GX_COLOR_FMT_RGBA8888)\
		 &&    (color != GX_COLOR_FMT_ARGB8888)\
		 &&    (color != GX_COLOR_FMT_ARGB4444)\
		 &&    (color != GX_COLOR_FMT_ABGR4444)\
		 &&    (color != GX_COLOR_FMT_ARGB1555)\
		 &&    (color != GX_COLOR_FMT_ABGR1555)\
		 &&    (color != GX_COLOR_FMT_A8)\
		 &&    (color != GX_COLOR_FMT_Y8)\
		 &&    (color != GX_COLOR_FMT_CLUT8)\
		 &&    (color != GX_COLOR_FMT_YCBCRA6442))


typedef enum {
	VPU_DAC_0   = 0x1<<0,
	VPU_DAC_1   = 0x1<<1,
	VPU_DAC_2   = 0x1<<2,
	VPU_DAC_3   = 0x1<<3,
	VPU_DAC_ALL = 0xF,
}GxVpuDacID;

typedef enum {
	VPU_DAC_SRC_VPU  = 0x1<<0,
	VPU_DAC_SRC_SVPU = 0x1<<1,
}GxVpuDacSrc;

typedef struct {
	unsigned int buf_addrs[4];
}SvpuSurfaceInfo;

extern int gxav_videoout_get_dve(int id);

extern int gxav_vpu_init(struct gxav_module_ops* ops);
extern int gxav_vpu_cleanup(void);
extern int gxav_vpu_open(void);
extern int gxav_vpu_close(void);
extern int gxav_vpu_reset(void);

extern int gxav_vpu_WPalette              ( GxVpuProperty_RWPalette            *property);
extern int gxav_vpu_DestroyPalette        ( GxVpuProperty_DestroyPalette       *property);
extern int gxav_vpu_SurfaceBindPalette    ( GxVpuProperty_SurfaceBindPalette   *property);
extern int gxav_vpu_SetLayerViewport      ( GxVpuProperty_LayerViewport        *property);
extern int gxav_vpu_SetLayerMainSurface   ( GxVpuProperty_LayerMainSurface     *property);
extern int gxav_vpu_UnsetLayerMainSurface ( GxVpuProperty_LayerMainSurface     *property);
extern int gxav_vpu_SetLayerEnable        ( GxVpuProperty_LayerEnable          *property);
extern int gxav_vpu_SetLayerAntiFlicker   ( GxVpuProperty_LayerAntiFlicker     *property);
extern int gxav_vpu_SetLayerOnTop         ( GxVpuProperty_LayerOnTop           *property);
extern int gxav_vpu_SetLayerVideoMode     ( GxVpuProperty_LayerVideoMode       *property);
extern int gxav_vpu_SetLayerMixConfig     ( GxVpuProperty_LayerMixConfig       *property);
extern int gxav_vpu_SetDestroySurface     ( GxVpuProperty_DestroySurface       *property);
extern int gxav_vpu_ModifySurface         ( GxVpuProperty_ModifySurface        *property);
extern int gxav_vpu_SetPalette            ( GxVpuProperty_Palette              *property);
extern int gxav_vpu_SetAlpha              ( GxVpuProperty_Alpha                *property);
extern int gxav_vpu_SetColorFormat        ( GxVpuProperty_ColorFormat          *property);
extern int gxav_vpu_SetColorKey           ( GxVpuProperty_ColorKey             *property);
extern int gxav_vpu_SetBackColor          ( GxVpuProperty_BackColor            *property);
extern int gxav_vpu_SetPoint              ( GxVpuProperty_Point                *property);
extern int gxav_vpu_SetDrawLine           ( GxVpuProperty_DrawLine             *property);
extern int gxav_vpu_SetConvertColor       ( GxVpuProperty_ConvertColor         *property);
extern int gxav_vpu_SetVirtualResolution  ( GxVpuProperty_Resolution           *property);
extern int gxav_vpu_SetVbiEnable          ( GxVpuProperty_VbiEnable            *property);
extern int gxav_vpu_SetAspectRatio        ( GxVpuProperty_AspectRatio          *property);
extern int gxav_vpu_SetTvScreen           ( GxVpuProperty_TvScreen             *property);
extern int gxav_vpu_SetLayerEnablePatch   ( GxVpuProperty_LayerEnable          *property);
extern int gxav_vpu_GetEntries            ( GxVpuProperty_GetEntries           *property);
extern int gxav_vpu_RPalette              ( GxVpuProperty_RWPalette            *property);
extern int gxav_vpu_GetCreatePalette      ( GxVpuProperty_CreatePalette        *property);
extern int gxav_vpu_GetLayerViewport      ( GxVpuProperty_LayerViewport        *property);
extern int gxav_vpu_GetLayerClipport      ( GxVpuProperty_LayerClipport        *property);
extern int gxav_vpu_GetLayerMainSurface   ( GxVpuProperty_LayerMainSurface     *property);
extern int gxav_vpu_GetLayerEnable        ( GxVpuProperty_LayerEnable          *property);
extern int gxav_vpu_GetLayerAntiFlicker   ( GxVpuProperty_LayerAntiFlicker     *property);
extern int gxav_vpu_GetLayerOnTop         ( GxVpuProperty_LayerOnTop           *property);
extern int gxav_vpu_GetLayerVideoMode     ( GxVpuProperty_LayerVideoMode       *property);
extern int gxav_vpu_GetLayerCapture       ( GxVpuProperty_LayerCapture         *property);
extern int gxav_vpu_GetCreateSurface      ( GxVpuProperty_CreateSurface        *property);
extern int gxav_vpu_GetReadSurface        ( GxVpuProperty_ReadSurface          *property);
extern int gxav_vpu_GetPalette            ( GxVpuProperty_Palette              *property);
extern int gxav_vpu_GetAlpha              ( GxVpuProperty_Alpha                *property);
extern int gxav_vpu_GetColorFormat        ( GxVpuProperty_ColorFormat          *property);
extern int gxav_vpu_GetColorKey           ( GxVpuProperty_ColorKey             *property);
extern int gxav_vpu_GetBackColor          ( GxVpuProperty_BackColor            *property);
extern int gxav_vpu_GetPoint              ( GxVpuProperty_Point                *property);
extern int gxav_vpu_GetConvertColor       ( GxVpuProperty_ConvertColor         *property);
extern int gxav_vpu_GetVirtualResolution  ( GxVpuProperty_Resolution           *property);
extern int gxav_vpu_GetVbiEnable          ( GxVpuProperty_VbiEnable            *property);
extern int gxav_vpu_GetVbiCreateBuffer    ( GxVpuProperty_VbiCreateBuffer      *property);
extern int gxav_vpu_GetVbiReadAddress     ( GxVpuProperty_VbiReadAddress       *property);
extern int gxav_vpu_SetLayerClipport      ( GxVpuProperty_LayerClipport        *property);

extern int gxav_vpu_SetBlit               ( GxVpuProperty_Blit                 *property);
extern int gxav_vpu_SetDfbBlit            ( GxVpuProperty_DfbBlit              *property);
extern int gxav_vpu_SetBatchDfbBlit       ( GxVpuProperty_BatchDfbBlit         *property);
extern int gxav_vpu_SetFillRect           ( GxVpuProperty_FillRect             *property);
extern int gxav_vpu_SetFillPolygon        ( GxVpuProperty_FillPolygon          *property);
extern int gxav_vpu_SetGAMode             ( GxVpuProperty_SetGAMode            *property);
extern int gxav_vpu_SetWaitUpdate         ( GxVpuProperty_WaitUpdate           *property);
extern int gxav_vpu_SetBeginUpdate        ( GxVpuProperty_BeginUpdate          *property);
extern int gxav_vpu_SetEndUpdate          ( GxVpuProperty_EndUpdate            *property);
extern int gxav_vpu_ZoomSurface           ( GxVpuProperty_ZoomSurface          *property);
extern int gxav_vpu_Complet               ( GxVpuProperty_Complet              *property);
extern int gxav_vpu_TurnSurface           ( GxVpuProperty_TurnSurface          *property);
extern int gxav_vpu_Ga_Interrupt          ( int irq );

extern int gxav_vpu_RegistSurface(GxVpuProperty_RegistSurface *property);
extern int gxav_vpu_UnregistSurface(GxVpuProperty_UnregistSurface *property);
extern int gxav_vpu_GetIdleSurface(GxVpuProperty_GetIdleSurface *property);
extern int gxav_vpu_FlipSurface(GxVpuProperty_FlipSurface *property);
extern int gxav_vpu_PanDisplay(GxLayerID layer, void *buffer);
extern int gxav_vpu_VpuIsrCallback(void *arg);

extern int gxav_vpu_AddRegion(GxVpuProperty_AddRegion *property);
extern int gxav_vpu_RemoveRegion(GxVpuProperty_RemoveRegion *property);
extern int gxav_vpu_UpdateRegion(GxVpuProperty_UpdateRegion *property);

extern int gxav_vpu_GetOsdOverlay(GxVpuProperty_OsdOverlay *property);
extern int gxav_vpu_SetOsdOverlay(GxVpuProperty_OsdOverlay *property);

extern int gxav_vpu_SetGAMMA(GxVpuProperty_GAMMAConfig *property);
extern int gxav_vpu_GetGAMMA(GxVpuProperty_GAMMAConfig *property);
extern int gxav_vpu_SetGreenBlue(GxVpuProperty_GreenBlueConfig *property);
extern int gxav_vpu_GetGreenBlue(GxVpuProperty_GreenBlueConfig *property);
extern int gxav_vpu_SetEnhance(GxVpuProperty_EnhanceConfig *property);
extern int gxav_vpu_GetEnhance(GxVpuProperty_EnhanceConfig *property);
extern int gxav_vpu_SetMixFilter(GxVpuProperty_MixFilter *property);
extern int gxav_vpu_GetMixFilter(GxVpuProperty_MixFilter *property);

extern int gxav_vpu_SetSVPUConfig(GxVpuProperty_SVPUConfig *property);

extern int gxav_vpu_Sync(GxVpuProperty_Sync *property);
extern int gxav_vpu_SetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property);
extern int gxav_vpu_GetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property);

extern int gxav_vpu_SetGAMMACorrect(GxVpuProperty_GAMMACorrect *property);

extern int gxav_vpu_SetVoutIsready        ( int ready );
extern int gxav_vpu_DACEnable             ( GxVpuDacID id, int enable );
extern int gxav_vpu_DACSource             ( GxVpuDacID id, GxVpuDacSrc source );
extern int gxav_vpu_DACPower              ( GxVpuDacID id);

extern int gxav_vpu_SetMixerBackcolor(GxVpuProperty_MixerBackcolor *property);
extern int gxav_vpu_SetMixerLayers(GxVpuProperty_MixerLayers *property);

extern int gxav_vpu_vpp_get_base_line(void);
extern int gxav_vpu_vpp_set_base_line(unsigned int value);
extern int gxav_vpu_disp_get_buff_id(void);
extern int gxav_vpu_disp_set_rst(void);
extern int gxav_vpu_disp_clr_rst(void);
extern int gxav_vpu_disp_get_view_active_cnt(void);
extern int gxav_vpu_disp_clr_field_error_int(void);
extern int gxav_vpu_disp_clr_field_start_int(void);
extern int gxav_vpu_disp_field_error_int_en(void);
extern int gxav_vpu_disp_field_start_int_en(void);
extern int gxav_vpu_disp_field_error_int_dis(void);
extern int gxav_vpu_disp_field_start_int_dis(void);
extern int gxav_vpu_disp_get_buff_cnt(void);
extern int gxav_vpu_disp_get_view_field(void);
extern int gxav_vpu_disp_get_display_buf(int i);
extern int gxav_vpu_disp_layer_enable(GxLayerID layer, int enable);

extern int gxav_vpu_vpp_set_stream_ratio(unsigned int ratio);
extern int gxav_vpu_vpp_get_actual_viewrect(GxAvRect *view_rect);
extern int gxav_vpu_vpp_get_actual_cliprect(GxAvRect *clip_rect);
extern int gxav_vpu_vpp_set_actual_cliprect(GxAvRect *clip_rect);
extern int gxav_vpu_vpp_play_frame(struct frame *frame);

extern unsigned int gxav_vpu_get_scan_line(void);
extern int gxav_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info);
extern int gxav_svpu_config_buf(SvpuSurfaceInfo *buf_info);
extern int gxav_svpu_get_buf(SvpuSurfaceInfo *buf_info);
extern int gxav_svpu_run(void);
extern int gxav_svpu_stop(void);

extern void gxav_vpu_set_triming_offset(GxVpuDacID dac_id, char offset);

struct vpu_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(void);
	int (*close)(void);
	int (*reset)(void);

	int (*WPalette              )( GxVpuProperty_RWPalette            *property);
	int (*DestroyPalette        )( GxVpuProperty_DestroyPalette       *property);
	int (*SurfaceBindPalette    )( GxVpuProperty_SurfaceBindPalette   *property);
	int (*SetLayerViewport      )( GxVpuProperty_LayerViewport        *property);
	int (*SetLayerMainSurface   )( GxVpuProperty_LayerMainSurface     *property);
	int (*UnsetLayerMainSurface )( GxVpuProperty_LayerMainSurface     *property);
	int (*SetLayerEnable        )( GxVpuProperty_LayerEnable          *property);
	int (*SetLayerAntiFlicker   )( GxVpuProperty_LayerAntiFlicker     *property);
	int (*SetLayerOnTop         )( GxVpuProperty_LayerOnTop           *property);
	int (*SetLayerVideoMode     )( GxVpuProperty_LayerVideoMode       *property);
	int (*SetLayerMixConfig     )( GxVpuProperty_LayerMixConfig       *property);
	int (*SetDestroySurface     )( GxVpuProperty_DestroySurface       *property);
	int (*ModifySurface         )( GxVpuProperty_ModifySurface        *property);
	int (*SetPalette            )( GxVpuProperty_Palette              *property);
	int (*SetAlpha              )( GxVpuProperty_Alpha                *property);
	int (*SetColorFormat        )( GxVpuProperty_ColorFormat          *property);
	int (*SetColorKey           )( GxVpuProperty_ColorKey             *property);
	int (*SetBackColor          )( GxVpuProperty_BackColor            *property);
	int (*SetPoint              )( GxVpuProperty_Point                *property);
	int (*SetDrawLine           )( GxVpuProperty_DrawLine             *property);
	int (*SetConvertColor       )( GxVpuProperty_ConvertColor         *property);
	int (*SetVirtualResolution  )( GxVpuProperty_Resolution           *property);
	int (*SetVbiEnable          )( GxVpuProperty_VbiEnable            *property);
	int (*SetAspectRatio        )( GxVpuProperty_AspectRatio          *property);
	int (*SetTvScreen           )( GxVpuProperty_TvScreen             *property);
	int (*SetLayerEnablePatch   )( GxVpuProperty_LayerEnable          *property);
	int (*GetEntries            )( GxVpuProperty_GetEntries           *property);
	int (*RPalette              )( GxVpuProperty_RWPalette            *property);
	int (*GetCreatePalette      )( GxVpuProperty_CreatePalette        *property);
	int (*GetLayerViewport      )( GxVpuProperty_LayerViewport        *property);
	int (*GetLayerClipport      )( GxVpuProperty_LayerClipport        *property);
	int (*GetLayerMainSurface   )( GxVpuProperty_LayerMainSurface     *property);
	int (*GetLayerEnable        )( GxVpuProperty_LayerEnable          *property);
	int (*GetLayerAntiFlicker   )( GxVpuProperty_LayerAntiFlicker     *property);
	int (*GetLayerOnTop         )( GxVpuProperty_LayerOnTop           *property);
	int (*GetLayerVideoMode     )( GxVpuProperty_LayerVideoMode       *property);
	int (*GetLayerCapture       )( GxVpuProperty_LayerCapture         *property);
	int (*GetCreateSurface      )( GxVpuProperty_CreateSurface        *property);
	int (*GetReadSurface        )( GxVpuProperty_ReadSurface          *property);
	int (*GetPalette            )( GxVpuProperty_Palette              *property);
	int (*GetAlpha              )( GxVpuProperty_Alpha                *property);
	int (*GetColorFormat        )( GxVpuProperty_ColorFormat          *property);
	int (*GetColorKey           )( GxVpuProperty_ColorKey             *property);
	int (*GetBackColor          )( GxVpuProperty_BackColor            *property);
	int (*GetPoint              )( GxVpuProperty_Point                *property);
	int (*GetConvertColor       )( GxVpuProperty_ConvertColor         *property);
	int (*GetVirtualResolution  )( GxVpuProperty_Resolution           *property);
	int (*GetVbiEnable          )( GxVpuProperty_VbiEnable            *property);
	int (*GetVbiCreateBuffer    )( GxVpuProperty_VbiCreateBuffer      *property);
	int (*GetVbiReadAddress     )( GxVpuProperty_VbiReadAddress       *property);
	int (*SetLayerClipport      )( GxVpuProperty_LayerClipport        *property);
	int (*SetBlit               )( GxVpuProperty_Blit                 *property);
	int (*SetDfbBlit            )( GxVpuProperty_DfbBlit              *property);
	int (*SetBatchDfbBlit       )( GxVpuProperty_BatchDfbBlit         *property);
	int (*SetFillRect           )( GxVpuProperty_FillRect             *property);
	int (*SetFillPolygon        )( GxVpuProperty_FillPolygon          *property);
	int (*SetGAMode             )( GxVpuProperty_SetGAMode            *property);
	int (*SetWaitUpdate         )( GxVpuProperty_WaitUpdate           *property);
	int (*SetBeginUpdate        )( GxVpuProperty_BeginUpdate          *property);
	int (*SetEndUpdate          )( GxVpuProperty_EndUpdate            *property);
	int (*ZoomSurface           )( GxVpuProperty_ZoomSurface          *property);
	int (*Complet               )( GxVpuProperty_Complet              *property);
	int (*TurnSurface           )( GxVpuProperty_TurnSurface          *property);

	int (*RegistSurface  )(GxVpuProperty_RegistSurface   *property);
	int (*UnregistSurface)(GxVpuProperty_UnregistSurface *property);
	int (*GetIdleSurface )(GxVpuProperty_GetIdleSurface  *property);
	int (*FlipSurface    )(GxVpuProperty_FlipSurface     *property);
	int (*PanDisplay     )(GxLayerID layer, void *buffer);

	int (*AddRegion   )(GxVpuProperty_AddRegion *property);
	int (*RemoveRegion)(GxVpuProperty_RemoveRegion *property);
	int (*UpdateRegion)(GxVpuProperty_UpdateRegion *property);

	int (*GetOsdOverlay)(GxVpuProperty_OsdOverlay *property);
	int (*SetOsdOverlay)(GxVpuProperty_OsdOverlay *property);

	int (*SetLayerAutoZoom)(GxVpuProperty_LayerAutoZoom *property);
	int (*GetLayerAutoZoom)(GxVpuProperty_LayerAutoZoom *property);

	int (*SetGAMMAConfig)(GxVpuProperty_GAMMAConfig *property);
	int (*GetGAMMAConfig)(GxVpuProperty_GAMMAConfig *property);
	int (*SetGreenBlueConfig)(GxVpuProperty_GreenBlueConfig *property);
	int (*GetGreenBlueConfig)(GxVpuProperty_GreenBlueConfig *property);
	int (*SetEnhanceConfig)(GxVpuProperty_EnhanceConfig *property);
	int (*GetEnhanceConfig)(GxVpuProperty_EnhanceConfig *property);
	int (*SetMixFilter)(GxVpuProperty_MixFilter *property);
	int (*GetMixFilter)(GxVpuProperty_MixFilter *property);

	int (*SetSVPUConfig)(GxVpuProperty_SVPUConfig *property);

	int (*SetGAMMACorrect)(GxVpuProperty_GAMMACorrect *property);

	int (*VpuIsrCallback )(void *arg);

	int (*Ga_Interrupt)( int irq );
	int (*SetVoutIsready)( int ready );
	int (*DACEnable)( GxVpuDacID id, int enable );
	int (*DACPower)( GxVpuDacID id );
	int (*DACSource)( GxVpuDacID id, GxVpuDacSrc source );

	int (*vpp_get_base_line)(void);
	int (*vpp_set_base_line)(unsigned int value);
	int (*disp_get_buff_id)(void);
	int (*disp_set_rst)(void);
	int (*disp_clr_rst)(void);
	int (*disp_get_view_active_cnt)(void);
	int (*disp_clr_field_error_int)(void);
	int (*disp_clr_field_start_int)(void);
	int (*disp_field_error_int_en)(void);
	int (*disp_field_start_int_en)(void);
	int (*disp_field_error_int_dis)(void);
	int (*disp_field_start_int_dis)(void);
	int (*disp_get_buff_cnt)(void);
	int (*disp_get_view_field)(void);
	int (*disp_get_display_buf)(int i);
	int (*disp_layer_enable)(GxLayerID layer, int enable);

	int (*vpp_set_stream_ratio)(unsigned int ratio);
	int (*vpp_set_viewrect)(GxAvRect *clip_rect, GxAvRect *view_rect);
	int (*vpp_get_actual_viewrect)(GxAvRect *view_rect);
	int (*vpp_get_actual_cliprect)(GxAvRect *clip_rect);
	int (*vpp_set_actual_cliprect)(GxAvRect *clip_rect);
	int (*vpp_play_frame)(struct frame *frame);
	unsigned int (*vpu_get_scan_line)(void);
	int (*svpu_config)(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info);
	int (*svpu_config_buf)(SvpuSurfaceInfo *buf_info);
	int (*svpu_get_buf)(SvpuSurfaceInfo *buf_info);
	int (*svpu_run)(void);
	int (*svpu_stop)(void);
	int (*SetMixerBackcolor)(GxVpuProperty_MixerBackcolor *property);
	int (*SetMixerLayers)(GxVpuProperty_MixerLayers *property);
	int (*GetMixerLayers)(GxVpuProperty_MixerLayers *property);
	void (*set_triming_offset)(GxVpuDacID dac_id, char offset);
};

#endif
