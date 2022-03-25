#include "chip_media.h"
#include "aspect_ratio.h"

static int chip_media_io_map = 0;

int chip_media_register_buf(int start_addr)
{
	return 0;
}

void chip_media_int_enable(int type)
{
	int tmp;
	tmp = VpuReadReg(BIT_INT_ENABLE);
	tmp |= type;

	VpuWriteReg(BIT_INT_ENABLE, tmp);
}

void chip_media_int_disable(int type)
{
	int tmp;
	tmp = VpuReadReg(BIT_INT_ENABLE);
	tmp &=~type;
	VpuWriteReg(BIT_INT_ENABLE, tmp);
}

void chip_media_clrint(int type)
{
	int tmp = VpuReadReg(BIT_INT_REASON);

	tmp &=~type;
	VpuWriteReg(BIT_INT_REASON, tmp);
	VpuWriteReg(BIT_INT_CLEAR, 1);
}

int chip_media_get_inttype( void )
{
	return VpuReadReg(BIT_INT_REASON);
}

int chip_media_get_intenable( void )
{
	return VpuReadReg(BIT_INT_ENABLE);
}

int chip_media_get_seqinitret( void )
{
	return VpuReadReg(RET_DEC_SEQ_SUCCESS);
}

static unsigned int BODA_BIT_BASE;
static unsigned int BODA_GDMA_BASE;
static unsigned int BODA_MBC_BASE;

int chip_media_reg_iounmap( void )
{
	if( chip_media_io_map ) {
		if( MBC_BASE ) {
			gx_iounmap( MBC_BASE );
			gx_release_mem_region(BODA_MBC_BASE,0x040) ;
			MBC_BASE = 0;
		}

		if ( GDMA_BASE ) {
			gx_iounmap( GDMA_BASE );
			gx_release_mem_region(BODA_GDMA_BASE,0x410) ;
			GDMA_BASE = 0;
		}

		if( BIT_BASE ) {
			gx_iounmap( BIT_BASE );
			gx_release_mem_region(BODA_BIT_BASE, 0x200) ;
			BIT_BASE = 0;
		}

		chip_media_io_map = 0;
	}

	return 0;
}

int chip_media_reg_ioremap( void )
{
	if( chip_media_io_map )
		return 0;

	if (CHIP_IS_SIRIUS) {
		BODA_BIT_BASE  = 0x8A300000;
		BODA_GDMA_BASE = 0x8A301000;
		BODA_MBC_BASE  = 0x8A300400;
	} else {
		BODA_BIT_BASE  = 0x04200000;
		BODA_GDMA_BASE = 0x04201000;
		BODA_MBC_BASE  = 0x04200400;
	}

	chip_media_io_map = 1;

	if( BIT_BASE == 0 ) {
		if (!gx_request_mem_region(BODA_BIT_BASE, 0x200)) {
			VIDEODEC_PRINTF("%s request_mem_region failed",__func__);
			return -1;
		}
		BIT_BASE = (int)gx_ioremap(BODA_BIT_BASE,0x200);
		if (!BIT_BASE) {
			VIDEODEC_PRINTF("%x ioremap failed.\n",BODA_BIT_BASE);
			goto VIDEO_IO_MAP_ERR;
		} else {
			VIDEODEC_DBG("BODA_BIT_BASE base addr = %x\n",BIT_BASE);
		}
	}

	if( GDMA_BASE == 0 ) {
		if (!gx_request_mem_region(BODA_GDMA_BASE, 0x410)) {
			VIDEODEC_PRINTF("%s request_mem_region failed",__func__);
			return -1;
		}
		GDMA_BASE = (int)gx_ioremap(BODA_GDMA_BASE,0x410);
		if (!GDMA_BASE) {
			VIDEODEC_PRINTF("%x ioremap failed.\n",BODA_GDMA_BASE);
			goto VIDEO_IO_MAP_ERR;
		} else {
			VIDEODEC_DBG("BODA_GDMA_BASE base addr = %x\n",GDMA_BASE);
		}
	}

	if( MBC_BASE == 0 ) {
		if (!gx_request_mem_region(BODA_MBC_BASE, 0x040)) {
			VIDEODEC_PRINTF("%s request_mem_region failed",__func__);
			return -1;
		}
		MBC_BASE = (int)gx_ioremap(BODA_MBC_BASE,0x040);
		if (!MBC_BASE) {
			VIDEODEC_PRINTF("%x ioremap failed.\n",BODA_MBC_BASE);
			goto VIDEO_IO_MAP_ERR;
		} else {
			VIDEODEC_DBG("BODA_MBC_BASE base addr = %x\n",MBC_BASE);
		}
	}

	return 0;
VIDEO_IO_MAP_ERR:
#ifdef _linux_
	chip_media_reg_iounmap( );
#endif
	return -1;
}

