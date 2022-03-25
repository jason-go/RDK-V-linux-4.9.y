/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    gx3113c_mtc.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */


#include "mtc_hal.h"
#include "mtc_module.h"
#include "gx3113c_reg_mtc.h"

#define MIN_ALIGNMENT_NUM              (256)
#define SYS_MALLOC_ALIGNMENT_LEN       (32)
#define M2M_MALLOC_ALIGNMENT(x)  (x + SYS_MALLOC_ALIGNMENT_LEN - (x % SYS_MALLOC_ALIGNMENT_LEN))

#define ASSERT(ret)  {if(ret<0) gx_printf("error \n");}

MTC_Regs_t  *gx3113c_MTC_reg = NULL ;

int gx3113c_mtc_hal_UpdateKey(unsigned int m2m_key_table_num)
{
	int key_full ;

	key_full = MTC_KEY_TABLE_IS_FULL(gx3113c_MTC_reg);

	if (key_full)
	{
		MTC_KEY_TABLE_CLEAR_FULL(gx3113c_MTC_reg);
	}

	MTC_DONOT_UPDATE_KEY(gx3113c_MTC_reg);

	MTC_DO_UPDATE_KEY(gx3113c_MTC_reg)   ;           // begin to update the key from nds interface

	MTC_WRITE_KEY_ADDR(gx3113c_MTC_reg, m2m_key_table_num)  ;  // write the key addr (0~7)to this addr 

	return 0 ;
}


int gx3113c_mtc_hal_GetKeyWriteDoneEvent(unsigned int *pEvent)
{
	*pEvent = MTC_KEY_WRITE_DONE(gx3113c_MTC_reg);

	return 0 ;
}

int gx3113c_mtc_hal_Key_Ready(unsigned char bEnable)
{
	if ( bEnable )
	{
		MTC_KEY_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_KEY_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

int gx3113c_mtc_hal_NDS_Mode_AES_DES_Ready(unsigned char bEnable)
{
	if ( bEnable )
	{
		MTC_NDS_MODE_AES_DES_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_NDS_MODE_AES_DES_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

int gx3113c_mtc_hal_ControlUnit_Write_Enable(unsigned char bEnable)
{
	if ( bEnable )
	{
		MTC_CONTROL_UNIT_WRITE_ENABLE_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CONTROL_UNIT_WRITE_ENABLE_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

int gx3113c_mtc_hal_NDS_Control_Set(unsigned char e_d,unsigned char algorithm,unsigned char opition,
							unsigned char residue,unsigned char shortmessage)
{

	MTC_NDS_CONTROL_SET(gx3113c_MTC_reg, algorithm, opition, residue, shortmessage, e_d);

	return 0 ;
}

int gx3113c_mtc_hal_KeyTableSelectEnable(unsigned char bEnable)
{
	if ( bEnable )
	{
		MTC_TABLE_KEY_SELECT_ENABLE_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_TABLE_KEY_SELECT_ENABLE_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

int gx3113c_mtc_hal_KeyTableSelect(unsigned int m2m_key_table_num)
{
	MTC_KEY_TABLE_SELECT(gx3113c_MTC_reg,m2m_key_table_num);

	return 0 ;
}

int gx3113c_mtc_hal_ControlUnitWriteKeyAddr(unsigned int m2m_key_table_num)
{
	MTC_CONTROL_UNIT_WRITE_KEY_ADDR(gx3113c_MTC_reg, m2m_key_table_num);

	return 0 ;
}

int gx3113c_mtc_hal_SelectKeyMode(unsigned int m2m_key_mode)
{
	MTC_KEY_MODE_SELECT(gx3113c_MTC_reg, m2m_key_mode);

	return 0 ;
}


int gx3113c_mtc_hal_SelectKeyFromInterface(unsigned int m2m_key_from_interface)
{
	MTC_KEY_SELECT_FROM_INTERFACE(gx3113c_MTC_reg, m2m_key_from_interface);

	return 0 ;
}

int gx3113c_mtc_hal_SetCounter(unsigned char *pCounter)
{
	int i = 0 ;

	for(i=0;i<4;i++)
	{
		MTC_SET_COUNTER(gx3113c_MTC_reg, i,
		((pCounter[i*4+0]<<24) | (pCounter[i*4+1]<<16) | (pCounter[i*4+2]<<8) | pCounter[i*4+3] ));

	}

	return 0 ;
}

int gx3113c_mtc_hal_SetIV1(unsigned char *pIV1,unsigned int bSelect_High)
{
	int i = 0 ;

	if ( bSelect_High )
	{
		for(i=0;i<2;i++)
		{
			MTC_SET_IV1(gx3113c_MTC_reg, (2+i),
					((pIV1[i*4+0]<<24) | (pIV1[i*4+1]<<16) | (pIV1[i*4+2]<<8) | pIV1[i*4+3] ));
		}
	}
	else
	{
		for(i=0;i<4;i++)
		{
			MTC_SET_IV1(gx3113c_MTC_reg,i,
					((pIV1[i*4+0]<<24) | (pIV1[i*4+1]<<16) | (pIV1[i*4+2]<<8) | pIV1[i*4+3] ));
		}
	}

	return 0 ;
}

int gx3113c_mtc_hal_SetIV2(unsigned char *pIV2,unsigned int bSelect_High)
{
	int i = 0 ;

	if ( bSelect_High )
	{
		for(i=0;i<2;i++)
		{
			MTC_SET_IV2(gx3113c_MTC_reg, (2+i),
					((pIV2[i*4+0]<<24) | (pIV2[i*4+1]<<16) | (pIV2[i*4+2]<<8) | pIV2[i*4+3] ));
		}
	}
	else
	{
		for(i=0;i<4;i++)
		{
			MTC_SET_IV2(gx3113c_MTC_reg, i,
					((pIV2[i*4+0]<<24) | (pIV2[i*4+1]<<16) | (pIV2[i*4+2]<<8) | pIV2[i*4+3] ));
		}
	}

	return 0 ;
}

int gx3113c_mtc_hal_SetDateAddSize(unsigned int read_addr,unsigned int write_addr,unsigned int size)
{
	MTC_SET_READ_ADDR(gx3113c_MTC_reg, read_addr&0x1fffffff);

	MTC_SET_WRITE_ADDR(gx3113c_MTC_reg, write_addr&0x1fffffff);

	MTC_SET_DATA_SIZE(gx3113c_MTC_reg, size);

	return 0 ;
}

int gx3113c_mtc_hal_EnableInterrupt(unsigned char bEnable)
{
	if ( bEnable )
	{
		MTC_DATA_FINISH_INT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DATA_FINISH_INT_EN_F(gx3113c_MTC_reg);
	}
	return 0 ;
}

int gx3113c_mtc_hal_GetInterrupStatue(unsigned int *pStatus)
{

	*pStatus =	MTC_GET_DATA_FINISH_INT(gx3113c_MTC_reg);

	return 0 ;
}

int gx3113c_mtc_hal_ClearInterrupStatue(void)
{

	MTC_CLEAR_DATA_FINISH_INT(gx3113c_MTC_reg);

	return 0 ;
}

int gx3113c_mtc_hal_Get_DemuxAddr(unsigned int *pDemuxAddr)
{

	*pDemuxAddr  =  (*(volatile unsigned int* )(M2M_DEMUX_ADDR)) | 0x90000000;

	return 0 ;
}

int gx3113c_mtc_hal_Get_DemuxLen(unsigned int *pBufSize)
{
	*pBufSize  = *(volatile unsigned int* )(M2M_DEMUX_SIZE) ;

	return 0 ;
}


//////////////////////////////////////////////
/* interface */

static int gx3113c_mtc_init(void)
{
	if(!gx_request_mem_region(MTC_BASE_ADDR, sizeof(struct MTC_Regs_s)))
	{
		return -1 ;
	}

	gx3113c_MTC_reg = gx_ioremap(MTC_BASE_ADDR, sizeof(struct MTC_Regs_s));
	if (gx3113c_MTC_reg== NULL)
	{
		gx_printf("mtc iremmap error \n");
		return -1;
	}

	return 0 ;
}

static int gx3113c_mtc_cleanup(void)
{
	if ( gx3113c_MTC_reg )
	{
		gx_iounmap(gx3113c_MTC_reg);

		gx_release_mem_region(MTC_BASE_ADDR, sizeof(struct MTC_Regs_s));

		gx3113c_MTC_reg = NULL;
	}

	return 0 ;
}

static int gx3113c_mtc_open(int id)
{
	return 0 ;
}

static int gx3113c_mtc_close(int id)
{
	return 0 ;
}

static int gx3113c_mtc_updatekey(int id ,GxMTCProperty_M2M_UpdateKey *p_M2M_UpdateKey)
{
	return gx3113c_mtc_hal_UpdateKey(p_M2M_UpdateKey->m_m2m_key_table_num);
}

static int gx3113c_mtc_set_params_execute(int id ,GxMTCProperty_M2M_SetParamsAndExecute *p_M2M_SetParamsAndExecute)
{
	unsigned int m2m_algorithm,m2m_option;

	unsigned int value ;

	unsigned int timeout ;

	unsigned int user_source_address,user_destination_address ;

	unsigned int source_address,destination_address,size ;

	unsigned char m2m_key_table_num ;

	unsigned char *pIV1,*pIV2 ;

	unsigned char *pAddrMalloc = NULL ;
	unsigned int uCheckAddr,offset ;

	unsigned int demux_addr,demux_size ;

	m2m_key_table_num = p_M2M_SetParamsAndExecute->m_m2m_key_table_num ;

	user_source_address      = p_M2M_SetParamsAndExecute->m_m2m_src_addr ;
	user_destination_address = p_M2M_SetParamsAndExecute->m_m2m_dst_addr ;
	size                     = p_M2M_SetParamsAndExecute->m_m2m_size     ;

	m2m_algorithm   = p_M2M_SetParamsAndExecute->m_m2m_algorithm ;
	m2m_option      = p_M2M_SetParamsAndExecute->m_m2m_option    ;

	pIV1  = p_M2M_SetParamsAndExecute->m_m2m_iv1 ;
	pIV2  = p_M2M_SetParamsAndExecute->m_m2m_iv2 ;


	value = (p_M2M_SetParamsAndExecute->m_m2m_algorithm<<8) | (p_M2M_SetParamsAndExecute->m_m2m_option<<12)|
		(p_M2M_SetParamsAndExecute->m_m2m_residue<<16)  | (p_M2M_SetParamsAndExecute->m_m2m_shortmessage<<20)|
			(p_M2M_SetParamsAndExecute->m_m2m_encrypt_or_decrypt);


	gx3113c_mtc_hal_Get_DemuxAddr(&demux_addr);
	gx3113c_mtc_hal_Get_DemuxLen(&demux_size);

	if ( size > demux_size )
	{
		return -1 ;
	}

	pAddrMalloc = NULL ;

	pAddrMalloc = gx_malloc(MIN_ALIGNMENT_NUM + M2M_MALLOC_ALIGNMENT(size));
	if (pAddrMalloc == NULL)
	{
		gx_printf("malloc is null  \n");
		return -1 ;
	}
	uCheckAddr = (unsigned int)pAddrMalloc;

	if ( (uCheckAddr % MIN_ALIGNMENT_NUM)  > 0 )
	{
		offset = uCheckAddr % MIN_ALIGNMENT_NUM ;
		offset = MIN_ALIGNMENT_NUM - offset ;

		uCheckAddr = (uCheckAddr + offset ) ;

	}

	// jia mi :  demux -> cpu
	// jie mi :  cpu -> demux  

	if ( p_M2M_SetParamsAndExecute->m_m2m_encrypt_or_decrypt == TYPE_M2M_MODE_ENCRYPT )
	{
		gx_copy_from_user((void*)demux_addr,(void*)user_source_address,size);

		source_address      = demux_addr ;
		destination_address = uCheckAddr ;
	}
	else
	{
		gx_copy_from_user((void*)uCheckAddr,(void*)user_source_address,size);

		source_address      = uCheckAddr ;
		destination_address = demux_addr ;
	}

	gx_dcache_inv_range(0,0);


	gx3113c_mtc_hal_Key_Ready(0);
	gx3113c_mtc_hal_NDS_Mode_AES_DES_Ready(0);


	// 产生控制模式 
	gx3113c_mtc_hal_ControlUnit_Write_Enable(0);

	gx3113c_mtc_hal_NDS_Control_Set(p_M2M_SetParamsAndExecute->m_m2m_encrypt_or_decrypt,
			p_M2M_SetParamsAndExecute->m_m2m_algorithm,p_M2M_SetParamsAndExecute->m_m2m_option,
			p_M2M_SetParamsAndExecute->m_m2m_residue,p_M2M_SetParamsAndExecute->m_m2m_shortmessage);


	gx3113c_mtc_hal_ControlUnitWriteKeyAddr(m2m_key_table_num);

	// enable control unit write enable 
	gx3113c_mtc_hal_ControlUnit_Write_Enable(1);

	// read key 
	//disable  read the key from key table
	gx3113c_mtc_hal_KeyTableSelectEnable(0);

	// key table selelt
	gx3113c_mtc_hal_KeyTableSelect(m2m_key_table_num);

	//enable  read the key from key tabl
	gx3113c_mtc_hal_KeyTableSelectEnable(1);

	// set m2m nds mode 
	// NDS KEY Module
	gx3113c_mtc_hal_SelectKeyMode(2);

	// NDS Interface
	gx3113c_mtc_hal_SelectKeyFromInterface(2);

	if ( m2m_algorithm == TYPE_M2M_ALGORITHM_AES )
	{
		if ( m2m_option == TYPE_M2M_OPTION_CTR )
		{
			gx3113c_mtc_hal_SetCounter(pIV1);
		}
		else
		{
			gx3113c_mtc_hal_SetIV1(pIV1,0);
		}
	}
	else
	{
		gx3113c_mtc_hal_SetIV1(pIV1,1);
	}

	if ( m2m_algorithm == TYPE_M2M_ALGORITHM_AES )
	{
		gx3113c_mtc_hal_SetIV2(pIV2,0);
	}
	else
	{
		gx3113c_mtc_hal_SetIV2(pIV2,1);
	}


	gx3113c_mtc_hal_SetDateAddSize(source_address,destination_address,size);

	// enable intc
	gx3113c_mtc_hal_EnableInterrupt(1);


	gx3113c_mtc_hal_Key_Ready(1);
	gx3113c_mtc_hal_NDS_Mode_AES_DES_Ready(1);

	timeout = 0xfffffff ;

	while(1)
	{
		value = 0 ;
		gx3113c_mtc_hal_GetInterrupStatue(&value);

	//	test_printf("m2m intc value  0x%x,timeout 0x%x \n", value,timeout);

		if ( value || timeout <= 0 )
		{
		//	test_printf("m2m intc value  0x%x,timeout 0x%x \n", value,timeout);

			gx3113c_mtc_hal_ClearInterrupStatue();
			break;
		}

		timeout-- ;
	}


	gx3113c_mtc_hal_EnableInterrupt(0);

	gx3113c_mtc_hal_Key_Ready(0);
	gx3113c_mtc_hal_NDS_Mode_AES_DES_Ready(0);


	// jia mi :  demux -> cpu
	// jie mi :  cpu -> demux  
	if ( p_M2M_SetParamsAndExecute->m_m2m_encrypt_or_decrypt == TYPE_M2M_MODE_ENCRYPT )
	{
		gx_copy_to_user((void*)user_destination_address,(void*)uCheckAddr,size);
	}
	else
	{
		gx_copy_to_user((void*)user_destination_address,(void*)demux_addr,size);
	}

	gx_dcache_inv_range(0,0);

	return 0 ;
}

static int gx3113c_mtc_get_event(int id ,GxMTCProperty_M2M_Event *p_M2M_Event)
{
	p_M2M_Event->m_m2m_event_interrupt = 0 ;

	if ( p_M2M_Event->m_m2m_event_index == TYPE_MTC_EVNET_KEY_WRITE_DONE )
	{
		gx3113c_mtc_hal_GetKeyWriteDoneEvent(&(p_M2M_Event->m_m2m_event_interrupt));
	}

	return 0 ;
}


/*
-------------------------------------------------------
                  start 2   
*/


#define MTC_HAL_DES_KEY_BIT_LEN             (64)
#define MTC_HAL_AES_KEY_BIT_LEN_1           (128)
#define MTC_HAL_AES_KEY_BIT_LEN_2           (192)
#define MTC_HAL_AES_KEY_BIT_LEN_3           (256)


#define MTC_HAL_MAX_NUMBER_INT                (8)
#define MTC_HAL_MEMORY_OPERATE_ILLEGAL_INT    (7)
#define MTC_HAL_READ_MEMORY_KEY_FINISH_INT    (6)
#define MTC_HAL_M2M_OPERATE_ILLEGAL_INT       (5)
#define MTC_HAL_ERR_OPERATE_INT               (4)
#define MTC_HAL_TDES_ROUND_FINISH_INT         (3)
#define MTC_HAL_ASE_ROUND_FINISH_INT          (2)
#define MTC_HAL_K3_FINISH_INT                 (1)
#define MTC_HAL_DATA_FINISH_INT               (0)



#define MTC_HAL_ALGORITM_AES          (0)
#define MTC_HAL_ALGORITM_3DES         (1)
#define MTC_HAL_ALGORITM_DES          (2)

#define MTC_HAL_OPTION_MODE_ECB       (0)
#define MTC_HAL_OPTION_MODE_CBC       (1)
#define MTC_HAL_OPTION_MODE_CTR       (2)

#define MTC_HAL_RESIDUE_MODE_CLEAR    (0)
#define MTC_HAL_RESIDUE_MODE_CTS      (1)
#define MTC_HAL_RESIDUE_MODE_RBT      (2)


#define MTC_HAL_SHORT_MESSAGE_MODE_CLEAR (0)
#define MTC_HAL_SHORT_MESSAGE_MODE_IV1   (1)
#define MTC_HAL_SHORT_MESSAGE_MODE_IV2   (2)

#define MTC_HAL_DATA_ENCRYPT    (1)
#define MTC_HAL_DATA_DECRYPT    (0)

int gx3113c_mtc_des_key_multi_layer_use_low(unsigned int bSelect)
{
	if ( bSelect )
	{
		MTC_DES_KEY_MULTI_LAYER_USE_LOW_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DES_KEY_MULTI_LAYER_USE_LOW_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

int gx3113c_mtc_short_mode(unsigned int mode)
{
	MTC_SHORT_MODE(gx3113c_MTC_reg,mode);

	return 0 ;
}
//
int gx3113c_mtc_residue_mode(unsigned int mode)
{
	MTC_RESIDUE_MODE(gx3113c_MTC_reg,mode);

	return 0 ;
}
//
int gx3113c_mtc_des_key_use_low(unsigned int bSelect)
{
	if ( bSelect )
	{
		MTC_DES_KEY_USE_LOW_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DES_KEY_USE_LOW_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_set_key_from(unsigned int from)
{
	MTC_DATA_ENCRY_KEY_SELECT(gx3113c_MTC_reg,from);

	return 0 ;
}
//
int gx3113c_mtc_big_endian(unsigned int bSelect)
{
	if ( bSelect )
	{
		MTC_BIG_ENDIAN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_BIG_ENDIAN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_m(unsigned int value)
{
	MTC_M(gx3113c_MTC_reg,value);

	return 0 ;
}
//
int gx3113c_mtc_option_mode(unsigned int mode)
{
	MTC_OPTION_MODE(gx3113c_MTC_reg,mode);

	return 0 ;
}
//
int gx3113c_mtc_work_mode(unsigned int mode)
{
	MTC_WORK_MODE(gx3113c_MTC_reg,mode);

	return 0 ;
}
//
int gx3113c_mtc_aes_mode(unsigned int mode)
{
	MTC_AES_MODE(gx3113c_MTC_reg,mode);

	return 0 ;
}
//
int gx3113c_mtc_key_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_KEY_RDY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_KEY_RDY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_encrypt_or_decrypt(unsigned int bEncrypt)
{
	if ( bEncrypt )
	{
		MTC_ENCRYPT_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_ENCRYPT_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_tri_des(unsigned int bSelect)
{
	if ( bSelect )
	{
		MTC_TRI_DES_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_TRI_DES_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_read_sdc_start_addr(unsigned int addr)
{
	MTCR_SDR_START_ADDR(gx3113c_MTC_reg,addr);

	return 0 ;
}
//
int gx3113c_mtc_write_sdc_start_addr(unsigned int addr)
{
	MTCW_SDR_START_ADDR(gx3113c_MTC_reg,addr);

	return 0 ;
}
//
int gx3113c_mtc_rw_sdc_data_len(unsigned int len)
{
	MTCR_SDR_DATA_LEN(gx3113c_MTC_reg,len);

	return 0 ;
}
//
int gx3113c_mtc_nds_mode_aes_or_des_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_NDS_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_NDS_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_sms4_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_SMS4_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_SMS4_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_multi2_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_MULTI2_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_MULTI2_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_aes_ready(unsigned int bReady)
{

	if ( bReady )
	{
		MTC_ASE_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_ASE_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_des_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_DES_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DES_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_otp_key_length(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_OTP_KEY_LENGTH_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_OTP_KEY_LENGTH_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_ca_key_length(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CA_KEY_LENGTH_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CA_KEY_LENGTH_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_dcw_ready(unsigned int bReady)
{

	if ( bReady )
	{
		MTC_DCW_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DCW_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_dck_ready(unsigned int bReady)
{

	if ( bReady )
	{
		MTC_DCK_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DCK_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_dsk_ready(unsigned int bReady)
{
	if ( bReady )
	{
		MTC_DSK_READY_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DSK_READY_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_ca_mode(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CA_MODE_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CA_MODE_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_nds_control_set(unsigned char algorithm,unsigned char opition,
		unsigned char residue,unsigned char shortmessage,unsigned char encrypt_decrypt)
{
	unsigned char en_de ;

	if ( encrypt_decrypt == MTC_HAL_DATA_ENCRYPT )
	{
		en_de = 0x1 ;
	}
	else
	{
		en_de = 0x2 ;
	}

	MTC_CONTROL_SET(gx3113c_MTC_reg,algorithm,opition,residue,shortmessage,en_de);

	return 0 ;
}
//
int gx3113c_mtc_nds_key_table_select_en(unsigned int bEnable)
{

	if ( bEnable )
	{
		MTC_NDS_KEY_TABLE_SELECT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_NDS_KEY_TABLE_SELECT_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_nds_key_table_select(unsigned int value)
{

	MTC_NDS_KEY_TABLE_SELECT(gx3113c_MTC_reg,value);

	return 0 ;
}
//
int gx3113c_mtc_nds_control_unit_write_en(unsigned int bEnable)
{

	if ( bEnable )
	{
		MTC_NDS_CONTROL_UNIT_WRITE_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_NDS_CONTROL_UNIT_WRITE_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_nds_control_unit_write_addr(unsigned int value)
{

	MTC_NDS_CONTROL_UNIT_WRITE_ADDR(gx3113c_MTC_reg,value);

	return 0 ;
}
//
int gx3113c_mtc_multi_layer_key_en(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_MULTI_LAYER_KEY_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_MULTI_LAYER_KEY_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_cw_aes_high_low_select(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CW_AES_HIGH_LOW_SELECT_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CW_AES_HIGH_LOW_SELECT_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_cw_3_round_cacas_type(unsigned int type)
{

	MTC_CW_3_ROUND_CDCAS_TYPE(gx3113c_MTC_reg,type);

	return 0 ;
}
//
int gx3113c_mtc_cw_3_round_type(unsigned int type)
{

	MTC_CW_3_ROUND_TYPE(gx3113c_MTC_reg,type);

	return 0 ;
}
//
int gx3113c_mtc_cw_3_round(unsigned int bSelect)
{
	if ( bSelect )
	{
		MTC_CW_3_ROUND_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CW_3_ROUND_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_cw_2_round(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CW_2_ROUND_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CW_2_ROUND_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_cw_1_round(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CW_1_ROUND_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CW_1_ROUND_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_cw_en(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_CW_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_CW_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_vender_sysid(unsigned int vender_sysid)
{

	MTC_VENDER_SYSID(gx3113c_MTC_reg,vender_sysid);

	return 0 ;
}
//
int gx3113c_mtc_module_id(unsigned int module_id)
{

	MTC_MODULE_ID(gx3113c_MTC_reg,module_id);

	return 0 ;
}
//
int gx3113c_mtc_k3_gen_en(unsigned int bSelect)
{

	if ( bSelect )
	{
		MTC_K3_GEN_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_K3_GEN_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}

//
int gx3113c_mtc_tdes_round_finish_int_en(unsigned int bEnable)
{

	if ( bEnable )
	{
		MTC_TDES_ROUND_FINISH_INT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_TDES_ROUND_FINISH_INT_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_aes_round_finish_int_en(unsigned int bEnable)
{

	if ( bEnable )
	{
		MTC_ASE_ROUND_FINISH_INT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_ASE_ROUND_FINISH_INT_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_k3_finish_int_en(unsigned int bEnable)
{
	if ( bEnable )
	{
		MTC_K3_FINISH_INT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_K3_FINISH_INT_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_data_finish_int_en(unsigned int bEnable)
{
	if ( bEnable )
	{
		MTC_DATA_FINISH_INTTERUPT_EN_T(gx3113c_MTC_reg);
	}
	else
	{
		MTC_DATA_FINISH_INTTERUPT_EN_F(gx3113c_MTC_reg);
	}

	return 0 ;
}
//
int gx3113c_mtc_all_int_en(unsigned int value)
{

	MTC_ALL_INT_EN(gx3113c_MTC_reg,value);

	return 0 ;
}
//
int gx3113c_mtc_set_ca_addr(unsigned int value)
{

	MTC_CA_ADDR(gx3113c_MTC_reg,value);

	return 0 ;
}
//
int gx3113c_mtc_set_key(unsigned int *pData,unsigned int len)
{
	unsigned int *p32data ;

	p32data = (unsigned int*)pData ;

	if ( MTC_HAL_DES_KEY_BIT_LEN == len )
	{
		MTC_KEY_1_VALUE_H(gx3113c_MTC_reg,p32data[0]);
		MTC_KEY_1_VALUE_L(gx3113c_MTC_reg,p32data[1]);
	}
	else if ( MTC_HAL_AES_KEY_BIT_LEN_1 == len )
	{
		MTC_KEY_2_VALUE_H(gx3113c_MTC_reg,p32data[0]);
		MTC_KEY_2_VALUE_L(gx3113c_MTC_reg,p32data[1]);
		MTC_KEY_1_VALUE_H(gx3113c_MTC_reg,p32data[2]);
		MTC_KEY_1_VALUE_L(gx3113c_MTC_reg,p32data[3]);
	}
	else if ( MTC_HAL_AES_KEY_BIT_LEN_2 == len )
	{
		MTC_KEY_3_VALUE_H(gx3113c_MTC_reg,p32data[0]);
		MTC_KEY_3_VALUE_L(gx3113c_MTC_reg,p32data[1]);
		MTC_KEY_2_VALUE_H(gx3113c_MTC_reg,p32data[2]);
		MTC_KEY_2_VALUE_L(gx3113c_MTC_reg,p32data[3]);
		MTC_KEY_1_VALUE_H(gx3113c_MTC_reg,p32data[4]);
		MTC_KEY_1_VALUE_L(gx3113c_MTC_reg,p32data[5]);
	}
	else if ( MTC_HAL_AES_KEY_BIT_LEN_3 == len )
	{
		MTC_KEY_4_VALUE_H(gx3113c_MTC_reg,p32data[0]);
		MTC_KEY_4_VALUE_L(gx3113c_MTC_reg,p32data[1]);
		MTC_KEY_3_VALUE_H(gx3113c_MTC_reg,p32data[2]);
		MTC_KEY_3_VALUE_L(gx3113c_MTC_reg,p32data[3]);
		MTC_KEY_2_VALUE_H(gx3113c_MTC_reg,p32data[4]);
		MTC_KEY_2_VALUE_L(gx3113c_MTC_reg,p32data[5]);
		MTC_KEY_1_VALUE_H(gx3113c_MTC_reg,p32data[6]);
		MTC_KEY_1_VALUE_L(gx3113c_MTC_reg,p32data[7]);
	}
	else
	{
		return -1 ;
	}
	return 0;
}
//
int gx3113c_mtc_set_dsk(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<6;i++)
	{
		MTC_DSK_VALUE_BY_INDEX(gx3113c_MTC_reg,5-i,pData[i]);
	}

	return 0;
}
//
int gx3113c_mtc_set_dck(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<6;i++)
	{
		MTC_DCK_VALUE_BY_INDEX(gx3113c_MTC_reg,5-i,pData[i]);
	}

	return 0;
}
//
int gx3113c_mtc_set_dcw(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<4;i++)
	{
		MTC_DCW_VALUE_BY_INDEX(gx3113c_MTC_reg,3-i,pData[i]);
	}

	return 0;
}
//
int gx3113c_mtc_set_counter(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<4;i++)
	{
		MTC_COUNTER_VALUE_BY_INDEX(gx3113c_MTC_reg,i,pData[i]);
	}

	return 0;
}
//
int gx3113c_mtc_set_iv_1(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<4;i++)
	{
		MTC_IV1_VALUE_BY_INDEX(gx3113c_MTC_reg,i,pData[i]);
	}

	return 0 ;
}
//
int gx3113c_mtc_set_iv_2(unsigned int *pData)
{
	int i = 0 ;

	for(i=0;i<4;i++)
	{
		MTC_IV2_VALUE_BY_INDEX(gx3113c_MTC_reg,i,pData[i]);
	}

	return 0 ;
}
//
int gx3113c_mtc_get_item_int(unsigned int int_bit,unsigned int timeout)
{
	unsigned int value = 0 ;

	if ( int_bit >= MTC_HAL_MAX_NUMBER_INT)
	{
		return -1 ;
	}

	while(1)
	{

		MTC_GET_ALL_INT_VALUE(gx3113c_MTC_reg,value);

		//debug_printf("m2m intc value  0x%x,timeout 0x%x \n", value,timeout);

		value = ( value >> int_bit ) & 0x1 ;

		if ( value )
		{
		//	debug_printf(" int_bit %d, timeout %d \n", int_bit,timeout);

			MTC_CLEAR_INT(gx3113c_MTC_reg,int_bit);

			break;
		}

		if ( timeout <= 0 )
		{
			gx_printf("!!!! ERROR exit by time out  int_bit  %d \n", int_bit);

			return -1 ;
		}

		timeout-- ;
	}

	return 0 ;

}

#define INT_TIMEOUT_VALUE  (500000)

int _gx3113c_check_data_info_para(GxMTCProperty_Run *p_Run)
{
	return 0 ;
}

unsigned int _gx3113c_mtc_control_set(GxMTCProperty_Run *pInfo)
{

	unsigned char algorithm ;
	unsigned char opition   ;
	unsigned char residue   ;
	unsigned char shortmessage ;
	unsigned char encrypt_decrypt = MTC_HAL_DATA_ENCRYPT;

	unsigned int value = 0 ;

	if ( pInfo->m_algoritm == MTC_API_ALGORITM_DES )
	{
		if ( pInfo->m_group == MTC_API_GROUP_DES )
		{
			algorithm = MTC_HAL_ALGORITM_DES ;
		}
		else if ( pInfo->m_group == MTC_API_GROUP_3_DES )
		{
			algorithm = MTC_HAL_ALGORITM_3DES ;
		}
		else
		{
			algorithm = MTC_HAL_ALGORITM_AES ;
		}
	}
	else if ( pInfo->m_algoritm == MTC_API_ALGORITM_AES )
	{
		algorithm = MTC_HAL_ALGORITM_AES ;
	}
	else
	{
		algorithm = MTC_HAL_ALGORITM_AES ;
	}

	if ( pInfo->m_option_mode == MTC_API_OPTION_MODE_ECB )
	{
		opition = MTC_HAL_OPTION_MODE_ECB ;
	}
	else if ( pInfo->m_option_mode == MTC_API_OPTION_MODE_CBC )
	{
		opition = MTC_HAL_OPTION_MODE_CBC;
	}
	else if ( pInfo->m_option_mode == MTC_API_OPTION_MODE_CTR )
	{
		opition = MTC_HAL_OPTION_MODE_CTR;
	}
	else
	{
		opition = MTC_HAL_OPTION_MODE_ECB;
	}

	if ( pInfo->m_residue_mode == MTC_API_RESIDUE_MODE_CLEAR )
	{
		 residue  = MTC_HAL_RESIDUE_MODE_CLEAR ;
	}
	else if ( pInfo->m_residue_mode == MTC_API_RESIDUE_MODE_CTS )
	{
		residue  = MTC_HAL_RESIDUE_MODE_CTS ;
	}
	else if ( pInfo->m_residue_mode == MTC_API_RESIDUE_MODE_RBT )
	{
		residue  = MTC_HAL_RESIDUE_MODE_RBT ;
	}
	else
	{
		residue   = MTC_HAL_RESIDUE_MODE_CLEAR ;
	}

	if ( pInfo->m_short_mode == MTC_API_SHORT_MESSAGE_MODE_CLEAR )
	{
		shortmessage  = MTC_HAL_SHORT_MESSAGE_MODE_CLEAR ;
	}
	else if ( pInfo->m_short_mode == MTC_API_SHORT_MESSAGE_MODE_IV )
	{
		shortmessage  =  MTC_HAL_SHORT_MESSAGE_MODE_IV1 ;
	}
	else if ( pInfo->m_short_mode == MTC_API_SHORT_MESSAGE_MODE_IV2 )
	{
		shortmessage  =  MTC_HAL_SHORT_MESSAGE_MODE_IV2 ;
	}
	else
	{
		shortmessage = MTC_HAL_SHORT_MESSAGE_MODE_CLEAR ;
	}


	if ( pInfo->encrypt_decrypt == MTC_API_DATA_ENCRYPT )
	{
		encrypt_decrypt = MTC_HAL_DATA_ENCRYPT ;
	}
	else //if ( pInfo->encrypt_decrypt == MTC_API_DATA_DECRYPT )
	{
		encrypt_decrypt = MTC_HAL_DATA_DECRYPT ;
	}

	gx3113c_mtc_nds_control_set(algorithm,opition,residue,shortmessage,encrypt_decrypt) ;

	return value ;
}


static int algoritm_set_group(unsigned char algoritm,unsigned char group)
{
	if ( algoritm == MTC_API_ALGORITM_DES )
	{
		gx3113c_mtc_tri_des(group);
	}
	else if ( algoritm == MTC_API_ALGORITM_AES )
	{
		gx3113c_mtc_aes_mode(group);
	}
	else
	{
		return  -1 ;
	}

	return 0 ;
}

static int algoritm_set_ready(unsigned char algoritm,unsigned char ready)
{
	if ( algoritm == MTC_API_ALGORITM_DES )
	{
		gx3113c_mtc_des_ready(ready);
	}
	else if ( algoritm == MTC_API_ALGORITM_AES )
	{
		gx3113c_mtc_aes_ready(ready);
	}
	else
	{
		return  -1 ;
	}

	return 0 ;
}

static int algoritm_set_cdcas(unsigned char algoritm,unsigned ca_or_cp)
{
	if ( algoritm == MTC_API_ALGORITM_DES )
	{
		if (ca_or_cp ==  0)
		{
			gx3113c_mtc_cw_3_round_cacas_type(1);
		}
		else
		{
			gx3113c_mtc_cw_3_round_cacas_type(3);
		}
	}
	else if ( algoritm == MTC_API_ALGORITM_AES )
	{
		if (ca_or_cp ==  0)
		{
			gx3113c_mtc_cw_3_round_cacas_type(0);
		}
		else
		{
			gx3113c_mtc_cw_3_round_cacas_type(2);
		}
	}
	else
	{
		return  -1 ;
	}

	return 0 ;
}


static int algoritm_get_which_interrupt(unsigned char algoritm,unsigned char *p_which)
{
	unsigned char which ;

	if ( algoritm == MTC_API_ALGORITM_DES )
	{
		which = MTC_HAL_TDES_ROUND_FINISH_INT ;
	}
	else if ( algoritm == MTC_API_ALGORITM_AES )
	{
		which = MTC_HAL_ASE_ROUND_FINISH_INT ;
	}
	else
	{
		which  = MTC_HAL_MAX_NUMBER_INT ;
	}

	*p_which = which  ;

	return 0 ;
}

static int gx3113c_mtc_set_config(int id ,GxMTCProperty_Config *p_config)
{
	int bit_value ;

	bit_value =  (p_config->m_config_bits>>CONFIG_BIT_BIG_MODE)&0x1 ;
	gx3113c_mtc_big_endian(bit_value);

	bit_value =  (p_config->m_config_bits>>CONFIG_BIT_DES_KEY_LOW)&0x1 ;
	gx3113c_mtc_des_key_use_low(bit_value);

	bit_value =  (p_config->m_config_bits>>CONFIG_BIT_DES_KEY_MULTI_LAYER_LOW)&0x1 ;
	gx3113c_mtc_des_key_multi_layer_use_low(bit_value);

	bit_value = (p_config->m_config_bits>>CONFIG_BIT_INT_ALL_OPEN_EN)&0x1 ;
	if(bit_value)
	{
		gx3113c_mtc_all_int_en(0xf);
	}

	bit_value = (p_config->m_config_bits>>CONFIG_BIT_INT_ALL_CLOSE_EN)&0x1 ;
	if(bit_value)
	{
		gx3113c_mtc_all_int_en(0x0);
	}


	//gx_printf("mtc config 0x%x \n",p_Config);
	return 0 ;
}

static int gx3113c_mtc_runing(int id ,GxMTCProperty_Run *pInfo)
{
	int ret = 0 ;
	unsigned char  interrupt_finish ;
	unsigned int  ca_addr ;
	unsigned int  client_item,client_data;

	if ( _gx3113c_check_data_info_para(pInfo) != 0 )
	{
		return -1 ;
	}

	if (  LAYER_NUM_OTHER == pInfo->m_layer )
	{
		gx3113c_mtc_ca_mode(MTC_API_NORMAL_MODE);

		if ( pInfo->m_key_from == MTC_API_KEY_FROM_IPBUS )
		{
			gx3113c_mtc_work_mode(pInfo->m_algoritm);
			ret = algoritm_set_group(pInfo->m_algoritm,pInfo->m_group);
			ASSERT(ret);
			gx3113c_mtc_option_mode(pInfo->m_option_mode);
			gx3113c_mtc_residue_mode(pInfo->m_residue_mode);
			gx3113c_mtc_short_mode(pInfo->m_short_mode);

			gx3113c_mtc_encrypt_or_decrypt(pInfo->encrypt_decrypt);

			gx3113c_mtc_set_key_from(pInfo->m_key_from);

			gx3113c_mtc_set_key(pInfo->m_pkeydata,pInfo->m_keylen );

			gx3113c_mtc_read_sdc_start_addr(pInfo->m_read_addr);
			gx3113c_mtc_write_sdc_start_addr(pInfo->m_write_addr);
			gx3113c_mtc_rw_sdc_data_len(pInfo->m_rwlen);

			gx3113c_mtc_set_counter(pInfo->m_counter);
			gx3113c_mtc_set_iv_1(pInfo->m_iv_1);
			gx3113c_mtc_set_iv_2(pInfo->m_iv_2);

			gx_dcache_inv_range(0,0);

			gx3113c_mtc_key_ready(1);

			ret = algoritm_set_ready(pInfo->m_algoritm,1);
			ASSERT(ret);

			ret = gx3113c_mtc_get_item_int(MTC_HAL_DATA_FINISH_INT,INT_TIMEOUT_VALUE);
			ASSERT(ret);

			gx_dcache_inv_range(0,0);

			gx3113c_mtc_key_ready(0);
			ret = algoritm_set_ready(pInfo->m_algoritm,0);

		}
		else
		{
			return -1 ;
		}
	}
	else
	{
		gx3113c_mtc_ca_mode(MTC_API_CA_MODE);

		gx3113c_mtc_work_mode(pInfo->m_algoritm);

		ret = algoritm_set_group(pInfo->m_algoritm,pInfo->m_group);
		ASSERT(ret);
		gx3113c_mtc_option_mode(pInfo->m_option_mode);
		gx3113c_mtc_residue_mode(pInfo->m_residue_mode);
		gx3113c_mtc_short_mode(pInfo->m_short_mode);

		gx3113c_mtc_encrypt_or_decrypt(pInfo->encrypt_decrypt);

		ca_addr = ((pInfo->m_desc_id*2 +0xa0)<<16) + (pInfo->m_desc_id*2 +0xa1 );

		gx3113c_mtc_set_ca_addr(ca_addr);

		switch(pInfo->m_layer)
		{
			case LAYER_NUM_1:
				{
					gx3113c_mtc_cw_1_round(1);

					gx3113c_mtc_cw_en(1);
					gx3113c_mtc_set_dcw(pInfo->m_dcw);
					gx3113c_mtc_dcw_ready(1);
				}
				break;

			case LAYER_NUM_2:
				{
					gx3113c_mtc_cw_2_round(1);

					gx3113c_mtc_cw_en(1);

					gx3113c_mtc_set_dck(pInfo->m_dck);
					gx3113c_mtc_set_dcw(pInfo->m_dcw);

					gx3113c_mtc_dck_ready(1);
					gx3113c_mtc_dcw_ready(1);
				}
				break;

			case LAYER_NUM_3:
				{
					gx3113c_mtc_cw_3_round(1);

					if (pInfo->m_main_client ==  CLIENT_AD_YXSB  )
					{
						gx3113c_mtc_cw_3_round_type(0);
					}
					else  if (pInfo->m_main_client ==  CLIENT_AD_CDCAS  )
					{
						client_item   =   (pInfo->m_item_client & 0xffff) ;
						client_data   =  ((pInfo->m_item_client>>16) & 0xffff);
						if ( client_item == MTC_API_CHANNEL_MODE_CW )
						{
							gx3113c_mtc_cw_3_round_type(1);
						}
						else
						{
							gx3113c_mtc_cw_3_round_type(2);
						}

						ret = algoritm_set_cdcas(pInfo->m_algoritm,0);
						ASSERT(ret);

						gx3113c_mtc_module_id(client_data);

					}
					else
					{
						ASSERT(-1);
						return -1 ;
					}

					gx3113c_mtc_cw_en(1);

					gx3113c_mtc_set_dsk(pInfo->m_dsk);
					gx3113c_mtc_set_dck(pInfo->m_dck);
					gx3113c_mtc_set_dcw(pInfo->m_dcw);

					gx3113c_mtc_dsk_ready(1);
					gx3113c_mtc_dck_ready(1);
					gx3113c_mtc_dcw_ready(1);
				}
				break;

			case LAYER_NUM_4:
			case LAYER_NUM_5:
			default:
				return -1 ;
		}

		gx3113c_mtc_key_ready(1);

		ret = algoritm_set_ready(pInfo->m_algoritm,1);
		ASSERT(ret);

		algoritm_get_which_interrupt(pInfo->m_algoritm,&interrupt_finish);

		ret = gx3113c_mtc_get_item_int(interrupt_finish,INT_TIMEOUT_VALUE);
		ASSERT(ret);

		gx3113c_mtc_cw_1_round(0);
		gx3113c_mtc_cw_2_round(0);
		gx3113c_mtc_cw_3_round(0);
		gx3113c_mtc_cw_en(0);

		gx3113c_mtc_dsk_ready(0);
		gx3113c_mtc_dck_ready(0);
		gx3113c_mtc_dcw_ready(0);

		gx3113c_mtc_key_ready(0);

		ret = algoritm_set_ready(pInfo->m_algoritm,0);
		ASSERT(ret);
	}

	return ret ;
}

/*
------------------------------------------------------------------------------
*/

static struct mtc_ops gx3113c_mtc_ops = {
	.init          = gx3113c_mtc_init,
	.cleanup       = gx3113c_mtc_cleanup,
	.open          = gx3113c_mtc_open,
	.close         = gx3113c_mtc_close,
	.updatekey     = gx3113c_mtc_updatekey,
	.set_params_execute = gx3113c_mtc_set_params_execute,
	.get_event     = gx3113c_mtc_get_event,
	.set_config    = gx3113c_mtc_set_config,
	.runing        = gx3113c_mtc_runing,
};

struct gxav_module_ops gx3113c_mtc_module = {
	.module_type  = GXAV_MOD_MTC,
	.count        = 1,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gxav_mtc_open,
	.close        = gxav_mtc_close,
	.init         = gxav_mtc_init,
	.cleanup      = gxav_mtc_cleanup,
	.set_property = gxav_mtc_set_property,
	.get_property = gxav_mtc_get_property,
	.priv         = &gx3113c_mtc_ops
};
