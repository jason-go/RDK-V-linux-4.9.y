#ifndef __csc_h__
#define __csc_h__

typedef enum{
	SDTV_525 = 0,
	SDTV_625, 
	HDTV,
} HdmiFormat_t;

int* get_csc_params(int colorprimaries, HdmiFormat_t outFormat, int contrast, int brightness, int saturation, int rgb_flag);
#endif
