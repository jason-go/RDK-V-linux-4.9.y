/*
 * baord.c
 *
 *  Synopsys Inc.
 *  DWC PT02
 */
#include "board.h"
#include "access.h"
#include "../util/log.h"
#include "../util/error.h"

//#define BASIC_VIDMODES

/* Logical register address. */
static const u8 DRP_DCMRST = 0x00;
/* Logical register address. */
static const u8 DRP_ADDR = 0x01;
/* Logical register address. */
static const u8 DRP_DATAO_L = 0x02;
/* Logical register address. */
static const u8 DRP_DATAO_H = 0x03;
/* data in low logical register address. */
static const u8 DRP_DATAI_L = 0x04;
/* Logical register address. */
static const u8 DRP_DATAI_H = 0x05;
/* Logical register address. */
static const u8 DRP_CMD = 0x06;
/* Video data multiplexer 16-bit base address */
static const u16 VIDEO_MUX_ADDRESS = 0x7010;
/* Video DRP PLL controller 16-bit base address */
static const u16 VIDEO_PLL_BASE_ADDRESS = 0x7400;
/* Audio DRP PLL controller 16-bit base address */
static const u16 AUDIO_PLL_BASE_ADDRESS = 0x7300;
/*  DRP PLL controller 16-bit nase address */
static const u16 TCLK_PLL_BASE_ADDRESS = 0x7500;
/* Audio multiplexer 16-bit base address */
static const u16 AUDIO_MUX_ADDRESS = 0x7106;
/* Repeater Mode Enable information from demo application */
extern unsigned repeater_enabled;

int board_Initialize(u16 baseAddr, u16 pixelClock, u8 cd)
{
	unsigned i = 0;
	struct
	{
		const u16 addr;
		u8 data;
	} cfg[] =
	{
			/* reset */
			{ 0x7600, 0x00 },
			{ 0x7600, 0x01 },
			/* by default mask all interrupts */
			{ 0x0807, 0xFF }, /* VP */
			{ 0x10D2, 0xFF }, /* Packets */
			{ 0x10D6, 0xFF }, /* Packets */
			{ 0x10DA, 0xFF }, /* Packets */
			{ 0x3006, 0xFF }, /* PHY */
			{ 0x3027, 0xFF }, /* I2CM PHY */
			{ 0x3028, 0xFF }, /* I2CM PHY */
			{ 0x3102, 0xFF }, /* AS - I2S */
			{ 0x3302, 0xFF }, /* AS - SPDIF */
			{ 0x3404, 0xFF }, /* AS - HBR */
			{ 0x3506, 0xFF }, /* AS - GPA */
			{ 0x5008, 0xFF }, /* HDCP */
			{ 0x7D02, 0xFF }, /* CEC */
			{ 0x7E05, 0xFF }, /* I2C EDID */
			{ 0x7E06, 0xFF }, /* I2C EDID */
	};
	LOG_TRACE1(pixelClock);
	for (i = 0; i < (sizeof(cfg) / sizeof(cfg[0])); i++)
	{
		access_CoreWriteByte(cfg[i].data, cfg[i].addr);
	}
	/* Pull down the ZCALRST - for PHY GEN 2, on the HAPS51 DEMO BOARD */
	/* access_CoreWrite(0, 0x7900, 0, 1); // moved to phy.c, address updated */
	return TRUE;
}

int board_AudioClock(u16 baseAddr, u32 value)
{
	LOG_TRACE1(value);
	board_PllReset(AUDIO_PLL_BASE_ADDRESS, 0x01);
	switch (value)
	{
	case (128 * 32000): /* I2S (or SPDIF when DRU in bypass or GPA) 128 Factor, 32.0kHz Fs */
					/* default */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x1249);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x1db6);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0198);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21df);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (128 * 44100): /* I2S (or SPDIF when DRU in bypass or GPA) 128 Factor, 44.1kHz Fs */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x12cc);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0080);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x1cb2);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0218);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (128 * 48000): /* I2S (or SPDIF when DRU in bypass or GPA) 128 Factor, 48.0kHz Fs */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x128b);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0080);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x1aaa);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0298);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21df);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (128 * 88200): /* I2S (or SPDIF when DRU in bypass or GPA) 128 Factor, 88.2kHz Fs */
	case (128 * 96000): /* I2S (or SPDIF when DRU in bypass or GPA) 128 Factor, 96.0kHz Fs */
	error_Set(ERR_AUDIO_CLOCK_NOT_SUPPORTED);
	LOG_ERROR2("audio clock not supported", value);
	return FALSE;
	break;
	case (512 * 32000): /* 512 Factor, 32.0kHz Fs */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x128a);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x13cf);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0218);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (512 * 44100): /* SPDIF 44.1kHz or (128 fsf) 176.4kHz */
					error_Set(ERR_AUDIO_CLOCK_NOT_SUPPORTED);
	LOG_ERROR2("audio clock not supported", value);
	return FALSE;
	break;
	case (512 * 48000): /* SPDIF 48.0kHz or (128 fsf) 192.0kHz */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x128a);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x128a);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0218);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (512 * 192000): /* SPDIF 192.0kHz: M = 20, N = 5 */
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0c, 0x128a);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1b, 0x1083);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x00, 0x0218);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
	board_PllWrite(AUDIO_PLL_BASE_ADDRESS, 0x06, 0x3001);
	break;
	case (512 * 88200): /* SPDIF 88.2kHz */
	case (512 * 96000): /* SPDIF 96.0kHz */
	case (512 * 176400): /* SPDIF 176.4kHz */
	default:
		error_Set(ERR_AUDIO_CLOCK_NOT_SUPPORTED);
		LOG_ERROR2("audio clock not supported", value);
		return FALSE;
	}
	board_PllReset(AUDIO_PLL_BASE_ADDRESS, 0x01);
	board_PllReset(AUDIO_PLL_BASE_ADDRESS, 0x00);
	return TRUE;
}

int board_PixelClock(u16 baseAddr, u16 value, u8 cd)
{
	/* Check if we are working in a repeater setup */
	/* if so, PLL must be configure 1:1 ratio by repeater demo, and not hdmi tx demo */
	if (repeater_enabled == 0) {

		LOG_TRACE1(value);
		board_PllReset(VIDEO_PLL_BASE_ADDRESS, 0x01);
		switch (value) {
		case 2520: /* M = 14, N = 15 */
			/* default */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1555);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1597);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0118);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d4);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 2700: /* M = 1, N = 1 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1249);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1249);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0198);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 5040: /* M = 28, N = 15 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x138e);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x11c8);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0218);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 5400: /* M = 2, N = 1 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1249);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1105);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0198);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 5940: /* M = 11, N = 5 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1146);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1083);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0118);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 7200: /* M = 8, N = 3 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x130c);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1105);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0298);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 7425: /* M = 11, N = 4 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x12cb);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1104);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0298);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 10080: /* M = 56, N = 15 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x171c);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x11c8);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0218);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 10800: /* M = 4, N = 1 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1492);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1105);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0098);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d7);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 11880: /* M = 22, N = 5 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x12CB);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1083);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0218);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 14400: /* M = 16, N = 3 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1208);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1042);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0118);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 14850: /* M = 11, N = 2 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x12cb);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1082);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0298);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 21600: /* M = 8, N = 1 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1104);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1249);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0098);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d1);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 23760: /* M = 44, N = 5 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x1596);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1083);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0080);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0218);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21d2);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		case 29700: /* M = 22, N = 2 */
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0c, 0x12cb);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x0d, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1b, 0x1041);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x1c, 0x0000);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x00, 0x0298);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x01, 0x21df);
			board_PllWrite(VIDEO_PLL_BASE_ADDRESS, 0x06, 0x3001);
			break;
		default:
			error_Set(ERR_PIXEL_CLOCK_NOT_SUPPORTED);
			LOG_ERROR2("pixel clock not supported", value);
			return FALSE;
		}
		board_PllReset(VIDEO_PLL_BASE_ADDRESS, 0x01);
		board_PllReset(VIDEO_PLL_BASE_ADDRESS, 0x00);
	}

	board_PllReset(TCLK_PLL_BASE_ADDRESS, 0x01);
	if (((value * cd) / 8) < 7425)
	{ /* tclk N = 10, M = 10 */
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0c, 0x1145);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0d, 0x0000);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1b, 0x9145);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1c, 0x0002);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x00, 0x0118);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x01, 0x21D1);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x06, 0x3001);
	}
	else if (((value * cd) / 8) > 14850)
	{ /* tclk N = 3, M = 3 */
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0c, 0x1042);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0d, 0x0080);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1b, 0xF042);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1c, 0x0081);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x00, 0x0198);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x01, 0x21D1);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x06, 0x3001);
		board_PllReset(TCLK_PLL_BASE_ADDRESS, 0x01);
		board_PllReset(TCLK_PLL_BASE_ADDRESS, 0x00);
	}
	else
	{   /* tclk N = 7, M = 7 */
		//fase 90 M= 7, N= 7
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0c, 0x10c4);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x0d, 0x80  );
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1b, 0xd0c4);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x1c, 0x81  );
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x00, 0x0318);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x01, 0x21d1);
		board_PllWrite(TCLK_PLL_BASE_ADDRESS, 0x06, 0x3001);
	}
	board_PllReset(TCLK_PLL_BASE_ADDRESS, 0x01);
	board_PllReset(TCLK_PLL_BASE_ADDRESS, 0x00);

	//log_notice(" Video PLL lock = 0x%x", access_CoreReadByte(VIDEO_PLL_BASE_ADDRESS + 0x08) == 1);

	return TRUE;
}

void board_PllWrite(u16 pllBaseAddress, u8 regAddr, u16 data)
{
	access_CoreWriteByte(0x00, pllBaseAddress + DRP_CMD);
	access_CoreWriteByte(regAddr, pllBaseAddress + DRP_ADDR);
	access_CoreWriteByte((u8) (data >> 8), pllBaseAddress + DRP_DATAI_H);
	access_CoreWriteByte((u8) (data >> 0), pllBaseAddress + DRP_DATAI_L);
	access_CoreWriteByte(0x01, pllBaseAddress + DRP_CMD);
	access_CoreWriteByte(0x00, pllBaseAddress + DRP_CMD);
}

void board_VideoGeneratorBypass(u16 baseAddr, u8 enable)
{
	access_CoreWriteByte(enable ? 0x01 : 0x00, VIDEO_MUX_ADDRESS);
}

u16 board_PllRead(u16 pllBaseAddress, u8 regAddr)
{
	access_CoreWriteByte(0x00, pllBaseAddress + DRP_CMD); /* DRP_CMD 0x0 */
	access_CoreWriteByte(regAddr, pllBaseAddress + DRP_ADDR); /* DRP_ADDR REG_ADDR */
	access_CoreWriteByte(0x02, pllBaseAddress + DRP_CMD); /* DRP_CMD 0x2 */
	access_CoreWriteByte(0x00, pllBaseAddress + DRP_CMD); /* DRP_CMD 0x0 */
	return (access_CoreReadByte(pllBaseAddress + DRP_DATAO_H) << 8)
					| access_CoreReadByte(pllBaseAddress + DRP_DATAO_L);
}

void board_PllReset(u16 pllBaseAddress, u8 value)
{
	access_CoreWriteByte(value, pllBaseAddress + DRP_DCMRST); /* DRP_DCMRST 0x1 */
}

void board_AudioGeneratorBypass(u16 baseAddr, u8 enable)
{
	access_CoreWriteByte(enable ? 0x01 : 0x00, AUDIO_MUX_ADDRESS);
}

u32 board_SupportedRefreshRate(u8 code)
{
	LOG_TRACE1(code);
	/* 	the following values are specific to the compliance board
		Board No.:
		PLL Board No.:
	 */
	switch (code)
	{
	case 1:
		return 60000;
	case 2:
	case 3:
		return 59940;
	case 4:
	case 5:
		return 60000;
	case 6:
	case 7:
		return 59940;
#ifndef BASIC_VIDMODES
	case 8:
	case 9:
		return 60054;
	case 10:
	case 11:
		return 59940;
	case 12:
	case 13:
		return 60054;
	case 14:
	case 15:
		return 59940;
#endif
	case 16:
		return 60000;
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
#ifndef BASIC_VIDMODES
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
#endif
	case 31:
		return 50000;
	case 32:
		return 24000;
#ifndef BASIC_VIDMODES
	case 33:
		return 25000;
	case 34:
		return 30000;
	/*case 35:*/ /* not supported by the platform */
	/*case 36:*/ /* not supported by the platform */
	/*	return 59940;*/
	/*case 37:*/ /* not supported by the platform */
	/*case 38:*/ /* not supported by the platform */
	/*	return 50000;*/
	/*case 39:*/ /* not supported by the platform */
	/*	return 50000;*/
	case 40:
	case 41:
	case 42:
	case 43:
		return 100000;
	case 44:
	case 45:
		return 50000;
	case 46:
	case 47:
		return 120000;
	case 48:
	case 49:
	case 50:
	case 51:
		return 119880;
		break;
	/*case 52:*/ /* not supported by the platform */
	/*case 53:*/ /* not supported by the platform */
	/*case 54:*/ /* not supported by the platform */
	/*case 55:*/ /* not supported by the platform */
	/*	return 200000;*/
	/*case 56:*/ /* not supported by the platform */
	/*case 57:*/ /* not supported by the platform */
	/*case 58:*/ /* not supported by the platform */
	/*case 59:*/ /* not supported by the platform */
	/*	return 239760;*/
	case 60:
		return 24000;
	case 61:
		return 25000;
	case 62:
		return 30000;
	case 63:
		return 120000;
	case 64:
		return 100000;
#endif
	default:
		break;
	}
	return -1;
}
