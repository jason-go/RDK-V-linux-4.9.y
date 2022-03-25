/*
 * @file:audio.c
 *
 *  Created on: Jul 2, 2010
 *  Modified: Oct 2010: GPA interface added
 *  Modified: May 2011: DMA added
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "audio.h"
#include "halFrameComposerAudio.h"
#include "halAudioI2s.h"
#include "halAudioSpdif.h"
#include "halAudioHbr.h"
#include "halAudioGpa.h"
#include "halAudioClock.h"
#include "halAudioGenerator.h"
#include "halAudioDma.h"
#include "../util/log.h"
#include "halMainController.h"

static struct cts_map{
	u16 pixel_clock;
	struct{
		u32 sample_freq;
		u32 cts;
	}val;
}g_cts_map[] = {
	{2700 , {32000 , 27000}},
	{2700 , {44100 , 30000}},
	{2700 , {48000 , 27000}},
	{2700 , {88200 , 30000}},
	{2700 , {96000 , 27000}},
	{2700 , {176400, 30000}},
	{2700 , {192000, 27000}},

	{7425 , {32000 , 74250}},
	{7425 , {44100 , 82500}},
	{7425 , {48000 , 74250}},
	{7425 , {88200 , 82500}},
	{7425 , {96000 , 74250}},
	{7425 , {176400, 82500}},
	{7425 , {192000, 74250}},

	{14850, {32000 , 148500}},
	{14850, {44100 , 165000}},
	{14850, {48000 , 148500}},
	{14850, {88200 , 165000}},
	{14850, {96000 , 148500}},
	{14850, {176400, 165000}},
	{14850, {192000, 148500}},
};

static u32 _get_cts(u16 pixel_clock, u32 sample_freq, u16 ratioClk)
{
	u32 cts = 0;
	int i;
	u32 ratio = 100;

	for(i = 0; i < sizeof(g_cts_map) / sizeof(g_cts_map[0]); i++){
		if(pixel_clock == g_cts_map[i].pixel_clock){
			if(sample_freq == g_cts_map[i].val.sample_freq) {
				if (ratioClk == ratio) {
					return g_cts_map[i].val.cts;
				} else {
					return (g_cts_map[i].val.cts * (ratioClk / (ratioClk - ratio)));
				}
			}
		}
	}
	return cts;
}
/* block offsets */
static const u16 FC_BASE_ADDR = 0x1000;
static const u16 AUD_BASE_ADDR = 0x3100;
static const u16 MC_BASE_ADDR = 0x4000;
static const u16 AG_BASE_ADDR = 0x7100;
/* interface offset*/
static const u16 AUDIO_I2S   = 0x0000;
static const u16 AUDIO_CLOCK = 0x0100;
static const u16 AUDIO_SPDIF = 0x0200;
static const u16 AUDIO_HBR   = 0x0300;
static const u16 AUDIO_GPA   = 0x0400;
static const u16 AUDIO_DMA   = 0x0500;

int audio_Initialize(u16 baseAddr)
{
	LOG_TRACE();
	halAudioDma_DmaInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, ~0);
	halAudioDma_BufferInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, ~0);
	return audio_Mute(baseAddr, 1);
}

int audio_Configure(u16 baseAddr, audioParams_t *params, u16 pixelClk, unsigned int ratioClk)
{
	u16 i = 0;
	LOG_TRACE();

	/* more than 2 channels => layout 1 else layout 0 */
	halFrameComposerAudio_PacketLayout(baseAddr + FC_BASE_ADDR,
			((audioParams_ChannelCount(params) + 1) > 2) ? 1 : 0);
	/* iec validity and user bits (IEC 60958-1 */
	for (i = 0; i < 4; i++)
	{
		/* audioParams_IsChannelEn considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		halFrameComposerAudio_ValidityRight(baseAddr + FC_BASE_ADDR,
				!(audioParams_IsChannelEn(params, (2 * i))), i);
		halFrameComposerAudio_ValidityLeft(baseAddr + FC_BASE_ADDR,
				!(audioParams_IsChannelEn(params, (2 * i) + 1)), i);
		halFrameComposerAudio_UserRight(baseAddr + FC_BASE_ADDR,
				USER_BIT, i);
		halFrameComposerAudio_UserLeft(baseAddr + FC_BASE_ADDR,
				USER_BIT, i);
	}
	/* IEC - not needed if non-linear PCM */
	halFrameComposerAudio_IecCgmsA(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCgmsA(params));
	halFrameComposerAudio_IecCopyright(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCopyright(params) ? 0 : 1);
	halFrameComposerAudio_IecCategoryCode(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecCategoryCode(params));
	halFrameComposerAudio_IecPcmMode(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecPcmMode(params));
	halFrameComposerAudio_IecSource(baseAddr + FC_BASE_ADDR,
			audioParams_GetIecSourceNumber(params));
	for (i = 0; i < 4; i++)
	{ 	/* 0, 1, 2, 3 */
		halFrameComposerAudio_IecChannelLeft(baseAddr + FC_BASE_ADDR,
				2* i + 1, i); /* 1, 3, 5, 7 */
		halFrameComposerAudio_IecChannelRight(baseAddr + FC_BASE_ADDR,
				2* (i + 1), i); /* 2, 4, 6, 8 */
	}
	halFrameComposerAudio_IecClockAccuracy(baseAddr + FC_BASE_ADDR, audioParams_GetIecClockAccuracy(params));
    halFrameComposerAudio_IecSamplingFreq(baseAddr + FC_BASE_ADDR, audioParams_IecSamplingFrequency(params));
    halFrameComposerAudio_IecOriginalSamplingFreq(baseAddr + FC_BASE_ADDR, audioParams_IecOriginalSamplingFrequency(params));
    halFrameComposerAudio_IecWordLength(baseAddr + FC_BASE_ADDR, audioParams_IecWordLength(params));

	halAudioI2s_Select(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, (audioParams_GetInterfaceType(params) == I2S)? 1 : 0);
 	/*
 	 * ATTENTION: fixed I2S data enable configuration
 	 * is equivalent to 0x1 for 1 or 2 channels
 	 * is equivalent to 0x3 for 3 or 4 channels
 	 * is equivalent to 0x7 for 5 or 6 channels
 	 * is equivalent to 0xF for 7 or 8 channels
 	 */
 	halAudioI2s_DataEnable(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 0xF);
 	/* ATTENTION: fixed I2S data mode (standard) */
 	halAudioI2s_DataMode(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 0);
 	halAudioI2s_DataWidth(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, audioParams_GetSampleSize(params));
 	halAudioI2s_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 3);
 	halAudioI2s_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, 3);
 	halMainController_AudioI2sReset(baseAddr + MC_BASE_ADDR, 0);
 	halAudioI2s_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_I2S);

 	halAudioSpdif_DataEnable(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 0x1);
 	halAudioSpdif_NonLinearPcm(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, audioParams_IsLinearPCM(params)? 0 : 1);
 	halAudioSpdif_DataWidth(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, audioParams_GetSampleSize(params));
 	halAudioSpdif_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 3);
 	halAudioSpdif_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, 3);
 	halMainController_AudioSpdifReset(baseAddr + MC_BASE_ADDR, 0);
 	halAudioSpdif_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF);

 	halAudioHbr_Select(baseAddr + AUD_BASE_ADDR + AUDIO_HBR, (audioParams_GetInterfaceType(params) == HBR)? 1 : 0);
 	halAudioHbr_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_HBR, 7);
 	halAudioHbr_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_HBR, 7);
 	halAudioHbr_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_HBR);

 	halAudioClock_N(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeN(baseAddr, audioParams_GetSamplingFrequency(params), pixelClk, ratioClk));

 	if (audioParams_GetInterfaceType(params) == GPA)
	{
		if (audioParams_GetPacketType(params) == HBR_STREAM)
		{
			halAudioGpa_HbrEnable(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, 1);
			for (i = 0; i < 8; i++)
			{	/* 8 channels must be enabled */
				halAudioGpa_ChannelEnable(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, 1, i);
			}
			/* layout 0 always for HBR*/
			halFrameComposerAudio_PacketLayout(baseAddr + FC_BASE_ADDR, 0);
			/* N is Fs divided by 4 */
			halAudioClock_N(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeN(baseAddr, audioParams_GetSamplingFrequency(params) / 4, pixelClk, ratioClk));
			/* CTS is Fs divided by 4 */
			halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeCts(baseAddr, audioParams_GetSamplingFrequency(params) / 4, pixelClk, ratioClk));
		}
		else
		{	/* When insert_pucv is active (1) any input data is ignored  and the
			 parity and B pulse bits are generated in run time, while the
			 Channel status, User bit and Valid bit are retrieved from registers
			  FC_AUDSCHNLS0 to FC_AUDSCHNLS8, FC_AUDSU and FC_AUDSV. */
			halAudioGpa_InsertPucv(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, audioParams_GetGpaSamplePacketInfoSource(params));
			halAudioGpa_HbrEnable(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, 0);
			for (i = 0; i < 8; i++)
			{
				halAudioGpa_ChannelEnable(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, audioParams_IsChannelEn(params, i), i);
			}
			/* compute CTS */
			halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeCts(baseAddr, audioParams_GetSamplingFrequency(params), pixelClk, ratioClk));
		}
		halAudioGpa_InterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, 3);
		halAudioGpa_InterruptPolarity(baseAddr + AUD_BASE_ADDR + AUDIO_GPA, 3);
		halMainController_AudioGpaReset(baseAddr + MC_BASE_ADDR, 0);
		halAudioGpa_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_GPA);
	}
	else if (audioParams_GetInterfaceType(params) == DMA)
	{
		if (audioParams_GetPacketType(params) == HBR_STREAM)
		{
			halAudioDma_HbrEnable(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, 1);
			for (i = 2; i < 8; i++)
			{	/* 8 channels must be enabled */
				halAudioDma_ChannelEnable(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, 1, i);
			}
		}
		else
		{
			halAudioDma_HbrEnable(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, 0);
			for (i = 2; i < 8; i++)
			{
				halAudioDma_ChannelEnable(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, audioParams_IsChannelEn(params, i), i);
			}
		}
		halAudioDma_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
		halAudioDma_FixBurstMode(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, audioParams_GetDmaBeatIncrement(params));
		halAudioDma_Threshold(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, audioParams_GetDmaThreshold(params));
		/* compute CTS */
		halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, audio_ComputeCts(baseAddr, audioParams_GetSamplingFrequency(params), pixelClk, ratioClk));
	}
	else
	{
		u16 fsFactor = 0;
		/* if NOT GPA interface, use automatic mode CTS */
		halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, _get_cts(pixelClk, audioParams_GetSamplingFrequency(params), ratioClk));
		//halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 30000);
		//halAudioClock_Cts(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 0);
		if (audioParams_GetInterfaceType(params) == I2S)
			fsFactor = 64;
		else
			fsFactor = audioParams_GetClockFsFactor(params);

		switch (fsFactor)
		{
			/* This version does not support DRU Bypass - found in controller 1v30a on
			 *	0 128Fs		I2S- (SPDIF when DRU in bypass)
			 *	1 256Fs		I2S-
			 *	2 512Fs		I2S-HBR-(SPDIF - when NOT DRU bypass)
			 *	3 1024Fs	I2S-(SPDIF - when NOT DRU bypass)
			 *	4 64Fs		I2S
			 */
			case 64:
				if (audioParams_GetInterfaceType(params) != I2S)
				{
					return FALSE;
				}
				halAudioClock_F(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 4);
				break;
			case 128:
				if (audioParams_GetInterfaceType(params) != I2S)
				{
					return FALSE;
				}
				halAudioClock_F(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 0);
				break;
			case 256:
				if (audioParams_GetInterfaceType(params) != I2S)
				{
					return FALSE;
				}
				halAudioClock_F(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 1);
				break;
			case 512:
				halAudioClock_F(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 2);
				break;
			case 1024:
				if (audioParams_GetInterfaceType(params) == HBR)
				{
					return FALSE;
				}
				halAudioClock_F(baseAddr + AUD_BASE_ADDR + AUDIO_CLOCK, 3);
				break;
			default:
				/* Fs clock factor not supported */
				return FALSE;
		}
	}
// 	if (audio_AudioGenerator(baseAddr, params) != TRUE)
// 	{
// 		return FALSE;
// 	}

 	return TRUE;
 }

int audio_Mute(u16 baseAddr, u8 state)
{
	/* audio mute priority: AVMUTE, sample flat, validity */
	/* AVMUTE also mutes video */
	halAudioI2s_DataEnable(baseAddr + AUD_BASE_ADDR + AUDIO_I2S, state?0x0: 0xF);
	halMainController_AudioI2sReset(baseAddr + MC_BASE_ADDR, 0);
	halAudioI2s_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_I2S);

	//halAudioSpdif_DataEnable(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF, state?0x0: 0x1);
	//halMainController_AudioSpdifReset(baseAddr + MC_BASE_ADDR, 0);
	//halAudioSpdif_ResetFifo(baseAddr + AUD_BASE_ADDR + AUDIO_SPDIF);

	return TRUE;
}

int audio_AudioGenerator(u16 baseAddr, audioParams_t *params)
{
	/*
	 * audio generator block is not included in real application hardware,
	 * Then the external audio sources are used and this code has no effect
	 */
	u32 tmp;
	LOG_TRACE();
	halAudioGenerator_I2sMode(baseAddr + AG_BASE_ADDR, 1); /* should be coherent with I2S config? */
	tmp = (1500 * 65535) / audioParams_GetSamplingFrequency(params); /* 1500Hz */
	halAudioGenerator_FreqIncrementLeft(baseAddr + AG_BASE_ADDR, (u16) (tmp));
	tmp = (3000 * 65535) / audioParams_GetSamplingFrequency(params); /* 3000Hz */
	halAudioGenerator_FreqIncrementRight(baseAddr + AG_BASE_ADDR,
			(u16) (tmp));

	halAudioGenerator_IecCgmsA(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecCgmsA(params));
	halAudioGenerator_IecCopyright(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecCopyright(params) ? 0 : 1);
	halAudioGenerator_IecCategoryCode(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecCategoryCode(params));
	halAudioGenerator_IecPcmMode(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecPcmMode(params));
	halAudioGenerator_IecSource(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecSourceNumber(params));
	for (tmp = 0; tmp < 4; tmp++)
	{
		/* 0 -> iec spec 60958-3 means "do not take into account" */
		halAudioGenerator_IecChannelLeft(baseAddr + AG_BASE_ADDR, 0, tmp);
		halAudioGenerator_IecChannelRight(baseAddr + AG_BASE_ADDR, 0, tmp);
		/* USER_BIT 0 default by spec */
		halAudioGenerator_UserLeft(baseAddr + AG_BASE_ADDR, USER_BIT, tmp);
		halAudioGenerator_UserRight(baseAddr + AG_BASE_ADDR, USER_BIT, tmp);
	}
	halAudioGenerator_IecClockAccuracy(baseAddr + AG_BASE_ADDR,
			audioParams_GetIecClockAccuracy(params));
	halAudioGenerator_IecSamplingFreq(baseAddr + AG_BASE_ADDR,
			audioParams_IecSamplingFrequency(params));
	halAudioGenerator_IecOriginalSamplingFreq(baseAddr + AG_BASE_ADDR,
			audioParams_IecOriginalSamplingFrequency(params));
	halAudioGenerator_IecWordLength(baseAddr + AG_BASE_ADDR,
			audioParams_IecWordLength(params));

	halAudioGenerator_SpdifTxData(baseAddr + AG_BASE_ADDR, 1);
	/* HBR is synthesizable but not audible */
	halAudioGenerator_HbrDdrEnable(baseAddr + AG_BASE_ADDR, 0);
	halAudioGenerator_HbrDdrChannel(baseAddr + AG_BASE_ADDR, 0);
	halAudioGenerator_HbrBurstLength(baseAddr + AG_BASE_ADDR, 0);
	halAudioGenerator_HbrClockDivider(baseAddr + AG_BASE_ADDR, 128); /* 128 * fs */
	halAudioGenerator_HbrEnable(baseAddr + AG_BASE_ADDR, 1);
	/* GPA */
	if (audioParams_GetPacketType(params) != HBR_STREAM)
	{
		for (tmp = 0; tmp < 8; tmp++)
		{
			halAudioGenerator_GpaSampleValid(baseAddr + AG_BASE_ADDR, !(audioParams_IsChannelEn(params, tmp)), tmp);
			halAudioGenerator_ChannelSelect(baseAddr + AG_BASE_ADDR, audioParams_IsChannelEn(params, tmp), tmp);
		}
	}
	else
	{
		halAudioGenerator_GpaSampleValid(baseAddr + AG_BASE_ADDR, 0, 0);
		halAudioGenerator_GpaSampleValid(baseAddr + AG_BASE_ADDR, 0, 1);
		halAudioGenerator_ChannelSelect(baseAddr + AG_BASE_ADDR, 1, 0);
		halAudioGenerator_ChannelSelect(baseAddr + AG_BASE_ADDR, 1, 1);
	}
	halAudioGenerator_GpaReplyLatency(baseAddr + AG_BASE_ADDR, 0x03);
	halAudioGenerator_SwReset(baseAddr + AG_BASE_ADDR, 1);
	return TRUE;
}

u16 audio_ComputeN(u16 baseAddr, u32 freq, u16 pixelClk, u16 ratioClk)
{
	u32 n = (128 * freq) / 1000;
	u32 ratio = 100;

	switch (freq) {
		case 32000:
			if (pixelClk == 2517)
				n = 4576;
			else if (pixelClk == 2702)
				n = 4096;
			else if (pixelClk == 7417 || pixelClk == 14835)
				n = 11648;
			else
				n = 4096;
			break;
		case 44100:
			if (pixelClk == 2517)
				n = 7007;
			else if (pixelClk == 7417)
				n = 17836;
			else if (pixelClk == 14835)
				n = 8918;
			else
				n = 6272;
			break;
		case 48000:
			if (pixelClk == 2517)
				n = 6864;
			else if (pixelClk == 2702)
				n = 6144;
			else if (pixelClk == 7417)
				n = 11648;
			else if (pixelClk == 14835)
				n = 5824;
			else
				n = 6144;
			break;
		case 88200:
			n = audio_ComputeN(baseAddr, 44100, pixelClk, ratioClk) * 2;
			break;
		case 96000:
			n = audio_ComputeN(baseAddr, 48000, pixelClk, ratioClk) * 2;
			break;
		case 176400:
			n = audio_ComputeN(baseAddr, 44100, pixelClk, ratioClk) * 4;
			break;
		case 192000:
			n = audio_ComputeN(baseAddr, 48000, pixelClk, ratioClk) * 4;
			break;
		default:
			break;
	}

	if (ratioClk == ratio) {
		return n;
	} else {
		return (n * (ratio / (ratioClk - ratio)));
	}
}

u32 audio_ComputeCts(u16 baseAddr, u32 freq, u16 pixelClk, u16 ratioClk)
{
	u32 cts = 0;
	switch(freq)
	{
		case 32000:
			if (pixelClk == 29700)
			{
				cts = 222750;
				break;
			}
		case 48000:
		case 96000:
		case 192000:
			switch (pixelClk)
			{
				case 2520:
				case 2700:
				case 5400:
				case 7425:
				case 14850:
					cts = pixelClk * 10;
					break;
				case 29700:
					cts = 247500;
					break;
				default:
					/* All other TMDS clocks are not supported by DWC_hdmi_tx
					 * the TMDS clocks divided or multiplied by 1,001
					 * coefficients are not supported.
					 */
					LOG_WARNING("TMDS is not guaranteed to give proper CTS, refer to DWC_HDMI_TX databook");
					cts = (pixelClk * 100 * audio_ComputeN(baseAddr, freq, pixelClk, ratioClk)) / (128 * (freq /100));
					break;
			}
			break;
		case 44100:
		case 88200:
		case 176400:
			switch (pixelClk)
			{
				case 2520:
					cts = 28000;
					break;
				case 2700:
					cts = 30000;
					break;
				case 5400:
					cts = 60000;
					break;
				case 7425:
					cts = 82500;
					break;
				case 10800:
					cts = 120000;
					break;
				case 14850:
					cts = 165000;
					break;
				case 29700:
					cts = 247500;
					break;
				default:
					/* All other TMDS clocks are not supported by DWC_hdmi_tx
					 * the TMDS clocks divided or multiplied by 1,001
					 * coefficients are not supported.
					 */
					LOG_WARNING("TMDS is not guaranteed to give proper CTS, refer to DWC_HDMI_TX databook");
					cts = (pixelClk * 100 * audio_ComputeN(baseAddr, freq, pixelClk, ratioClk)) / (128 * (freq /100));
					break;
			}
			break;
		default:
			break;
	}
	return (cts * ratioClk) / 100;
}

void audio_DmaRequestAddress(u16 baseAddr, u32 startAddress, u32 stopAddress)
{
	LOG_TRACE();
	halAudioDma_StartAddress(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, startAddress);
	halAudioDma_StopAddress(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, stopAddress);
}

u16 audio_DmaGetCurrentBurstLength(u16 baseAddr)
{
	LOG_TRACE();
	return halAudioDma_CurrentBurstLength(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
}

u32 audio_DmaGetCurrentOperationAddress(u16 baseAddr)
{
	LOG_TRACE();
	return halAudioDma_CurrentOperationAddress(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
}


void audio_DmaStartRead(u16 baseAddr)
{
	LOG_TRACE();
	halAudioDma_Start(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
}


void audio_DmaStopRead(u16 baseAddr)
{
	LOG_TRACE();
	halAudioDma_Stop(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
}

void audio_DmaInterruptEnable(u16 baseAddr, u8 mask)
{
	LOG_TRACE();
	halAudioDma_DmaInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, ~mask & halAudioDma_DmaInterruptMaskStatus(baseAddr + AUD_BASE_ADDR + AUDIO_DMA));
}

void audio_DmaInterruptDisable(u16 baseAddr, u8 mask)
{
	LOG_TRACE();
	halAudioDma_DmaInterruptMask(baseAddr + AUD_BASE_ADDR + AUDIO_DMA, mask | halAudioDma_DmaInterruptMaskStatus(baseAddr + AUD_BASE_ADDR + AUDIO_DMA));
}

u8 audio_DmaInterruptEnableStatus(u16 baseAddr)
{
	LOG_TRACE();
	return halAudioDma_DmaInterruptMaskStatus(baseAddr + AUD_BASE_ADDR + AUDIO_DMA);
}
