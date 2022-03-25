#ifndef __GX32XX_AREG_H__
#define __GX32XX_AREG_H__

struct audiodec_regs
{
	unsigned int audio_mcu_rev0[64]                 ;
	unsigned int audio_mcu_buf_ctrl_addr            ;/* 0x100 */
	unsigned int audio_mcu_eear                     ;/* 0x104 */
	unsigned int audio_mcu_epcr                     ;
	unsigned int audio_mcu_expc                     ;
	unsigned int audio_mcu_apts                     ;
	unsigned int audio_mcu_inten                    ;
	unsigned int audio_mcu_int                      ;
	unsigned int audio_mcu_baseaddr                 ;
	unsigned int audio_mcu_decode_ctrl              ;
	unsigned int audio_mcu_rev1[7]                  ;
	unsigned int audio_mcu_esa1_buffer_start_addr   ;/* 0x140 */
	unsigned int audio_mcu_esa1_buffer_size         ;
	unsigned int audio_mcu_esa2_buffer_start_addr   ;
	unsigned int audio_mcu_esa2_buffer_size         ;
	unsigned int audio_mcu_pcm_buffer_start_addr    ;
	unsigned int audio_mcu_pcm_buffer_size          ;
	unsigned int audio_mcu_esa1_frame_saddr         ;
	unsigned int audio_mcu_esa1_w_addr              ;
	unsigned int audio_mcu_esa2_frame_saddr         ;
	unsigned int audio_mcu_esa2_w_addr              ;
	unsigned int audio_mcu_pcm_w_saddr              ;
	unsigned int audio_mcu_com_source_buffer_saddr  ;
	unsigned int audio_mcu_com_source_buffer_size   ;
	unsigned int audio_mcu_com_target_buffer_saddr  ;
	unsigned int audio_mcu_com_target_buffer_size   ;
	unsigned int audio_mcu_com_source_saddr         ;
	unsigned int audio_mcu_com_source_eaddr         ;
	unsigned int audio_mcu_com_target_saddr         ;
	union {
		struct {
			unsigned int audio_mcu_point_to_sdram1  ;/* 0x188 */
			unsigned int audio_mcu_point_to_sdram3  ;
		};
		struct {
			unsigned int audio_mcu_stream_pid       ;/* 0x188 */
			unsigned int audio_mcu_stream_type      ;
		};
	};
	unsigned int audio_mcu_audio_standard           ;/* 0x190 */
	unsigned int audio_mcu_sample_frequency         ;
	unsigned int audio_mcu_sample_bitrate           ;
	unsigned int audio_mcu_esa1_r_addr              ;
	unsigned int audio_mcu_esa2_r_addr              ;
	unsigned int audio_mcu_pcm_w_addr               ;
	unsigned int audio_mcu_decode_info              ;
	unsigned int audio_mcu_parse_info               ;
	unsigned int audio_mcu_savedesa1_w_addr         ;
	unsigned int audio_mcu_savedesa2_w_addr         ;
	unsigned int audio_mcu_channel_num              ;
	unsigned int audio_mcu_audioframe1_saddr        ;
	unsigned int audio_mcu_audioframe2_saddr        ;
	unsigned int audio_mcu_com_target_eaddr         ;
	union {
		struct {
			unsigned int audio_mcu_task_queue       ;
			unsigned int audio_mcu_point_to_sdram2  ;/* 0x1cc */
			unsigned int audio_mcu_point_to_sdram4  ;/* 0x1d0 */
			unsigned int audio_mcu_rev3[1]          ;
			unsigned int audio_mcu_aptsaddr         ;
		};
		struct {
			unsigned int audio_mcu_low_apts         ;
			unsigned int audio_mcu_rev3_1[3]        ;
			unsigned int audio_mcu_high_apts        ;
		};
	};
	unsigned int audio_mcu_com_source_r_addr        ;
	unsigned int audio_mcu_rev4[2]                  ;/* 0x1e0 */
	unsigned int audio_mcu_point_to_size1           ;
	unsigned int audio_mcu_axi_8to32_ctrl           ;
	unsigned int audio_mcu_pcmbuf_protect_saddr     ;/* 0x1f0*/
	unsigned int audio_mcu_pcmbuf_protect_size      ;
	unsigned int audio_mcu_protect_work_size        ;
	unsigned int audio_mcu_decode_ctrl2             ;
};
/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/* Private Macros -------------------------------------------------------- */
#define SET_BIT(reg,bit)                         \
	do                                           \
{                                                \
	gx_set_bit((bit), &(reg));                   \
}while(0)

#define CLR_BIT(reg,bit)                         \
	do                                           \
{                                                \
	gx_clear_bit((bit), &(reg));                 \
}while(0)

#define GET_BIT(reg,bit)                         \
	gx_test_bit((bit), &(reg))

#define SET_FEILD(reg,mask,val)                  \
	do                                           \
{                                                \
	unsigned int Reg = gx_ioread32(&(reg));      \
	((Reg) = ((Reg) & ~(mask)) | (val));         \
	gx_iowrite32(Reg, &(reg));                   \
}while(0)

#define CLR_FEILD(reg,mask)                      \
	do                                           \
{                                                \
	unsigned int Reg = gx_ioread32(&(reg));      \
	((Reg)   &=  ~(mask));                       \
	gx_iowrite32(Reg, &(reg));                   \
}while(0)

#define GET_FEILD(reg,mask,offset)                \
	(((gx_ioread32(&(reg))) & (mask)) >> (offset))

#define SET_VAL(reg,val)                          \
	do                                           \
{                                            \
	gx_iowrite32(val, &(reg));                   \
}while(0)

#define GET_VAL(reg)                              \
	gx_ioread32(&(reg))


/* Private Macros -------------------------------------------------------- */
#define CFG_HOT_SET_AUDIO_I2S(ptr)                                   \
	SET_VAL(ptr->cfg_mpeg_hot_reset_set,vCFG_MPEG_HOT_AUDIO_I2S)
#define CFG_HOT_CLR_AUDIO_I2S(ptr)                                   \
	SET_VAL(ptr->cfg_mpeg_hot_reset_clr,vCFG_MPEG_HOT_AUDIO_I2S)

#define CFG_HOT_SET_AUDIO_SPDIF(ptr)                                 \
	SET_VAL(ptr->cfg_mpeg_hot_reset_set,vCFG_MPEG_HOT_AUDIO_SPDIF)
#define CFG_HOT_CLR_AUDIO_SPDIF(ptr)                                 \
	SET_VAL(ptr->cfg_mpeg_hot_reset_clr,vCFG_MPEG_HOT_AUDIO_SPDIF)

// 音频时钟配置
#if 0
#define CFG_AUDIO_AD_SET_DTO(base,val)                                   \
	SET_FEILD(                                \
			base->cfg_dto1,       \
			0xffffffff,                \
			(vCFG_DTO_WSEL | vCFG_DTO_LOAD | (val & 0x3fffffff)))

#define CFG_AUDIO_CODEC_SET_DTO(base,val)                                   \
	SET_FEILD(                                \
			base->cfg_dto4,       \
			0xffffffff,                \
			(vCFG_DTO_WSEL | vCFG_DTO_LOAD | (val & 0x3fffffff)))

#define CFG_AUDIO_I2S_SET_DTO(base,val)                                     \
	SET_FEILD(                                \
			base->cfg_dto3,       \
			0xffffffff,                \
			(vCFG_DTO_WSEL | vCFG_DTO_LOAD | (val & 0x3fffffff)))



#define CFG_DEV_CLOCK_SOURCE_DISABLE(base,bit)                                  \
	CLR_BIT(base->cfg_source_sel,bit)
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++Audio Mcu++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/* AUDIO_MCU_INT register*/
#define bOR_INT_AUDIO1_FINISH           (0)
#define mOR_INT_AUDIO1_FINISH           (0x01 << bOR_INT_AUDIO1_FINISH)
#define vOR_INT_AUDIO1_FINISH           (0x01 << bOR_INT_AUDIO1_FINISH)

#define bOR_INT_AUDIO1_NEED_DATA        (1)
#define mOR_INT_AUDIO1_NEED_DATA        (0x01 << bOR_INT_AUDIO1_NEED_DATA)
#define vOR_INT_AUDIO1_NEED_DATA        (0x01 << bOR_INT_AUDIO1_NEED_DATA)

#define bOR_INT_AUDIO1_EMPTY_FINISH     (2)
#define mOR_INT_AUDIO1_EMPTY_FINISH     (0x01 << bOR_INT_AUDIO1_EMPTY_FINISH)
#define vOR_INT_AUDIO1_EMPTY_FINISH     (0x01 << bOR_INT_AUDIO1_EMPTY_FINISH)

#define bOR_INT_AUDIO1_SYNC_FINISH      (3)
#define mOR_INT_AUDIO1_SYNC_FINISH      (0x01 << bOR_INT_AUDIO1_SYNC_FINISH)
#define vOR_INT_AUDIO1_SYNC_FINISH      (0x01 << bOR_INT_AUDIO1_SYNC_FINISH)

#define bOR_INT_OR_INIT_OK              (4)
#define mOR_INT_OR_INIT_OK              (0x01 << bOR_INT_OR_INIT_OK)
#define vOR_INT_OR_INIT_OK              (0x01 << bOR_INT_OR_INIT_OK)

#define bOR_INT_OR_RST_OK               (5)
#define mOR_INT_OR_RST_OK               (0x01 << bOR_INT_OR_RST_OK)
#define vOR_INT_OR_RST_OK               (0x01 << bOR_INT_OR_RST_OK)

#define bOR_INT_OR_OTP_ERR              (6)
#define mOR_INT_OR_OTP_ERR              (0x01 << bOR_INT_OR_OTP_ERR)
#define vOR_INT_OR_OTP_ERR              (0x01 << bOR_INT_OR_OTP_ERR)

#define bOR_INT_OR_OTP_ERR1             (7)
#define mOR_INT_OR_OTP_ERR1             (0x01 << bOR_INT_OR_OTP_ERR1)
#define vOR_INT_OR_OTP_ERR1             (0x01 << bOR_INT_OR_OTP_ERR1)

#define bOR_INT_AUDIO2_FINISH           (8)
#define mOR_INT_AUDIO2_FINISH           (0x01 << bOR_INT_AUDIO2_FINISH)
#define vOR_INT_AUDIO2_FINISH           (0x01 << bOR_INT_AUDIO2_FINISH)

#define bOR_INT_AUDIO2_NEED_DATA        (9)
#define mOR_INT_AUDIO2_NEED_DATA        (0x01 << bOR_INT_AUDIO2_NEED_DATA)
#define vOR_INT_AUDIO2_NEED_DATA        (0x01 << bOR_INT_AUDIO2_NEED_DATA)

#define bOR_INT_AUDIO2_EMPTY_FINISH     (10)
#define mOR_INT_AUDIO2_EMPTY_FINISH     (0x01 << bOR_INT_AUDIO2_EMPTY_FINISH)
#define vOR_INT_AUDIO2_EMPTY_FINISH     (0x01 << bOR_INT_AUDIO2_EMPTY_FINISH)

#define bOR_INT_AUDIO2_SYNC_FINISH      (11)
#define mOR_INT_AUDIO2_SYNC_FINISH      (0x01 << bOR_INT_AUDIO2_SYNC_FINISH)
#define vOR_INT_AUDIO2_SYNC_FINISH      (0x01 << bOR_INT_AUDIO2_SYNC_FINISH)

#define bOR_INT_DECODE_KEY              (12)
#define mOR_INT_DECODE_KEY              (0x01 << bOR_INT_DECODE_KEY)
#define vOR_INT_DECODE_KEY              (0x01 << bOR_INT_DECODE_KEY)

#define bOR_INT_OR_INIT_OK_1            (15)
#define mOR_INT_OR_INIT_OK_1            (0x01 << bOR_INT_OR_INIT_OK_1)
#define vOR_INT_OR_INIT_OK_1            (0x01 << bOR_INT_OR_INIT_OK_1)

#define bOR_INT_SPD_PARSE_DONE          (16)
#define mOR_INT_SPD_PARSE_DONE          (0x01 << bOR_INT_SPD_PARSE_DONE)
#define vOR_INT_SPD_PARSE_DONE          (0x01 << bOR_INT_SPD_PARSE_DONE)

#define bOR_INT_NEED_DATA2              (17)
#define mOR_INT_NEED_DATA2              (0x01 << bOR_INT_NEED_DATA2)
#define vOR_INT_NEED_DATA2              (0x01 << bOR_INT_NEED_DATA2)

#define bOR_INT_DATA2_EMPTY_FINISH      (18)
#define mOR_INT_DATA2_EMPTY_FINISH      (0x01 << bOR_INT_DATA2_EMPTY_FINISH)
#define vOR_INT_DATA2_EMPTY_FINISH      (0x01 << bOR_INT_DATA2_EMPTY_FINISH)

#define bOR_INT_SYNC2_FINISH            (19)
#define mOR_INT_SYNC2_FINISH            (0x01 << bOR_INT_SYNC2_FINISH)
#define vOR_INT_SYNC2_FINISH            (0x01 << bOR_INT_SYNC2_FINISH)

#define bOR_INT_CONVERT_FINISH          (20)
#define mOR_INT_CONVERT_FINISH          (0x01 << bOR_INT_CONVERT_FINISH)
#define vOR_INT_CONVERT_FINISH          (0x01 << bOR_INT_CONVERT_FINISH)

#define bOR_INT_CONVERT_NEED_DATA       (21)
#define mOR_INT_CONVERT_NEED_DATA       (0x01 << bOR_INT_CONVERT_NEED_DATA)
#define vOR_INT_CONVERT_NEED_DATA       (0x01 << bOR_INT_CONVERT_NEED_DATA)

#define bOR_INT_CONVERT_EMPTY_FINISH    (22)
#define mOR_INT_CONVERT_EMPTY_FINISH    (0x01 << bOR_INT_CONVERT_EMPTY_FINISH)
#define vOR_INT_CONVERT_EMPTY_FINISH    (0x01 << bOR_INT_CONVERT_EMPTY_FINISH)

#define bOR_INT_DECODE_ERR              (24)
#define mOR_INT_DECODE_ERR              (0x01 << bOR_INT_DECODE_ERR)
#define vOR_INT_DECODE_ERR              (0x01 << bOR_INT_DECODE_ERR)

#define bOR_INT_STATUS                  (0)
#define mOR_INT_STATUS                  (0xFFFFFFFF << bOR_INT_STATUS)
#define vOR_INT_STATUS                  (0xFFFFFFFF << bOR_INT_STATUS)

#define bOR_INT_EN_VAL                  (0)
#define mOR_INT_EN_VAL                  (0xFFFFFFFF << bOR_INT_EN_VAL)
#define vOR_INT_EN_VAL                  (0xFFFFFFFF << bOR_INT_EN_VAL)

/* AUDIO_MCU_DECODE_CTRL register*/

////////////////////////////start//////////////////////////////////////
//sOR_DECODE_TASK for protect 31bit

#define bOR_MCU_TASK_START_TO_WORK      (31)
#define mOR_MCU_TASK_START_TO_WORK      (0x01 << bOR_MCU_TASK_START_TO_WORK)
#define vOR_MCU_TASK_START_TO_WORK      (0x01 << bOR_MCU_TASK_START_TO_WORK)

#define bOR_DECODE_TASK                 (0)
#define mOR_DECODE_TASK                 (0x0F << bOR_DECODE_TASK)
#define vOR_DECODE_TASK                 (0x0F << bOR_DECODE_TASK)
#define sOR_DECODE_TASK                 (mOR_DECODE_TASK|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DEC_RESTART_OR_CONTINUE     (4)
#define mOR_DEC_RESTART_OR_CONTINUE     (0x01 << bOR_DEC_RESTART_OR_CONTINUE)
#define vOR_DEC_RESTART_OR_CONTINUE     (0x01 << bOR_DEC_RESTART_OR_CONTINUE)
#define sOR_DEC_RESTART_OR_CONTINUE     (mOR_DEC_RESTART_OR_CONTINUE|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DATA1_EMPTY                 (5)
#define mOR_DATA1_EMPTY                 (0x01 << bOR_DATA1_EMPTY)
#define vOR_DATA1_EMPTY                 (0x01 << bOR_DATA1_EMPTY)
#define sOR_DATA1_EMPTY                 (mOR_DATA1_EMPTY|mOR_MCU_TASK_START_TO_WORK)

#define bOR_ESA1_ENCRYPTED              (6)
#define mOR_ESA1_ENCRYPTED              (0x01 << bOR_ESA1_ENCRYPTED)
#define vOR_ESA1_ENCRYPTED              (0x01 << bOR_ESA1_ENCRYPTED)
#define sOR_ESA1_ENCRYPTED              (mOR_ESA1_ENCRYPTED|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DEC_RESTART_OR_CONTINUE2    (12)
#define mOR_DEC_RESTART_OR_CONTINUE2    (0x01 << bOR_DEC_RESTART_OR_CONTINUE2)
#define vOR_DEC_RESTART_OR_CONTINUE2    (0x01 << bOR_DEC_RESTART_OR_CONTINUE2)
#define sOR_DEC_RESTART_OR_CONTINUE2    (mOR_DEC_RESTART_OR_CONTINUE2|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DATA1_EMPTY2                (13)
#define mOR_DATA1_EMPTY2                (0x01 << bOR_DATA1_EMPTY2)
#define vOR_DATA1_EMPTY2                (0x01 << bOR_DATA1_EMPTY2)
#define sOR_DATA1_EMPTY2                (mOR_DATA1_EMPTY2|mOR_MCU_TASK_START_TO_WORK)

#define bOR_ESA1_ENCRYPTED2             (14)
#define mOR_ESA1_ENCRYPTED2             (0x01 << bOR_ESA1_ENCRYPTED2)
#define vOR_ESA1_ENCRYPTED2             (0x01 << bOR_ESA1_ENCRYPTED2)
#define sOR_ESA1_ENCRYPTED2             (mOR_ESA1_ENCRYPTED2|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DEC_FRAME_NUM               (8)
#define mOR_DEC_FRAME_NUM               (0x0F << bOR_DEC_FRAME_NUM)
#define vOR_DEC_FRAME_NUM               (0x0F << bOR_DEC_FRAME_NUM)
#define sOR_DEC_FRAME_NUM               (mOR_DEC_FRAME_NUM|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DEC_FRAME_NUM2              (20)
#define mOR_DEC_FRAME_NUM2              (0x0F << bOR_DEC_FRAME_NUM2)
#define vOR_DEC_FRAME_NUM2              (0x0F << bOR_DEC_FRAME_NUM2)
#define sOR_DEC_FRAME_NUM2              (mOR_DEC_FRAME_NUM2|mOR_MCU_TASK_START_TO_WORK)

#define bOR_DATA2_EMPTY                 (16)
#define mOR_DATA2_EMPTY                 (0x01 << bOR_DATA2_EMPTY)
#define vOR_DATA2_EMPTY                 (0x01 << bOR_DATA2_EMPTY)
#define sOR_DATA2_EMPTY                 (mOR_DATA2_EMPTY|mOR_MCU_TASK_START_TO_WORK)

#define bOR_ESA2_ENCRYPTED              (17)
#define mOR_ESA2_ENCRYPTED              (0x01 << bOR_ESA2_ENCRYPTED)
#define vOR_ESA2_ENCRYPTED              (0x01 << bOR_ESA2_ENCRYPTED)
#define sOR_ESA2_ENCRYPTED              (mOR_ESA2_ENCRYPTED|mOR_MCU_TASK_START_TO_WORK)

#define bOR_AUDIO_PRINT                 (18)
#define mOR_AUDIO_PRINT                 (0x01 << bOR_AUDIO_PRINT)
#define vOR_AUDIO_PRINT                 (0x01 << bOR_AUDIO_PRINT)
#define sOR_AUDIO_PRINT                 (mOR_AUDIO_PRINT|mOR_MCU_TASK_START_TO_WORK)

#define bOR_CODEC_TYPE                  (20)
#define mOR_CODEC_TYPE                  (0x0F << bOR_CODEC_TYPE)
#define vOR_CODEC_TYPE                  (0x0F << bOR_CODEC_TYPE)
#define sOR_CODEC_TYPE                  (mOR_CODEC_TYPE|mOR_MCU_TASK_START_TO_WORK)

#define bOR_CPU_RST_OR                  (24)
#define mOR_CPU_RST_OR                  (0x01 << bOR_CPU_RST_OR)
#define vOR_CPU_RST_OR                  (0x01 << bOR_CPU_RST_OR)
#define sOR_CPU_RST_OR                  (mOR_CPU_RST_OR|mOR_MCU_TASK_START_TO_WORK)

#define bOR_PCM_OUT_MODE                (25)
#define mOR_PCM_OUT_MODE                (0x01 << bOR_PCM_OUT_MODE)
#define vOR_PCM_OUT_MODE                (0x01 << bOR_PCM_OUT_MODE)
#define sOR_PCM_OUT_MODE                (mOR_PCM_OUT_MODE|mOR_MCU_TASK_START_TO_WORK)

#define bOR_CPU_CHIP_INFO               (26)
#define mOR_CPU_CHIP_INFO               (0x01 << bOR_CPU_CHIP_INFO)
#define vOR_CPU_CHIP_INFO               (0x01 << bOR_CPU_CHIP_INFO)
#define sOR_CPU_CHIP_INFO               (mOR_CPU_CHIP_INFO|mOR_MCU_TASK_START_TO_WORK)

#define bOR_FIRMWARE_ENDIAN             (29)
#define mOR_FIRMWARE_ENDIAN             (0x03 << bOR_FIRMWARE_ENDIAN)
#define vOR_FIRMWARE_ENDIAN             (0x03 << bOR_FIRMWARE_ENDIAN)
#define sOR_FIRMWARE_ENDIAN             (mOR_FIRMWARE_ENDIAN|mOR_MCU_TASK_START_TO_WORK)

////////////////////////////end//////////////////////////////////////////


/* AUDIO_MCU_DECODE_INFO register*/
#define bOR_RA_ESA_CNT_UPDATA           (31)
#define mOR_RA_ESA_CNT_UPDATA           (0x01 << bOR_RA_ESA_CNT_UPDATA)
#define vOR_RA_ESA_CNT_UPDATA           (0x01 << bOR_RA_ESA_CNT_UPDATA)

#define bOR_DECODE_ERR_INDEX            (16)
#define mOR_DECODE_ERR_INDEX            (0x7FFF << bOR_DECODE_ERR_INDEX)
#define vOR_DECODE_ERR_INDEX            (0x7FFF << bOR_DECODE_ERR_INDEX)

#define bOR_DECODE_ERR_INDEX_X          (0)
#define mOR_DECODE_ERR_INDEX_X          (0xFFFF << bOR_DECODE_ERR_INDEX_X)
#define vOR_DECODE_ERR_INDEX_X          (0xFFFF << bOR_DECODE_ERR_INDEX_X)

#define bOR_NEED_DATA1_NUM              (0)
#define mOR_NEED_DATA1_NUM              (0xFFFF << bOR_NEED_DATA1_NUM)
#define vOR_NEED_DATA1_NUM              (0xFFFF << bOR_NEED_DATA1_NUM)

#define bOR_NEED_DATA_NUM_X             (16)
#define mOR_NEED_DATA_NUM_X             (0xFFFF << bOR_NEED_DATA_NUM_X)
#define vOR_NEED_DATA_NUM_X             (0xFFFF << bOR_NEED_DATA_NUM_X)

/* AUDIO_MCU_PARSE_INFO register*/
#define bOR_PARSE_ERR_INDEX            (16)
#define mOR_PARSE_ERR_INDEX            (0xFFFF << bOR_PARSE_ERR_INDEX)
#define vOR_PARSE_ERR_INDEX            (0xFFFF << bOR_PARSE_ERR_INDEX)
#define bOR_NEED_DATA2_NUM              (0)
#define mOR_NEED_DATA2_NUM              (0xFFFF << bOR_NEED_DATA2_NUM)
#define vOR_NEED_DATA2_NUM              (0xFFFF << bOR_NEED_DATA2_NUM)

/* AUDIO_MCU_AUDIO_STANDARD register*/
#define bOR_AUDIO_STANDARD              (24)
#define mOR_AUDIO_STANDARD              (0xFF << bOR_AUDIO_STANDARD)
#define vOR_AUDIO_STANDARD              (0xFF << bOR_AUDIO_STANDARD)

#define bOR_AUDIO_DOLBY_VER             (2)
#define mOR_AUDIO_DOLBY_VER             (0x1 << bOR_AUDIO_DOLBY_VER)
#define vOR_AUDIO_DOLBY_VER             (0x1 << bOR_AUDIO_DOLBY_VER)

#define bOR_AUDIO_DOLBY_TYPE            (0)
#define mOR_AUDIO_DOLBY_TYPE            (0x1 << bOR_AUDIO_DOLBY_TYPE)
#define vOR_AUDIO_DOLBY_TYPE            (0x1 << bOR_AUDIO_DOLBY_TYPE)

/* AUDIO_MCU_CHANNEL_NUM register*/
#define bOR_AUDIO_FRAME_LENGTH          (16)
#define mOR_AUDIO_FRAME_LENGTH          (0xFFFF << bOR_AUDIO_FRAME_LENGTH)
#define vOR_AUDIO_FRAME_LENGTH          (0xFFFF << bOR_AUDIO_FRAME_LENGTH)

#define bOR_AUDIO_FRAME_LENGTH_X        (0)
#define mOR_AUDIO_FRAME_LENGTH_X        (0xFFFF << bOR_AUDIO_FRAME_LENGTH_X)
#define vOR_AUDIO_FRAME_LENGTH_X        (0xFFFF << bOR_AUDIO_FRAME_LENGTH_X)

#define bOR_AUDIO_CHANNEL               (0)
#define mOR_AUDIO_CHANNEL               (0xFF << bOR_AUDIO_CHANNEL)
#define vOR_AUDIO_CHANNEL               (0xFF << bOR_AUDIO_CHANNEL)

#define bOR_AUDIO_CHANNEL_X             (24)
#define mOR_AUDIO_CHANNEL_X             (0xFF << bOR_AUDIO_CHANNEL_X)
#define vOR_AUDIO_CHANNEL_X             (0xFF << bOR_AUDIO_CHANNEL_X)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define OR_GET_EXPC(rp) \
	REG_GET_VAL(&(rp->audio_mcu_expc))

#define OR_SET_PRG_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_baseaddr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_GET_PRG_START_ADDR(rp) \
	GET_VAL(rp->audio_mcu_baseaddr)

#define OR_SET_DEC_START(rp) \
	SET_BIT(rp->audio_mcu_baseaddr, 0)

#define OR_SET_DEC_RESET(rp) \
	CLR_BIT(rp->audio_mcu_baseaddr, 0)

//decode ctrl info
#define OR_SET_MCU_TASK_START_TO_WORK(rp) \
	SET_BIT(rp->audio_mcu_decode_ctrl, bOR_MCU_TASK_START_TO_WORK)

#define OR_GET_MCU_TASK_START_TO_WORK(rp) \
	GET_BIT(rp->audio_mcu_decode_ctrl, bOR_MCU_TASK_START_TO_WORK)

#define OR_SET_CPU_RST_OR(rp) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_CPU_RST_OR,                    \
			(1 << bOR_CPU_RST_OR))

#define OR_CLR_CPU_RST_OR(rp) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_CPU_RST_OR,                    \
			(0 << bOR_CPU_RST_OR))

#define OR_SET_PCM_MODE_NON_INTERLACE(rp) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_PCM_OUT_MODE,                    \
			(1 << bOR_PCM_OUT_MODE))

#define OR_SET_PCM_MODE_INTERLACE(rp) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_PCM_OUT_MODE,                    \
			(0 << bOR_PCM_OUT_MODE))

#define OR_SET_DECODE_TASK(rp,value)                    \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DECODE_TASK,                \
			((value & 0xF) << bOR_DECODE_TASK))

#define OR_SET_NORMAL_DEC_RESTART_OR_CONTINUE(rp,  value) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DEC_RESTART_OR_CONTINUE,                    \
			((value & 0x01) << bOR_DEC_RESTART_OR_CONTINUE))

#define OR_SET_ADUIO2_DEC_RESTART_OR_CONTINUE(rp,  value) \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DEC_RESTART_OR_CONTINUE2,                    \
			((value & 0x01) << bOR_DEC_RESTART_OR_CONTINUE2))

#define OR_SET_NORMAL_FRAME_NUM( rp, value)       \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DEC_FRAME_NUM,                \
			((value & 0xF) << bOR_DEC_FRAME_NUM))

#define OR_SET_AUDIO2_FRAME_NUM( rp, value)       \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DEC_FRAME_NUM2,                \
			((value & 0xF) << bOR_DEC_FRAME_NUM2))

#define OR_SET_NORMAL_DATA_EMPTY(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DATA1_EMPTY,                    \
			((value & 0x01) << bOR_DATA1_EMPTY)) \

#define OR_SET_AUDIO2_DATA_EMPTY(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DATA1_EMPTY2,                    \
			((value & 0x01) << bOR_DATA1_EMPTY2)) \

#define OR_SET_NORMAL_DATA_ENCRYPTED(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_ESA1_ENCRYPTED,                    \
			((value & 0x01) << bOR_ESA1_ENCRYPTED))

#define OR_SET_AUDIO2_DATA_ENCRYPTED(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_ESA1_ENCRYPTED2,                    \
			((value & 0x01) << bOR_ESA1_ENCRYPTED2))

#define OR_SET_SPD_DATA_EMPTY(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_DATA2_EMPTY,                    \
			((value & 0x01) << bOR_DATA2_EMPTY)) \

#define OR_SET_SPD_DATA_ENCRYPTED(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_ESA2_ENCRYPTED,                    \
			((value & 0x01) << bOR_ESA2_ENCRYPTED))

#define OR_SET_AUDIO_PRINT(rp, value)        \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_AUDIO_PRINT,                    \
			((value & 0x01) << bOR_AUDIO_PRINT))

#define OR_SET_CODEC_TYPE(rp,value)                    \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_CODEC_TYPE,                \
			((value & 0xF) << bOR_CODEC_TYPE))

#define OR_SET_FIRMWARE_ENDIAN(rp, value)                    \
	SET_FEILD(                                \
			rp->audio_mcu_decode_ctrl,       \
			sOR_FIRMWARE_ENDIAN,                \
			((value & 0x3) << bOR_FIRMWARE_ENDIAN))

//audio mcu int
//or 清中断时,保证其它位为零,中断写零不影响,保护其它中断不被意外clear
#define OR_INT_OTP_ERR_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_OR_OTP_ERR)

#define OR_INT_OTP_ERR_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_OR_OTP_ERR)

#define OR_GET_OTP_ERR_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_OR_OTP_ERR)

#define OR_GET_OTP_ERR_INT_STATUS0(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_OR_OTP_ERR)

#define OR_CLR_OTP_ERR_INT_STATUS0(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_OR_OTP_ERR)

#define OR_GET_OTP_ERR_INT_STATUS1(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_OR_OTP_ERR1)

#define OR_CLR_OTP_ERR_INT_STATUS1(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_OR_OTP_ERR1)

#define OR_INT_DECODE_KEY_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DECODE_KEY)

#define OR_INT_DECODE_KEY_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DECODE_KEY)

#define OR_CLR_DECODE_KEY_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_DECODE_KEY)

#define OR_INT_DECODE_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_FINISH)

#define OR_INT_DECODE_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_FINISH)

#define OR_INT_AUDIO2_DECODE_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_FINISH)

#define OR_INT_AUDIO2_DECODE_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_FINISH)

#define OR_INT_CONVERT_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_CONVERT_FINISH)

#define OR_INT_CONVERT_FINISH_DISABLE(rp)     \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_CONVERT_FINISH)

#define OR_GET_DECODE_FINISH_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_FINISH)

#define OR_GET_DECODE_FINISH_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_AUDIO1_FINISH)

#define OR_CLR_AUDIO1_DECODE_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO1_FINISH)

#define OR_CLR_AUDIO2_DECODE_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO2_FINISH)

#define OR_CLR_CONVERT_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_CONVERT_FINISH)

#define OR_INT_NORMAL_NEED_DATA_ENABLE(rp)   \
	SET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_AUDIO1_NEED_DATA)

#define OR_INT_AUDIO2_NEED_DATA_ENABLE(rp)   \
	SET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_AUDIO2_NEED_DATA)

#define OR_INT_CONVERT_NEED_DATA_ENABLE(rp)  \
	SET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_CONVERT_NEED_DATA)

#define OR_INT_NORMAL_NEED_DATA_DISABLE(rp)   \
	CLR_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_AUDIO1_NEED_DATA)

#define OR_INT_AUDIO2_NEED_DATA_DISABLE(rp)   \
	CLR_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_AUDIO2_NEED_DATA)

#define OR_INT_CONVERT_NEED_DATA_DISABLE(rp)  \
	CLR_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_CONVERT_NEED_DATA)

#define OR_GET_NORMAL_NEED_DATA_INTEN(rp)     \
	GET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_AUDIO1_NEED_DATA)

#define OR_GET_NORMAL_NEED_DATA_INT_STATUS(rp) \
	GET_BIT(    \
			rp->audio_mcu_int,    \
			bOR_INT_AUDIO1_NEED_DATA)


#define OR_CLR_AUDIO1_NEED_DATA_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO1_NEED_DATA)

#define OR_CLR_AUDIO2_NEED_DATA_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO2_NEED_DATA)

#define OR_CLR_CONVERT_NEED_DATA_INT_STATUS(rp) \
	SET_VAL(                                    \
			rp->audio_mcu_int,                  \
			vOR_INT_CONVERT_NEED_DATA)

#define OR_INT_NORMAL_DATA_EMPTY_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_EMPTY_FINISH)

#define OR_INT_AUDIO2_DATA_EMPTY_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_EMPTY_FINISH)

#define OR_INT_CONVERT_DATA_EMPTY_FINISH_ENABLE(rp)    \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_CONVERT_EMPTY_FINISH)

#define OR_INT_NORMAL_DATA_EMPTY_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_EMPTY_FINISH)

#define OR_INT_AUDIO2_DATA_EMPTY_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_EMPTY_FINISH)

#define OR_INT_CONVERT_DATA_EMPTY_FINISH_DISABLE(rp)   \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_CONVERT_EMPTY_FINISH)

#define OR_GET_NORMAL_DATA_EMPTY_FINISH_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_EMPTY_FINISH)

#define OR_GET_NORMAL_DATA_EMPTY_FINISH_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_AUDIO1_EMPTY_FINISH)

#define OR_CLR_AUDIO1_DATA_EMPTY_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO1_EMPTY_FINISH)

#define OR_CLR_AUDIO2_DATA_EMPTY_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO2_EMPTY_FINISH)

#define OR_CLR_CONVERT_DATA_EMPTY_FINISH_INT_STATUS(rp)\
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
	        vOR_INT_CONVERT_EMPTY_FINISH)

#define OR_INT_NORMAL_SYNC_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_SYNC_FINISH)

#define OR_INT_AUDIO2_SYNC_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_SYNC_FINISH)

#define OR_INT_NORMAL_SYNC_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_SYNC_FINISH)

#define OR_INT_AUDIO2_SYNC_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO2_SYNC_FINISH)

#define OR_GET_NORMAL_SYNC_FINISH_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_AUDIO1_SYNC_FINISH)

#define OR_GET_NORMAL_SYNC_FINISH_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_AUDIO1_SYNC_FINISH)

#define OR_CLR_AUDIO1_SYNC_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO1_SYNC_FINISH)

#define OR_CLR_AUDIO2_SYNC_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_AUDIO2_SYNC_FINISH)

#define OR_GET_OR_INIT_OK_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_OR_INIT_OK)

#define OR_GET_OR_INIT_OK_INT1_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_OR_INIT_OK_1)

#define OR_CLR_OR_INIT_OK_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_OR_INIT_OK)

#define OR_GET_OR_RST_OK_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_OR_RST_OK)

#define OR_CLR_OR_RST_OK_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_OR_RST_OK)

#define OR_INT_SPD_PARSE_DONE_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SPD_PARSE_DONE)

#define OR_INT_SPD_PARSE_DONE_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SPD_PARSE_DONE)

#define OR_GET_SPD_PARSE_DONE_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SPD_PARSE_DONE)

#define OR_GET_SPD_PARSE_DONE_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_SPD_PARSE_DONE)

#define OR_CLR_SPD_PARSE_DONE_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_SPD_PARSE_DONE)

#define OR_INT_SPD_NEED_DATA_ENABLE(rp)   \
	SET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_NEED_DATA2)

#define OR_INT_SPD_NEED_DATA_DISABLE(rp)   \
	CLR_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_NEED_DATA2)

#define OR_GET_SPD_NEED_DATA_INTEN(rp)   \
	GET_BIT(    \
			rp->audio_mcu_inten,  \
			bOR_INT_NEED_DATA2)

#define OR_GET_SPD_NEED_DATA_INT_STATUS(rp) \
	GET_BIT(    \
			rp->audio_mcu_int,    \
			bOR_INT_NEED_DATA2)

#define OR_CLR_SPD_NEED_DATA_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_NEED_DATA2)

#define OR_INT_SPD_DATA_EMPTY_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DATA2_EMPTY_FINISH)

#define OR_INT_SPD_DATA_EMPTY_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DATA2_EMPTY_FINISH)

#define OR_GET_SPD_DATA_EMPTY_FINISH_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DATA2_EMPTY_FINISH)

#define OR_GET_SPD_DATA_EMPTY_FINISH_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_DATA2_EMPTY_FINISH)

#define OR_CLR_SPD_DATA_EMPTY_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_DATA2_EMPTY_FINISH)

#define OR_INT_SPD_SYNC_FINISH_ENABLE(rp)     \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SYNC2_FINISH)

#define OR_INT_SPD_SYNC_FINISH_DISABLE(rp)    \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SYNC2_FINISH)

#define OR_GET_SPD_SYNC_FINISH_INTEN(rp)      \
	GET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_SYNC2_FINISH)

#define OR_GET_SPD_SYNC_FINISH_INT_STATUS(rp) \
	GET_BIT(                                 \
			rp->audio_mcu_int,               \
			bOR_INT_SYNC2_FINISH)

#define OR_CLR_SPD_SYNC_FINISH_INT_STATUS(rp) \
	SET_VAL(                                 \
			rp->audio_mcu_int,               \
			vOR_INT_SYNC2_FINISH)

#define OR_GET_INT_STATUS(rp) \
	GET_FEILD(  \
			rp->audio_mcu_int,     \
			mOR_INT_STATUS,     \
			bOR_INT_STATUS)

#define OR_CLR_ALL_INT_STATUS(rp) \
	SET_VAL(                                \
			rp->audio_mcu_int,       \
			vOR_INT_STATUS)

#define OR_GET_INT_EN_VAL(rp) \
	GET_FEILD(  \
			rp->audio_mcu_inten,     \
			mOR_INT_EN_VAL,     \
			bOR_INT_EN_VAL)

#define OR_INT_DECODE_ERR_ENABLE(rp)         \
	SET_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DECODE_ERR)

#define OR_INT_DECODE_ERR_DISABLE(rp)        \
	CLR_BIT(                                 \
			rp->audio_mcu_inten,             \
			bOR_INT_DECODE_ERR)

//buffer info
#define OR_SET_NORMAL_ESA_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_esa1_buffer_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_SET_NORMAL_ESA_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_mcu_esa1_buffer_size, value)

#define OR_SET_SPD_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_esa2_buffer_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_SET_SPD_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_mcu_esa2_buffer_size, value)

#define OR_SET_PCM_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_pcm_buffer_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_SET_PCM_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_mcu_pcm_buffer_size, value)

#define OR_SET_COM_SOURCE_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_com_source_buffer_saddr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_SET_COM_SOURCE_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_mcu_com_source_buffer_size, value)

#define OR_SET_COM_TARGET_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_com_target_buffer_saddr, (value & AUDIO_BUF_ADDR_MASK))

#define OR_SET_COM_TARGET_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_mcu_com_target_buffer_size, value)

//decode frame info
#define OR_GET_AUDIO_STANDARD(rp)           \
	GET_FEILD(                              \
			rp->audio_mcu_audio_standard,   \
			mOR_AUDIO_STANDARD,             \
			bOR_AUDIO_STANDARD)

#define OR_GET_AUDIO_DOLBY_TYPE(rp)           \
	GET_FEILD(                              \
			rp->audio_mcu_audio_standard,   \
			mOR_AUDIO_DOLBY_TYPE,             \
			bOR_AUDIO_DOLBY_TYPE)

#define OR_GET_AUDIO_DOLBY_VERSION(rp)      \
	GET_BIT(rp->audio_mcu_audio_standard, bOR_AUDIO_DOLBY_VER)

#define OR_GET_SAMPLE_RATE(rp) \
	GET_VAL(rp->audio_mcu_sample_frequency)

#define OR_GET_BIT_RATE(rp) \
	GET_VAL(rp->audio_mcu_sample_bitrate)

#define OR_GET_AUDIO_CHANNEL(rp)    \
	GET_FEILD( \
			rp->audio_mcu_channel_num, \
			mOR_AUDIO_CHANNEL, \
			bOR_AUDIO_CHANNEL)

#define OR_GET_AUDIO2_CHANNEL(rp)    \
	GET_FEILD( \
			rp->audio_mcu_channel_num, \
			mOR_AUDIO_CHANNEL_X, \
			bOR_AUDIO_CHANNEL_X)

#define OR_GET_DEC_FRAMELENGTH(rp) \
	GET_FEILD( \
			rp->audio_mcu_channel_num, \
			mOR_AUDIO_FRAME_LENGTH, \
			bOR_AUDIO_FRAME_LENGTH)

#define OR_GET_AUDIO2_DEC_FRAMELENGTH(rp) \
	GET_FEILD( \
			rp->audio_mcu_channel_num, \
			mOR_AUDIO_FRAME_LENGTH_X, \
			bOR_AUDIO_FRAME_LENGTH_X)

//decode frame addr info
#define OR_SET_NORMAL_FRAME_S_ADDR(rp, value)    \
	SET_VAL(rp->audio_mcu_esa1_frame_saddr, value)

#define OR_SET_NORMAL_ESA_W_ADDR(rp, value)      \
	SET_VAL(rp->audio_mcu_esa1_w_addr, value)

#define OR_SET_NORMAL_PCM_W_SADDR(  rp, value)     \
	SET_VAL(rp->audio_mcu_pcm_w_saddr, value)

#define OR_SET_COM_SOURCE_S_ADDR(  rp, value)     \
	SET_VAL(rp->audio_mcu_com_source_saddr, value)

#define OR_SET_COM_SOURCE_W_SADDR(  rp, value)     \
	SET_VAL(rp->audio_mcu_com_source_eaddr, value)

#define OR_SET_COM_TARGET_W_SADDR(  rp, value)     \
	SET_VAL(rp->audio_mcu_com_target_saddr, value)

#define OR_GET_NORMAL_COM_SOURCE_R_ADDR(rp)     \
	GET_VAL(rp->audio_mcu_com_source_r_addr)

#define OR_SET_NORMAL_AXI_8TO32_CTRL(rp, value)     \
	SET_VAL(rp->audio_mcu_axi_8to32_ctrl, value)

#define OR_GET_NORMAL_RA_APTSADDR(rp)     \
	GET_VAL(rp->audio_mcu_aptsaddr)

#define OR_GET_NORMAL_ESA_R_ADDR(rp)     \
	GET_VAL(rp->audio_mcu_esa1_r_addr)

#define OR_GET_NORMAL_PCM_W_ADDR(rp)    \
	GET_VAL(rp->audio_mcu_pcm_w_addr)

#define OR_GET_MCU_RA_ESA_CNT_UPDATA(rp) \
	GET_BIT(rp->audio_mcu_decode_info, bOR_RA_ESA_CNT_UPDATA)

#define OR_GET_NORMAL_ERR_INDEX(rp)  \
	GET_FEILD( \
			rp->audio_mcu_decode_info, \
			mOR_DECODE_ERR_INDEX, \
			bOR_DECODE_ERR_INDEX)

#define OR_GET_AUDIO2_ERR_INDEX(rp)  \
	GET_FEILD( \
			rp->audio_mcu_decode_info, \
			mOR_DECODE_ERR_INDEX_X, \
			bOR_DECODE_ERR_INDEX_X)

#define OR_GET_NORMAL_NEED_DATA_NUM(rp)  \
	GET_FEILD( \
			rp->audio_mcu_decode_info, \
			mOR_NEED_DATA1_NUM, \
			bOR_NEED_DATA1_NUM)

#define OR_GET_AUDIO2_NEED_DATA_NUM(rp)  \
	GET_FEILD( \
			rp->audio_mcu_decode_info, \
			mOR_NEED_DATA_NUM_X , \
			bOR_NEED_DATA_NUM_X)

#define OR_GET_NORMAL_SAVED_ESAW_ADDR(rp) \
	GET_VAL(rp->audio_mcu_savedesa1_w_addr)


#define OR_SET_SPD_FRAME_S_ADDR(rp, value)     \
	SET_VAL(rp->audio_mcu_esa2_frame_saddr, value)


#define OR_SET_SPD_ESA_W_ADDR(rp, value)        \
	SET_VAL(rp->audio_mcu_esa2_w_addr, value)

#define OR_GET_SPD_ESA_R_ADDR(rp)    \
	GET_VAL(rp->audio_mcu_esa2_r_addr)

#define OR_GET_PARSE_ERR_INDEX(rp)  \
	GET_FEILD( \
			rp->audio_mcu_parse_info, \
			mOR_PARSE_ERR_INDEX, \
			bOR_PARSE_ERR_INDEX)

#define OR_GET_SPD_NEED_DATA_NUM(rp)     \
	GET_FEILD( \
			rp->audio_mcu_parse_info, \
			mOR_NEED_DATA2_NUM, \
			bOR_NEED_DATA2_NUM)


#define OR_GET_SPD_SAVED_ESAW_ADDR(rp)     \
	GET_VAL(rp->audio_mcu_savedesa2_w_addr)

#define OR_GET_COM_TARGET_EADDR(rp)     \
	GET_VAL(rp->audio_mcu_com_target_eaddr)

//point info
#define OR_SET_MCU_POINT_TO_SDRAM_1_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_point_to_sdram1, value)

#define OR_SET_MCU_POINT_TO_SDRAM_1_SIZE(rp, value) \
	SET_VAL(rp->audio_mcu_point_to_size1, value)

#define OR_SET_MCU_POINT_TO_SDRAM_2_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_point_to_sdram2, value)

#define OR_SET_MCU_POINT_TO_SDRAM_3_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_point_to_sdram3, value)

#define OR_SET_MCU_POINT_TO_SDRAM_4_ADDR(rp,value) \
	SET_VAL(rp->audio_mcu_point_to_sdram4, value)

#define OR_SET_MCU_PROTECT_PCMBUF_S_ADDR(rp, value) \
	SET_VAL(rp->audio_mcu_pcmbuf_protect_saddr, value)

#define OR_SET_MCU_PROTECT_PCMBUF_SIZE(rp, value) \
	SET_VAL(rp->audio_mcu_pcmbuf_protect_size, value)

#define OR_SET_MCU_PROTECT_WORKBUF_SIZE(rp, value) \
	SET_VAL(rp->audio_mcu_protect_work_size, value)

#define OR_SET_MCU_HW_INT_ENABLE(rp)         \
	SET_BIT(rp->audio_mcu_decode_ctrl2, 1)

#define OR_SET_MCU_HW_INT_DISABLE(rp)        \
	CLR_BIT(rp->audio_mcu_decode_ctrl2, 1)

#define OR_SET_MCU_HW_RST_ENABLE(rp)         \
	SET_BIT(rp->audio_mcu_decode_ctrl2, 0)

#define OR_SET_MCU_HW_RST_DISABLE(rp)        \
	CLR_BIT(rp->audio_mcu_decode_ctrl2, 0)

#endif
