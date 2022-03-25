#ifndef GXCHIP_INIT_H
#define GXCHIP_INIT_H

#ifdef ECOS_OS
#define DEFINE_CHIP(name)                           \
    extern "C" int gxav_chip_register_##name(void); \
    class cyg_chip_class_##name {                   \
        public:                                     \
            cyg_chip_class_##name(void) {           \
                gxav_chip_register_##name();        \
            }                                       \
    }
#else
#define DEFINE_CHIP(name)                           \
    extern int gxav_chip_register_##name(void);
#define REGISTER_GXAVDEV(name)                      \
    do {                                            \
        gxav_chip_register_##name();                \
        gxav_init(NULL);                            \
    } while (0);
#endif

DEFINE_CHIP( gx3201  );
DEFINE_CHIP( gx3211  );
DEFINE_CHIP( gx3113c );
DEFINE_CHIP( gx6605s );
DEFINE_CHIP( sirius  );
DEFINE_CHIP( taurus  );
DEFINE_CHIP( gemini  );
DEFINE_CHIP( cygnus  );
#endif
