#ifndef __cynus_hdr_h__
#define __cynus_hdr_h__

typedef enum {
	HDR_TO_SDR,
	HLG_TO_SDR,
	BYPASS,
} HdrWorkMode; 

void hdr_init(void);
void hdr_set_work_mode(HdrWorkMode mode);

#endif

