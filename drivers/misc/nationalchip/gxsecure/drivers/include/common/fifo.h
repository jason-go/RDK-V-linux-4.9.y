#ifndef GXSE_FIFO_H
#define GXSE_FIFO_H

#define FIFO_FLAG_MALLOC  (1<<31)
typedef struct {
	unsigned char *buffer;
	unsigned int len;
	unsigned int in;
	unsigned int out;
	unsigned int flags;
} GxSeFifo;

int gxse_common_fifo_init(GxSeFifo *fifo, void *buffer, unsigned int size);
void gxse_common_fifo_free(GxSeFifo *fifo);
unsigned int gxse_common_fifo_len(GxSeFifo *fifo);
unsigned int gxse_common_fifo_freelen(GxSeFifo *fifo);
unsigned int gxse_common_fifo_put(GxSeFifo *fifo, unsigned char *buf, unsigned int len);
unsigned int gxse_common_fifo_get(GxSeFifo *fifo, unsigned char *buf, unsigned int len);

#endif
