#ifndef __TAURUS_PID_FILTER_H__
#define __TAURUS_PID_FILTER_H__

int taurus_pfilter_init(void);
int taurus_pfilter_deinit(void);
void taurus_pfilter_set_ctrl(struct dvr_pfilter *pfilter);

#endif

