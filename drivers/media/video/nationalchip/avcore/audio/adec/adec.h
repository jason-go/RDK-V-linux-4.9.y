#ifndef __ADEC_H__
#define __ADEC_H__

#define AUDIO_DECODER_MAX (2)

typedef enum {
	PCM_SKIP_FRAME,
	PCM_DEC_FRAME,
	PCM_FILL_DATA,
	PCM_REAPT_FRAME,

	I2S_PLAY_PCM,
	SPD_PLAY_PCM
} PCM_DataOperate;

typedef enum {
	SPD_PARSE_FRAME,
	SPD_FILL_DATA,
	SPD_FIND_SYNC,

	SPD_PLAY_CDDATA
} SPD_DataOperate;

typedef enum {
	CVT_ENC_FRAME,
	CVT_FILL_DATA,
} CVT_DataOperate;

typedef enum {
	MOVE_ALL = 0,
	MOVE_128KHZ
} Audio_Move;

struct dolby_info_header {
	int b_exist     ; //-b
	int pcmwordtype ;
	int c_exist     ; //-c
	int kmode       ;
	int d_exist     ; //-d
	int f_exist     ; //-f
	int f           ;
	int dp_exist    ;
	int p           ;
	int h_exist     ; //-h
	int i_exist     ; //-i
	int k_exist     ; //-k
	int compmode    ;
	int l_exist     ; //-l
	int outlfe      ;
	int m_exist     ; //-m
	int mr_exist    ; //-mr
	int m           ;
	int n_exist     ; //-n
	int outnchans   ;
	int o_exist     ; //-o
	int oc_exist    ;
	int op_exist    ;
	int od_exist    ;
	int p_exist     ; //-p
	int pcmscale    ; //((x + 0.005) * 100)
	int q_exist     ; //-q
	int r_exist     ; //-r
	int framestart  ;
	int frameend    ;
	int s_exist     ; //-s
	int stereomode  ;
	int u_exist     ; //-u
	int dualmode    ;
	int v_exist     ; //-v
	int verbose     ;
	int w_exist     ; //-w
	int upsample    ;
	int x_exist     ; //-x
	int dynscalehigh; //((x + 0.005) * 100)
	int y_exist     ; //-y
	int dynscalelow ; //((x + 0.005) * 100);
	int z_exist     ; //-z
	int zp_exist    ;
	int zd_exist    ;
	int num_exist   ; //how much -0~7
	int chanval[8]  ;
	int routing_index[8];
	int byterev; //1--little endian,
	int ddoutbaseaddr;
	int ddoutlength;
	int ddoutid;
} ;

struct audiodec_params_info {
#define SRC_KEY_LEN (SCR_MAP_LEN+SCR_MAP_KEY_LEN+SCR_RAND_DATA_LEN+SCR_CHIP_ID_LEN+SCR_SCR_KEY_LEN)
	unsigned int or_clk;
	unsigned int dataless_wait_time;
	unsigned int dec_volume;
	unsigned int header_addr; //dolby, real
	unsigned int frame_nums;
	unsigned int extern_downmix_en;
	unsigned int dec_volume2;
	unsigned int ad_ctrl_vol;
	unsigned int dec_mov_ac3_data;
	unsigned int extern_mono_en;
	unsigned int anti_error_dec;
	unsigned char scr_key[SRC_KEY_LEN];
	unsigned char dec_key[DEC_KEY_LEN];
	unsigned int convert_en;
	unsigned int aac_parse_en;
	int          mag_value;
	int          mag_value_ad;
	int          aac_downsrc_en;
	int          des_key_result;
	unsigned int pcm_sample_rate;
	unsigned int pcm_channel_num;
	unsigned int pcm_bit_width;
	unsigned int pcm_interlace;
	unsigned int pcm_endian;
	unsigned int pcm_float_en;
	unsigned int or_key_type;
	unsigned int or_chip_type;
	unsigned int encode_info[11];
	unsigned int dec_vol_tab_set_en;
	unsigned int dec_vol_tab[36];
};

struct gxav_apts_comparectrl {
	unsigned int  esabuf_start_addr;
	unsigned int  esabuf_end_addr;
	unsigned int  esa_buf_id;
	unsigned int  ptsbuf_start_addr;
	unsigned int  ptsbuf_end_addr;//固定的
	unsigned int  esaStartAddr;
	unsigned int  esaEndAddr;
	unsigned int  pts_buf_id;
	unsigned int  pts_buf_target_addr;
	unsigned int  pts_buf_end_addr;
	unsigned int  esa_buf_count;
	unsigned int  total_esa_buf_count;
	unsigned int  pts_esa_buf_count;
	unsigned int  pts_sdram_addr_save;
	unsigned int  frame_size;
	unsigned int  bitrate;
	unsigned int  error;
	unsigned int  esa_buf_size;

	unsigned int  ad_ctrl_invaild;
	unsigned int  ad_ctrl_esa_buf_addr;
	unsigned int  ad_ctrl_fade_byte;
	unsigned int  ad_ctrl_pan_byte;

	//fix pts params
	int           last_pts;
	unsigned int  pcm_len;
	unsigned int  sample_fre;
	unsigned int  unit_time;     //ms
	unsigned int  unit_time_dis; //1/1000(ms)
};

struct gxav_audiodec {
	int                 dec_id;
#define UNUSED 0
#define USED   1
	int                 dec_used;
	int                 dec_type;
	int                 dec_mode;
	int                 dec_state;
	int                 dec_errno;
	int                 stream_type;
	int                 stream_pid;
#define DEC_RESTART  1
#define DEC_CONTINUE 0
	int                 decode_framelength;
	unsigned int        decode_samplefre;
	int                 decode_channelnum;
	int                 decode_bitrate;
	int                 decode_err;
	int                 decode_esa_empty; // 标记esa_fifo数据空，供esa_w_callbk使用
	int                 decode_pcm_full;  // 标记pcm_fifo(或缓存)数据满，供pcm_r_callbk使用
	unsigned int        decode_need_data_num;
	int                 decode_esa_id;
	int                 decode_pts_id;
	unsigned int        decode_esa_start_addr;
	unsigned int        decode_esa_size;
	unsigned int        decode_pts_size;
	unsigned int        decode_pts_read_addr;//相对地址
	unsigned int        decode_esa_read_addr;//相对地址
	unsigned int        decode_esa_writed_addr;//相对地址
	unsigned int        decode_pts_writed_addr;
	PCM_DataOperate     decode_data_operate;
	struct gxav_apts_comparectrl decode_apts_ctrl;
	int                 decode_frame_num;    //写入一次数据要求解的帧数
	int                 decode_frame_param;
	int                 decode_restart_continue;
	int                 decode_ctrlinfo_enable;
	int                 decode_first_flag;
	unsigned int        decode_frame_cnt;

	unsigned int        bypass_samplefre;
	int                 bypass_bitrate;
	int                 bypass_err;
	int                 bypass_esa_id;
	int                 bypass_pts_id;
	unsigned int        bypass_esa_start_addr;
	unsigned int        bypass_esa_size;
	unsigned int        bypass_esa_read_addr;//相对地址
	unsigned int        bypass_esa_writed_addr;//相对地址
	unsigned int        bypass_frame_type;
	unsigned int        bypass_sample_rate;
	unsigned int        bypass_frame_cnt;
	SPD_DataOperate     bypass_data_operate;
	int                 bypass_esa_empty;
	int                 bypass_fifo_full;
	unsigned int        bypass_need_data_num;
	int                 bypass_first_flag;
	int                 out_bypass_id;
	int                 out_bypass_size;
	struct gxav_apts_comparectrl bypass_apts_ctrl;

	int                 convert_err;
	int                 convert_esa_id;
	int                 convert_pts_id;
	unsigned int        convert_esa_start_addr;
	unsigned int        convert_esa_size;
	unsigned int        convert_pts_size;
	unsigned int        convert_pts_read_addr;//相对地址
	unsigned int        convert_esa_read_addr;//相对地址
	unsigned int        convert_esa_writed_addr;//相对地址
	unsigned int        convert_pts_writed_addr;
	CVT_DataOperate     convert_data_operate;
	int                 convert_esa_empty;
	int                 convert_fifo_full;
	unsigned int        convert_need_data_num;
	int                 convert_first_flag;
	struct gxav_apts_comparectrl convert_apts_ctrl;
	unsigned int        convert_sample_rate;
	unsigned int        convert_frame_cnt;

	int                 out_pcm_id;            //Decode data fifo
	unsigned int        out_pcm_start_addr;
	unsigned int        out_pcm_size;
	unsigned int        out_pcm_writed_addr;

	unsigned int        out_degrade_id;        //EAC3 degrade data fifo
	unsigned int        out_degrade_start_addr;
	unsigned int        out_degrade_size;
	unsigned int        out_degrade_writed_addr;

	unsigned int        out_convert_id;        //Convert data fifo
	unsigned int        out_convert_start_addr;
	unsigned int        out_convert_size;
	unsigned int        out_convert_writed_addr;

	//两路数据结束时分别需要上层告知
	int                 update;
	unsigned int        pcm_frame_len;
	unsigned int        or_virt_addr;
	unsigned int        or_phys_addr;
	unsigned int        firmware_addr;
	unsigned int        firmware_size;
	unsigned int        firmware_id;
	unsigned int        dolby_firmware_support_dolby_plus;
	unsigned int        all_firmware_supoort_dolby;
	unsigned int        all_firmware_test_dolby;
	unsigned int        ormem_start;
	unsigned int        ormem_size;
	unsigned int        regmem_virt_addr;
	unsigned int        regmem_virt_size;

	int                 audioout_id;
	int                 downmix;
	int                 downmix_active;
	int                 volumedb;
	int                 volumedb_active;
	int                 boostdb;
	int                 boostdb_active;
	int                 mono_en;
	int                 mono_en_active;
	int                 enough_data;

	int                        pcm_active;
	GxAudioDecProperty_PcmInfo pcm_info;

	void *dec_header;
	void *dec_params;
	struct audiodec_regs* addec_reg;
	struct gxav_audio_syncinfo*    syncinfo;
	struct gxav_audio_convertinfo* convertinfo;

	struct audiodec_regs* audiodec_reg;
	struct audiodec_fifo* decode_fifo;
	struct audiodec_fifo* convert_fifo;
	unsigned int decode_fifo_num;
	unsigned int decode_channel_buffer;
	unsigned int decode_channel_size;
	unsigned int max_channel_num;
	unsigned int convert_channel_buffer;
	unsigned int convert_channel_size;

	GxAudioDecProperty_Debug  debug;

	struct gxfifo* pcm_fifo;
	struct gxfifo* degrade_fifo;
	struct gxfifo* bypass_fifo;
	struct gxfifo* encode_fifo;
	struct gxfifo* ctrlinfo_fifo;

	void* inode;
	struct audiodec_hal_ops *ops;
};

struct gxav_audio_syncinfo { // 成员顺序不能变
	unsigned int parse_num;            // 一次解析的帧数
	unsigned int bit_rate;
	unsigned int frame_cur_len;        // 一帧当前length
	unsigned int frame_ave_len;        // 一帧平均length
	unsigned int frame_first_flg;      // 首帧标志
	unsigned int spd_counter_notuse;   // 主cpu去维护此变量！！！
	unsigned int frame_blk_num;
	unsigned int stream_big_or_little; // 0:big 1:16_little 2 32_little
	unsigned int stream_data_encry;    // 0:normal 1:nds
	unsigned int frame_type;
	unsigned int sampling_rate;

	unsigned int frame_r_ptr;          // 缓存中帧的读指针
	unsigned int frame_w_ptr;          // 缓存中帧的写指针
	unsigned int frame_cnt;            // 缓存中帧数 主cpu去维护此变量！！！

	unsigned int frame_start_addr;     // 相对地址
	unsigned int frame_end_addr;
	unsigned int frame_size;
	unsigned int frame_spdbuftime;     // 结束地址对应的循环次数
	unsigned int rubish_s_addr;
	unsigned int frame_offset;         // 帧之间冗余数据
	unsigned int frame_bsmod;          // ec3时有用

	unsigned int out_bypass_start_addr;
	unsigned int out_bypass_size;
	unsigned int new_frame_start_addr;
	unsigned int new_frame_end_addr;
	unsigned int out_bypass_id;
	unsigned int frame_add_num;
	unsigned int esa_start_addr;
	unsigned int movetype;
	unsigned int frame_bytes;
	unsigned int rubish_start_addr;
	unsigned int rubish_data_len;
	unsigned int unsupport_streamtype;
	unsigned int bypass_code_type;
};

struct gxav_audio_convertinfo { // 成员顺序不能变
	unsigned int convert_num;
	unsigned int channel_num;
	unsigned int bit_rate;
	unsigned int sampling_rate;
	unsigned int stream_big_or_little;
	unsigned int frame_size;
	unsigned int interlace;  //1:interlace 0:uninterlace
	unsigned int bsmod;
};

struct audiodec_fifo {
	unsigned char   channel_id;
	unsigned char   buffer_id;
	unsigned int    buffer_start_addr;
	unsigned int    buffer_end_addr;

	unsigned char   channel_pts_id;
	unsigned char   pts_buffer_id;
	unsigned int    pts_start_addr;
	unsigned int    pts_end_addr;

	struct gxav_channel* channel;
	int            direction;
	int            pin_id;
};

struct audiodec_hal_ops {
	int (*init)     (void);
	int (*uninit)   (void);
	int (*open)     (struct gxav_audiodec *audiodec);
	int (*close)    (struct gxav_audiodec *audiodec);
	int (*link)     (struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo);
	int (*unlink)   (struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo);
	int (*config)   (struct gxav_audiodec *audiodec, GxAudioDecProperty_Config *config);
	int (*run)      (struct gxav_audiodec *audiodec);
	int (*stop)     (struct gxav_audiodec *audiodec);
	int (*pause)    (struct gxav_audiodec *audiodec);
	int (*resume)   (struct gxav_audiodec *audiodec);
	int (*update)   (struct gxav_audiodec *audiodec);
	int (*get_pts)  (struct gxav_audiodec *audiodec, GxAudioDecProperty_Pts   *pts);
	int (*get_state)(struct gxav_audiodec *audiodec, GxAudioDecProperty_State *state);
	int (*get_error)(struct gxav_audiodec *audiodec, GxAudioDecProperty_Errno *err_no);
	int (*get_cap)        (GxAudioDecProperty_Capability *cap);
	int (*get_outinfo)    (struct gxav_audiodec *audiodec, GxAudioDecProperty_OutInfo    *type);
	int (*get_frameinfo)  (struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameInfo  *frameinfo);
	int (*get_frame)      (struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameData  *frame);
	int (*get_fifo)       (struct gxav_audiodec *audiodec, GxAvPinId pin_id, struct gxfifo **fifo);
	int (*get_max_channel)(struct gxav_audiodec *audiodec, unsigned int *max_channel);
	int (*clr_frame)      (struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameData  *frame);
	int (*write_ctrlinfo) (struct gxav_audiodec *audiodec, GxAudioDecProperty_ContrlInfo *ctrlinfo);
	int (*decode_key)     (struct gxav_audiodec *audiodec, GxAudioDecProperty_DecodeKey  *key);
	int (*get_key)        (struct gxav_audiodec *audiodec, GxAudioDecProperty_GetKey     *key);
	int (*get_dolbytype)  (void);
	int (*set_volume)     (struct gxav_audiodec *audiodec, int volumedb);
	int (*set_ad_ctrlinfo)(struct gxav_audiodec *audiodec, int enable);
	int (*set_mono)       (struct gxav_audiodec *audiodec, int mono_en);
	int (*set_pcminfo)    (struct gxav_audiodec *audiodec, GxAudioDecProperty_PcmInfo     *info);
	int (*boost_volume)   (struct gxav_audiodec* audiodec, GxAudioDecProperty_BoostVolume *boost);
	int (*irq)            (struct gxav_audiodec *audiodec);
	int (*set_dump)       (struct gxav_audiodec *audiodec, int enable);
	int (*get_dump)       (struct gxav_audiodec *audiodec, GxAvDebugDump *dump);
	int (*support_ad)     (GxAudioDecProperty_SupportAD  *sup);
};

extern struct gxav_audiodec* gxav_audiodec_find_instance(int sub);
extern int gxav_audiodec_init   (struct gxav_module_ops* ops);
extern int gxav_audiodec_uninit (void);
extern int gxav_audiodec_open   (int id);
extern int gxav_audiodec_close  (int id);
extern int gxav_audiodec_link   (int id, struct audiodec_fifo *fifo);
extern int gxav_audiodec_unlink (int id, struct audiodec_fifo *fifo);
extern int gxav_audiodec_config (int id, GxAudioDecProperty_Config  *config);
extern int gxav_audiodec_outinfo(int id, GxAudioDecProperty_OutInfo *outinfo);
extern int gxav_audiodec_run    (int id);
extern int gxav_audiodec_stop   (int id);
extern int gxav_audiodec_pause  (int id);
extern int gxav_audiodec_pause  (int id);
extern int gxav_audiodec_resume (int id);
extern int gxav_audiodec_update (int id);
extern int gxav_audiodec_state  (int id, GxAudioDecProperty_State *state);
extern int gxav_audiodec_pts    (int id, GxAudioDecProperty_Pts *pts);
extern int gxav_audiodec_err    (int id, GxAudioDecProperty_Errno *err_no);
extern int gxav_audiodec_cap    (int id, GxAudioDecProperty_Capability *cap);
extern int gxav_audiodec_support_ad     (int id, GxAudioDecProperty_SupportAD *sup);
extern int gxav_audiodec_boost_volume   (int id, GxAudioDecProperty_BoostVolume *boost);
extern int gxav_audiodec_decode_key     (int id, GxAudioDecProperty_DecodeKey *key);
extern int gxav_audiodec_get_key        (int id, GxAudioDecProperty_GetKey *key);
extern int gxav_audiodec_write_ctrlinfo (int id, GxAudioDecProperty_ContrlInfo *ctrlinfo);
extern int gxav_audiodec_frameinfo      (int id, GxAudioDecProperty_FrameInfo  *frame_info);
extern int gxav_audiodec_irq(int irq, int (*callback)(unsigned int event, void *priv), void *priv);
extern int gxav_audiodec_set_pcminfo(int id, GxAudioDecProperty_PcmInfo *info);
extern int gxav_audiodec_set_debug  (int id, GxAudioDecProperty_Debug *debug);
extern int gxav_audiodec_set_dump   (int id, int enable);
extern int gxav_audiodec_get_dump   (int id, GxAvDebugDump *dump);

#endif

