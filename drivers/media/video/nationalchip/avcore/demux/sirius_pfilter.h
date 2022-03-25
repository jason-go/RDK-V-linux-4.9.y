#ifndef __SIRIUS_PID_FILTER_H__
#define __SIRIUS_PID_FILTER_H__

int sirius_pfilter_init(void);
int sirius_pfilter_deinit(void);
int sirius_pfilter_open(struct dvr_pfilter *pfilter);
int sirius_pfilter_close(struct dvr_pfilter *pfilter);
void sirius_pfilter_start(struct dvr_pfilter *pfilter);
void sirius_pfilter_stop(struct dvr_pfilter *pfilter);
void sirius_pfilter_set_ctrl(struct dvr_pfilter *pfilter);
void sirius_pfilter_set_gate(struct dvr_pfilter *pfilter);
void sirius_pfilter_check_left_data(struct dvr_pfilter *pfilter);
void sirius_pfilter_add_pid(struct dvr_pfilter *pfilter, int pid_handle, int pid);
void sirius_pfilter_del_pid(struct dvr_pfilter *pfilter, int pid_handle);
void sirius_pfilter_dealwith(struct dvr_pfilter *pfilter);
int sirius_pfilter_irq_entry(struct dvr_pfilter *pfilter);

#endif

