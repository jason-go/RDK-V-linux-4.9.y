/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    gx3211_icam.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */


#include "icam_hal.h"
#include "icam_28_include.h"
#include "icam_module.h"
#include "gx3211_reg.h"
#include "icam_28_reg.h"

#define ICAM_IRQ_SOURCE_NUMBER         (35)



extern Reg_NdsFunction_t *gx28_icam_reg ;
Smart_Regs_t      *gx3201_smartcard_reg = NULL;



static int gx3211_reg_icam_init(void)
{
	if(!gx_request_mem_region(ICAM_BASE_ADDR, sizeof(struct Reg_NdsFunction_s)))
	{
		return -1 ;
	}

	gx28_icam_reg = gx_ioremap(ICAM_BASE_ADDR, sizeof(struct Reg_NdsFunction_s));
	if (!gx28_icam_reg)
	{
		return -1;
	}

	if(!gx_request_mem_region(SMART_CARD_REG_ADDR, sizeof(struct  Smart_Regs_s)))
	{
		return -1 ;
	}

	gx3201_smartcard_reg = gx_ioremap(SMART_CARD_REG_ADDR, sizeof(struct Smart_Regs_s));
	if (!gx3201_smartcard_reg)
	{
		return -1;
	}

	return 0 ;
}

static int gx3211_reg_icam_cleanup(void)
{
	if ( gx28_icam_reg )
	{
		gx_iounmap(gx28_icam_reg);
		gx_release_mem_region(ICAM_BASE_ADDR, sizeof(struct Reg_NdsFunction_s));
		gx28_icam_reg = NULL;
	}

	if ( gx3201_smartcard_reg )
	{
		gx_iounmap(gx3201_smartcard_reg);
		gx_release_mem_region(SMART_CARD_REG_ADDR, sizeof(struct Smart_Regs_s));
		gx3201_smartcard_reg = NULL;
	}

	return 0 ;
}

static int gx3211_reg_icam_open(int id)
{
	return 0 ;
}

static int gx3211_reg_icam_close(int id)
{
	return 0 ;
}

static int gx3211_reg_icam_set_cardclockdivisor(int id ,unsigned freq)
{
	unsigned int ClkDiv = 0;

	CARD_ENABLE_WORK(gx3201_smartcard_reg);

	ClkDiv = (( SMART_CARD_FREQUENCY / freq )/2) -1;

	SET_BAUDRATE(gx3201_smartcard_reg, ClkDiv);

	CARD_COLD_RESET(gx3201_smartcard_reg);

	return 0 ;
}

static int gx3211_reg_icam_card_insert_remove(int id,unsigned char bRemove)
{
	if (bRemove)
	{
		CARD_DISABLE_WORK(gx3201_smartcard_reg);
	}
	return 0 ;
}

static struct icam_reg_ops gx3211_reg_icam_ops = {
	.init                 = gx3211_reg_icam_init,
	.cleanup              = gx3211_reg_icam_cleanup,
	.open                 = gx3211_reg_icam_open,
	.close                = gx3211_reg_icam_close,
	.set_cardclockdivisor = gx3211_reg_icam_set_cardclockdivisor,
	.card_insert_remove   = gx3211_reg_icam_card_insert_remove,
};

static struct icam_hal_ops gx28_icam_ops = {
	.priv                  = &gx3211_reg_icam_ops,
	.init                  = gx28_icam_init,
	.cleanup               = gx28_icam_cleanup,
	.open                  = gx28_icam_open,
	.close                 = gx28_icam_close,
	.set_cardclockdivisor  = gx28_icam_set_cardclockdivisor,
	.set_vcclevel          = gx28_icam_set_vcclevel,
	.set_uartconvdirection = gx28_icam_set_uartconvdirection,
	.set_uartbaudrate      = gx28_icam_set_uartbaudrate,
	.set_vccswitch         = gx28_icam_set_vccswitch,
	.set_uartguardtime     = gx28_icam_set_uartguardtime,
	.set_resetcard         = gx28_icam_set_resetcard,
	.set_uartcommand       = gx28_icam_set_uartcommand,
	.send_receive          = gx28_icam_send_receive,
	.receive               = gx28_icam_receive,
	.card_insert_remove    = gx28_icam_card_insert_remove,
	.set_controlword       = gx28_icam_set_controlword,
	.set_config            = gx28_icam_set_config,
	.get_uartstatue        = gx28_icam_get_uartstatue,
	.get_uartcommond       = gx28_icam_get_uartcommond,
	.get_card_event        = gx28_icam_get_card_event,
	.get_event             = gx28_icam_get_event,
	.get_chipproperties    = gx28_icam_get_chipproperties,
	.get_responsetochallenge = gx28_icam_get_responsetochallenge,
	.get_encryptdata       = gx28_icam_get_encryptdata,
	.get_read_register     = gx28_icam_get_read_register,
	.get_write_register    = gx28_icam_get_write_register,
	.get_config            = gx28_icam_get_config,
	.get_read_otp          = gx28_icam_get_read_otp,
	.get_write_otp         = gx28_icam_get_write_otp,
	.interrupt             = gx28_icam_interrupt,
};

struct gxav_module_ops gx3211_icam_module = {
	.module_type  = GXAV_MOD_ICAM,
	.count        = 1,
	.irqs         = {ICAM_IRQ_SOURCE_NUMBER, -1},
	.irq_flags    = {GXAV_IRQ_FAST,-1},
	.event_mask   = 0xff,
	.open         = gxav_icam_open,
	.close        = gxav_icam_close,
	.init         = gxav_icam_init,
	.cleanup      = gxav_icam_cleanup,
	.set_property = gxav_icam_set_property,
	.get_property = gxav_icam_get_property,
	.interrupts[ICAM_IRQ_SOURCE_NUMBER]    = gxav_icam_interrupt,
	.priv = &gx28_icam_ops
};
