#include "cec.h"
#include "../bsp/access.h"
#include "../util/log.h"
#include "../edid/edid.h"

#include "fifo.h"

static struct gx_cec {
	unsigned enable;
	unsigned device_type;
	unsigned logical_addr;
	unsigned physical_addr;

	gx_sem_id      int_sem;
	CecRet         int_ret;
	struct gxfifo  rx_fifo;
} cec;

#define CEC_BASE_ADDR    (0x7d00)
#define CEC_CTRL         (0x00)
#define CEC_MASK         (0x02)
#define CEC_ADDR_I       (0x05)
#define CEC_ADDR_H       (0x06)
#define CEC_TX_CNT       (0x07)
#define CEC_RX_CNT       (0x08)
#define CEC_TX_DATA0     (0x10)
#define CEC_TX_DATA1     (0x11)
#define CEC_TX_DATA2     (0x12)
#define CEC_TX_DATA3     (0x13)
#define CEC_TX_DATA4     (0x14)
#define CEC_RX_DATA0     (0x20)
#define CEC_RX_DATA1     (0x21)
#define CEC_RX_DATA2     (0x22)
#define CEC_RX_DATA3     (0x23)
#define CEC_RX_DATA4     (0x24)
#define CEC_LOCK         (0x30)
#define CEC_WAKEUPCTRL   (0x31)
#define CEC_INTER_STATUS (0x0106)
#define CEC_CLOCK_GATE   (0x4001)


#define TV_LOGICAL_ADDRESS            0x0
#define RECORD1_LOGICAL_ADDRESS       0x1
#define RECORD2_LOGICAL_ADDRESS       0x2
#define TUNER1_LOGICAL_ADDRESS        0x3
#define PLAYBACK1_LOGICAL_ADDRESS     0x4
#define AUDIOSYSTEM_LOGICAL_ADDRESS   0x5
#define TUNER2_LOGICAL_ADDRESS        0x6
#define TUNER3_LOGICAL_ADDRESS        0x7
#define PLAYBACK2_LOGICAL_ADDRESS     0x8
#define RECORD3_LOGICAL_ADDRESS       0x9
#define TUNER4_LOGICAL_ADDRESS        0xA
#define PLAYBACK3_LOGICAL_ADDRESS     0xB
#define RESERVER1_LOGICAL_ADDRESS     0xC
#define RESERVER2_LOGICAL_ADDRESS     0xD
#define FREEUSE_LOGICAL_ADDRESS       0xE
#define UNREGISTERED_LOGICAL_ADDRESS  0xF

//CEC device types
#define HDMI_CEC_TV           0x00
#define HDMI_CEC_RECORDING    0x01
#define HDMI_CEC_TUNER        0x03
#define HDMI_CEC_PLAYBACK     0x04
#define HDMI_CEC_AUDIOSYSTEM  0x05

#define CEC_FLAG_DONE            (1<<0)
#define CEC_FLAG_EOM             (1<<1)
#define CEC_FLAG_NACK            (1<<2)
#define CEC_FLAG_ARB_LOST        (1<<3)
#define CEC_FLAG_ERROR_INITIATOR (1<<4)
#define CEC_FLAG_ERROR_FLOW      (1<<5)
#define CEC_FLAG_WAKEUP          (1<<6)

#define CEC_TIMEOUT_VALUE  ((unsigned)0x001FFFFF)

// HDMI CEC specific commands
#define CEC_OPCODE_ACTIVE_SOURCE                  (0x82)
#define CEC_OPCODE_IMAGE_VIEW_ON                  (0x04)
#define CEC_OPCODE_TEXT_VIEW_ON                   (0x0D)
#define CEC_OPCODE_INACTIVE_SOURCE                (0x9D)
#define CEC_OPCODE_REQUEST_ACTIVE_SOURCE          (0x85)
#define CEC_OPCODE_ROUTING_CHANGE                 (0x80)
#define CEC_OPCODE_ROUTING_INFORMATION            (0x81)
#define CEC_OPCODE_SET_STREAM_PATH                (0x86)
#define CEC_OPCODE_STANDBY                        (0x36)
#define CEC_OPCODE_RECORD_OFF                     (0x0B)
#define CEC_OPCODE_RECORD_ON                      (0x09)
#define CEC_OPCODE_RECORD_STATUS                  (0x0A)
#define CEC_OPCODE_RECORD_TV_SCREEN               (0x0F)
#define CEC_OPCODE_CLEAR_ANALOGUE_TIMER           (0x33)
#define CEC_OPCODE_CLEAR_DIGITAL_TIMER            (0x99)
#define CEC_OPCODE_CLEAR_EXTERNAL_TIMER           (0xA1)
#define CEC_OPCODE_SET_ANALOGUE_TIMER             (0x34)
#define CEC_OPCODE_SET_DIGITAL_TIMER              (0x97)
#define CEC_OPCODE_SET_EXTERNAL_TIMER             (0xA2)
#define CEC_OPCODE_SET_TIMER_PROGRAM_TITLE        (0x67)
#define CEC_OPCODE_TIMER_CLEARED_STATUS           (0x43)
#define CEC_OPCODE_TIMER_STATUS                   (0x35)
#define CEC_OPCODE_CEC_VERSION                    (0x9E)
#define CEC_OPCODE_GET_CEC_VERSION                (0x9F)
#define CEC_OPCODE_GIVE_PHYSICAL_ADDRESS          (0x83)
#define CEC_OPCODE_GET_MENU_LANGUAGE              (0x91)
#define CEC_OPCODE_REPORT_PHYSICAL_ADDRESS        (0x84)
#define CEC_OPCODE_SET_MENU_LANGUAGE              (0x32)
#define CEC_OPCODE_DECK_CONTROL                   (0x42)
#define CEC_OPCODE_DECK_STATUS                    (0x1B)
#define CEC_OPCODE_GIVE_DECK_STATUS               (0x1A)
#define CEC_OPCODE_PLAY                           (0x41)
#define CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS       (0x08)
#define CEC_OPCODE_SELECT_ANALOGUE_SERVICE        (0x92)
#define CEC_OPCODE_SELECT_DIGITAL_SERVICE         (0x93)
#define CEC_OPCODE_TUNER_DEVICE_STATUS            (0x07)
#define CEC_OPCODE_TUNER_STEP_DECREMENT           (0x06)
#define CEC_OPCODE_TUNER_STEP_INCREMENT           (0x05)
#define CEC_OPCODE_DEVICE_VENDOR_ID               (0x87)
#define CEC_OPCODE_GIVE_DEVICE_VENDOR_ID          (0x8C)
#define CEC_OPCODE_VENDOR_COMMAND                 (0x89)
#define CEC_OPCODE_VENDOR_COMMAND_WITH_ID         (0xA0)
#define CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN      (0x8A)
#define CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP        (0x8B)
#define CEC_OPCODE_SET_OSD_STRING                 (0x64)
#define CEC_OPCODE_GIVE_OSD_NAME                  (0x46)
#define CEC_OPCODE_SET_OSD_NAME                   (0x47)
#define CEC_OPCODE_MENU_REQUEST                   (0x8D)
#define CEC_OPCODE_MENU_STATUS                    (0x8E)
#define CEC_OPCODE_USER_CONTROL_PRESSED           (0x44)
#define CEC_OPCODE_USER_CONTROL_RELEASED          (0x45)
#define CEC_OPCODE_GIVE_DEVICE_POWER_STATUS       (0x8F)
#define CEC_OPCODE_REPORT_POWER_STATUS            (0x90)
#define CEC_OPCODE_FEATURE_ABORT                  (0x00)
#define CEC_OPCODE_ABORT                          (0xFF)
#define CEC_OPCODE_GIVE_AUDIO_STATUS              (0x71)
#define CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS  (0x7D)
#define CEC_OPCODE_REPORT_AUDIO_STATUS            (0x7A)
#define CEC_OPCODE_SET_SYSTEM_AUDIO_MODE          (0x72)
#define CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST      (0x70)
#define CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS       (0x7E)
#define CEC_OPCODE_SET_AUDIO_RATE                 (0x9A)

static struct {
	char     *str_code;
	unsigned code;
} code_map[] = {
	{.str_code = "CEC_OPCODE_ACTIVE_SOURCE                ", .code = 0x82},
	{.str_code = "CEC_OPCODE_IMAGE_VIEW_ON                ", .code = 0x04},
	{.str_code = "CEC_OPCODE_TEXT_VIEW_ON                 ", .code = 0x0D},
	{.str_code = "CEC_OPCODE_INACTIVE_SOURCE              ", .code = 0x9D},
	{.str_code = "CEC_OPCODE_REQUEST_ACTIVE_SOURCE        ", .code = 0x85},
	{.str_code = "CEC_OPCODE_ROUTING_CHANGE               ", .code = 0x80},
	{.str_code = "CEC_OPCODE_ROUTING_INFORMATION          ", .code = 0x81},
	{.str_code = "CEC_OPCODE_SET_STREAM_PATH              ", .code = 0x86},
	{.str_code = "CEC_OPCODE_STANDBY                      ", .code = 0x36},
	{.str_code = "CEC_OPCODE_RECORD_OFF                   ", .code = 0x0B},
	{.str_code = "CEC_OPCODE_RECORD_ON                    ", .code = 0x09},
	{.str_code = "CEC_OPCODE_RECORD_STATUS                ", .code = 0x0A},
	{.str_code = "CEC_OPCODE_RECORD_TV_SCREEN             ", .code = 0x0F},
	{.str_code = "CEC_OPCODE_CLEAR_ANALOGUE_TIMER         ", .code = 0x33},
	{.str_code = "CEC_OPCODE_CLEAR_DIGITAL_TIMER          ", .code = 0x99},
	{.str_code = "CEC_OPCODE_CLEAR_EXTERNAL_TIMER         ", .code = 0xA1},
	{.str_code = "CEC_OPCODE_SET_ANALOGUE_TIMER           ", .code = 0x34},
	{.str_code = "CEC_OPCODE_SET_DIGITAL_TIMER            ", .code = 0x97},
	{.str_code = "CEC_OPCODE_SET_EXTERNAL_TIMER           ", .code = 0xA2},
	{.str_code = "CEC_OPCODE_SET_TIMER_PROGRAM_TITLE      ", .code = 0x67},
	{.str_code = "CEC_OPCODE_TIMER_CLEARED_STATUS         ", .code = 0x43},
	{.str_code = "CEC_OPCODE_TIMER_STATUS                 ", .code = 0x35},
	{.str_code = "CEC_OPCODE_CEC_VERSION                  ", .code = 0x9E},
	{.str_code = "CEC_OPCODE_GET_CEC_VERSION              ", .code = 0x9F},
	{.str_code = "CEC_OPCODE_GIVE_PHYSICAL_ADDRESS        ", .code = 0x83},
	{.str_code = "CEC_OPCODE_GET_MENU_LANGUAGE            ", .code = 0x91},
	{.str_code = "CEC_OPCODE_REPORT_PHYSICAL_ADDRESS      ", .code = 0x84},
	{.str_code = "CEC_OPCODE_SET_MENU_LANGUAGE            ", .code = 0x32},
	{.str_code = "CEC_OPCODE_DECK_CONTROL                 ", .code = 0x42},
	{.str_code = "CEC_OPCODE_DECK_STATUS                  ", .code = 0x1B},
	{.str_code = "CEC_OPCODE_GIVE_DECK_STATUS             ", .code = 0x1A},
	{.str_code = "CEC_OPCODE_PLAY                         ", .code = 0x41},
	{.str_code = "CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS     ", .code = 0x08},
	{.str_code = "CEC_OPCODE_SELECT_ANALOGUE_SERVICE      ", .code = 0x92},
	{.str_code = "CEC_OPCODE_SELECT_DIGITAL_SERVICE       ", .code = 0x93},
	{.str_code = "CEC_OPCODE_TUNER_DEVICE_STATUS          ", .code = 0x07},
	{.str_code = "CEC_OPCODE_TUNER_STEP_DECREMENT         ", .code = 0x06},
	{.str_code = "CEC_OPCODE_TUNER_STEP_INCREMENT         ", .code = 0x05},
	{.str_code = "CEC_OPCODE_DEVICE_VENDOR_ID             ", .code = 0x87},
	{.str_code = "CEC_OPCODE_GIVE_DEVICE_VENDOR_ID        ", .code = 0x8C},
	{.str_code = "CEC_OPCODE_VENDOR_COMMAND               ", .code = 0x89},
	{.str_code = "CEC_OPCODE_VENDOR_COMMAND_WITH_ID       ", .code = 0xA0},
	{.str_code = "CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN    ", .code = 0x8A},
	{.str_code = "CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP      ", .code = 0x8B},
	{.str_code = "CEC_OPCODE_SET_OSD_STRING               ", .code = 0x64},
	{.str_code = "CEC_OPCODE_GIVE_OSD_NAME                ", .code = 0x46},
	{.str_code = "CEC_OPCODE_SET_OSD_NAME                 ", .code = 0x47},
	{.str_code = "CEC_OPCODE_MENU_REQUEST                 ", .code = 0x8D},
	{.str_code = "CEC_OPCODE_MENU_STATUS                  ", .code = 0x8E},
	{.str_code = "CEC_OPCODE_USER_CONTROL_PRESSED         ", .code = 0x44},
	{.str_code = "CEC_OPCODE_USER_CONTROL_RELEASED        ", .code = 0x45},
	{.str_code = "CEC_OPCODE_GIVE_DEVICE_POWER_STATUS     ", .code = 0x8F},
	{.str_code = "CEC_OPCODE_REPORT_POWER_STATUS          ", .code = 0x90},
	{.str_code = "CEC_OPCODE_FEATURE_ABORT                ", .code = 0x00},
	{.str_code = "CEC_OPCODE_ABORT                        ", .code = 0xFF},
	{.str_code = "CEC_OPCODE_GIVE_AUDIO_STATUS            ", .code = 0x71},
	{.str_code = "CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS", .code = 0x7D},
	{.str_code = "CEC_OPCODE_REPORT_AUDIO_STATUS          ", .code = 0x7A},
	{.str_code = "CEC_OPCODE_SET_SYSTEM_AUDIO_MODE        ", .code = 0x72},
	{.str_code = "CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST    ", .code = 0x70},
	{.str_code = "CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS     ", .code = 0x7E},
	{.str_code = "CEC_OPCODE_SET_AUDIO_RATE               ", .code = 0x9A},
};

static char* code_to_str(unsigned code)
{
	int i;
	char *ret = "";

	for (i = 0; i < sizeof(code_map)/sizeof(code_map[0]); i++) {
		if (code_map[i].code == code) {
			ret = code_map[i].str_code;
			break;
		}
	}

	return ret;
}

typedef enum {
	BIT_RESET = 0,
	BIT_SET   = !BIT_RESET
} FlagStatus;

static void cec_ownaddr_clear(void)
{
	access_Write(0x00, CEC_BASE_ADDR + CEC_ADDR_I);
	access_Write(0x80, CEC_BASE_ADDR + CEC_ADDR_H);
}

static void cec_ownaddr_config(unsigned addr)
{
	switch (addr) {
		case TUNER1_LOGICAL_ADDRESS:
		case TUNER2_LOGICAL_ADDRESS:
		case TUNER3_LOGICAL_ADDRESS:
			access_Write(1<<addr, CEC_BASE_ADDR + CEC_ADDR_I);
			access_Write(0,       CEC_BASE_ADDR + CEC_ADDR_H);
			break;
		case TUNER4_LOGICAL_ADDRESS:
			access_Write(0, CEC_BASE_ADDR + CEC_ADDR_I);
			access_Write(4, CEC_BASE_ADDR + CEC_ADDR_H);
			break;
		default:
			break;
	}
}

void print_cec_regs(void)
{
	return;
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_CTRL,     access_Read(CEC_BASE_ADDR+CEC_CTRL));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_MASK,     access_Read(CEC_BASE_ADDR+CEC_MASK));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_CNT,   access_Read(CEC_BASE_ADDR+CEC_TX_CNT));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_DATA0, access_Read(CEC_BASE_ADDR+CEC_TX_DATA0));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_DATA1, access_Read(CEC_BASE_ADDR+CEC_TX_DATA1));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_DATA2, access_Read(CEC_BASE_ADDR+CEC_TX_DATA2));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_DATA3, access_Read(CEC_BASE_ADDR+CEC_TX_DATA3));

	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_ADDR_I, access_Read(CEC_BASE_ADDR+CEC_ADDR_I));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_ADDR_H, access_Read(CEC_BASE_ADDR+CEC_ADDR_H));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_RX_CNT,   access_Read(CEC_BASE_ADDR+CEC_RX_CNT));

	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_LOCK, access_Read(CEC_BASE_ADDR+CEC_LOCK));
	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_INTER_STATUS, access_Read(CEC_INTER_STATUS));

	gx_printf("addr = 0x%x, val = 0x%x\n", CEC_CLOCK_GATE, access_Read(CEC_CLOCK_GATE));
}

static void cec_ping_msg(unsigned addr)
{
	access_Write(0x01, CEC_BASE_ADDR + CEC_TX_CNT);
	access_Write(addr, CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(0x03, CEC_BASE_ADDR + CEC_CTRL);
}

static void cec_report_physical_addr(void)
{
	access_Write(0x05,                               CEC_BASE_ADDR + CEC_TX_CNT  );
	access_Write(((cec.logical_addr<<4)|0xF),        CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_REPORT_PHYSICAL_ADDRESS, CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write((cec.physical_addr>>8)&0xFF,        CEC_BASE_ADDR + CEC_TX_DATA2);
	access_Write((cec.physical_addr>>0)&0xFF,        CEC_BASE_ADDR + CEC_TX_DATA3);
	access_Write(cec.device_type,                    CEC_BASE_ADDR + CEC_TX_DATA4);
	access_Write(0x03,                               CEC_BASE_ADDR + CEC_CTRL    );
}

static void cec_boradcast_device_vendorid(void)
{
	access_Write(0x05,                        CEC_BASE_ADDR + CEC_TX_CNT  );
	access_Write((cec.logical_addr<<4)|0xF,   CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_DEVICE_VENDOR_ID, CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write(0x00,                        CEC_BASE_ADDR + CEC_TX_DATA2); /* IEEE RAC */
	access_Write(0x00,                        CEC_BASE_ADDR + CEC_TX_DATA3); /* IEEE RAC */
	access_Write(0x00,                        CEC_BASE_ADDR + CEC_TX_DATA4); /* IEEE RAC */
	access_Write(0x03,                        CEC_BASE_ADDR + CEC_CTRL    );
}

static void cec_opt_active_source(void)
{
	/* Image View On */
	access_Write( 0x02,                     CEC_BASE_ADDR + CEC_TX_CNT  );
	access_Write((cec.logical_addr<<4)|0xf, CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_IMAGE_VIEW_ON,  CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write(0x03,                      CEC_BASE_ADDR + CEC_CTRL    );
#if 0
	gx_mdelay(20); /* delay > 7*2.4ms */
	/* Active Source */
	access_Write(0x04,                        CEC_BASE_ADDR + CEC_TX_CNT  );
	access_Write((cec.logical_addr<<4)|0xF,   CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_ACTIVE_SOURCE,    CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write((cec.physical_addr>>8)&0xFF, CEC_BASE_ADDR + CEC_TX_DATA2);
	access_Write((cec.physical_addr>>0)&0xFF, CEC_BASE_ADDR + CEC_TX_DATA3);
	access_Write(0x03,                        CEC_BASE_ADDR + CEC_CTRL    );
#endif
}

static void cec_opt_get_power_status(void)
{
	access_Write(0x00,                                CEC_BASE_ADDR + CEC_LOCK    );
	access_Write(0x02,                                CEC_BASE_ADDR + CEC_TX_CNT  );
	access_Write((cec.logical_addr<<4)|0x0,           CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_GIVE_DEVICE_POWER_STATUS, CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write(0x03,                                CEC_BASE_ADDR + CEC_CTRL    );
}

static void cec_opt_inactive_source(void)
{
	access_Write(0x04, CEC_BASE_ADDR + CEC_TX_CNT);
	access_Write((cec.logical_addr<<4)|0x0,   CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_INACTIVE_SOURCE,  CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write((cec.physical_addr>>8)&0xFF, CEC_BASE_ADDR + CEC_TX_DATA2);
	access_Write((cec.physical_addr>>0)&0xFF, CEC_BASE_ADDR + CEC_TX_DATA3);
	access_Write(0x03,                        CEC_BASE_ADDR + CEC_CTRL    );
}

static void cec_opt_standby(void)
{
	access_Write(0x02, CEC_BASE_ADDR + CEC_TX_CNT);
	gx_printf("** addr = 0x%x, val = 0x%x\n", CEC_BASE_ADDR+CEC_TX_CNT,   access_Read(CEC_BASE_ADDR+CEC_TX_CNT));
	access_Write((cec.logical_addr<<4)|0xf, CEC_BASE_ADDR + CEC_TX_DATA0);
	access_Write(CEC_OPCODE_STANDBY,        CEC_BASE_ADDR + CEC_TX_DATA1);
	access_Write(0x03,                      CEC_BASE_ADDR + CEC_CTRL    );
}

static CecRet cec_get_physical_addr(void)
{
	cec.physical_addr = edid_GetPhysicalAddr();

	return RET_CEC_OK;
}

void cec_clear_flag(unsigned flag)
{
	access_Write(flag&0xFF, CEC_BASE_ADDR + CEC_INTER_STATUS);
}

static FlagStatus cec_get_flag_status(unsigned flag)
{
	FlagStatus s = BIT_RESET;

	if ((access_Read(CEC_INTER_STATUS) & flag) != BIT_RESET)
		s = BIT_SET;
	else
		s = BIT_RESET;

	return s;
}

static CecRet cec_get_status(void)
{
	unsigned int_timeout_ms = 100;
	CecRet ret = RET_CEC_OK;

#if 1
	if (0 != gx_sem_wait_timeout(&cec.int_sem, int_timeout_ms)) {
		LOG_DEBUG(" --- %s: wait status timeout --- ");
		ret = RET_CEC_TIMEOUT;
	} else {
		LOG_DEBUG(" --- %s: get int --- ");
		ret = cec.int_ret;
	}
#else
	if (cec_get_flag_status(CEC_FLAG_WAKEUP) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: WAKEUP --- ");
		ret = RET_CEC_WAKEUP;
		cec_clear_flag(CEC_FLAG_WAKEUP);
	}
	if (cec_get_flag_status(CEC_FLAG_ERROR_FLOW) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ERROR_FLOW --- ");
		ret = RET_CEC_ERROR_FLOW;
		cec_clear_flag(CEC_FLAG_ERROR_FLOW);
	}
	if (cec_get_flag_status(CEC_FLAG_ERROR_INITIATOR) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ERROR_INITIATOR --- ");
		ret = RET_CEC_ERROR_INITIATOR;
		cec_clear_flag(CEC_FLAG_ERROR_INITIATOR);
	}
	if (cec_get_flag_status(CEC_FLAG_ARB_LOST) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ARB LOST --- ");
		ret = RET_CEC_ARB_LOST;
		cec_clear_flag(CEC_FLAG_ARB_LOST);
	}
	if (cec_get_flag_status(CEC_FLAG_NACK) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: NACK --- ");
		ret = RET_CEC_NACK;
		cec_clear_flag(CEC_FLAG_NACK);
	}
	if (cec_get_flag_status(CEC_FLAG_EOM) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: EOM --- ");
		ret = RET_CEC_EOM;
		cec_clear_flag(CEC_FLAG_EOM);
	}
	if (cec_get_flag_status(CEC_FLAG_DONE) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: DONE --- ");
		ret = RET_CEC_DONE;
		cec_clear_flag(CEC_FLAG_DONE);
	}
#endif

	return ret;
}

static CecRet cec_alloc_logical_addr(void)
{
	int i;
	unsigned addrs[] = {
		TUNER1_LOGICAL_ADDRESS,
		TUNER2_LOGICAL_ADDRESS,
		TUNER3_LOGICAL_ADDRESS,
		TUNER4_LOGICAL_ADDRESS,
	};
	CecRet ret = RET_CEC_OK;

	for (i = 0; i < sizeof(addrs)/sizeof(addrs[0]); i++) {
		cec_ownaddr_clear();
		cec_ownaddr_config(addrs[i]);
		cec_ping_msg((addrs[i]<<4)|addrs[i]);
		if (cec_get_status() == RET_CEC_NACK) {
			cec.logical_addr = addrs[i];
			ret = RET_CEC_OK;
			break;
		}
	}
	/*no avaliable addr */
	if (i == sizeof(addrs)/sizeof(addrs[0])) {
		cec.logical_addr = 0x0F;
		ret = RET_CEC_DEVICE_UNREGISTRED;
	}

	return ret;
}

static CecRet cec_check_connect_status(void)
{
	unsigned cnt = 0;
	CecRet ret = RET_CEC_OK;

	LOG_DEBUG(" --- cec check connet status start--- ");
	cec_ping_msg((cec.logical_addr<<4)|cec.logical_addr);
	while (cnt < CEC_TIMEOUT_VALUE) {
		ret = cec_get_status();
		if (ret == RET_CEC_NACK || ret == RET_CEC_DONE) {
			LOG_DEBUG(" --- cec check connet status :success! --- ");
			ret = RET_CEC_OK;
			break;
		}
		cnt++;
	}

	if (cnt == CEC_TIMEOUT_VALUE)
		ret = RET_CEC_TIMEOUT;

	return ret;
}

CecRet cec_enable(int enable)
{
	unsigned cnt = 0;
	CecRet ret = RET_CEC_OK;

	if (enable) {
		LOG_DEBUG(" --- cec enable = 1--- ");
		gx_sem_create(&cec.int_sem, 0);

		gxfifo_init(&cec.rx_fifo, NULL, 1024);
		/* enable clock-gate */
		access_Write((access_Read(CEC_CLOCK_GATE)&(~(1<<5))), CEC_CLOCK_GATE);
		/* enable interrupt */
		access_Write(0x00, CEC_BASE_ADDR + CEC_MASK);

		cec.device_type = HDMI_CEC_TUNER;
		cec_get_physical_addr();

		/* alloc logical addr*/
		cnt = 0;
		do {
			cnt ++;
			ret = cec_alloc_logical_addr();
		} while(ret != RET_CEC_OK && cnt < 5);
		if (ret != RET_CEC_OK) {
			LOG_DEBUG(" --- cec alloc logical-addr fail --- ");
			goto out;
		}

		ret = cec_check_connect_status();
		if (ret == RET_CEC_OK) {
			cec_ownaddr_config(cec.logical_addr);
			cec_report_physical_addr();
			cec_boradcast_device_vendorid();
		} else {
			LOG_DEBUG(" --- cec check connect status: fail --- ");
		}
		LOG_DEBUG(" --- cec enable success --- ");
	} else {
		LOG_DEBUG(" --- cec enable = 0--- ");
		/* disable clock-gate */
		access_Write((access_Read(CEC_CLOCK_GATE)|(1<<5)), CEC_CLOCK_GATE);
		/* disalbe interrupt */
		access_Write(0x7f, CEC_BASE_ADDR + CEC_MASK);
	}

out:
	return ret;
}

CecRet cec_send_cmd(GxHdmiCecOpcode code)
{
	CecRet ret = RET_CEC_OK;

	switch(code) {
	case CEC_OP_IMAGE_VIEW_ON:
		LOG_DEBUG(" --- cec cmd: image-view-on --- ");
		cec_opt_active_source();
		break;
#if 0
	case CEC_CMD_GET_POWER_STATUS:
		LOG_DEBUG(" --- cec cmd: get-power-status --- ");
		cec_opt_get_power_status();
		break;
	case CEC_CMD_INACTIVE_SOURCE:
		LOG_DEBUG(" --- cec cmd: inactive-source --- ");
		cec_opt_inactive_source();
		break;
#endif
	case CEC_OP_STANDBY:
		LOG_DEBUG(" --- cec cmd: standby --- ");
		cec_opt_standby();
		break;
	default:
		break;
	}

	//print_cec_regs();
	return ret;
}

CecRet cec_recv_cmd(GxHdmiCecOpcode *code, unsigned timeout_ms)
{
	CecRet ret = RET_CEC_TIMEOUT;
	unsigned fifo_len = gxfifo_len(&cec.rx_fifo);

	if (fifo_len) {
		ret = RET_CEC_OK;
		gxfifo_get(&cec.rx_fifo, code, sizeof(GxHdmiCecOpcode));
	}

	return ret;
}

CecRet cec_parse_state(unsigned state)
{
	int ret = -1;
	int code;

	if ((state&CEC_FLAG_WAKEUP) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: WAKEUP --- ");
		ret = RET_CEC_WAKEUP;
	}
	if ((state&CEC_FLAG_ERROR_FLOW) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ERROR_FLOW --- ");
		ret = RET_CEC_ERROR_FLOW;
	}
	if ((state&CEC_FLAG_ERROR_INITIATOR) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ERROR_INITIATOR --- ");
		ret = RET_CEC_ERROR_INITIATOR;
	}
	if ((state&CEC_FLAG_ARB_LOST) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: ARB LOST --- ");
		ret = RET_CEC_ARB_LOST;
	}
	if ((state&CEC_FLAG_NACK) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: NACK --- ");
		ret = RET_CEC_NACK;
	}
	if ((state&CEC_FLAG_EOM) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: EOM --- ");
		ret = RET_CEC_EOM;
		access_Write(0x00, CEC_BASE_ADDR + CEC_LOCK);
		code = access_Read(CEC_BASE_ADDR + CEC_RX_DATA1);
		gx_printf("\n### get cmd: %s\n", code_to_str(code&0xff));
		gxfifo_put(&cec.rx_fifo, &code, sizeof(GxHdmiCecOpcode));
	}
	if ((state&CEC_FLAG_DONE) != BIT_RESET) {
		LOG_DEBUG(" --- cec int: DONE --- ");
		ret = RET_CEC_DONE;
	}

	if (ret != -1) {
		cec.int_ret = ret;
		gx_sem_post(&cec.int_sem);
	}

	return ret;
}

#if 1
void test_cec_enable(int enable)
{
	cec_enable(enable);
}

void test_cec_cmd(int cmd)
{
	cec_send_cmd(cmd);
}
#endif
