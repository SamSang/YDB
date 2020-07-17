/****************************************************************
 *								*
 * Copyright (c) 2011-2016 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 * Copyright (c) 2020 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/
#ifndef EINTR_WRP_SEMOP_INCLUDED
#define EINTR_WRP_SEMOP_INCLUDED
#define NO_WAIT 0
#define FORCED_WAIT 1

#include <sys/types.h>
#include <errno.h>
#include "gtm_c_stack_trace_semop.h"
#include "eintr_wrappers.h"		/* for EINTR_HANLDING_CHECK macro */
#ifdef DEBUG
#ifdef SUNOS
#define SEMVALMAX 65535
#else
#define SEMVALMAX 32767
#endif
#endif

#define SEMOP(SEMID, SOPS, NSOPS, RC, TO_WAIT)								\
{													\
	int numsems; 											\
													\
	for (numsems = NSOPS - 1; numsems >= 0; --numsems)						\
	{												\
		CHECK_SEMVAL_GRT_SEMOP(SEMID, SOPS[numsems].sem_num, SOPS[numsems].sem_op);		\
	}												\
	if (FORCED_WAIT == TO_WAIT)									\
	{												\
		RC = try_semop_get_c_stack(SEMID, SOPS, NSOPS);						\
		if (0 != RC)										\
		{											\
			errno = RC;									\
			RC = -1;									\
		}											\
	} else												\
	{												\
		assert(NO_WAIT == TO_WAIT);								\
		do											\
		{											\
			RC = semop(SEMID, SOPS, NSOPS);							\
			if ((-1 != RC) || (EINTR != errno))						\
				break;									\
			EINTR_HANDLING_CHECK;								\
		} while (TRUE);										\
	}												\
}
#endif
