#ifndef __SIRIUS_RNG_H__
#define __SIRIUS_RNG_H__

extern GxSeModuleHwObj sirius_rng_hwobj;
int32_t sirius_misc_rng_request(GxSeModuleHwObj *obj, uint32_t *val);

#endif
