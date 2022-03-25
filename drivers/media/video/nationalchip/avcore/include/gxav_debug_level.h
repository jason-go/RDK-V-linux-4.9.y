#ifndef __GXAV_DEBUG_LEVEL_H__
#define __GXAV_DEBUG_LEVEL_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "gxav_common.h"

typedef enum {
	GXAV_DEBUG_LEVEL_0  = 0,  //不开启打印级别
	GXAV_DEBUG_LEVEL_1  = 1,  //开启打印级别: ERROR
	GXAV_DEBUG_LEVEL_2  = 2,  //开启打印级别: ERROR INFO
	GXAV_DEBUG_LEVEL_3  = 3,  //开启打印级别: ERROR INFO WARNING
	GXAV_DEBUG_LEVEL_4  = 4,  //开启打印级别: ERROR INFO WARNING DEBUG
} GxAvDebugLevel;

struct gxav_debug_config {
	GxAvModuleType  module;
	GxAvDebugLevel  level;
};

int gxav_debug_level_config(struct gxav_debug_config *config);

#ifdef __cplusplus
}
#endif


#endif
