/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    gx3211_iemm.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */


#include "iemm_hal.h"
#include "iemm_module.h"
#include "gx3211_reg.h"
#include "gxav_event_type.h"


#define ONE_BUF_SIZE             (256)
#define IEMM_BUFF_SIZE           (180)
#define IEMM_SOFT_BUFF_NUM       (50)

#define HAL_IEMM_TABLE_ID_FILTER_REJECT          (0)
#define	HAL_IEMM_TABLE_ID_FILTER_WITHOUT_CA_ADDR (1)
#define	HAL_IEMM_TABLE_ID_FILTER_WITH_CA_ADDR    (2)
#define	HAL_IEMM_TABLE_ID_FILTER_RESERVE         (3)

typedef struct
{
    unsigned char       m_used ;
    unsigned char       m_Buff[IEMM_BUFF_SIZE];
}HAL_IEMM_SoftBuff_t;

static HAL_IEMM_SoftBuff_t    s_iemm_softbuff_t[IEMM_SOFT_BUFF_NUM];

static unsigned int  s_iemm_out_buf = 0 ;
static unsigned int  s_iemm_interrupt_event = 0 ;

static unsigned int  s_iemm_read_buf_index  = 0 ;
static unsigned int  s_iemm_write_buf_index = 0 ;


Reg_Iemm_Demux_t  *gx3201_IEMM_reg = NULL ;

int HAL_IEMM_SetBuffAddr_TsNum(unsigned int addr,unsigned int TsNum)
{
	DEMUX_ICAM_SET_EMM_START_ADDR(gx3201_IEMM_reg, addr);

	if ( TsNum == 1)
	{
		DEMUX_ICAM_SET_DEMUX2_ICAM_MODE(gx3201_IEMM_reg);
	}
	else if ( TsNum == 2 )
	{
	}

	DEMUX_ICAM_SET_FREQ(gx3201_IEMM_reg, 12);

	return 0 ;
}

int HAL_IEMM_SetIntcMask(void)
{

	DEMUX_ICAM_OVERFLOWUNMASK_ARRIVEDUNMASK(gx3201_IEMM_reg);

	return 0;

}

int HAL_IEMM_SetTableIDMode(unsigned char mode, int Index)
{
	if (mode == HAL_IEMM_TABLE_ID_FILTER_REJECT)
	{
		DEMUX_ICAM_TID_FILTER_IGNORE(gx3201_IEMM_reg, Index);
	}
	else if (mode == HAL_IEMM_TABLE_ID_FILTER_WITHOUT_CA_ADDR)
	{
		DEMUX_ICAM_TID_WITHOUT_CA_ADDR(gx3201_IEMM_reg, Index);
	}
	else if (mode == HAL_IEMM_TABLE_ID_FILTER_WITH_CA_ADDR)
	{
		DEMUX_ICAM_TID_WITH_CA_ADDR(gx3201_IEMM_reg, Index);
	}
	else
	{
		DEMUX_ICAM_TID_FILTER_RESERVE(gx3201_IEMM_reg, Index);
	}

	return 0 ;
}


int HAL_IEMM_SetCAByte(unsigned int CA_Data,unsigned int CA_Mask,int  Index)
{

	DEMUX_ICAM_SET_CA_ADDR_VALUE(gx3201_IEMM_reg, Index, CA_Data);

	DEMUX_ICAM_SET_CA_ADDR_MASK(gx3201_IEMM_reg, Index, CA_Mask);

	return 0;
}

int HAL_IEMM_SetCACtrlID(unsigned char *pCtrlID)
{
	int Index = 0 ;
	unsigned int Value = 0 ;

	for (Index=0; Index<8; Index++)
	{
	    Value = (Value & ~(0x3<<(Index<<1))) | ((pCtrlID[Index]&0x3)<<(Index<<1));
	}

	DEMUX_ICAM_SET_CA_CTRL_ID(gx3201_IEMM_reg, Value);

	return 0 ;
}


int HAL_IEMM_SetPID(unsigned int pid)
{
	DEMUX_ICAM_SET_EMM_PID(gx3201_IEMM_reg, pid);

	return 0 ;
}


int HAL_IEMM_TableID_Enable_Disable(unsigned char Select)
{
	if (Select)
	{
		DEMUX_ICAM_USE_TID(gx3201_IEMM_reg);
	}
	else
	{
		DEMUX_ICAM_NONUSE_TID(gx3201_IEMM_reg);
	}

	return 0;
}

int HAL_IEMM_Pid_Enable_Disable(unsigned char Select)
{
	if (Select)
	{
		DEMUX_ICAM_PID_FILTER_ENABLE(gx3201_IEMM_reg);
	}
	else
	{
		DEMUX_ICAM_PID_FILTER_DISABLE(gx3201_IEMM_reg);
	}

	return 0;
}


int HAL_IEMM_Pid_And_TableID_Enable_Disable(unsigned char Select)
{
	if (Select)
	{
		DEMUX_ICAM_EMM_TID_MODE_ALL_ENABLE(gx3201_IEMM_reg);
	}
	else
	{
		DEMUX_ICAM_EMM_TID_MODE_ALL_DISABLE(gx3201_IEMM_reg);
	}

	return 0;
}


int HAL_IEMM_GetBuffOrder(unsigned int *pOrder)
{
	*pOrder = DEMUX_ICAM_GET_ARRIVED_BUF_ORDER(gx3201_IEMM_reg);

	DEMUX_ICAM_CLEAR_EMM_BUF(gx3201_IEMM_reg);

	return 0 ;
}


int HAL_IEMM_GetInterruptStatus(unsigned int *pOverFlow,unsigned int *pArrived)
{

	unsigned int OverflowIntOccur,ArrivedIntOccur ;

	*pOverFlow = 0 ;
	*pArrived  = 0 ;

	OverflowIntOccur = DEMUX_ICAM_GET_EMM_OVERFLOW(gx3201_IEMM_reg);
	if ( OverflowIntOccur )
	{
		DEMUX_ICAM_CLEAR_OVERFLOW(gx3201_IEMM_reg);

		*pOverFlow = 1 ;
	}
	else
	{
		ArrivedIntOccur = DEMUX_ICAM_GET_EMM_ARRIVED(gx3201_IEMM_reg);
		if (ArrivedIntOccur )
		{
			DEMUX_ICAM_CLEAR_ARRIVED(gx3201_IEMM_reg);

			*pArrived  = 1 ;
		}
	}

	return 0 ;
}



//////////////////////////////////////////////
/* interface */

static int s_gx3211_iemm_mem_region = 0 ;

static int gx3211_iemm_init(void)
{
	int i = 0 ;
	void *pEMMBuf = NULL ;

	if ( gx3201_IEMM_reg == NULL )
	{
		if(!gx_request_mem_region(IEMM_DEMUX_REG_ADDR_BASE, sizeof(struct Reg_Iemm_Demux_s)))
		{
			s_gx3211_iemm_mem_region = 0 ;
		}
		else
		{
			s_gx3211_iemm_mem_region = 1 ;
		}

		gx3201_IEMM_reg = gx_ioremap(IEMM_DEMUX_REG_ADDR_BASE, sizeof(struct Reg_Iemm_Demux_s));
		if (gx3201_IEMM_reg == NULL)
		{
			return -1;
		}
	}

	pEMMBuf = gx_malloc(256*10+1024) ;
	if ( NULL == pEMMBuf )
	{
		return -1;
	}

	i = (unsigned int)pEMMBuf ;

	if ( (i % 512))
	{
		pEMMBuf = (unsigned char*)(i + (512 - (i%512) )) ;
	}

	s_iemm_out_buf = (unsigned int)pEMMBuf ;

	for (i=0; i<IEMM_SOFT_BUFF_NUM; i++)
	{
		s_iemm_softbuff_t[i].m_used = 0;
	}

	HAL_IEMM_SetBuffAddr_TsNum(s_iemm_out_buf,1);

	return 0 ;
}

static int gx3211_iemm_cleanup(void)
{
	gx_iounmap(gx3201_IEMM_reg);

	if ( s_gx3211_iemm_mem_region )
	{
		gx_release_mem_region(IEMM_DEMUX_REG_ADDR_BASE, sizeof(struct Reg_Iemm_Demux_s));

		s_gx3211_iemm_mem_region = 0 ;
	}

	gx3201_IEMM_reg = NULL;

	return 0 ;
}

static int gx3211_iemm_open(int id)
{
	return 0 ;
}

static int gx3211_iemm_close(int id)
{
	return 0 ;
}

static int gx3211_iemm_request(int id,GxIemmProperty_Request *p_Request)
{
	int i = 0;

	HAL_IEMM_SetIntcMask();

	for(i=0;i<16;i++)
	{
		HAL_IEMM_SetTableIDMode(p_Request->m_table_id_filter_mode[i],i);
	}

	for(i=0;i<8;i++)
	{
		HAL_IEMM_SetCAByte(p_Request->m_CABytes[i].m_Iemm_CAData,p_Request->m_CABytes[i].m_Iemm_CAMask,i);
	}


	HAL_IEMM_SetCACtrlID(p_Request->m_CACtrlID);

	HAL_IEMM_SetPID(p_Request->m_emm_pid);

	HAL_IEMM_TableID_Enable_Disable(p_Request->m_table_id_en);

	HAL_IEMM_Pid_Enable_Disable(p_Request->m_pid_en);

	return 0 ;
}

static int gx3211_iemm_stop(int id,GxIemmProperty_Stop *p_Stop)
{
	int i = 0 ;

	HAL_IEMM_TableID_Enable_Disable(IEMM_TABLE_ID_FILTER_DISABLE);

	HAL_IEMM_Pid_Enable_Disable(IEMM_PID_DISABLE);

	for(i=0;i<16;i++)
	{
		HAL_IEMM_SetTableIDMode(IEMM_TABLE_ID_FILTER_REJECT,i);
	}

	s_iemm_interrupt_event = 0 ;

	for (i=0; i<IEMM_SOFT_BUFF_NUM; i++)
	{
		if (s_iemm_softbuff_t[i].m_used)
		{
			s_iemm_softbuff_t[i].m_used = 0;
		}
	}

	return 0 ;
}

static int gx3211_iemm_run(int id,GxIemmProperty_Run *p_Run)
{
	HAL_IEMM_Pid_And_TableID_Enable_Disable(p_Run->m_run_en ) ;

	if ( 0 == p_Run->m_run_en)
	{
		s_iemm_interrupt_event = 0 ;
	}

	return 0;
}

static int gx3211_iemm_update_read_index(int id,GxIemmProperty_UpdateReadIndex *p_UpdateReadIndex)
{
	int i ;
	int r,w ;

	r = s_iemm_read_buf_index  ;
	w = s_iemm_write_buf_index ;

//	gx_printf("discard r: %d ,w: %d \n",r,w);

	if ( r == w )
	{
		if (s_iemm_softbuff_t[r].m_used)
		{
			for(i=0;i<IEMM_SOFT_BUFF_NUM ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					s_iemm_softbuff_t[i].m_used = 0 ;

				//	gx_printf("discard id %d \n",i);
				}
			}
		}

		return 0;
	}
	else
	{
		if ( w > r )
		{
			for(i=r;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					s_iemm_softbuff_t[i].m_used = 0 ;

				//	gx_printf("a discard id %d \n",i);
				}
			}
		}
		else
		{
			for(i=r;i<IEMM_SOFT_BUFF_NUM ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					s_iemm_softbuff_t[i].m_used = 0 ;

				//	gx_printf("b discard id %d \n",i);
				}
			}

			for(i=0;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					s_iemm_softbuff_t[i].m_used = 0 ;

				//	gx_printf("c discard id %d \n",i);
				}
			}
		}

		s_iemm_read_buf_index = w ;
	}

	return 0 ;
}

static int gx3211_iemm_get_event(int id ,GxIemmProperty_InterruptEvent *p_InterruptEvent)
{
	int i;
	int bFind = 0 ;
	unsigned int event = 0 ;

	int r,w ;

	event = s_iemm_interrupt_event ;

	if ( event & 0x1 )
	{
		p_InterruptEvent->m_overflow_interrupt = 1 ;
		p_InterruptEvent->m_hardware_software_interrupt = ( (event>>16) & 0x1 );
	}
	else
	{
		p_InterruptEvent->m_overflow_interrupt = 0 ;
		p_InterruptEvent->m_hardware_software_interrupt = 0 ;
	}

	if ( p_InterruptEvent->m_only_get_overflow_interrupt )
	{
		return 0 ;
	}

	if ( p_InterruptEvent->m_p_data == NULL)
	{
		return -1 ;
	}

	event = 0 ;
	bFind = 0 ;

	r = s_iemm_read_buf_index  ;
	w = s_iemm_write_buf_index ;

	if ( r == w )
	{
		i = r ;
		if (s_iemm_softbuff_t[i].m_used)
		{
			bFind = 1 ;
		}
	}
	else
	{
		if ( w > r )
		{
			for(i=r;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					bFind = 1 ;
					break;
				}
			}
		}
		else
		{
			for(i=r;i<IEMM_SOFT_BUFF_NUM ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					bFind = 1 ;
					break;
				}
			}

			if ( bFind == 0 )
			{
				for(i=0;i<w ;i++)
				{
					if (s_iemm_softbuff_t[i].m_used)
					{
						bFind = 1 ;
						break;
					}
				}
			}
		}
	}

	if ( bFind )
	{
		gx_copy_to_user(p_InterruptEvent->m_p_data,s_iemm_softbuff_t[i].m_Buff, 180);

		if ( i < (IEMM_SOFT_BUFF_NUM-1) )
		{
			s_iemm_read_buf_index = (i+1);
		}
		else
		{
			s_iemm_read_buf_index = 0 ;
		}

		s_iemm_softbuff_t[i].m_used = 0;

		p_InterruptEvent->m_arriver_interrupt = 1 ;

	//	gx_printf("\n get event %d , %d \n",i,s_iemm_read_buf_index);

		return 0 ;
	}
	else
	{
		p_InterruptEvent->m_arriver_interrupt = 0 ;

		return 0 ;
	}
}

static int gx3211_iemm_copy_data(unsigned char *p_buff,unsigned int i)
{
//	gx_printf("copy data to 0x%x , id %d \n",(int)p_buff,i);

	gx_copy_to_user(p_buff,s_iemm_softbuff_t[i].m_Buff, 180);

	if ( i < (IEMM_SOFT_BUFF_NUM-1) )
	{
		s_iemm_read_buf_index = (i+1);
	}
	else
	{
		s_iemm_read_buf_index = 0 ;
	}

	s_iemm_softbuff_t[i].m_used = 0;

	return 0 ;
}

static int gx3211_iemm_read_data_buff(int id, GxIemmProperty_ReadDataBuff *p_ReadDataBuff)
{
	int i;
	unsigned int read_list = 0 ;
	unsigned int ret_list = 0 ;

	int r,w ;

	read_list =  p_ReadDataBuff->m_input_buf_list ;
	p_ReadDataBuff->m_output_buf_list = 0 ;

	if(read_list<=0 || read_list > 10)
	{
		return 0 ;
	}

	r = s_iemm_read_buf_index  ;
	w = s_iemm_write_buf_index ;

//	gx_printf("read buff: read %d, write %d \n",r,w);

	if ( w > r )
	{
		for(i=r;i<w ;i++)
		{
			if (s_iemm_softbuff_t[i].m_used)
			{
				gx3211_iemm_copy_data(p_ReadDataBuff->m_p_data[ret_list],i);
				ret_list++ ;
			}

			if (ret_list >= read_list )
				break;
		}
	}
	else
	{
		for(i=r;i<IEMM_SOFT_BUFF_NUM ;i++)
		{
			if (s_iemm_softbuff_t[i].m_used)
			{
				gx3211_iemm_copy_data(p_ReadDataBuff->m_p_data[ret_list],i);
				ret_list++ ;
			}

			if (ret_list >= read_list )
                break;
		}

		if ( ret_list < read_list )
		{
			for(i=0;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					gx3211_iemm_copy_data(p_ReadDataBuff->m_p_data[ret_list],i);
					ret_list++ ;
				}

				if (ret_list >= read_list )
					break;
			}
		}
	}

	p_ReadDataBuff->m_output_buf_list = ret_list ;

	return 0 ;
}

static int gx3211_iemm_read_data_list(int id, GxIemmProperty_ReadDataList *p_ReadDataList)
{
	int i;
	unsigned int list = 0 ;

	int r,w ;

	list = 0 ;

	r = s_iemm_read_buf_index  ;
	w = s_iemm_write_buf_index ;

	if ( r == w )
	{
		i = r ;
		if (s_iemm_softbuff_t[i].m_used)
		{
			for(i=r;i<IEMM_SOFT_BUFF_NUM ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					list++ ;
				}
			}

			for(i=0;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					list++ ;
				}
			}
		}
	}
	else
	{
		if ( w > r )
		{
			for(i=r;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					list++ ;
				}
			}
		}
		else
		{
			for(i=r;i<IEMM_SOFT_BUFF_NUM ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					list++ ;
				}
			}

			for(i=0;i<w ;i++)
			{
				if (s_iemm_softbuff_t[i].m_used)
				{
					list++ ;
				}
			}
		}
	}

//	if (list )
//		gx_printf("read list %d,%d,%d \n",list, r,w);

	p_ReadDataList->m_output_buf_list = list ;

	return 0 ;
}

static int  gx3211_iemm_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv)
{
	unsigned int OverflowIntOccur,ArrivedIntOccur ;

	unsigned int    Old_Order,New_Order;
	unsigned char    *BufAddr;

	int i,j;

	int bFind = 0 ;

	OverflowIntOccur = 0 ;
	ArrivedIntOccur  = 0 ;

	HAL_IEMM_GetInterruptStatus(&OverflowIntOccur,&ArrivedIntOccur);

	if ( OverflowIntOccur )
	{
//		gx_printf("!!!!!!!!!!!!!!!!!!!!!!! iemm hd of \n");

		s_iemm_interrupt_event = (1<<16)|(0x1) ;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
		if (callback)
			callback(EVENT_IEMM_OVERFLOW,priv);
#endif

		return 0 ;
	}

	if ( ArrivedIntOccur )
	{
		HAL_IEMM_GetBuffOrder(&Old_Order);

		BufAddr = (unsigned char*)( s_iemm_out_buf + Old_Order * ONE_BUF_SIZE);

		HAL_IEMM_GetBuffOrder(&New_Order);

                 //test_printf("\n iemm int aa order %d,%d \n",Old_Order,New_Order);

		do
		{
			bFind = 0 ;

			if ( s_iemm_write_buf_index > IEMM_SOFT_BUFF_NUM )
			{
				s_iemm_write_buf_index = 0 ;
			}

			for (i=s_iemm_write_buf_index; i<IEMM_SOFT_BUFF_NUM; i++)
			{
				if (!s_iemm_softbuff_t[i].m_used)
				{
					bFind = 1 ;
					break;
				}
			}
			if ( 0 == bFind )
			{
				for(i=0;i<s_iemm_write_buf_index ;i++)
				{
					if (!s_iemm_softbuff_t[i].m_used)
					{
						bFind = 1 ;
						break;
					}
				}
			}

			if ( bFind )
			{
				s_iemm_softbuff_t[i].m_used = 1;

				if ( i < (IEMM_SOFT_BUFF_NUM-1) )
				{
					s_iemm_write_buf_index = (i+1);
				}
				else
				{
					s_iemm_write_buf_index = 0 ;
				}

				gx_dcache_flush_range(0,0);

				for(j=0;j<IEMM_BUFF_SIZE;j++)
				{
					s_iemm_softbuff_t[i].m_Buff[j] = BufAddr[4+j] ;
				}

			        //test_printf("\n iemm int write %d,%d \n",i,s_iemm_write_buf_index);
			}
			else
			{
			//	gx_printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ iemm sf of \n");

				s_iemm_interrupt_event = (0<<16)|(0x1) ;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
				if (callback)
					callback(EVENT_IEMM_OVERFLOW,priv);
#endif

				return 0 ;
			}

			Old_Order = New_Order ;
			BufAddr = (unsigned char*)( s_iemm_out_buf + Old_Order * ONE_BUF_SIZE);

			HAL_IEMM_GetBuffOrder(&New_Order);

                        //test_printf("iemm int bb order %d,%d \n",Old_Order,New_Order);

	    }while(Old_Order != New_Order);

#ifdef ENABLE_INTTERUPT_SEND_EVENT
		if (callback)
			callback(EVENT_IEMM_DATA_ARRIVER,priv);
#endif

	}

	return 0 ;
}
static struct iemm_ops gx3211_iemm_ops = {
	.init          = gx3211_iemm_init,
	.cleanup       = gx3211_iemm_cleanup,
	.open          = gx3211_iemm_open,
	.close         = gx3211_iemm_close,
	.request       = gx3211_iemm_request,
	.stop          = gx3211_iemm_stop,
	.run           = gx3211_iemm_run,
	.update_read_index = gx3211_iemm_update_read_index,
	.get_event     = gx3211_iemm_get_event,
	.read_data_buff = gx3211_iemm_read_data_buff,
	.read_data_list = gx3211_iemm_read_data_list,
	.interrupt     = gx3211_iemm_interrupt,
};

struct gxav_module_ops gx3211_iemm_module = {
	.module_type  = GXAV_MOD_IEMM,
	.count        = 1,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gxav_iemm_open,
	.close        = gxav_iemm_close,
	.init         = gxav_iemm_init,
	.cleanup      = gxav_iemm_cleanup,
	.set_property = gxav_iemm_set_property,
	.get_property = gxav_iemm_get_property,
	.priv         = &gx3211_iemm_ops
};
