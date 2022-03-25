#ifndef _GX3201_AUDIO_I2S_H_
#define _GX3201_AUDIO_I2S_H_

#define SPD_FIX_CHIP (CHIP_IS_GX6605S && (CHIP_SUB_ID == 1))
#define I2S_FIX_CHIP (CHIP_IS_GX6605S && (CHIP_SUB_ID == 1))
#define NO_MULTI_I2S (!I2S_FIX_CHIP)

#define I2S_PLAY_BASE_ADDR       (0x04a00000)
#define I2S_PLAY_SDC_BASE_ADDR   (0x04b00000)
#define I2S_PLAY_BASE_ADDR1      (0x04a00040)
#define I2S_PLAY_SDC_BASE_ADDR1  (0x04100000)

struct i2s_regs
{
	unsigned int audio_play_i2s_dacinfo             ;/*0*/
	unsigned int audio_play_i2s_indata              ;
	unsigned int audio_play_i2s_outdata             ;
	unsigned int audio_play_i2s_channelsel          ;
	unsigned int audio_play_i2s_cpusetpcm           ;
	unsigned int audio_play_i2s_fifoctrl            ;
	unsigned int audio_play_i2s_datainv             ;
	unsigned int audio_play_i2s_inten               ;
	unsigned int audio_play_i2s_int                 ;
	unsigned int audio_play_i2s_time_gate           ;
	unsigned int audio_play_i2s_playing_indata      ;/*0x28 */
	unsigned int audio_play_i2s_new_frame_ctrl    ;
};

struct i2s_sdc_regs
{
	unsigned int audio_play_i2s_buffer_start_addr        ;
	unsigned int audio_play_i2s_buffer_size              ;
	unsigned int audio_play_i2s_buffer_sdc_addr          ;
	unsigned int audio_play_i2s_newframe_start_addr      ;
	unsigned int audio_play_i2s_newframe_end_addr        ;
	unsigned int audio_play_i2s_playingframe_start_addr  ;
	unsigned int audio_play_i2s_playingframe_end_addr    ;
	unsigned int audio_play_i2s_newframe_pcmlen          ;
	unsigned int audio_play_i2s_playingframe_pcmlen      ;/* 0x20 */
	unsigned int audio_play_i2s_rev[3]  ;
	unsigned int audio_play_i2s_new_frame_ctrl	;/*0x30*/
};

/**************************GX3201 AUDIO PLAY SDC MODULE******************/
/* AUDIO_PLAY_I2S_NEWFRAME_PCMLEN register */
#define bSRL_NEWFRAME_PCMLEN             (0)
#define mSRL_NEWFRAME_PCMLEN             (0xFFFF << bSRL_NEWFRAME_PCMLEN)
#define vSRL_NEWFRAME_PCMLEN             (0xFFFF << bSRL_NEWFRAME_PCMLEN)

/* AUDIO_PLAY_I2S_PLAYING_PCMLEN register */
#define bSPD_PLAYINGFRAME_PCMLEN         (0)
#define mSPD_PLAYINGFRAME_PCMLEN         (0xFFFF << bSPD_PLAYINGFRAME_PCMLEN)
#define vSPD_PLAYINGFRAME_PCMLEN         (0xFFFF << bSPD_PLAYINGFRAME_PCMLEN)

#define bSRL_PLAYING_PCMLEN              (0)
#define mSRL_PLAYING_PCMLEN              (0xFFFF << bSRL_PLAYING_PCMLEN)
#define vSRL_PLAYING_PCMLEN              (0xFFFF << bSRL_PLAYING_PCMLEN)

#define bSRL_PLAYED_PCMLEN               (16)
#define mSRL_PLAYED_PCMLEN               (0xFFFF << bSRL_PLAYED_PCMLEN)
#define vSRL_PLAYED_PCMLEN               (0xFFFF << bSRL_PLAYED_PCMLEN)

/**************************GX3201 AUDIO I2S PLAY MODULE******************/
/* AUDIO_PLAY_I2S_DACINFO register */
#define bAUDIO_MIX_PCM_TO_I2S             (9)
#define mAUDIO_MIX_PCM_TO_I2S             (0x03 << bAUDIO_MIX_PCM_TO_I2S)
#define vAUDIO_MIX_PCM_TO_I2S             (0x03 << bAUDIO_MIX_PCM_TO_I2S)

#define bAUDIO_MIX_PCM_TO_SPD             (11)
#define mAUDIO_MIX_PCM_TO_SPD             (0x03 << bAUDIO_MIX_PCM_TO_SPD)
#define vAUDIO_MIX_PCM_TO_SPD             (0x03 << bAUDIO_MIX_PCM_TO_SPD)

#define bAUDIO_SERIAL_CLK_SEL             (4)
#define mAUDIO_SERIAL_CLK_SEL             (0x07 << bAUDIO_SERIAL_CLK_SEL)
#define vAUDIO_SERIAL_CLK_SEL             (0x07 << bAUDIO_SERIAL_CLK_SEL)

#define bAUDIO_SERIAL_BCK_SEL             (2)
#define mAUDIO_SERIAL_BCK_SEL             (0x03 << bAUDIO_SERIAL_BCK_SEL)
#define vAUDIO_SERIAL_BCK_SEL             (0x03 << bAUDIO_SERIAL_BCK_SEL)

#define bAUDIO_SERIAL_PCM_FORMAT          (0)
#define mAUDIO_SERIAL_PCM_FORMAT          (0x03 << bAUDIO_SERIAL_PCM_FORMAT)
#define vAUDIO_SERIAL_PCM_FORMAT          (0x03 << bAUDIO_SERIAL_PCM_FORMAT)

/* AUDIO_PLAY_I2S_INDATA  register */
#define bAUDIO_CLEAR_THE_SET_FRAME        (14)
#define mAUDIO_CLEAR_THE_SET_FRAME        (0x01 << bAUDIO_CLEAR_THE_SET_FRAME)
#define vAUDIO_CLEAR_THE_SET_FRAME        (0x01 << bAUDIO_CLEAR_THE_SET_FRAME)

#define bAUDIO_SAMPLE_DATA_BIT_SIZE       (8)
#define mAUDIO_SAMPLE_DATA_BIT_SIZE       (0x1F << bAUDIO_SAMPLE_DATA_BIT_SIZE)
#define vAUDIO_SAMPLE_DATA_BIT_SIZE       (0x1F << bAUDIO_SAMPLE_DATA_BIT_SIZE)

#define bAUDIO_PCM_DATA_ENDIAN            (5)
#define mAUDIO_PCM_DATA_ENDIAN            (0x01 << bAUDIO_PCM_DATA_ENDIAN)
#define vAUDIO_PCM_DATA_ENDIAN            (0x01 << bAUDIO_PCM_DATA_ENDIAN)

#define bAUDIO_PCM_STORE_MODE             (4)
#define mAUDIO_PCM_STORE_MODE             (0x01 << bAUDIO_PCM_STORE_MODE)
#define vAUDIO_PCM_STORE_MODE             (0x01 << bAUDIO_PCM_STORE_MODE)

#define bAUDIO_PCM_CHANNEL_NUMS           (0)
#define mAUDIO_PCM_CHANNEL_NUMS           (0x0F << bAUDIO_PCM_CHANNEL_NUMS)
#define vAUDIO_PCM_CHANNEL_NUMS           (0x0F << bAUDIO_PCM_CHANNEL_NUMS)

/* AUDIO_PLAY_I2S_OUTDATA  register */
#define bAUDIO_PCM_SOURCE_SEL             (6)
#define mAUDIO_PCM_SOURCE_SEL             (0x03 << bAUDIO_PCM_SOURCE_SEL)
#define vAUDIO_PCM_SOURCE_SEL             (0x03 << bAUDIO_PCM_SOURCE_SEL)

#define bAUDIO_SERIAL_MAGN                (4)
#define mAUDIO_SERIAL_MAGN                (0x03 << bAUDIO_SERIAL_MAGN)
#define vAUDIO_SERIAL_MAGN                (0x03 << bAUDIO_SERIAL_MAGN)

#define bAUDIO_PCM_DO_SRC_EN              (3)
#define mAUDIO_PCM_DO_SRC_EN              (0x01 << bAUDIO_PCM_DO_SRC_EN)
#define vAUDIO_PCM_DO_SRC_EN              (0x01 << bAUDIO_PCM_DO_SRC_EN)

#define bAUDIO_SERIAL_PCM_WORD_LENGTH     (1)
#define mAUDIO_SERIAL_PCM_WORD_LENGTH     (0x03 << bAUDIO_SERIAL_PCM_WORD_LENGTH)
#define vAUDIO_SERIAL_PCM_WORD_LENGTH     (0x03 << bAUDIO_SERIAL_PCM_WORD_LENGTH)

#define bAUDIO_SERIAL_SILENT_END          (0)
#define mAUDIO_SERIAL_SILENT_END          (0x01 << bAUDIO_SERIAL_SILENT_END)
#define vAUDIO_SERIAL_SILENT_END          (0x01 << bAUDIO_SERIAL_SILENT_END)

/* AUDIO_PLAY_I2S_CHANNELSEL register */
#define bAUDIO_CHANNLE_RBS_SEL            (28)
#define mAUDIO_CHANNLE_RBS_SEL            (0x0F << bAUDIO_CHANNLE_RBS_SEL)
#define vAUDIO_CHANNLE_RBS_SEL            (0x0F << bAUDIO_CHANNLE_RBS_SEL)

#define bAUDIO_CHANNLE_LBS_SEL            (24)
#define mAUDIO_CHANNLE_LBS_SEL            (0x0F << bAUDIO_CHANNLE_LBS_SEL)
#define vAUDIO_CHANNLE_LBS_SEL            (0x0F << bAUDIO_CHANNLE_LBS_SEL)

#define bAUDIO_CHANNLE_LFE_SEL            (20)
#define mAUDIO_CHANNLE_LFE_SEL            (0x0F << bAUDIO_CHANNLE_LFE_SEL)
#define vAUDIO_CHANNLE_LFE_SEL            (0x0F << bAUDIO_CHANNLE_LFE_SEL)

#define bAUDIO_CHANNLE_C_SEL              (16)
#define mAUDIO_CHANNLE_C_SEL              (0x0F << bAUDIO_CHANNLE_C_SEL)
#define vAUDIO_CHANNLE_C_SEL              (0x0F << bAUDIO_CHANNLE_C_SEL)

#define bAUDIO_CHANNLE_RS_SEL             (12)
#define mAUDIO_CHANNLE_RS_SEL             (0x0F << bAUDIO_CHANNLE_RS_SEL)
#define vAUDIO_CHANNLE_RS_SEL             (0x0F << bAUDIO_CHANNLE_RS_SEL)

#define bAUDIO_CHANNLE_LS_SEL             (8)
#define mAUDIO_CHANNLE_LS_SEL             (0x0F << bAUDIO_CHANNLE_LS_SEL)
#define vAUDIO_CHANNLE_LS_SEL             (0x0F << bAUDIO_CHANNLE_LS_SEL)

#define bAUDIO_CHANNLE_R_SEL              (4)
#define mAUDIO_CHANNLE_R_SEL              (0x0F << bAUDIO_CHANNLE_R_SEL)
#define vAUDIO_CHANNLE_R_SEL              (0x0F << bAUDIO_CHANNLE_R_SEL)

#define bAUDIO_CHANNLE_L_SEL              (0)
#define mAUDIO_CHANNLE_L_SEL              (0x0F << bAUDIO_CHANNLE_L_SEL)
#define vAUDIO_CHANNLE_L_SEL              (0x0F << bAUDIO_CHANNLE_L_SEL)

/* AUDIO_PLAY_I2S_CPUSETPCM register */
#define bAUDIO_CPU_SET_PCM                (0)
#define mAUDIO_CPU_SET_PCM                (0xFFFFFFFF << bAUDIO_CPU_SET_PCM)
#define vAUDIO_CPU_SET_PCM                (0xFFFFFFFF << bAUDIO_CPU_SET_PCM)

/* AUDIO_PLAY_I2S_FIFOCTRL register */
#define bAUDIO_CLEAR_FIFO_STATUS          (0)
#define mAUDIO_CLEAR_FIFO_STATUS          (0x01 << bAUDIO_CLEAR_FIFO_STATUS)
#define vAUDIO_CLEAR_FIFO_STATUS          (0x01 << bAUDIO_CLEAR_FIFO_STATUS)

#define bAUDIO_CLEAR_BUFFER_READ_POINT    (1)
#define mAUDIO_CLEAR_BUFFER_READ_POINT    (0x01 << bAUDIO_CLEAR_BUFFER_READ_POINT)
#define vAUDIO_CLEAR_BUFFER_READ_POINT    (0x01 << bAUDIO_CLEAR_BUFFER_READ_POINT)

#define bAUDIO_FORBIDEEN_READ_REQUEST     (2)
#define mAUDIO_FORBIDEEN_READ_REQUEST     (0x01 << bAUDIO_FORBIDEEN_READ_REQUEST)
#define vAUDIO_FORBIDEEN_READ_REQUEST     (0x01 << bAUDIO_FORBIDEEN_READ_REQUEST)

#define bAUDIO_PLAY_BACK_ENABLE           (3)
#define mAUDIO_PLAY_BACK_ENABLE           (0x01 << bAUDIO_PLAY_BACK_ENABLE)
#define vAUDIO_PLAY_BACK_ENABLE           (0x01 << bAUDIO_PLAY_BACK_ENABLE)

#define bAUDIO_I2S_MODULE_ENABLE          (4)
#define mAUDIO_I2S_MODULE_ENABLE          (0x01 << bAUDIO_I2S_MODULE_ENABLE)
#define vAUDIO_I2S_MODULE_ENABLE          (0x01 << bAUDIO_I2S_MODULE_ENABLE)

#define bAUDIO_SRL_EXPORT_DATA_TIMES      (5)
#define mAUDIO_SRL_EXPORT_DATA_TIMES      (0x07 << bAUDIO_SRL_EXPORT_DATA_TIMES)
#define vAUDIO_SRL_EXPORT_DATA_TIMES      (0x07 << bAUDIO_SRL_EXPORT_DATA_TIMES)

/* AUDIO_PLAY_I2S_INT register */  //luna
#define bAUDIO_INT_I2S_PCM_DONE           (0)
#define mAUDIO_INT_I2S_PCM_DONE           (0x01 << bAUDIO_INT_I2S_PCM_DONE)
#define vAUDIO_INT_I2S_PCM_DONE           (0x01 << bAUDIO_INT_I2S_PCM_DONE)

#define bAUDIO_INT_I2S_SET_NEWPCM         (1)
#define mAUDIO_INT_I2S_SET_NEWPCM         (0x01 << bAUDIO_INT_I2S_SET_NEWPCM)
#define vAUDIO_INT_I2S_SET_NEWPCM         (0x01 << bAUDIO_INT_I2S_SET_NEWPCM)

#define bAUDIO_INT_I2S_NEED_PCM           (2)
#define mAUDIO_INT_I2S_NEED_PCM           (0x01 << bAUDIO_INT_I2S_NEED_PCM)
#define vAUDIO_INT_I2S_NEED_PCM           (0x01 << bAUDIO_INT_I2S_NEED_PCM)

#define bAUDIO_INT_I2S_FIFO_EMPTY         (3)
#define mAUDIO_INT_I2S_FIFO_EMPTY         (0x01 << bAUDIO_INT_I2S_FIFO_EMPTY)
#define vAUDIO_INT_I2S_FIFO_EMPTY         (0x01 << bAUDIO_INT_I2S_FIFO_EMPTY)

#define bAUDIO_I2S_INT_STATUS             (0)
#define mAUDIO_I2S_INT_STATUS             (0x0F << bAUDIO_I2S_INT_STATUS)
#define vAUDIO_I2S_INT_STATUS             (0x0F << bAUDIO_I2S_INT_STATUS)

#define bAUDIO_I2S_INT_EN_VAL             (0)
#define mAUDIO_I2S_INT_EN_VAL             (0x0F << bAUDIO_I2S_INT_EN_VAL)
#define vAUDIO_I2S_INT_EN_VAL             (0x0F << bAUDIO_I2S_INT_EN_VAL)

/* AUDIO_PLAY_I2S_TIME_GATE register */
#define bAUDIO_PCM_DONE_GATE              (0)
#define mAUDIO_PCM_DONE_GATE              (0xFF << bAUDIO_PCM_DONE_GATE)
#define vAUDIO_PCM_DONE_GATE              (0xFF << bAUDIO_PCM_DONE_GATE)

/**************************GX3201 AUDIO PLAY SDC MODULE*************/
#define AUDIO_SET_I2S_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_play_i2s_buffer_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define AUDIO_SET_I2S_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_play_i2s_buffer_size, value)

#define AUDIO_GET_I2S_BUFFER_SIZE(rp) \
	GET_VAL(rp->audio_play_i2s_buffer_size)

#define AUDIO_GET_I2S_BUFFER_SDC_ADDR(rp) \
	GET_VAL(rp->audio_play_i2s_buffer_sdc_addr)

#define AUDIO_SET_I2S_NEWFRAME_START_ADDR(rp,value) \
	SET_VAL(rp->audio_play_i2s_newframe_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define AUDIO_SET_I2S_NEWFRAME_END_ADDR(rp,value) \
	SET_VAL(rp->audio_play_i2s_newframe_end_addr, (value & AUDIO_BUF_ADDR_MASK))

#define AUDIO_GET_I2S_NEWFRAME_END_ADDR(rp) \
	GET_VAL(rp->audio_play_i2s_newframe_end_addr)

#define AUDIO_SET_I2S_NEWFRAME_PCMLEN(rp,value) \
	SET_FEILD(rp->audio_play_i2s_newframe_pcmlen, mSRL_NEWFRAME_PCMLEN, ((value & 0xFFFF) << bSRL_NEWFRAME_PCMLEN))

#define AUDIO_GET_I2S_PLAYING_PCMLEN(rp) \
	GET_FEILD(rp->audio_play_i2s_playingframe_pcmlen, mSRL_PLAYING_PCMLEN, bSRL_PLAYING_PCMLEN)

/**************************GX3201 AUDIO I2S PLAY MODULE******************/
#define AUDIO_MIX_PCM_TO_I2S(rp,value) \
	SET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_MIX_PCM_TO_I2S, ((value & 0x03) << bAUDIO_MIX_PCM_TO_I2S))

#define AUDIO_MIX_PCM_TO_SPD(rp,value) \
	SET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_MIX_PCM_TO_SPD, ((value & 0x03) << bAUDIO_MIX_PCM_TO_SPD))

#define AUDIO_I2S_SET_CLK_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_SERIAL_CLK_SEL, ((value & 0x07) << bAUDIO_SERIAL_CLK_SEL))

#define AUDIO_GET_I2S_MODULE_CLK(rp)\
	GET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_SERIAL_CLK_SEL, bAUDIO_SERIAL_CLK_SEL)

#define AUDIO_I2S_SET_BCK_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_SERIAL_BCK_SEL, ((value & 0x03) << bAUDIO_SERIAL_BCK_SEL))

#define AUDIO_I2S_SET_PCM_FORMAT(rp,value) \
	SET_FEILD(rp->audio_play_i2s_dacinfo, mAUDIO_SERIAL_PCM_FORMAT, ((value & 0x03) << bAUDIO_SERIAL_PCM_FORMAT))

#define AUDIO_CHANNEL_RBS_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_RBS_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_RBS_SEL))

#define AUDIO_CHANNEL_LBS_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_LBS_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_LBS_SEL))

#define AUDIO_CHANNLE_LFE_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_LFE_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_LFE_SEL))

#define AUDIO_CHANNLE_C_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_C_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_C_SEL))

#define AUDIO_CHANNLE_RS_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_RS_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_RS_SEL))

#define AUDIO_CHANNLE_LS_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_LS_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_LS_SEL))

#define AUDIO_CHANNLE_R_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_R_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_R_SEL))

#define AUDIO_CHANNLE_L_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_channelsel, mAUDIO_CHANNLE_L_SEL, ((value & 0x0F) << bAUDIO_CHANNLE_L_SEL))

#define AUDIO_SET_SRL_CHANNLE_OUTPUT(rp,value)    \
	SET_VAL(rp->audio_play_i2s_channelsel, value)

#define AUDIO_GET_SRL_CHANNLE_OUTPUT(rp)    \
	GET_VAL(rp->audio_play_i2s_channelsel)

#define AUDIO_SAMPLE_DATA_LEN_SEL(rp,value) \
	SET_FEILD(rp->audio_play_i2s_indata, mAUDIO_SAMPLE_DATA_BIT_SIZE, ((value & 0x1F) << bAUDIO_SAMPLE_DATA_BIT_SIZE))

#define AUDIO_GET_SAMPLE_DATA_LEN(rp) \
	GET_FEILD(rp->audio_play_i2s_indata, mAUDIO_SAMPLE_DATA_BIT_SIZE, bAUDIO_SAMPLE_DATA_BIT_SIZE)

#define AUDIO_SAMPLE_DATA_BIG_ENDIAN(rp) \
	CLR_BIT(rp->audio_play_i2s_indata, bAUDIO_PCM_DATA_ENDIAN)

#define AUDIO_SAMPLE_DATA_LITTLE_ENDIAN(rp) \
	SET_BIT(rp->audio_play_i2s_indata, bAUDIO_PCM_DATA_ENDIAN)

#define AUDIO_SET_SAMPLE_DATA_ENDIAN(rp,value) \
	SET_FEILD(rp->audio_play_i2s_indata, (0x3) << bAUDIO_PCM_DATA_ENDIAN, ((value & 0x3) << bAUDIO_PCM_DATA_ENDIAN))

#define AUDIO_GET_SAMPLE_DATA_STORE_MODE(rp) \
	GET_BIT(rp->audio_play_i2s_indata, bAUDIO_PCM_STORE_MODE)

#define AUDIO_SAMPLE_DATA_STORE_INTERLACE(rp) \
	CLR_BIT(rp->audio_play_i2s_indata, bAUDIO_PCM_STORE_MODE)

#define AUDIO_SAMPLE_DATA_STORE_NON_INTERLACE(rp) \
	SET_BIT(rp->audio_play_i2s_indata, bAUDIO_PCM_STORE_MODE)

#define AUDIO_CONFIG_PCM_CHANNEL_NUMS(rp,value) \
	SET_FEILD(rp->audio_play_i2s_indata, mAUDIO_PCM_CHANNEL_NUMS, ((value & 0x0F) << bAUDIO_PCM_CHANNEL_NUMS))

/* AUDIO_PLAY_I2S_OUTDATA  register */
#define AUDIO_SET_PCM_MAGNIFICATION(rp,value)\
	SET_FEILD(rp->audio_play_i2s_outdata, mAUDIO_SERIAL_MAGN, ((value & 0x03) << bAUDIO_SERIAL_MAGN))

#define AUDIO_I2S_SET_PCM_WORD_LEN(rp,value)\
	SET_FEILD(rp->audio_play_i2s_outdata, mAUDIO_SERIAL_PCM_WORD_LENGTH, ((value & 0x03) << bAUDIO_SERIAL_PCM_WORD_LENGTH))

#define AUDIO_I2S_SET_SILENT(rp)        \
	SET_BIT(rp->audio_play_i2s_outdata, bAUDIO_SERIAL_SILENT_END)

#define AUDIO_I2S_SET_DO_SRC_EN(rp)        \
	SET_BIT(rp->audio_play_i2s_outdata, bAUDIO_PCM_DO_SRC_EN)

#define AUDIO_I2S_CLR_DO_SRC_EN(rp)        \
	CLR_BIT(rp->audio_play_i2s_outdata, bAUDIO_PCM_DO_SRC_EN)

#define AUDIO_I2S_SET_PCM_SOURCE_SEL(rp, value) \
	SET_FEILD(rp->audio_play_i2s_outdata, mAUDIO_PCM_SOURCE_SEL, ((value & 0x03) << bAUDIO_PCM_SOURCE_SEL))

/* AUDIO_PLAY_I2S_FIFOCTRL register */
#define AUDIO_CLEAR_FIFO_STATUS(rp) \
	SET_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_CLEAR_FIFO_STATUS)

#define AUDIO_CLEAR_FIFO_STATUS_CLR(rp) \
	CLR_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_CLEAR_FIFO_STATUS)

#define AUDIO_CLEAR_BUFFER_READ_POINT(rp) \
	SET_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_CLEAR_BUFFER_READ_POINT)

#define AUDIO_CLEAR_BUFFER_READ_POINT_CLR(rp) \
	CLR_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_CLEAR_BUFFER_READ_POINT)

#define AUDIO_FORBIDEEN_READ_REQUEST(rp) \
	SET_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_FORBIDEEN_READ_REQUEST)

#define AUDIO_REQUEST_TO_READ(rp)           \
	CLR_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_FORBIDEEN_READ_REQUEST)

#define AUDIO_PLAY_BACK_ENABLE(rp) \
	SET_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_PLAY_BACK_ENABLE)

#define AUDIO_PLAY_BACK_DISABLE(rp) \
	CLR_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_PLAY_BACK_ENABLE)

#define AUDIO_I2S_MODULE_ENABLE(rp) \
	SET_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_I2S_MODULE_ENABLE)

#define AUDIO_I2S_MODULE_DISABLE(rp) \
	CLR_BIT(rp->audio_play_i2s_fifoctrl, bAUDIO_I2S_MODULE_ENABLE)

#define AUDIO_SET_SRL_EXPORT_DATA_TIMES(rp,value) \
	SET_FEILD(rp->audio_play_i2s_fifoctrl, mAUDIO_SRL_EXPORT_DATA_TIMES, ((value & 0x07) << bAUDIO_SRL_EXPORT_DATA_TIMES))

/* AUDIO_PLAY_I2S_CPUSETPCM register */
#define AUDIO_I2S_CPU_SET_PCM(rp, value) \
	SET_VAL(rp->audio_play_i2s_cpusetpcm, value)

/* AUDIO_PLAY_I2S_INTEN register */
#define AUDIO_I2S_SET_NEWPCM_INT_DISABLE(rp)          \
	CLR_BIT(rp->audio_play_i2s_inten, bAUDIO_INT_I2S_SET_NEWPCM)

#define AUDIO_I2S_SET_NEWPCM_INT_ENABLE(rp)          \
	SET_BIT(rp->audio_play_i2s_inten, bAUDIO_INT_I2S_SET_NEWPCM)

#define AUDIO_I2S_GET_INT_EN_VAL(rp) \
	GET_FEILD(rp->audio_play_i2s_inten, mAUDIO_I2S_INT_EN_VAL, bAUDIO_I2S_INT_EN_VAL)

#define AUDIO_I2S_NEED_PCM_INT_CLR(rp)          \
	SET_VAL(rp->audio_play_i2s_int, vAUDIO_INT_I2S_NEED_PCM)

#define AUDIO_I2S_SET_NEWPCM_INT_CLR(rp)          \
	SET_VAL(rp->audio_play_i2s_int, vAUDIO_INT_I2S_SET_NEWPCM)

#define AUDIO_I2S_GET_NEWPCM_INT_STATUS(rp) \
	GET_FEILD(rp->audio_play_i2s_int, mAUDIO_INT_I2S_SET_NEWPCM, bAUDIO_INT_I2S_SET_NEWPCM)

#define AUDIO_I2S_GET_INT_STATUS(rp) \
	GET_FEILD(rp->audio_play_i2s_int, mAUDIO_I2S_INT_STATUS, bAUDIO_I2S_INT_STATUS)


int gxav_init_i2s_module (int id);
int gxav_uinit_i2s_module(int id);
int gxav_get_i2s_multi(void);
int gxav_set_i2s_dac_params(int id, struct audio_dac* dac);
int gxav_set_i2s_dac_mix(int id, int value);
int gxav_set_i2s_channel_mute(int id);
int gxav_get_i2s_clock(int id);
int gxav_set_i2s_channel_init(int id);
int gxav_set_i2s_channel_output(int id, int value);
int gxav_get_i2s_channel_output(int id);
int gxav_set_i2s_channel_nums (int id, int value);
int gxav_set_i2s_magnification(int id, int value);
int gxav_set_i2s_src_enable   (int id, int value);
int gxav_set_i2s_interlace    (int id, int value);
int gxav_set_i2s_endian       (int id, int value);
int gxav_init_i2s_isr      (int id);
int gxav_set_i2s_isr_enable(int id, int value);
int gxav_get_i2s_isr_status(int id);
int gxav_set_i2s_buffer(int id, unsigned int s_addr, int size);
int gxav_set_i2s_sample_data_len(int id, int value);
int gxav_set_i2s_load_data(int id, unsigned int s_addr, unsigned int e_addr, int len);
int gxav_set_i2s_new_frame(int id, int value);
int gxav_get_i2s_data_len(int id);
int gxav_clr_i2s_isr_status(int id);
int gxav_set_i2s_pcm_source_sel(int id, enum i2s_pcm_mode mode);
int gxav_set_i2s_cpu_set_pcm(int id, unsigned int value);
int gxav_set_i2s_work_enable(int id);

#endif
