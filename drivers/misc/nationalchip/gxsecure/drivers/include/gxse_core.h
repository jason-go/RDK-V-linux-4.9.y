#ifndef __GXSE_CORE_H__
#define __GXSE_CORE_H__

#include <kernelcalls.h>
#include "gxtfm.h"
#include "gxfirewall.h"
#include "gxsecure.h"
#include "gxsci.h"
#include "gxse_baseaddr.h"

#define GXSE_SUCCESS      (0)
#define GXSE_ERR_GENERIC  (-1)
#define GXSE_ERR_PARAM    (-2)
#define GXSE_ERR_NODEV    (-3)
#define GXSE_ERR_NOIMP    (-4)

#define GXSE_MEM_SAME      (0)
#define GXSE_MEM_INSIDE    (-1)
#define GXSE_MEM_OUTSIDE   (-2)
#define GXSE_MEM_CROSSOVER (-3)
#define GXSE_MEM_FULL      (-4)

#define GXSE_MEM_VIRT      (0)
#define GXSE_MEM_PHYS      (1)

#define GXSE_MAX_SECMEM    (12)
#define GXSE_MAX_MEMHOLE   (32)
#define GXSE_MAX_DEV_NAME  (32)

#define GXSE_MAX_CRYPTO_COUNT   (GXSE_MOD_CRYPTO_MAX - GXSE_MOD_CRYPTO_FIFO)
#define GXSE_MAX_KLM_COUNT      (GXSE_MOD_KLM_MAX - GXSE_MOD_KLM_GENERIC)
#define GXSE_MAX_MISC_COUNT     (GXSE_MOD_MISC_MAX - GXSE_MOD_MISC_CHIP_CFG)
#define GXSE_MAX_SECURE_COUNT   (GXSE_MOD_SECURE_MAX - GXSE_MOD_SECURE_MBOX)
#define GXSE_MAX_FIREWALL_COUNT (GXSE_MOD_MISC_CHIP_CFG - GXSE_MOD_MISC_FIREWALL)

#define GXSE_CRYPTO_ID(n)   (n-GXSE_MOD_CRYPTO_FIFO)
#define GXSE_KLM_ID(n)      (n-GXSE_MOD_KLM_GENERIC)
#define GXSE_MISC_ID(n)     (n-GXSE_MOD_MISC_CHIP_CFG)
#define GXSE_FIREWALL_ID(n) (n-GXSE_MOD_MISC_FIREWALL)
#define GXSE_SECURE_ID(n)   (n-GXSE_MOD_SECURE_MBOX)

#define GXSE_FIREWALL_DEVNAME "/dev/gxfirewall"
#define GXSE_CRYPTO_DEVNAME   "/dev/gxcrypto"
#define GXSE_SECURE_DEVNAME   "/dev/gxsecure"
#define GXSE_MISC_DEVNAME     "/dev/gxmisc"
#define GXSE_KLM_DEVNAME      "/dev/gxklm"

#define GXSE_CMD_RX         (GXSE_IOR('g', 1, 0))
#define GXSE_CMD_TX         (GXSE_IOW('g', 2, 0))

#define PROBE_CHIP(name) \
	extern int gxse_chip_register_##name(void); \
	gxse_chip_register_##name();

enum {
	GXSE_POLL_NONE,
	GXSE_POLL_R,
	GXSE_POLL_W,
};

#ifdef CPU_ACPU
#define GXSE_MAX_MOD_COUNT (0x40)
#define GXSE_MAX_MOD_IRQ   (2)
#define GXSE_MAX_DEV_IRQ   (8)
#define GXSE_MAX_IRQ       (8)
typedef enum {
	GXSE_MOD_CRYPTO_FIFO = 0x0,
	GXSE_MOD_CRYPTO_DMA0,
	GXSE_MOD_CRYPTO_DMA1,
	GXSE_MOD_CRYPTO_DMA2,
	GXSE_MOD_CRYPTO_DMA3,
	GXSE_MOD_CRYPTO_DMA4,
	GXSE_MOD_CRYPTO_DMA5,
	GXSE_MOD_CRYPTO_DMA6,
	GXSE_MOD_CRYPTO_DMA7,
	GXSE_MOD_CRYPTO_MAX,

	GXSE_MOD_KLM_GENERIC = 0x10,
	GXSE_MOD_KLM_IRDETO,
	GXSE_MOD_KLM_SCPU_GENERIC,
	GXSE_MOD_KLM_SCPU_IRDETO,
	GXSE_MOD_KLM_SCPU_IRDETO_GENERIC,
	GXSE_MOD_KLM_MAX,

	GXSE_MOD_MISC_FIREWALL = 0x20,
	GXSE_MOD_MISC_CHIP_CFG,
	GXSE_MOD_MISC_OTP,
	GXSE_MOD_MISC_RNG,
	GXSE_MOD_MISC_DGST,
	GXSE_MOD_MISC_AKCIPHER,
	GXSE_MOD_MISC_SCI,
	GXSE_MOD_MISC_MAX,

	GXSE_MOD_SECURE_MBOX = 0x30,
	GXSE_MOD_SECURE_MBOX_TEE,
	GXSE_MOD_SECURE_MAX,
} GxSeModuleID;
#else

#define GXSE_MAX_MOD_COUNT (0x20)
#define GXSE_MAX_MOD_IRQ   (0)
#define GXSE_MAX_DEV_IRQ   (0)
#define GXSE_MAX_IRQ       (0)
typedef enum {
	GXSE_MOD_CRYPTO_FIFO = 0x0,
	GXSE_MOD_CRYPTO_DYNAMIC,
	GXSE_MOD_CRYPTO_DMA0,
	GXSE_MOD_CRYPTO_MAX,

	GXSE_MOD_KLM_GENERIC = 0x8,
	GXSE_MOD_KLM_IRDETO,
	GXSE_MOD_KLM_SCPU_GENERIC,
	GXSE_MOD_KLM_SCPU_IRDETO,
	GXSE_MOD_KLM_SCPU_IRDETO_GENERIC,
	GXSE_MOD_KLM_MAX,

	GXSE_MOD_MISC_ALL = 0x10,
	GXSE_MOD_MISC_DGST,
	GXSE_MOD_MISC_MAX,

	GXSE_MOD_SECURE_MBOX = 0x18,
	GXSE_MOD_SECURE_MBOX_TEE,
	GXSE_MOD_SECURE_MAX,
} GxSeModuleID;
#define GXSE_MOD_MISC_CHIP_CFG (GXSE_MOD_MISC_ALL)
#define GXSE_MOD_MISC_OTP      (GXSE_MOD_MISC_ALL)
#define GXSE_MOD_MISC_RNG      (GXSE_MOD_MISC_ALL)
#define GXSE_MOD_MISC_SENSOR   (GXSE_MOD_MISC_ALL)
#define GXSE_MOD_MISC_TIMER    (GXSE_MOD_MISC_ALL)
#endif

typedef struct {
	uint32_t reg_base;
	uint32_t reg_len;

	int32_t  irqs[GXSE_MAX_MOD_IRQ];
	const char *irq_names[GXSE_MAX_MOD_IRQ];

	uint32_t clk;
} GxSeModuleResource;

typedef enum {
	GXSE_HWOBJ_TYPE_MISC,
	GXSE_HWOBJ_TYPE_TFM,
	GXSE_HWOBJ_TYPE_FIRMWARE,
} GxSeModuleHwObjType;

typedef struct {
	GxSeModuleHwObjType type;
	uint32_t            sub;
	void               *ops;

	gx_mutex_t         *mutex;
	void               *reg;
	void               *priv;
} GxSeModuleHwObj;

struct gxse_module;
typedef struct {
	int32_t (*ioctl)  (struct gxse_module *module, uint32_t cmd, void *param, uint32_t size);
	int32_t (*init)   (struct gxse_module *module);
#ifdef CPU_ACPU
	int32_t (*deinit) (struct gxse_module *module);
	int32_t (*open)   (struct gxse_module *module);
	int32_t (*close)  (struct gxse_module *module);
	int32_t (*read)   (struct gxse_module *module, uint8_t *buf, uint32_t len);
	int32_t (*write)  (struct gxse_module *module, const uint8_t *buf, uint32_t len);
	int32_t (*poll)   (struct gxse_module *module, int32_t which, void **r_wait, void **w_wait);
	int32_t (*isr)    (struct gxse_module *module);
	int32_t (*dsr)    (struct gxse_module *module);

	// The following functinos can only be implemented by device-layer.
	int32_t (*copy_from_usr) (void **k_ptr, void *u_ptr, uint32_t len, uint32_t cmd);
	int32_t (*copy_to_usr)   (void *u_ptr, void **k_ptr, uint32_t len, uint32_t cmd);
#endif
} GxSeModuleOps;

typedef struct gxse_module {
	GxSeModuleID  id;
	GxSeModuleOps *ops;
	GxSeModuleResource res;
	GxSeModuleHwObj *hwobj;
} GxSeModule;

typedef struct {
	int32_t (*ioctl)      (GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
	int32_t (*init)       (GxSeModuleHwObj *obj);
#ifdef CPU_ACPU
	int32_t (*deinit)     (GxSeModuleHwObj *obj);
	int32_t (*open)       (GxSeModuleHwObj *obj);
	int32_t (*close)      (GxSeModuleHwObj *obj);
	int32_t (*read)       (GxSeModuleHwObj *obj, uint8_t *buf, uint32_t len);
	int32_t (*write)      (GxSeModuleHwObj *obj, const uint8_t *buf, uint32_t len);
	int32_t (*poll)       (GxSeModuleHwObj *obj, int32_t which, void **r_wait, void **w_wait);
	int32_t (*isr)        (GxSeModuleHwObj *obj);
	int32_t (*dsr)        (GxSeModuleHwObj *obj);
	int32_t (*setup)      (GxSeModuleHwObj *obj, GxSeModuleResource *res);
#endif
} GxSeModuleDevOps;

struct fetch_ops {
	void *devops;
	void *hwops;
};

#define GXSE_OBJ_HWOPS(obj)  (((struct fetch_ops *)(obj->ops))->hwops)
#define GXSE_OBJ_DEVOPS(obj) (((struct fetch_ops *)(obj->ops))->devops)
#define mod_ops(mod) ((GxSeModuleOps *)(mod->ops))

typedef struct {
	char         devname[GXSE_MAX_DEV_NAME];
	GxSeModule  *module;

	gx_mutex_t   mutex;
	void        *priv;
	uint32_t     refcount;
} GxSeDevice;

typedef struct {
	unsigned int phys;
	unsigned int virt;
	unsigned int size;
} GxSeMemhole;

typedef struct {
	unsigned int phys;
	unsigned int size;
} GxSeSecureMem;

typedef struct {
	uint16_t id;
	int16_t irq;
	uint32_t device_count;
	uint32_t device_mask;
	GxSeDevice *device[GXSE_MAX_DEV_IRQ];
} GxSeDeviceIRQ;

int32_t gxse_secmem_probe(uint32_t addr, uint32_t size);
int32_t gxse_secmem_probe_byname(char *name, uint32_t *addr, uint32_t *size);
int32_t gxse_secmem_is_illegal(char *name, uint32_t addr, uint32_t size);
int32_t gxse_secmem_register(uint32_t addr, uint32_t size);
int32_t gxse_memhole_probe(uint32_t addr, uint32_t size, uint32_t *_addr, uint32_t flags);
int32_t gxse_memhole_probe_addr(uint32_t addr, uint32_t *_addr, uint32_t flags);
int32_t gxse_memhole_register(uint32_t vaddr, uint32_t paddr, uint32_t size);

void gxse_irqlist_init(void);
GxSeDeviceIRQ *gxse_irqlist_get(uint16_t irq);
int32_t gxse_irqlist_remove(uint32_t id);

int32_t gxse_module_devname_2_moduleid(const char *devname);
GxSeModule *gxse_module_find_by_id(GxSeModuleID id);
GxSeModule *gxse_module_find_by_devname(const char *devname);
GxSeModule *gxse_module_find_by_tfmid(GxTfmModule mod, uint32_t sub);
int32_t gxse_module_register(GxSeModule *module);
int32_t gxse_module_unregister(GxSeModule *module);
int32_t gxse_module_init(GxSeModule *module);
int32_t gxse_module_deinit(GxSeModule *module);
int32_t gxse_module_open(GxSeModule *module);
int32_t gxse_module_close(GxSeModule *module);
int32_t gxse_module_isr(GxSeModule *module);
int32_t gxse_module_dsr(GxSeModule *module);
int32_t gxse_module_read(GxSeModule *module, uint8_t *buf, uint32_t len);
int32_t gxse_module_write(GxSeModule *module, const uint8_t *buf, uint32_t len);
int32_t gxse_module_poll(GxSeModule *module, int32_t which, void **r_wait, void **w_wait);
int32_t gxse_module_ioctl_check(GxSeModule *module, GxSeModuleHwObjType type, uint32_t cmd, uint32_t size);

int32_t gxse_device_init(GxSeDevice *dev);
int32_t gxse_device_deinit(GxSeDevice *dev);
int32_t gxse_device_open(GxSeDevice *dev);
int32_t gxse_device_close(GxSeDevice *dev);
int32_t gxse_device_read(GxSeDevice *dev, uint8_t *buf, uint32_t size);
int32_t gxse_device_write(GxSeDevice *dev, const uint8_t *buf, uint32_t size);
int32_t gxse_device_ioctl(GxSeDevice *dev, uint32_t cmd, void *param, uint32_t size);
int32_t gxse_device_poll(GxSeDevice *dev, int32_t which, void **r_wait, void **w_wait);
int32_t gxse_device_isr(GxSeDevice *dev);
int32_t gxse_device_dsr(GxSeDevice *dev);

struct mutex_static_priv {
	gx_mutex_t mutex;
};
int32_t gxse_hwobj_mutex_static_init(GxSeModuleHwObj *obj);
int32_t gxse_hwobj_mutex_static_deinit(GxSeModuleHwObj *obj);

#endif
