/****************************************************************
 *								*
 * Copyright (c) 2018-2022 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include <stdarg.h>
#include "gtm_string.h"

#include "libyottadb_int.h"
#include "error.h"
#include "op.h"
#include "stringpool.h"
#include "callg.h"
#include "mvalconv.h"
#include "namelook.h"
#include "outofband.h"

GBLREF	volatile int4	outofband;

/* Routine to incrementally release a lock (not unlocking everything first).
 *
 * Parameters:
 *   - varname   - Contains the basevar name (local or global). Must be a valid variable name.
 *   - subs_used - The number of subscripts specified in the subsarray parm
 *   - subsarray - An array of 'subs_used' ydb_buffer_t structures containing the definitions of the subscripts.
 */
int ydb_lock_decr_s(const ydb_buffer_t *varname, int subs_used, const ydb_buffer_t *subsarray)
{
	va_list		var;
	int		parmidx, lock_rc;
	gparam_list	plist;
	boolean_t	error_encountered;
	mval		varname_mval;
	mval		plist_mvals[YDB_MAX_SUBS + 1];
	ydb_var_types	var_type;
	int		var_svn_index;
	DCL_THREADGBL_ACCESS;

	SETUP_THREADGBL_ACCESS;
	VERIFY_NON_THREADED_API;	/* clears a global variable "caller_func_is_stapi" set by SimpleThreadAPI caller
					 * so needs to be first invocation after SETUP_THREADGBL_ACCESS to avoid any error
					 * scenarios from not resetting this global variable even though this function returns.
					 */
	/* Verify entry conditions, make sure YDB CI environment is up etc. */
	LIBYOTTADB_INIT(LYDB_RTN_LOCK_DECR, (int));	/* Note: macro could return from this function in case of errors */
	assert(0 == TREF(sapi_mstrs_for_gc_indx));	/* previously unused entries should have been cleared by that
							 * corresponding ydb_*_s() call.
							 */
	ESTABLISH_NORET(ydb_simpleapi_ch, error_encountered);
	if (error_encountered)
	{
		assert(0 == TREF(sapi_mstrs_for_gc_indx));	/* Should have been cleared by "ydb_simpleapi_ch" */
		REVERT;
		return ((ERR_TPRETRY == SIGNAL) ? YDB_TP_RESTART : -(TREF(ydb_error_code)));
	}
	/* Check if an outofband action that might care about has popped up */
	if (outofband)
		outofband_action(FALSE);
	/* First step, initialize the private lock list */
	op_lkinit();
	/* Setup and validate the varname */
	VALIDATE_VARNAME(varname, subs_used, FALSE, LYDB_RTN_LOCK_DECR, -1, var_type, var_svn_index);
	/* Setup parameter list for callg() invocation of op_lkname() */
	plist.arg[0] = NULL;				/* First arg is extended reference that simpleAPI doesn't support */
	varname_mval.mvtype = MV_STR;
	varname_mval.str.addr = varname->buf_addr;	/* Second arg is varname */
	varname_mval.str.len = varname->len_used;
	plist.arg[1] = &varname_mval;
	/* Note, no rebuffering is needed here as the lock name and subscripts are all copied into the private
	 * lock block space by routines called by op_lkname so are effectively already rebuffered. No need for
	 * us to do it again.
	 */
	COPY_PARMS_TO_CALLG_BUFFER(subs_used, subsarray, plist, plist_mvals, FALSE, 2, LYDBRTNNAME(LYDB_RTN_LOCK_DECR));
	callg((callgfnptr)op_lkname, &plist);
	lock_rc = op_decrlock(NULL);
	assert(0 == TREF(sapi_mstrs_for_gc_indx));	/* the counter should have never become non-zero in this function */
	LIBYOTTADB_DONE;
	REVERT;
	return lock_rc ? YDB_OK : -1;			/* op_decrlock() only ever returns TRUE */
}
