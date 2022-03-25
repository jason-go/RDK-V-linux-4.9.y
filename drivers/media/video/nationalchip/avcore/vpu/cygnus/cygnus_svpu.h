#ifndef __cygnus_svpu_h__
#define __cygnus_svpu_h__

int cygnus_svpu_init(void);
int cygnus_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo
		*buf_info);
int cygnus_svpu_run(void);
int cygnus_svpu_stop(void);

#endif

