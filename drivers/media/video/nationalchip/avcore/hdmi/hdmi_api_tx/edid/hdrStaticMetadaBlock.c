#include "hdrStaticMetadaBlock.h"
#include "../util/bitOperation.h"

void hdrStaticMetadataDataBlock_Reset(hdrStaticMetadataDataBlock_t *hdrdb)
{
	hdrdb->mElectroOpticalTransferFunction = 0;
	hdrdb->mStaticMetadataDescriptor       = 0;
	hdrdb->mValid = FALSE;
}

int hdrStaticMetadataDataBlock_Parse(hdrStaticMetadataDataBlock_t *hdrdb, u8 * data)
{
	hdrStaticMetadataDataBlock_Reset(hdrdb);

	/* check tag code and extended tag */
	if (data != 0 && bitOperation_BitField(data[0], 5, 3) == 0x7
			&& bitOperation_BitField(data[1], 0, 8) == 0x6
			&& bitOperation_BitField(data[0], 0, 5) == 0x2)
	{
		hdrdb->mElectroOpticalTransferFunction = data[2];
		hdrdb->mStaticMetadataDescriptor       = data[3];
		hdrdb->mValid = TRUE;
		return TRUE;
	}

	return FALSE;
}

int hdrStaticMetadataDataBlock_SupportsHDR10(hdrStaticMetadataDataBlock_t *hdrdb)
{
	return (bitOperation_BitField(hdrdb->mElectroOpticalTransferFunction, 2, 1) == 1) ? TRUE : FALSE;
}

int hdrStaticMetadataDataBlock_SupportsHLG(hdrStaticMetadataDataBlock_t *hdrdb)
{
	return (bitOperation_BitField(hdrdb->mElectroOpticalTransferFunction, 3, 1) == 1) ? TRUE : FALSE;
}

