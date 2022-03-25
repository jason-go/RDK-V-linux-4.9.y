#ifndef __SIRIUS_RNG_REG_H__
#define __SIRIUS_RNG_REG_H__

#define SIRIUS_MISC_REG_LEN 0x600

typedef union {
	unsigned int value;
	struct {
		unsigned req     : 1;
		unsigned valid   : 1;
	} bits;
} rRNG_CTRL;

#if defined (CPU_SCPU)
typedef struct {
	unsigned int      reserved[16];
	rRNG_CTRL         rng_ctrl;
	unsigned int      rng_data;
} SiriusRNGReg;

#else

typedef struct {
	unsigned int     reserved[0x300/4];
	rRNG_CTRL        rng_ctrl;                        // 0x300
	unsigned int     rng_data;                        // 0x304
	unsigned int     reserved1[(0x330-0x308)/4];
	unsigned int     CSSN[2];                         // 0x330
} SiriusRNGReg;
#endif

#endif
