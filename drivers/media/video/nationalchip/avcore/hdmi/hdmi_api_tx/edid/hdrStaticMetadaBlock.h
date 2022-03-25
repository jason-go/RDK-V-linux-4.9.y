#ifndef __hdr_static_metadata_h__
#define __hdr_static_metadata_h__

#include "../util/types.h"

typedef struct {
	int mElectroOpticalTransferFunction;
	int mStaticMetadataDescriptor;
	int mValid;
} hdrStaticMetadataDataBlock_t;

void hdrStaticMetadataDataBlock_Reset(hdrStaticMetadataDataBlock_t *hdrdb);

int hdrStaticMetadataDataBlock_Parse(hdrStaticMetadataDataBlock_t *hdrdb, u8* data);

int hdrStaticMetadataDataBlock_SupportsHDR10(hdrStaticMetadataDataBlock_t *hdrdb);

int hdrStaticMetadataDataBlock_SupportsHLG(hdrStaticMetadataDataBlock_t *hdrdb);

#endif

