#include "kernelcalls.h"
#include "osdep-dma.h"

#define GX_DMAHOLE_MAX (64)

static struct gxav_dmainfo  _gxav_dmainfo[GX_DMAHOLE_MAX];
static int                _gxav_dmainfo_inited = 0;
#ifdef LINUX_OS
static gx_spin_lock_t     _gxav_dmainfo_lock;
#endif

void gxav_dmainfo_init(void)
{
	if (_gxav_dmainfo_inited == 0) {
		gx_printf("[GXAVDEV|INFO]: %s called!\n", __func__);
		gx_spin_lock_init(&_gxav_dmainfo_lock);
		memset(&_gxav_dmainfo, 0, sizeof(_gxav_dmainfo));
		_gxav_dmainfo_inited = 1;
	}
}

int gxav_dmainfo_add(unsigned int virt, unsigned int phys, unsigned int size)
{
	int i = 0;
	unsigned long flags;

	gx_spin_lock_irqsave(&_gxav_dmainfo_lock, flags);

	for (i = 0; i < GX_DMAHOLE_MAX; i++) {
		if (_gxav_dmainfo[i].virt == 0) {
			_gxav_dmainfo[i].virt = virt;
			_gxav_dmainfo[i].phys = phys;
			_gxav_dmainfo[i].size = size;
			gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);
			return 0;
		}
	}

	gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);

	gx_printf("[ERROR]: %s: out of size\n", __func__);
	return -1;
}

int gxav_dmainfo_remove(unsigned int virt)
{
	int i = 0;
	unsigned long flags;

	gx_spin_lock_irqsave(&_gxav_dmainfo_lock, flags);

	for (i = 0; i < GX_DMAHOLE_MAX; i++) {
		if (virt >= _gxav_dmainfo[i].virt && virt < (_gxav_dmainfo[i].virt + _gxav_dmainfo[i].size)) {
			memset(&_gxav_dmainfo[i], 0, sizeof(_gxav_dmainfo[i]));
			gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);
			return 0;
		}
	}

	gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);

	gx_printf("[ERROR]: %s: out of size\n", __func__);
	return -1;
}

struct gxav_dmainfo *gxav_dmainfo_find(unsigned int virt)
{
	int i = 0;
	unsigned long flags;

	if (_gxav_dmainfo_inited == 0) {
		gx_printf("[GXAVDEV|WARNING]: Call %s before initialized!\n", __func__);
		return NULL;
	}

	gx_spin_lock_irqsave(&_gxav_dmainfo_lock, flags);

	for (i = 0; i < GX_DMAHOLE_MAX; i++) {
		if (virt >= _gxav_dmainfo[i].virt && virt < (_gxav_dmainfo[i].virt + _gxav_dmainfo[i].size)) {
			gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);
			return &_gxav_dmainfo[i];
		}
	}

	gx_spin_unlock_irqrestore(&_gxav_dmainfo_lock, flags);

	return NULL;
}

