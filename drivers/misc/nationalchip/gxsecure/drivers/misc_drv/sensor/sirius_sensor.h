#ifndef __SIRIUS_SENSOR_H__
#define __SIRIUS_SENSOR_H__

int32_t sirius_sensor_init(GxSeModuleHwObj *obj);
int32_t sirius_sensor_get_err_status(GxSeModuleHwObj *obj, GxSeSensorErrStatus *param);
int32_t sirius_sensor_get_err_value(GxSeModuleHwObj *obj, uint32_t *status);

#endif
