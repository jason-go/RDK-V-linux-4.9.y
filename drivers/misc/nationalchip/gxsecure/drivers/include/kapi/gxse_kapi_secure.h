#ifndef __GXSE_SECURE_KAPI_H__
#define __GXSE_SECURE_KAPI_H__

extern int gxse_secure_load_firmware         ( GxSecureLoader *firm);
extern int gxse_secure_reset_firmware        ( void );
extern int gxse_secure_get_firmware_status   ( void );
extern int gxse_secure_set_BIV               ( GxSecureImageVersion *param );
extern int gxse_secure_get_BIV               ( GxSecureImageVersion *param );
extern int gxse_secure_autotest_send         ( void *cmd );
extern int gxse_secure_autotest_recv         ( void *rst );
extern int gxse_secure_send_irdeto_cmd       ( GxTfmKlm *key );
extern int gxse_secure_send_root_key         ( GxTfmKlm *key );
extern int gxse_secure_send_kn               ( GxTfmKlm *key, int final );
extern int gxse_secure_req_misc_key          ( void *key );
extern int gxse_secure_get_resp              ( GxTfmKlm *key );
extern int gxse_secure_create_K3_by_vendorid ( int vendorid );
extern int gxse_secure_set_pvrk              ( int key_id );
extern int gxse_secure_get_user_data         ( GxSecureUserData *data );
extern int gxse_secure_set_user_data         ( GxSecureUserData *data );
extern int gxse_secure_send_user_key         ( GxSecureUserKey *param );

#endif
