#ifndef __GX3211_AKLM_REG_H__
#define __GX3211_AKLM_REG_H__

typedef union {
    unsigned int value;
    struct {
        unsigned tri_des             : 1;
        unsigned encrypt             : 1;
        unsigned key_rdy             : 1;
        unsigned aes_mode            : 2;
        unsigned work_mode           : 2;
        unsigned opt                 : 3;
        unsigned M                   : 6;
        unsigned big_endian          : 1;
        unsigned gx3211_key_sel      : 3;
        unsigned des_key_low         : 1;
        unsigned residue             : 2;
        unsigned short_msg           : 2;
        unsigned des_key_multi_low   : 1;
        unsigned memory_key_read_en  : 1;
        unsigned memory_key_low      : 1;
        unsigned des_iv_sel          : 1;
        unsigned flash_encrypt       : 1;
    } bits;
} rMTC_CTRL1;

typedef union {
    unsigned int value;
    struct {
        unsigned des_ready           : 1;
        unsigned aes_ready           : 1;
        unsigned multi2_ready        : 1;
        unsigned sms4_ready          : 1;
        unsigned nds_ready           : 1;
        unsigned memory_ready        : 1;
    } bits;
} rMTC_CTRL2;

typedef union {
    unsigned int value;
    struct {
        unsigned des_rst           : 1;
        unsigned aes_rst           : 1;
        unsigned sms4_rst          : 1;
        unsigned multi2_rst        : 1;
        unsigned memory_rst        : 1;
        unsigned ctr_rst           : 1;
        unsigned sdc_rst           : 1;
    } bits;
} rMTC_RST;

typedef union {
    unsigned int value;
    struct {
        unsigned data_finish            : 1;
        unsigned k3_finish              : 1;
        unsigned aes_round_finish       : 1;
        unsigned tdes_round_finish      : 1;
        unsigned err_operate_nds        : 1;
        unsigned m2m_operate_illegal    : 1;
        unsigned memory_read_finish     : 1;
        unsigned otp_flash_finish : 1;
    } bits;
} rMTC_INT;

typedef union {
    unsigned int value;
    struct {
        unsigned cw_en            : 1;
        unsigned cw_round_1       : 1;
        unsigned cw_round_2       : 1;
        unsigned cw_round_3       : 1;
        unsigned cw_round_3_type  : 2;
        unsigned cw_round_3_key   : 2;
        unsigned cw_aes_HL_sel    : 1;
        unsigned multi_key_en     : 1;
        unsigned cw_round_0       : 1;
        unsigned cw_round_0_valid : 1;
        unsigned cw_round_4       : 1;
        unsigned cw_round_5       : 1;
    } bits;
} rMTC_CW_SEL;

typedef union {
    unsigned int value;
    struct {
        unsigned enable           : 1;
        unsigned DSK_ready        : 1;
        unsigned DCK_ready        : 1;
        unsigned DCW_ready        : 1;
        unsigned ca_key_length    : 1;
        unsigned otp_key_length   : 1;
        unsigned VCW_ready        : 1;
        unsigned MCW_ready        : 1;
    } bits;
} rMTC_CA_MODE;

typedef struct {
    rMTC_CTRL1      ctrl1;                        // 0x00
    unsigned int    reserved11[(0x1c-0x00)/4-1];  // 0x1c
    unsigned int    m2mr_addr;                    // 0x1c
    unsigned int    m2mw_addr;                    // 0x20
    unsigned int    data_len;                     // 0x24
    rMTC_CTRL2      ctrl2;                        // 0x28
    unsigned int    reserved22[(0x34-0x28)/4-1];  // 0x2c~34
    unsigned int    counter[4];                   // 0x34
    unsigned int    iv1[4];                       // 0x44
    unsigned int    reserved_0[2];                // 0x54
    rMTC_CA_MODE    ca_mode;                      // 0x5c
    rMTC_RST        rst_ctrl;                     // 0x60
    unsigned int    reserved_1[(0x90-0x60)/4-1];  // 0x64~0x90
    unsigned int    ca_addr;                      // 0x90
    unsigned int    reserved_2[(0x120-0x90)/4-1]; // 0x94~0x120
    rMTC_CW_SEL     ca_cw_select;                 // 0x120
    unsigned int    ca_nonce[4];                  // 0x124
    unsigned int    ca_DA[4];                     // 0x134-0x140
    unsigned int    reserved_3[(0x160-0x140)/4-1];// 0x144~0x160
    rMTC_INT        isr_en;                       // 0x160
    rMTC_INT        isr;                          // 0x164
    unsigned int    reserved_4[(0x200-0x164)/4-1];// 0x168~0x200
    unsigned int    ca_DSK[8];                    // 0x200
    unsigned int    ca_DCK[8];                    // 0x220
    unsigned int    ca_DCW[8];                    // 0x240
    unsigned int    ca_VCW[8];                    // 0x260
    unsigned int    ca_MCW[8];                    // 0x280
} Gx3211KLMReg;

#endif
