/*
 * mutex.c
 *
 *  Created on: Jun 25, 2010
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#include "mutex.h"
#include "../util/log.h"
#include "../util/error.h"
#ifdef __XMK__
#include <sys/process.h>
#include "errno.h"
#endif

int mutex_Initialize(void* pHandle)
{
#ifdef __XMK__
	/* *pHandle = (void *) PTHREAD_MUTEX_INITIALIZER; */
	if (pthread_mutex_init((pthread_mutex_t*)(pHandle), NULL) != 0)
	{
		error_Set(ERR_CANNOT_CREATE_MUTEX);
		LOG_ERROR("Cannot create mutex");
		return FALSE; /* kernel error */
	}
	return TRUE;
#else
	error_Set(ERR_NOT_IMPLEMENTED);
	return FALSE;
#endif
}

int mutex_Destruct(void* pHandle)
{
#ifdef __XMK__

	if (pthread_mutex_destroy((pthread_mutex_t*)(pHandle)) != 0)
	{
		error_Set(ERR_CANNOT_DESTROY_MUTEX);
		LOG_ERROR("Cannot destroy mutex");
		return FALSE; /* kernel error */
	}
	return TRUE;
#else
	error_Set(ERR_NOT_IMPLEMENTED);
	LOG_ERROR("Not implemented");
	return FALSE;
#endif
}

int mutex_Lock(void* pHandle)
{
#ifdef __XMK__
	if (pthread_mutex_lock((pthread_mutex_t*)(pHandle)) != 0)
	{
		error_Set(ERR_CANNOT_LOCK_MUTEX);
		LOG_ERROR("Cannot lock mutex");
		return FALSE;
	}
	return TRUE;
#else
	error_Set(ERR_NOT_IMPLEMENTED);
	LOG_ERROR("Not implemented");
	return FALSE;
#endif
}

int mutex_Unlock(void* pHandle)
{
#ifdef __XMK__
	if (pthread_mutex_unlock((pthread_mutex_t*)(pHandle)) != 0)
	{
		error_Set(ERR_CANNOT_UNLOCK_MUTEX);
		LOG_ERROR("Cannot unlock mutex");
		return FALSE;
	}
	return TRUE;
#else
	error_Set(ERR_NOT_IMPLEMENTED);
	LOG_ERROR("Not implemented");
	return FALSE;
#endif
}
