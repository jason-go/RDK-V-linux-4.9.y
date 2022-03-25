#ifndef __CYGNUS_DEMUX__
#define __CYGNUS_DEMUX__

#include "gxdemux.h"

void cygnus_set_reverse_pid(void *vreg, unsigned int dmxid, unsigned short pid);
void cygnus_tsw_update_rptr(void *dev, int id, unsigned int pread);
int  cygnus_tsw_isr_en(void *dev);
void cygnus_tsw_isr(void *dev, int manual_mode);
void cygnus_tsw_full_isr(void *dev);

#endif
