#include "chip_media/VpuApi.h"
#include "chip_media/chip_media.h"
#include "video_userdata.h"
#include "kernelcalls.h"
#include "gxav_common.h"

static int userdata_change_endian(unsigned start_addr, unsigned data_length)
{
	int cnt = 0, *word1, *word2, tmp1, tmp2;
	char byte0, byte1, byte2, byte3;

	while(cnt < 0x88) {
		word1 = (int*)(start_addr+cnt);
		word2 = (int*)(start_addr+cnt + 4);

		byte0 = (*word1>>0 )&0xff;
		byte1 = (*word1>>8 )&0xff;
		byte2 = (*word1>>16)&0xff;
		byte3 = (*word1>>24)&0xff;
		tmp1 = (byte1<<24)|(byte0<<16)|(byte3<<8)|(byte2<<0);

		byte0 = (*word2>>0 )&0xff;
		byte1 = (*word2>>8 )&0xff;
		byte2 = (*word2>>16)&0xff;
		byte3 = (*word2>>24)&0xff;
		tmp2 = (byte1<<24)|(byte0<<16)|(byte3<<8)|(byte2<<0);

		*word1 = tmp2;
		*word2 = tmp1;

		cnt += 8;
	}

	while(cnt < data_length) {
		word1 = (int*)(start_addr+cnt);
		word2 = (int*)(start_addr+cnt + 4);

		byte0 = (*word1>>0 )&0xff;
		byte1 = (*word1>>8 )&0xff;
		byte2 = (*word1>>16)&0xff;
		byte3 = (*word1>>24)&0xff;
		tmp1 = (byte0<<24)|(byte1<<16)|(byte2<<8)|(byte3<<0);

		byte0 = (*word2>>0 )&0xff;
		byte1 = (*word2>>8 )&0xff;
		byte2 = (*word2>>16)&0xff;
		byte3 = (*word2>>24)&0xff;
		tmp2 = (byte0<<24)|(byte1<<16)|(byte2<<8)|(byte3<<0);

		*word1 = tmp2;
		*word2 = tmp1;

		cnt += 8;
	}

	return 0;
}

int userdata_copy(void *to, void *from, int size)
{
	userdata_change_endian((unsigned)from, size);
	gx_memcpy(to, from, size);
	return 0;
}

int userdata_payload_get(int codec_type, void *src_data, int src_data_len, unsigned int *payload, unsigned int *payload_len)
{
	int   payload_offset = 0;
	short dst_type = 0;
	short user_data_num = 0, full = 0, *p = NULL;

	*payload_len = 0;
	*payload     = 0;

	p = (short*)src_data;
	switch(codec_type)
	{
	case STD_AVS:
		dst_type = 0; break;
	case STD_MPEG2:
		dst_type = 2; break;
	case STD_AVC:
		dst_type = 4; break;
	default:
		return -1;
	}

	user_data_num = *p;
	p += 3;//jump to full
	full = *p;
	if(full)
		gx_printf("cc: payload full!\n");
	p += 1;//jump to payload index
	if(user_data_num <= 16) {
		for( ; user_data_num>0; user_data_num--, p+=4) {
			if(*p == dst_type) {
				if(0x88 + payload_offset < src_data_len) {
					*payload_len = *(p+1);
					*payload     = (int)src_data + 0x88 + payload_offset;
					break;
				}
				else {
					gx_printf("cc: user data buf too small!\n");
					return -1;
				}
			}
			else {
				payload_offset += *(p+1);
			}
		}
		if(user_data_num==0 && payload==NULL)
			return -1;
	}
	else {
		gx_printf("cc: user data num error!\n");
		return -1;
	}

	return 0;
}



static int cc_parse(void *vd, void *payload, unsigned payload_len, void* parse_to)
{
	char *p = NULL;
	int process_cc_data_flag = 0, additional_data_flag = 0;
	short i, cc_count = 0, cc_valid = 0, cc_type = 0, cc_data;
	short valid_cc_num = 0, *output = NULL, tmp_parse_to[500];

	if(!payload)
		return -1;
	gx_memset((void*)tmp_parse_to, 0, sizeof(tmp_parse_to));

	//data example: 0x34394147 0xfc004103 0x00ff8080
	p = (char*)payload;
	while(p <= (char*)payload+payload_len-3) {
		if( * p    == 0x47 && *(p+1) == 0x41 &&
			*(p+2) == 0x39 && *(p+3) == 0x34 ) {
			p += 5;//skip 0x47 0x41 0x39 0x34 0x03
			process_cc_data_flag = (*p>>6)&0x1;
			if(process_cc_data_flag) {
				additional_data_flag = (*p>>5)&0x1;
				cc_count = (*p>>0)&0x1f;
				p += 2;//skip 0x41 0x00
				output = &tmp_parse_to[1];//write each cc_data first, totle cc_num would be written at last
				//gx_printf("------------------------------------\n");
				for(i = 0; i < cc_count; i++, p+=3) {
					cc_valid = (*p>>2)&0x1;
					if(cc_valid) {
						valid_cc_num ++;
						cc_type = (*p>>0)&0x3;//[STD 608]: 00(top), 01(bottom);[STD 708]: 10(top), 11(bottom);
						cc_data = (short)((*(p+2)<<8)|*(p+1));
						//gx_printf("h_vd : cc_type = %d, cc_data = 0x%04x\n",cc_type, cc_data);
						//write a group cc to BufCc
						*output++ = cc_type;
						*output++ = cc_data;
					}
				}
				//gx_printf("------------------------------------\n");
				//write valid cc_num to the head
				 tmp_parse_to[0] = valid_cc_num;
				//data structure in fb[i].vir.bufCc is :
				//totle_cc_num cc0.type cc0.data cc1.type, cc1.data ...
			}
		}
		else
			p++;
	}
	gx_memcpy(parse_to, tmp_parse_to, sizeof(tmp_parse_to));

	return 0;
}

#define SKIP_BYTES(num_byte)\
	do {\
		p           += (num_byte);\
		payload_len -= (num_byte);\
	}while(0)
static int afd_parse(void *vd, void *payload, unsigned payload_len, void* parse_to)
{
	char *p = NULL;
	int active_format;
	int active_format_flag;
	struct vd_chip_media *chip_media = vd;

	if(payload_len < 2)
		return -1;

	p = (char*)payload;
	//SKIP_BYTES(1);// country_code
	//SKIP_BYTES(2);// provider_code
	SKIP_BYTES(4);// identifier

	active_format_flag = ((*p)>>6) & 1;
	if(active_format_flag) {
		SKIP_BYTES(1);
		chip_media->AFD.enable = 1;
		chip_media->AFD.active_format = active_format = (*p)&0xF;
	}
	else {
		chip_media->AFD.enable = 0;
	}

	return 0;
}

static struct userdata_node {
	UserdataType  type;
	unsigned int identifier;
	int          (*parse)(void *vd, void *payload, unsigned payload_len, void* parse_to);
} userdatas[] = {
	{
		.type       = UDT_CC,
		.identifier = 0x47413934,
		.parse      = cc_parse,
	},
	{
		.type       = UDT_AFD,
		.identifier = 0x44544731,
		.parse      = afd_parse,
	}
};

int userdata_payload_probe(void *vd, void *payload, unsigned payload_len)
{
	int i, exit;
	char *p = NULL, *max = NULL;
	unsigned identifier;
	int type = UDT_NULL;

	if(payload_len < 7)
		return -1;

	p    = (char*)payload;
	max  = (char*)payload+payload_len-3;
	exit = sizeof(userdatas)/sizeof(struct userdata_node);
	while (p <= max) {
		identifier = (*(p+0)<<24 | *(p+1)<<16 | *(p+2)<<8 | *(p+3)<<0);
		for (i = 0; i < exit; i++) {
			if (identifier == userdatas[i].identifier) {
				type = userdatas[i].type;
				break;
			}
		}
		if(i == exit)
			SKIP_BYTES(1);

		if (type != UDT_NULL)
			break;
	}

	//gx_printf("userdata type = %s\n", type == 0 ? "CC" : "AFD");
	return type;
}
//user_data, ITU_T_T35
int userdata_payload_parse(void *vd, void *payload, unsigned payload_len, void* parse_to)
{
	int i, exit;
	char *p = NULL, *max = NULL;
	unsigned identifier;

	if(payload_len < 7)
		return -1;

	p    = (char*)payload;
	max  = (char*)payload+payload_len-3;
	exit = sizeof(userdatas)/sizeof(struct userdata_node);
	while(p <= max) {
		identifier = (*(p+0)<<24 | *(p+1)<<16 | *(p+2)<<8 | *(p+3)<<0);
		for(i = 0; i < exit; i++) {
			if(identifier == userdatas[i].identifier)
				return userdatas[i].parse(vd, p, payload_len, parse_to);
		}
		if(i == exit)
			SKIP_BYTES(1);
	}

	return -1;
}

int userdata_proc_write(void *payload, unsigned payload_len, unsigned pts, struct gxfifo *fifo)
{
	unsigned int free_len = gxfifo_freelen(fifo);
	if(free_len >= payload_len)
		gxfifo_put(fifo, payload, payload_len);

	return 0;
}

int userdata_proc_read(void *buf, unsigned count, struct gxfifo *fifo)
{
	return gxfifo_user_get(fifo, buf, count);
}

int cc_parse_per_frame(void *data_start, CcPerFrame *frame_cc)
{
	short i, data_type, *p = NULL;

	if(data_start) {
		gx_memset(frame_cc, 0, sizeof(CcPerFrame));
		p = (short*)data_start;
		frame_cc->cc_num = *p++;
		if(frame_cc->cc_num <= MAX_CC_PER_FRAME) {
			for(i = 0; i < frame_cc->cc_num; i++ ) {
				data_type = *p++;
				if((data_type==0x00||data_type==0x10) && frame_cc->top.data_num<MAX_CC_PER_FIELD) {
					frame_cc->top.data[frame_cc->top.data_num] = *p++;
					frame_cc->top.data_num ++;
				}
				if((data_type==0x01||data_type==0x11) && frame_cc->bottom.data_num<MAX_CC_PER_FIELD) {
					frame_cc->bottom.data[frame_cc->bottom.data_num] = *p++;
					frame_cc->bottom.data_num ++;
				}
			}
		}
		else {
			return -1;
			gx_printf("cc unsupport! cc per frame = %d, max cc we can support = %d!", frame_cc->cc_num, MAX_CC_PER_FRAME);
		}

		return 0;
	}

	return -1;
}

