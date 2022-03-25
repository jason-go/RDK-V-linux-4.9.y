#ifndef __GX6605S_DEMUX__
#define __GX6605S_DEMUX__

#include "gxdemux.h"

void gx6605s_set_even_valid(void *vreg, unsigned int slotid);
void gx6605s_set_odd_valid(void *vreg, unsigned int slotid);
void gx6605s_set_oddkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int flags);
void gx6605s_set_evenkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int flags);
void gx6605s_set_descid(void *vreg, unsigned int slotid,unsigned int caid, unsigned int flags);

void gx6605s_set_input_source(void *vreg, unsigned int dmxid,unsigned int source);
void gx6605s_set_dvr_source(int dmxid, int tswid, void *vreg, struct dvr_info *info);

#endif
