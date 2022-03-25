/*
 * hdcpParams.h
 *
 *  Created on: Jul 20, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02 
 */

#ifndef HDCPPARAMS_H_
#define HDCPPARAMS_H_
#include "../util/types.h"

typedef struct
{
	/** Enable Feature 1.1 */
	int mEnable11Feature;
	/** Check Ri every 128th frame */
	int mRiCheck;
	/** I2C fast mode */
	int mI2cFastMode;
	/** Enhanced link verification */
	int mEnhancedLinkVerification;
	/** Number of supported devices
	 * (depending on instantiated KSV MEM RAM – Revocation Memory to support
	 * HDCP repeaters)
	 */
	u8  mNoOfDevices;
	/** KSV List buffer
	 * Shall be dimensioned to accommodate 5[bytes] x No. of supported devices
	 * (depending on instantiated KSV MEM RAM – Revocation Memory to support
	 * HDCP repeaters)
	 * plus 8 bytes (64-bit) M0 secret value
	 * plus 2 bytes Bstatus
	 * Plus 20 bytes to calculate the SHA-1 (VH0-VH4)
	 * Total is (30[bytes] + 5[bytes] x Number of supported devices)
	 */
	u8* mKsvListBuffer;
	/** aksv total of 14 chars**/
	u8 *mAksv;
	/** Keys list
	 * 40 Keys of 14 characters each
	 * stored in segments of 8 bits (2 chars)
	 * **/
	u8 *mKeys;
	u8 *mSwEncKey;
}
hdcpParams_t;

void hdcpParams_Reset(hdcpParams_t *params);

int hdcpParams_GetEnable11Feature(hdcpParams_t *params);

int hdcpParams_GetRiCheck(hdcpParams_t *params);

int hdcpParams_GetI2cFastMode(hdcpParams_t *params);

int hdcpParams_GetEnhancedLinkVerification(hdcpParams_t *params);

u8 hdcpParams_GetNoOfSupportedDevices(hdcpParams_t *params);

u8 * hdcpParams_GetKsvListBuffer(hdcpParams_t *params);

void hdcpParams_SetEnable11Feature(hdcpParams_t *params, int value);

void hdcpParams_SetEnhancedLinkVerification(hdcpParams_t *params, int value);

void hdcpParams_SetI2cFastMode(hdcpParams_t *params, int value);

void hdcpParams_SetRiCheck(hdcpParams_t *params, int value);

void hdcpParams_SetNoOfSupportedDevices(hdcpParams_t *params, u8 value);

void hdcpParams_SetKsvListBuffer(hdcpParams_t *params, u8 *buffer);

void hdcpParams_SetDpkInfo(hdcpParams_t *params, u8* aksv, u8 *swEncKey, u8 *dpkKeys);

#endif /* HDCPPARAMS_H_ */
