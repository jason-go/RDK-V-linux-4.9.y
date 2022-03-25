#ifndef _VIDEO_CC_H_
#define _VIDEO_CC_H_

#include "fifo.h"

#define CC_FIFO_LENGTH		(10)
#define MAX_CC_PER_FIELD	(1)
#define MAX_CC_PER_FRAME	(2)

typedef enum {
	UDT_CC,
	UDT_AFD,
	UDT_NULL,
} UserdataType;

typedef struct{
	short cc_num;
	struct {
		short data_num;
		short data[MAX_CC_PER_FIELD];
	}top, bottom;
}CcPerFrame;

int userdata_copy         (void *to, void *from, int size);
int userdata_payload_get  (int codec_type, void *src_data, int src_data_len, unsigned *payload, unsigned *payload_len);

int userdata_proc_read    (void *buf,      unsigned count,       struct gxfifo *fifo);
int userdata_proc_write   (void *payload,  unsigned payload_len, unsigned pts, struct gxfifo *fifo);
int userdata_payload_parse(void *vd, void *payload, unsigned payload_len, void* parse_to);
int userdata_payload_probe(void *vd, void *payload, unsigned payload_len);
//data format after parse:
//totle_cc_num cc0.type cc0.data cc1.type, cc1.data ...(short int for each)

int cc_parse_per_frame(void *data_start, CcPerFrame *frame_cc);
#endif

