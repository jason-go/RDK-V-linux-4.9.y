#ifndef SOFT_FRONTED_H
#define SOFT_FRONTED_H

#include <linux/dvb/frontend.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

struct soft_frontend {
	int soft;
	struct tp_node *tp_list;
	int tp_count;
	int tp_max;
	int current_tp_id;
	struct kfifo fifo;
};

int sfe_read(struct soft_frontend *sfe, void *buf, size_t size);
int sfe_write(struct soft_frontend *sfe, struct ts_data *data);
void sfe_reset(struct soft_frontend *sfe);
int sfe_find(struct soft_frontend *sfe, struct tp_node *tp);
struct tp_node *sfe_find_byfreq(struct soft_frontend *sfe, unsigned int freq);
void sfe_del(struct soft_frontend *sfe, int id);
void sfe_add(struct soft_frontend *sfe, struct tp_node *tp);
int sfe_init(struct soft_frontend *sfe);

#endif
