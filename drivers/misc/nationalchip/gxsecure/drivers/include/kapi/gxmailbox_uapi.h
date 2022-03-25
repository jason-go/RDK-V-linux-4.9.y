#ifndef __GX_MBOX_UAPI_H__
#define __GX_MBOX_UAPI_H__

#include "gxsecure.h"
#include "gxtfm.h"

extern int gx_mbox_load_firmware         ( GxSecureLoader *firm, int mbox_is_tee );
extern int gx_mbox_reset_firmware        ( int mbox_is_tee );
extern int gx_mbox_get_status            ( int mbox_is_tee );
extern int gx_mbox_set_BIV               ( GxSecureImageVersion *param, int mbox_is_tee );
extern int gx_mbox_get_BIV               ( GxSecureImageVersion *param, int mbox_is_tee );
extern int gx_mbox_autotest_send         ( void *cmd, int mbox_is_tee );
extern int gx_mbox_autotest_recv         ( void *rst, int mbox_is_tee );
extern int gx_mbox_send_irdeto_cmd       ( GxTfmKlm *key, int mbox_is_tee );
extern int gx_mbox_send_root_key         ( GxTfmKlm *key, int mbox_is_tee );
extern int gx_mbox_send_kn               ( GxTfmKlm *key, int final, int mbox_is_tee );
extern int gx_mbox_req_misc_key          ( void *key, int mbox_is_tee );
extern int gx_mbox_get_resp              ( GxTfmKlm *key, int mbox_is_tee );
extern int gx_mbox_create_K3_by_vendorid ( int vendorid, int mbox_is_tee );
extern int gx_mbox_set_pvrk              ( int key_id, int mbox_is_tee );
extern int gx_mbox_get_user_data         ( GxSecureUserData *data, int mbox_is_tee );
extern int gx_mbox_set_user_data         ( GxSecureUserData *data, int mbox_is_tee );
extern int gx_mbox_send_user_key         ( GxSecureUserKey *param, int mbox_is_tee );

#endif
