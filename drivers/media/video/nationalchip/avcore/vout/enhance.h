#ifndef __ENHANCE_H__
#define __ENHANCE_H__

typedef enum {
	HD_ENHANCE,
	SD_ENHANCE,
}EnhanceID;

int enhance_init(void);

int enhance_set_brightness(EnhanceID id, unsigned brightness);

int enhance_set_saturation(EnhanceID id, unsigned saturation);

int enhance_set_contrast(EnhanceID id, unsigned contrast);

int enhance_set_sharpness(EnhanceID id, unsigned sharpness);

int enhance_set_hue(EnhanceID id, unsigned hue);

#endif

