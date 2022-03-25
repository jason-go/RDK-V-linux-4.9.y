#ifndef _GX3201_AUDIO_REG_H_
#define _GX3201_AUDIO_REG_H_


#define AUDIO_SPDIF_BASE_ADDR     (0x04c00000)
#define AUDIO_SPDIF_RAW_BASE_ADDR (0x04c00074)

struct audiospdif_regs
{
	unsigned int audio_play_spdif_ctrl;                /* 0x00 */
	unsigned int audio_play_spdif_inputdata_info;
	unsigned int audio_play_spdif_int_en;
	unsigned int audio_play_spdif_int;
	unsigned int audio_play_spdif_pa_pb;
	unsigned int audio_play_spdif_pc_pd;
	unsigned int audio_play_spdif_cl1;
	unsigned int audio_play_spdif_cl2;
	unsigned int audio_play_spdif_cr1;
	unsigned int audio_play_spdif_cr2;
	unsigned int audio_play_spdif_u;
	unsigned int audio_play_spdif_lat_pause;
	unsigned int audio_play_spdif_data_pau_len;
	unsigned int audio_play_spdif_newframe_len;
	unsigned int audio_play_spdif_newframe_start_addr;
	unsigned int audio_play_spdif_newframe_end_addr;
	unsigned int audio_play_spdif_buffer_start_addr;
	unsigned int audio_play_spdif_buffer_size;
	unsigned int audio_play_spdif_pause_pc_pd;
	unsigned int audio_play_spdif_playingframe_len;            /* 0x14c */
	unsigned int audio_play_spdif_playingframe_start_addr;
	unsigned int audio_play_spdif_playingframe_end_addr;
	unsigned int audio_play_spdif_buffer_read_addr;
	unsigned int audio_play_spdif_playingpcmframe_info;
	unsigned int audio_play_spdif_pcm_time_gate;
	unsigned int audio_play_spdif_rubish_data_saddr;
	unsigned int audio_play_spdif_rubish_length;
	unsigned int audio_play_spdif_newframe_pcmlen;
	unsigned int audio_play_spdif_playingframe_pcmlen;/* 0x70 */
};

struct audiospdif_raw_regs
{
	unsigned int audio_play_spdif_inputdata_info; //0x74
	unsigned int audio_play_spdif_int_en;
	unsigned int audio_play_spdif_int;
	unsigned int audio_play_spdif_pa_pb;
	unsigned int audio_play_spdif_pc_pd;
	unsigned int audio_play_spdif_cl1;
	unsigned int audio_play_spdif_cl2;
	unsigned int audio_play_spdif_cr1;
	unsigned int audio_play_spdif_cr2;
	unsigned int audio_play_spdif_u;
	unsigned int audio_play_spdif_lat_pause;
	unsigned int audio_play_spdif_data_pau_len;
	unsigned int audio_play_spdif_newframe_len;
	unsigned int audio_play_spdif_newframe_start_addr;
	unsigned int audio_play_spdif_newframe_end_addr;
	unsigned int audio_play_spdif_buffer_start_addr;
	unsigned int audio_play_spdif_buffer_size;
	unsigned int audio_play_spdif_pause_pc_pd;
	unsigned int audio_play_spdif_playingframe_len;
	unsigned int audio_play_spdif_playingframe_start_addr;
	unsigned int audio_play_spdif_playingframe_end_addr;
	unsigned int audio_play_spdif_buffer_read_addr;
	unsigned int audio_play_spdif_rubish_data_saddr;
	unsigned int audio_play_spdif_rubish_length;
	//pegasus fix dts play
	unsigned int audio_play_spdif_ac3_cl3;
	unsigned int audio_play_spdif_ac3_cl4;
	unsigned int audio_play_spdif_ac3_cl5;
	unsigned int audio_play_spdif_ac3_cl6;
	unsigned int audio_play_spdif_ac3_cr3;
	unsigned int audio_play_spdif_ac3_cr4;
	unsigned int audio_play_spdif_ac3_cr5;
	unsigned int audio_play_spdif_ac3_cr6;
	unsigned int audio_play_spdif_count_stuff;
	unsigned int audio_play_spdif_nds_config;
	unsigned int audio_play_spdif_cd_fifo_ctrl;
	unsigned int audio_play_spdif_cd_fifo_state;
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

// 咄撞扮嶝塘崔
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
//++++++++++++++++++++++++++++++Audio Out++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**************************GX3201 AUDIO SPDIF PLAY MODULE******************/
/*register AUDIO_PLAY_SPDIF_CTRL bit defines、mask defines、value defines*/
#define bAUDIO_SPD_SOFT_RESET           (31)
#define mAUDIO_SPD_SOFT_RESET           (0x01 <<  bAUDIO_SPD_SOFT_RESET          )
#define vAUDIO_SPD_SOFT_RESET           (0x01 <<  bAUDIO_SPD_SOFT_RESET          )

#define bAUDIO_SPD_RAW_SOFT_RESET       (30)
#define mAUDIO_SPD_RAW_SOFT_RESET       (0x01 <<  bAUDIO_SPD_RAW_SOFT_RESET          )
#define vAUDIO_SPD_RAW_SOFT_RESET       (0x01 <<  bAUDIO_SPD_RAW_SOFT_RESET          )

#define bAUDIO_HDMI_SPD_SEL            (29)
#define mAUDIO_HDMI_SPD_SEL            (0x01 <<  bAUDIO_HDMI_SPD_SEL           )
#define vAUDIO_HDMI_SPD_SEL            (0x01 <<  bAUDIO_HDMI_SPD_SEL           )

#define bAUDIO_SPDIF_SPD_SEL           (28)
#define mAUDIO_SPDIF_SPD_SEL           (0x01 <<  bAUDIO_SPDIF_SPD_SEL           )
#define vAUDIO_SPDIF_SPD_SEL           (0x01 <<  bAUDIO_SPDIF_SPD_SEL           )


#define bAUDIO_SPD_RAW_CLEAR_FIFO_STATUS      (11)
#define mAUDIO_SPD_RAW_CLEAR_FIFO_STATUS      (0x01 <<  bAUDIO_SPD_RAW_CLEAR_FIFO_STATUS          )
#define vAUDIO_SPD_RAW_CLEAR_FIFO_STATUS      (0x01 <<  bAUDIO_SPD_RAW_CLEAR_FIFO_STATUS          )

#define bAUDIO_SPD_RAW_PLAY_MODE             (8)
#define mAUDIO_SPD_RAW_PLAY_MODE             (0x07 <<  bAUDIO_SPD_RAW_PLAY_MODE           )
#define vAUDIO_SPD_RAW_PLAY_MODE             (0x07 <<  bAUDIO_SPD_RAW_PLAY_MODE           )

#define bAUDIO_HDMI_SOURCE_SEL            (7)
#define mAUDIO_HDMI_SOURCE_SEL            (0x01 <<  bAUDIO_HDMI_SOURCE_SEL           )
#define vAUDIO_HDMI_SOURCE_SEL            (0x01 <<  bAUDIO_HDMI_SOURCE_SEL           )

#define bAUDIO_HBR_ESA_CUT_MODE           (6)
#define mAUDIO_HBR_ESA_CUT_MODE           (0x01 <<  bAUDIO_HBR_ESA_CUT_MODE           )
#define vAUDIO_HBR_ESA_CUT_MODE           (0x01 <<  bAUDIO_HBR_ESA_CUT_MODE           )

#define bAUDIO_SPD_CLEAR_FIFO_STATUS      (3)
#define mAUDIO_SPD_CLEAR_FIFO_STATUS      (0x01 <<  bAUDIO_SPD_CLEAR_FIFO_STATUS          )
#define vAUDIO_SPD_CLEAR_FIFO_STATUS      (0x01 <<  bAUDIO_SPD_CLEAR_FIFO_STATUS          )

#define bAUDIO_SPD_PLAY_MODE             (0)
#define mAUDIO_SPD_PLAY_MODE             (0x07 <<  bAUDIO_SPD_PLAY_MODE           )
#define vAUDIO_SPD_PLAY_MODE             (0x07 <<  bAUDIO_SPD_PLAY_MODE           )

/*register AUDIO_PLAY_SPDIF_INPUTDATA_INFO defines、mask defines、value defines*/
#define bAUDIO_SPD_CLEAR_THE_SET_FRAME          (31)
#define mAUDIO_SPD_CLEAR_THE_SET_FRAME          (0x01 << bAUDIO_SPD_CLEAR_THE_SET_FRAME          )
#define vAUDIO_SPD_CLEAR_THE_SET_FRAME          (0x01 << bAUDIO_SPD_CLEAR_THE_SET_FRAME          )

#define bAUDIO_SPD_SET_NEW_FRAME                 (30)
#define mAUDIO_SPD_SET_NEW_FRAME                 (0x01 << bAUDIO_SPD_SET_NEW_FRAME          )
#define vAUDIO_SPD_SET_NEW_FRAME                 (0x01 << bAUDIO_SPD_SET_NEW_FRAME          )

#define bAUDIO_SPD_CDDATA_FORMAT                 (24)
#define mAUDIO_SPD_CDDATA_FORMAT                 (0x03 << bAUDIO_SPD_CDDATA_FORMAT          )
#define vAUDIO_SPD_CDDATA_FORMAT                 (0x03 << bAUDIO_SPD_CDDATA_FORMAT          )

#define bAUDIO_SPD_CHANNEL_L_SEL                 (16)
#define mAUDIO_SPD_CHANNEL_L_SEL                 (0x0F << bAUDIO_SPD_CHANNEL_L_SEL          )
#define vAUDIO_SPD_CHANNEL_L_SEL                 (0x0F << bAUDIO_SPD_CHANNEL_L_SEL          )
#define sAUDIO_SPD_CHANNEL_L_SEL                 (mAUDIO_SPD_CHANNEL_L_SEL|mAUDIO_SPD_SET_NEW_FRAME)

#define bAUDIO_SPD_CHANNEL_R_SEL                 (20)
#define mAUDIO_SPD_CHANNEL_R_SEL                 (0x0F << bAUDIO_SPD_CHANNEL_R_SEL          )
#define vAUDIO_SPD_CHANNEL_R_SEL                 (0x0F << bAUDIO_SPD_CHANNEL_R_SEL          )
#define sAUDIO_SPD_CHANNEL_R_SEL                 (mAUDIO_SPD_CHANNEL_R_SEL|mAUDIO_SPD_SET_NEW_FRAME)

#define bAUDIO_SPD_CHANNEL_SEL                 (16)
#define mAUDIO_SPD_CHANNEL_SEL                 (0xFF << bAUDIO_SPD_CHANNEL_SEL          )
#define vAUDIO_SPD_CHANNEL_SEL                 (0xFF << bAUDIO_SPD_CHANNEL_SEL          )
#define sAUDIO_SPD_CHANNEL_SEL                 (mAUDIO_SPD_CHANNEL_SEL|mAUDIO_SPD_SET_NEW_FRAME)

#define bAUDIO_SPD_CDDATA_CONTINUOUS        (15)
#define mAUDIO_SPD_CDDATA_CONTINUOUS        (0x01 << bAUDIO_SPD_CDDATA_CONTINUOUS              )
#define vAUDIO_SPD_CDDATA_CONTINUOUS        (0x01 << bAUDIO_SPD_CDDATA_CONTINUOUS              )

#define bAUDIO_SPD_PCM_DO_SRC_EN               (14)
#define mAUDIO_SPD_PCM_DO_SRC_EN              (0x01 << bAUDIO_SPD_PCM_DO_SRC_EN   )
#define vAUDIO_SPD_PCM_DO_SRC_EN              (0x01 << bAUDIO_SPD_PCM_DO_SRC_EN   )

#define bAUDIO_SPD_SERIAL_SILENT_END        (13)
#define mAUDIO_SPD_SERIAL_SILENT_END        (0x01 << bAUDIO_SPD_SERIAL_SILENT_END              )
#define vAUDIO_SPD_SERIAL_SILENT_END        (0x01 << bAUDIO_SPD_SERIAL_SILENT_END              )

#define bAUDIO_SPD_RAW_MUTE                 (13)
#define mAUDIO_SPD_RAW_MUTE                 (0x01 << bAUDIO_SPD_RAW_MUTE              )
#define vAUDIO_SPD_RAW_MUTE                 (0x01 << bAUDIO_SPD_RAW_MUTE              )

#define bAUDIO_SPD_SAMPLE_DATA_BIT_SIZE       (8)
#define mAUDIO_SPD_SAMPLE_DATA_BIT_SIZE       (0x1F << bAUDIO_SPD_SAMPLE_DATA_BIT_SIZE          )
#define vAUDIO_SPD_SAMPLE_DATA_BIT_SIZE       (0x1F << bAUDIO_SPD_SAMPLE_DATA_BIT_SIZE          )
#define sAUDIO_SPD_SAMPLE_DATA_BIT_SIZE       (mAUDIO_SPD_SAMPLE_DATA_BIT_SIZE|mAUDIO_SPD_SET_NEW_FRAME  )

#define bAUDIO_SPD_PCM_MAGN            (6)
#define mAUDIO_SPD_PCM_MAGN            (0x03 << bAUDIO_SPD_PCM_MAGN              )
#define vAUDIO_SPD_PCM_MAGN            (0x03 << bAUDIO_SPD_PCM_MAGN              )
#define sAUDIO_SPD_PCM_MAGN            (mAUDIO_SPD_PCM_MAGN|mAUDIO_SPD_SET_NEW_FRAME)

#define bAUDIO_SPD_PCM_DATA_ENDIAN            (5)
#define mAUDIO_SPD_PCM_DATA_ENDIAN            (0x01 << bAUDIO_SPD_PCM_DATA_ENDIAN              )
#define vAUDIO_SPD_PCM_DATA_ENDIAN            (0x01 << bAUDIO_SPD_PCM_DATA_ENDIAN              )

#define bAUDIO_SPD_PCM_STORE_MODE             (4)
#define mAUDIO_SPD_PCM_STORE_MODE             (0x01 << bAUDIO_SPD_PCM_STORE_MODE           )
#define vAUDIO_SPD_PCM_STORE_MODE             (0x01 << bAUDIO_SPD_PCM_STORE_MODE           )

#define bAUDIO_SPD_PCM_CHANNEL_NUMS           (0)
#define mAUDIO_SPD_PCM_CHANNEL_NUMS           (0x0F << bAUDIO_SPD_PCM_CHANNEL_NUMS         )
#define vAUDIO_SPD_PCM_CHANNEL_NUMS           (0x0F << bAUDIO_SPD_PCM_CHANNEL_NUMS         )

/*register AUDIO_PLAY_SPDIF_INT bit defines、mask defines、value defines*/
#define bAUDIO_SPD_STOP_STATUS           (5)
#define mAUDIO_SPD_STOP_STATUS           (0x01 << bAUDIO_SPD_STOP_STATUS       )
#define vAUDIO_SPD_STOP_STATUS           (0x01 << bAUDIO_SPD_STOP_STATUS       )

#define bAUDIO_SPD_ESA_CUT               (6)
#define mAUDIO_SPD_ESA_CUT               (0x01 <<  bAUDIO_SPD_ESA_CUT  )
#define vAUDIO_SPD_ESA_CUT               (0x01 <<  bAUDIO_SPD_ESA_CUT  )

#define bAUDIO_SPD_SET_NEW_PCM_INT            (9)
#define mAUDIO_SPD_SET_NEW_PCM_INT            (0x01 <<  bAUDIO_SPD_SET_NEW_PCM_INT  )
#define vAUDIO_SPD_SET_NEW_PCM_INT            (0x01 <<  bAUDIO_SPD_SET_NEW_PCM_INT  )

#define bAUDIO_SPD_CD_PLAY_DONE          (2)
#define mAUDIO_SPD_CD_PLAY_DONE          (0x01 <<  bAUDIO_SPD_CD_PLAY_DONE  )
#define vAUDIO_SPD_CD_PLAY_DONE          (0x01 <<  bAUDIO_SPD_CD_PLAY_DONE  )

#define bAUDIO_SPD_PAUSE_DONE            (1)
#define mAUDIO_SPD_PAUSE_DONE            (0x01 <<  bAUDIO_SPD_PAUSE_DONE   )
#define vAUDIO_SPD_PAUSE_DONE            (0x01 <<  bAUDIO_SPD_PAUSE_DONE   )

#define bAUDIO_SPD_STUFF_DONE            (0)
#define mAUDIO_SPD_STUFF_DONE            (0x01 <<  bAUDIO_SPD_PAUSE_DONE   )
#define vAUDIO_SPD_STUFF_DONE            (0x01 <<  bAUDIO_SPD_PAUSE_DONE   )

#define bAUDIO_SPD_INT_STATUS            (0)
#define mAUDIO_SPD_INT_STATUS            (0x30FFF << bAUDIO_SPD_INT_STATUS          )
#define vAUDIO_SPD_INT_STATUS            (0x30FFF << bAUDIO_SPD_INT_STATUS          )

#define bAUDIO_SPD_INT_EN_VAL            (0)
#define mAUDIO_SPD_INT_EN_VAL            (0x30FFF << bAUDIO_SPD_INT_EN_VAL          )
#define vAUDIO_SPD_INT_EN_VAL            (0x30FFF << bAUDIO_SPD_INT_EN_VAL          )

/*register AUDIO_SPDIF_PA_PB bit defines、mask defines、value defines*/
#define bAUDIO_SPD_PA            (16)
#define mAUDIO_SPD_PA            (0xFFFF << bAUDIO_SPD_PA          )
#define vAUDIO_SPD_PA            (0xFFFF << bAUDIO_SPD_PA          )

#define bAUDIO_SPD_PB            (0)
#define mAUDIO_SPD_PB            (0xFFFF << bAUDIO_SPD_PB          )
#define vAUDIO_SPD_PB            (0xFFFF << bAUDIO_SPD_PB          )

/*register AUDIO_SPDIF_PC_PD bit defines、mask defines、value defines*/
#define bAUDIO_SPD_PC            (16)
#define mAUDIO_SPD_PC            (0xFFFF << bAUDIO_SPD_PC          )
#define vAUDIO_SPD_PC            (0xFFFF << bAUDIO_SPD_PC          )

#define bAUDIO_SPD_PD            (0)
#define mAUDIO_SPD_PD            (0xFFFF << bAUDIO_SPD_PD          )
#define vAUDIO_SPD_PD            (0xFFFF << bAUDIO_SPD_PD          )

/*register AUDIO_SPDIF_LAT_PAUSE bit defines、mask defines、value defines*/
#define bAUDIO_SPD_PAUSE_NUM            (0)
#define mAUDIO_SPD_PAUSE_NUM            (0xFFFF << bAUDIO_SPD_PAUSE_NUM          )
#define vAUDIO_SPD_PAUSE_NUM            (0xFFFF << bAUDIO_SPD_PAUSE_NUM          )

/*register AUDIO_SPDIF_DATA_PAU_LEN bit defines、mask defines、value defines*/
#define bAUDIO_SPD_DATA_LENTH            (16)
#define mAUDIO_SPD_DATA_LENTH            (0xFFFF << bAUDIO_SPD_DATA_LENTH          )
#define vAUDIO_SPD_DATA_LENTH            (0xFFFF << bAUDIO_SPD_DATA_LENTH          )

#define bAUDIO_SPD_PAUSE_LENTH            (0)
#define mAUDIO_SPD_PAUSE_LENTH            (0xFFFF << bAUDIO_SPD_PAUSE_LENTH          )
#define vAUDIO_SPD_PAUSE_LENTH            (0xFFFF << bAUDIO_SPD_PAUSE_LENTH          )

/*register AUDIO_SPDIF_NEWFRAME_LEN bit defines、mask defines、value defines*/
#define bAUDIO_SPD_NEWFRAME_LENTH            (0)
#define mAUDIO_SPD_NEWFRAME_LENTH            (0xFFFF << bAUDIO_SPD_NEWFRAME_LENTH          )
#define vAUDIO_SPD_NEWFRAME_LENTH            (0xFFFF << bAUDIO_SPD_NEWFRAME_LENTH          )

/*register AUDIO_SPDIF_PAUSE_PC_PD bit defines、mask defines、value defines*/
#define bAUDIO_SPD_PAUSE_PC            (16)
#define mAUDIO_SPD_PAUSE_PC            (0xFFFF << bAUDIO_SPD_PAUSE_PC          )
#define vAUDIO_SPD_PAUSE_PC            (0xFFFF << bAUDIO_SPD_PAUSE_PC          )

#define bAUDIO_SPD_PAUSE_PD            (0)
#define mAUDIO_SPD_PAUSE_PD            (0xFFFF << bAUDIO_SPD_PAUSE_PD          )
#define vAUDIO_SPD_PAUSE_PD            (0xFFFF << bAUDIO_SPD_PAUSE_PD          )

/*register AUDIO_SPDIF_RUBISH_DATA_LENGTH bit defines、mask defines、value defines*/
#define bAUDIO_SPD_RUBISH_DATA_LEN            (0)
#define mAUDIO_SPD_RUBISH_DATA_LEN            (0xFFFFF << bAUDIO_SPD_RUBISH_DATA_LEN          )
#define vAUDIO_SPD_RUBISH_DATA_LEN            (0xFFFFF << bAUDIO_SPD_RUBISH_DATA_LEN          )

#define bAUDIO_SPD_RUBISH_SET_OVER            (31)
#define mAUDIO_SPD_RUBISH_SET_OVER            (0x01 << bAUDIO_SPD_RUBISH_SET_OVER          )
#define vAUDIO_SPD_RUBISH_SET_OVER            (0x01 << bAUDIO_SPD_RUBISH_SET_OVER          )

/* AUDIO_SPDIF_NEWFRAME_PCMLEN register */
#define bAUDIO_SPD_NEWFRAME_PCMLEN             (0)
#define mAUDIO_SPD_NEWFRAME_PCMLEN             (0xFFFF << bAUDIO_SPD_NEWFRAME_PCMLEN          )
#define vAUDIO_SPD_NEWFRAME_PCMLEN             (0xFFFF << bAUDIO_SPD_NEWFRAME_PCMLEN          )

/* AUDIO_SPDIF_PLAYINGPCMFRAME_INFO register */
#define bAUDIO_SPD_FIFO_STATUS             (24)
#define mAUDIO_SPD_FIFO_STATUS             (0x1F << bAUDIO_SPD_FIFO_STATUS          )
#define vAUDIO_SPD_FIFO_STATUS             (0x1F << bAUDIO_SPD_FIFO_STATUS          )

/*******************************************************************/
/**************************GX3201 AUDIO SPDIF PLAY MODULE******************/
/* audio_play_spdif_ctrl register */
#define AUDIO_SET_SPD_RAW_PLAY_MODE(rp,value)\
	SET_FEILD(  \
			rp->audio_play_spdif_ctrl,    \
			mAUDIO_SPD_RAW_PLAY_MODE,  \
			((value & 0x07) << bAUDIO_SPD_RAW_PLAY_MODE))

#define AUDIO_SPD_RAW_CLEAR_FIFO_STATUS(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPD_RAW_CLEAR_FIFO_STATUS)

#define AUDIO_SPD_RAW_CLEAR_FIFO_STATUS_CLR(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPD_RAW_CLEAR_FIFO_STATUS)

#define AUDIO_SPD_SET_HDMI_SPD_SEL(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HDMI_SPD_SEL)

#define AUDIO_SPD_SET_HDMI_SPD_RAW_SEL(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HDMI_SPD_SEL)

#define AUDIO_SPD_SET_SPDIF_SPD_SEL(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPDIF_SPD_SEL)

#define AUDIO_SPD_SET_SPDIF_SPD_RAW_SEL(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPDIF_SPD_SEL)

#define AUDIO_SPD_SET_HDMI_SOURCE_HBR(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HDMI_SOURCE_SEL)

#define AUDIO_SPD_SET_HDMI_SOURCE_I2S(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HDMI_SOURCE_SEL)

#define AUDIO_SPD_SET_PLAY_MODE_NON_DATA(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HBR_ESA_CUT_MODE)

#define AUDIO_SPD_CLR_PLAY_MODE_NON_DATA(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_HBR_ESA_CUT_MODE)

#define AUDIO_SET_SPD_PLAY_MODE(rp,value)\
	SET_FEILD(  \
			rp->audio_play_spdif_ctrl,    \
			mAUDIO_SPD_PLAY_MODE,  \
			((value & 0x07) << bAUDIO_SPD_PLAY_MODE))

#define AUDIO_GET_SPD_PLAY_MODE(rp)\
	GET_FEILD(  \
			rp->audio_play_spdif_ctrl,    \
			mAUDIO_SPD_PLAY_MODE,  \
			bAUDIO_SPD_PLAY_MODE)

#define AUDIO_SPD_CLEAR_FIFO_STATUS(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPD_CLEAR_FIFO_STATUS)

#define AUDIO_SPD_CLEAR_FIFO_STATUS_CLR(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,     \
			bAUDIO_SPD_CLEAR_FIFO_STATUS)

#define AUDIO_SPD_SET_RESET_STATUS(rp)  \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,      \
			bAUDIO_SPD_SOFT_RESET)

#define AUDIO_SPD_CLEAR_RESET_STATUS(rp)  \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,      \
			bAUDIO_SPD_SOFT_RESET)

#define AUDIO_SPD_RAW_SET_RESET_STATUS(rp)  \
	SET_BIT(    \
			rp->audio_play_spdif_ctrl,      \
			bAUDIO_SPD_RAW_SOFT_RESET)

#define AUDIO_SPD_RAW_CLEAR_RESET_STATUS(rp)  \
	CLR_BIT(    \
			rp->audio_play_spdif_ctrl,      \
			bAUDIO_SPD_RAW_SOFT_RESET)

/* audio_play_spdif_inputdata_info register */
#define AUDIO_SPD_SET_NEW_FRAME(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_inputdata_info,     \
			bAUDIO_SPD_SET_NEW_FRAME)

#define AUDIO_SPD_SET_CDDATA_FORMAT(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,    \
			mAUDIO_SPD_CDDATA_FORMAT,  \
			((value & 0x03) << bAUDIO_SPD_CDDATA_FORMAT))

#define AUDIO_SPD_SAMPLE_DATA_LEN_SEL(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,   \
			sAUDIO_SPD_SAMPLE_DATA_BIT_SIZE,   \
			((value & 0x1F) << bAUDIO_SPD_SAMPLE_DATA_BIT_SIZE))

#define AUDIO_SPD_GET_SAMPLE_DATA_LEN(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,   \
			mAUDIO_SPD_SAMPLE_DATA_BIT_SIZE,   \
			bAUDIO_SPD_SAMPLE_DATA_BIT_SIZE)

#define AUDIO_SPD_CHANNEL_L_SEL(rp,value)\
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,    \
			sAUDIO_SPD_CHANNEL_L_SEL,  \
			((value & 0x0F) << bAUDIO_SPD_CHANNEL_L_SEL))

#define AUDIO_SPD_CHANNEL_R_SEL(rp,value)\
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,    \
			sAUDIO_SPD_CHANNEL_R_SEL,  \
			((value & 0x0F) << bAUDIO_SPD_CHANNEL_R_SEL))

#define AUDIO_GET_SPD_CHANNEL_OUTPUT(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,     \
			mAUDIO_SPD_CHANNEL_SEL,     \
			bAUDIO_SPD_CHANNEL_SEL)

#define AUDIO_SET_SPD_CHANNEL_OUTPUT(rp,value) \
	SET_FEILD(                                \
			rp->audio_play_spdif_inputdata_info,     \
			sAUDIO_SPD_CHANNEL_SEL,     \
			((value & 0xFF) << bAUDIO_SPD_CHANNEL_SEL))

#define AUDIO_SET_SPD_RAW_MUTE(rp, value) \
	SET_FEILD(                            \
			rp->audio_play_spdif_inputdata_info, \
			mAUDIO_SPD_RAW_MUTE,                 \
			(value << bAUDIO_SPD_RAW_MUTE))

#define AUDIO_SPD_SET_SRC_EN(rp) \
	SET_BIT(rp->audio_play_spdif_inputdata_info, bAUDIO_SPD_PCM_DO_SRC_EN)

#define AUDIO_SPD_CLR_SRC_EN(rp) \
	CLR_BIT(rp->audio_play_spdif_inputdata_info, bAUDIO_SPD_PCM_DO_SRC_EN)

#define AUDIO_SPD_SET_SLIENT(rp) \
	SET_BIT(rp->audio_play_spdif_inputdata_info, bAUDIO_SPD_SERIAL_SILENT_END)

#define AUDIO_SET_SPD_PCM_MAGNIFICATION(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,     \
			sAUDIO_SPD_PCM_MAGN,     \
			((value & 0x03) << bAUDIO_SPD_PCM_MAGN))

#define AUDIO_SPD_SAMPLE_DATA_STORE_INTERLACE(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_inputdata_info,     \
			bAUDIO_SPD_PCM_STORE_MODE)

#define AUDIO_SPD_SAMPLE_DATA_STORE_NON_INTERLACE(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_inputdata_info,     \
			bAUDIO_SPD_PCM_STORE_MODE)

#define AUDIO_SPD_SAMPLE_DATA_BIG_ENDIAN(rp) \
	CLR_BIT(    \
			rp->audio_play_spdif_inputdata_info,     \
			bAUDIO_SPD_PCM_DATA_ENDIAN)

#define AUDIO_SPD_SAMPLE_DATA_LITTLE_ENDIAN(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_inputdata_info,     \
			bAUDIO_SPD_PCM_DATA_ENDIAN)

#define AUDIO_SET_SPD_SAMPLE_DATA_ENDIAN(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info, \
			(0x3) << bAUDIO_SPD_PCM_DATA_ENDIAN, \
			(value & 0x3) << bAUDIO_SPD_PCM_DATA_ENDIAN)

#define AUDIO_SPD_CONFIG_PCM_CHANNEL_NUMS(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_inputdata_info,   \
			mAUDIO_SPD_PCM_CHANNEL_NUMS,   \
			((value & 0x0F) << bAUDIO_SPD_PCM_CHANNEL_NUMS))

/* audio_play_spdif_int register */
#define AUDIO_SPDINT_ESA_CUT_CLR(rp)          \
	SET_VAL(    \
			rp->audio_play_spdif_int,  \
			vAUDIO_SPD_ESA_CUT)

#define AUDIO_SPDINT_SET_NEW_PCM_CLR(rp)          \
	SET_VAL(    \
			rp->audio_play_spdif_int,  \
			vAUDIO_SPD_SET_NEW_PCM_INT)

#define AUDIO_SPDINT_GET_NEW_PCM_STATUS(rp)          \
	GET_FEILD(  \
			rp->audio_play_spdif_int,     \
			mAUDIO_SPD_SET_NEW_PCM_INT,     \
			bAUDIO_SPD_SET_NEW_PCM_INT)

#define AUDIO_SPDINT_CD_PLAY_DONE_CLR(rp)          \
	SET_VAL(    \
			rp->audio_play_spdif_int,  \
			vAUDIO_SPD_CD_PLAY_DONE)

#define AUDIO_SPDINT_GET_CD_PLAY_DONE_STATUS(rp)          \
	GET_FEILD(  \
			rp->audio_play_spdif_int,     \
			mAUDIO_SPD_CD_PLAY_DONE,     \
			bAUDIO_SPD_CD_PLAY_DONE)

#define AUDIO_SPDINT_PAUSE_DONE_CLR(rp)          \
	SET_VAL(    \
			rp->audio_play_spdif_int,  \
			vAUDIO_SPD_PAUSE_DONE)

#define AUDIO_SPDINT_STUFF_DONE_CLR(rp)          \
	SET_VAL(    \
			rp->audio_play_spdif_int,  \
			vAUDIO_SPD_STUFF_DONE)

#define AUDIO_SPD_GET_INT_STATUS(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_int,     \
			mAUDIO_SPD_INT_STATUS,     \
			bAUDIO_SPD_INT_STATUS)

#define AUDIO_SPD_CLR_ALL_INT_STATUS(rp) \
	SET_FEILD(                                \
			rp->audio_play_spdif_int,       \
			mAUDIO_SPD_INT_STATUS,                \
			((0xFFFFFFFF) << bAUDIO_SPD_INT_STATUS))

#define AUDIO_GET_SPD_STOP_STATUS(rp)      \
	GET_BIT(    \
			rp->audio_play_spdif_int,  \
			bAUDIO_SPD_STOP_STATUS)

#define AUDIO_SPD_GET_SPD_BUFFER_STAUS(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_int,     \
			mAUDIO_SPD_BUFFER_STATUS,     \
			bAUDIO_SPD_BUFFER_STATUS)

/* SPDIF_INT_EN register */
#define AUDIO_SPDINT_ESA_CUT_ENABLE(rp)      \
	SET_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_ESA_CUT)

#define AUDIO_SPDINT_ESA_CUT_DISABLE(rp)      \
	CLR_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_ESA_CUT)

#define AUDIO_SPDINT_SET_NEW_PCM_ENABLE(rp)      \
	SET_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_SET_NEW_PCM_INT)

#define AUDIO_SPDINT_SET_NEW_PCM_DISABLE(rp)      \
	CLR_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_SET_NEW_PCM_INT)

#define AUDIO_SPDINT_CD_PLAY_DONE_ENABLE(rp)      \
	SET_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_CD_PLAY_DONE)

#define AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(rp)      \
	CLR_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_CD_PLAY_DONE)

#define AUDIO_SPDINT_PAUSE_DONE_ENABLE(rp)          \
	SET_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_PAUSE_DONE)

#define AUDIO_SPDINT_PAUSE_DONE_DISABLE(rp)          \
	CLR_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_PAUSE_DONE)

#define AUDIO_SPDINT_STUFF_DONE_ENABLE(rp)          \
	SET_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_STUFF_DONE)

#define AUDIO_SPDINT_STUFF_DONE_DISABLE(rp)          \
	CLR_BIT(    \
			rp->audio_play_spdif_int_en,  \
			bAUDIO_SPD_STUFF_DONE)

#define AUDIO_SPD_GET_INT_EN_VAL(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_int_en,     \
			mAUDIO_SPD_INT_EN_VAL,     \
			bAUDIO_SPD_INT_EN_VAL)

/* SPDIF_PA_PB register */
#define AUDIO_SPD_SET_PA_PB(rp,value)    \
	SET_VAL(    \
			rp->audio_play_spdif_pa_pb,     \
			value)

/* SPDIF_PC_PD register */
#define AUDIO_SPD_SET_PC(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_pc_pd,   \
			mAUDIO_SPD_PC,   \
			((value & 0xFFFF) << bAUDIO_SPD_PC))

#define AUDIO_SPD_SET_PD(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_pc_pd,   \
			mAUDIO_SPD_PD,   \
			((value & 0xFFFF) << bAUDIO_SPD_PD))

/* audio_play_spdif_cl1 register */
#define AUDIO_SET_CHANNEL_STATUS_CL1(rp,value)  \
	SET_VAL(    \
			rp->audio_play_spdif_cl1,     \
			value)

/* audio_play_spdif_cl2 register */
#define AUDIO_SET_CHANNEL_STATUS_CL2(rp,value)  \
	SET_VAL(    \
			rp->audio_play_spdif_cl2,     \
			value)

/* audio_play_spdif_cr1 register */
#define AUDIO_SET_CHANNEL_STATUS_CR1(rp,value)  \
	SET_VAL(    \
			rp->audio_play_spdif_cr1,     \
			value)

/* audio_play_spdif_cr2 register */
#define AUDIO_SET_CHANNEL_STATUS_CR2(rp,value)  \
	SET_VAL(    \
			rp->audio_play_spdif_cr2,     \
			value)

/* SPDIF_LAT_PAUSE register */
#define AUDIO_SPD_SET_PAUSE_NUM(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_lat_pause,   \
			mAUDIO_SPD_PAUSE_NUM,   \
			((value & 0xFFFF) << bAUDIO_SPD_PAUSE_NUM))

/* SPDIF_DATA_PAU_LEN register */
#define AUDIO_SPD_SET_PAUSE_LENTH(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_data_pau_len,   \
			mAUDIO_SPD_PAUSE_LENTH,   \
			((value & 0xFFFF) << bAUDIO_SPD_PAUSE_LENTH))

#define AUDIO_SPD_SET_DATA_LENTH(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_data_pau_len,   \
			mAUDIO_SPD_DATA_LENTH,   \
			((value & 0xFFFF) << bAUDIO_SPD_DATA_LENTH))

#define AUDIO_SPD_GET_DATA_LENTH(rp)    \
	GET_FEILD(  \
			rp->audio_play_spdif_data_pau_len,   \
			mAUDIO_SPD_DATA_LENTH,   \
			bAUDIO_SPD_DATA_LENTH)

/* AUDIO_SPDIF_NEWFRAME_LEN register */
#define AUDIO_SPD_SET_NEWFRAME_LENTH(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_newframe_len,   \
			mAUDIO_SPD_NEWFRAME_LENTH,   \
			((value & 0xFFFF) << bAUDIO_SPD_NEWFRAME_LENTH))

/* AUDIO_SPDIF_PAUSE_PC_PD register */
#define AUDIO_SPD_PAUSE_SET_PC(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_pause_pc_pd,   \
			mAUDIO_SPD_PAUSE_PC,   \
			((value & 0xFFFF) << bAUDIO_SPD_PAUSE_PC))

#define AUDIO_SPD_PAUSE_SET_PD(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_pause_pc_pd,   \
			mAUDIO_SPD_PAUSE_PD,   \
			((value & 0xFFFF) << bAUDIO_SPD_PAUSE_PD))

/* audio_play_spdif_NEWFRAME_start_addr register */
#define AUDIO_SPD_NEWFRAME_START_ADDR(rp,value)    \
	SET_VAL(rp->audio_play_spdif_newframe_start_addr, (value & AUDIO_BUF_ADDR_MASK))


/* audio_play_spdif_NEWFRAME_end_addr register */
#define AUDIO_SPD_NEWFRAME_END_ADDR(rp,value)    \
	SET_VAL(rp->audio_play_spdif_newframe_end_addr, (value & AUDIO_BUF_ADDR_MASK))

/* audio_play_spdif_buf_start_addr register */
#define AUDIO_SET_SPD_BUFFER_START_ADDR(rp,value) \
	SET_VAL(rp->audio_play_spdif_buffer_start_addr, (value & AUDIO_BUF_ADDR_MASK))

#define AUDIO_GET_SPD_BUFFER_START_ADDR(rp) \
	GET_VAL(rp->audio_play_spdif_buffer_start_addr)

/* audio_play_spdif_buf_size register */
#define AUDIO_SET_SPD_BUFFER_SIZE(rp,value) \
	SET_VAL(rp->audio_play_spdif_buffer_size, value)

#define AUDIO_GET_SPD_BUFFER_SIZE(rp) \
	GET_VAL(rp->audio_play_spdif_buffer_size)

/* audio_play_spdif_rubish_data_saddr register */
#define AUDIO_SPD_SET_RUBISH_DATA_SADDR(rp,value) \
	SET_VAL(rp->audio_play_spdif_rubish_data_saddr, (value & AUDIO_BUF_ADDR_MASK))

/* audio_play_spdif_rubish_length register */
#define AUDIO_SPD_SET_RUBISH_LENGTH(rp,value) \
	SET_FEILD(  \
			rp->audio_play_spdif_rubish_length,   \
			mAUDIO_SPD_RUBISH_DATA_LEN,   \
			((value & 0xFFFFF) << bAUDIO_SPD_RUBISH_DATA_LEN))

#define AUDIO_SPD_SET_RUBISH_OVER(rp) \
	SET_BIT(    \
			rp->audio_play_spdif_rubish_length,  \
			bAUDIO_SPD_RUBISH_SET_OVER)

/* AUDIO_SPDIF_PLAYINGPCMFRAME_INFO register */
#define AUDIO_SPD_GET_FIFO_STATUS(rp) \
	GET_FEILD(  \
			rp->audio_play_spdif_playingpcmframe_info,   \
			mAUDIO_SPD_FIFO_STATUS,   \
			bAUDIO_SPD_FIFO_STATUS)

/* AUDIO_PLAY_SPDIF_BUFFER_READ_ADDR register */
#define AUDIO_GET_SPD_BUFFER_READ_ADDR(rp) \
	GET_VAL(rp->audio_play_spdif_buffer_read_addr)

#define AUDIO_GET_SPD_NEWFRAME_END_ADDR(rp) \
	GET_VAL(rp->audio_play_spdif_playingframe_end_addr)

/* AUDIO_SPDIF_NEWFRAME_PCMLEN register */
#define AUDIO_SPD_SET_NEWFRAME_PCMLEN(rp,value)    \
	SET_FEILD(  \
			rp->audio_play_spdif_newframe_pcmlen,   \
			mAUDIO_SPD_NEWFRAME_PCMLEN,   \
			((value & 0xFFFF) << bAUDIO_SPD_NEWFRAME_PCMLEN))

// ############### pegasus new function #######################
static unsigned int mask[33] = {
	0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
	0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

#define AOUT_REG_SET_VAL(reg, value) do {                                \
	(*(volatile unsigned int *)reg) = value;                             \
} while(0)

#define AOUT_REG_GET_VAL(reg)                                            \
	(*(volatile unsigned int *)reg)

#define AOUT_REG_SET_FIELD(reg, val, c, offset) do {                     \
	unsigned int Reg = *(volatile unsigned int *)reg;                    \
	Reg &= ~((mask[c])<<(offset));                                       \
	Reg |= ((val)&(mask[c]))<<(offset);                                  \
	(*(volatile unsigned int *)reg) = Reg;                               \
} while(0)

#define AOUT_REG_GET_FIELD(reg, c, offset)                               \
	(((*(volatile unsigned int *)reg)>>(offset))&(mask[c]))

#define AUDIO_SPD_ENABLE_CACHE_FIFO(rp, value)                           \
		AOUT_REG_SET_FIELD(&(rp->audio_play_spdif_ctrl), value, 1, 24);

#define AUDIO_SPD_SET_CACHE_FIFO_COUNT(rp, value) do {                     \
	AOUT_REG_SET_FIELD(&(rp->audio_play_spdif_cd_fifo_ctrl), 0x1,   1, 5); \
	AOUT_REG_SET_FIELD(&(rp->audio_play_spdif_cd_fifo_ctrl), 0x1,   1, 4); \
	AOUT_REG_SET_FIELD(&(rp->audio_play_spdif_cd_fifo_ctrl), value, 4, 0); \
} while(0)

#define AUDIO_SPD_GET_CACHE_FIFO_COUNT(rp) \
	AOUT_REG_GET_FIELD(&(rp->audio_play_spdif_cd_fifo_ctrl), 4,  8);

#define AUDIO_SPD_RAW_GET_CACHE_FIFO_COUNT(rp) \
	AOUT_REG_GET_FIELD(&(rp->audio_play_spdif_cd_fifo_ctrl), 4,  0);

#define AUDIO_SPD_SET_CACHE_FIFO_INT_EN(rp, value) \
	AOUT_REG_SET_FIELD(&(rp->audio_play_spdif_int_en), value, 1, 18);

#define AUDIO_SPD_GET_CACHE_FIFO_INT_EN(rp) \
	AOUT_REG_GET_FIELD(&(rp->audio_play_spdif_int_en), 1,  18);

#define AUDIO_SPD_GET_CACHE_FIFO_INT_ST(rp) \
	AOUT_REG_GET_FIELD(&(rp->audio_play_spdif_int),    1,  18);

#define AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(rp) \
	AOUT_REG_SET_VAL(&(rp->audio_play_spdif_int), 0x1<<18);

#endif

