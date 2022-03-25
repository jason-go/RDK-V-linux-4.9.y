#ifndef __GXSE_CHIP_ALL_H__
#define __GXSE_CHIP_ALL_H__

#define DEFINE_CHIP_REGISTER(chip, base)            \
		extern int gxse_chip_register_##base(void); \
		int gxse_chip_register_##chip(void) {       \
			return gxse_chip_register_##base();     \
		}

#define DEFINE_DEV_REGISTER(chip, base, mod)                  \
		extern int gxse_device_register_##base##_##mod(void); \
		int gxse_device_register_##chip##_##mod(void) {         \
			return gxse_device_register_##base##_##mod();     \
		}

#define DEFINE_MODULE_REGISTER(chip, base, major, sub)                  \
		extern int gxse_device_register_##base##_##major##_##sub(void); \
		int gxse_device_register_##chip##_##major##_##sub(void) {       \
			return gxse_device_register_##base##_##major##_##sub();     \
		}

int gxse_chip_register_all(void);
#endif

