#ifndef __cygnus_vpu_hdr_h__
#define __cygnus_vpu_hdr_h__

struct hdr_channel_param {
	unsigned MAP_VAL;
	unsigned CENTER_VAL0;
	unsigned CENTER_VAL1;
	unsigned CENTER_VAL2;
	unsigned CENTER_VAL3;
	unsigned CENTER_VAL4;
	unsigned CENTER_VAL5;
	unsigned CURVE_VAL[96];
};

struct hdr_param {
	struct hdr_channel_param r;
	struct hdr_channel_param g;
	struct hdr_channel_param b;
};

#endif

