/*
 * hdcpParams.c
 *
 *  Created on: Jul 21, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "hdcpParams.h"

void hdcpParams_Reset(hdcpParams_t *params)
{
	params->mEnable11Feature = 0;
	params->mRiCheck = 1;
	params->mI2cFastMode = 0;
	params->mEnhancedLinkVerification = 0;
	params->mNoOfDevices = 0;
	params->mKsvListBuffer = 0;
}

int hdcpParams_GetEnable11Feature(hdcpParams_t *params)
{
	return params->mEnable11Feature;
}

int hdcpParams_GetRiCheck(hdcpParams_t *params)
{
	return params->mRiCheck;
}

int hdcpParams_GetI2cFastMode(hdcpParams_t *params)
{
	return params->mI2cFastMode;
}

int hdcpParams_GetEnhancedLinkVerification(hdcpParams_t *params)
{
	return params->mEnhancedLinkVerification;
}

u8 hdcpParams_GetNoOfSupportedDevices(hdcpParams_t *params)
{
	return params->mNoOfDevices;
}

u8 * hdcpParams_GetKsvListBuffer(hdcpParams_t *params)
{
	return params->mKsvListBuffer;
}

void hdcpParams_SetEnable11Feature(hdcpParams_t *params, int value)
{
	params->mEnable11Feature = value;
}

void hdcpParams_SetEnhancedLinkVerification(hdcpParams_t *params, int value)
{
	params->mEnhancedLinkVerification = value;
}

void hdcpParams_SetI2cFastMode(hdcpParams_t *params, int value)
{
	params->mI2cFastMode = value;
}

void hdcpParams_SetRiCheck(hdcpParams_t *params, int value)
{
	params->mRiCheck = value;
}

void hdcpParams_SetNoOfSupportedDevices(hdcpParams_t *params, u8 value)
{
	params->mNoOfDevices = value;
}

void hdcpParams_SetKsvListBuffer(hdcpParams_t *params, u8 *buffer)
{
	params->mKsvListBuffer = buffer;
}

void hdcpParams_SetDpkInfo(hdcpParams_t *params, u8* aksv, u8 *swEncKey, u8 *dpkKeys)
{
	params->mAksv = aksv;
	params->mSwEncKey = swEncKey;
	params->mKeys = dpkKeys;
}
