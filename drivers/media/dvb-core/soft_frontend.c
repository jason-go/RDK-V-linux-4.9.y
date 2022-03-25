#include "soft_frontend.h"
#include <linux/kfifo.h>
#include <linux/slab.h>

#define INC_SIZE  (128)
#define FIFO_SIZE (32 * 1024)

int sfe_init(struct soft_frontend *sfe)
{
	int err;
	err = kfifo_alloc(&sfe->fifo, FIFO_SIZE, GFP_KERNEL);
	if (err < 0 )
		return -1;

	sfe->soft = 1;
	sfe->tp_list = kmalloc(sizeof(struct tp_node) * INC_SIZE, GFP_KERNEL);
	sfe->tp_count = 0;
	sfe->tp_max = INC_SIZE;

	return 0;
}

void sfe_add(struct soft_frontend *sfe, struct tp_node *tp)
{
	if (sfe->tp_count == sfe->tp_max) {
		struct tp_node *p;

		sfe->tp_max += INC_SIZE;
		p = (struct tp_node*)kmalloc(sizeof(struct tp_node) * sfe->tp_max, GFP_KERNEL);
		if (p == NULL)
			return;

		memcpy(p, sfe->tp_list, sizeof(struct tp_node) * sfe->tp_count);
		kfree(sfe->tp_list);
		sfe->tp_list = p;
	}

	memcpy(&sfe->tp_list[sfe->tp_count], tp, sizeof(struct tp_node));
	sfe->tp_count++;
}

void sfe_del(struct soft_frontend *sfe, int id)
{
	if (id < sfe->tp_count - 1)
		memcpy(&sfe->tp_list[id], &sfe->tp_list[id + 1], sizeof(struct tp_node) * (sfe->tp_count - id) - 1);
}

int sfe_find(struct soft_frontend *sfe, struct tp_node *tp)
{
	int i;

	for (i=0; i < sfe->tp_count; i ++) {
		if (sfe->tp_list[i].id == tp->id)
			return i;
	}

	return -1;
}

void sfe_reset(struct soft_frontend *sfe)
{
	sfe->tp_count = 0;
	kfifo_reset(&sfe->fifo);
}

int sfe_write(struct soft_frontend *sfe, struct ts_data *data)
{
	if (sfe->current_tp_id == data->tp_id)
		return kfifo_in(&sfe->fifo, data->buffer, data->size);
	return 0;
}

int sfe_read(struct soft_frontend *sfe, void *buf, size_t size)
{
	return kfifo_out(&sfe->fifo, buf, size);
}

struct tp_node *sfe_find_byfreq(struct soft_frontend *sfe, unsigned int freq)
{
	int i;

	for (i=0; i < sfe->tp_count; i ++) {
		if (sfe->tp_list[i].freq == freq)
			return &sfe->tp_list[i];
	}

	return NULL;
}

