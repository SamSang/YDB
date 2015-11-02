/****************************************************************
 *								*
 *	Copyright 2001, 2010 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "compiler.h"
#include "opcode.h"
#include "toktyp.h"
#include "mdq.h"

GBLREF char window_token;

int glvn(oprtype *a)
{

	triple *ref, *oldchain, tmpchain, *triptr;
	oprtype x1;
	error_def(ERR_VAREXPECTED);
	DCL_THREADGBL_ACCESS;

	SETUP_THREADGBL_ACCESS;
	switch(window_token)
	{
	case TK_IDENT:
		if (!lvn(a,OC_GETINDX,0))
			return FALSE;
		return TRUE;
	case TK_CIRCUMFLEX:
		if (!gvn())
			return FALSE;
		*a = put_tref(newtriple(OC_GVGET));
		return TRUE;
	case TK_ATSIGN:
		if (TREF(shift_side_effects))
		{
			dqinit(&tmpchain, exorder);
			oldchain = setcurtchain(&tmpchain);
			if (!indirection(&x1))
			{
				setcurtchain(oldchain);
				return FALSE;
			}
			ref = newtriple(OC_INDGLVN);
			newtriple(OC_GVSAVTARG);
			setcurtchain(oldchain);
			dqadd(TREF(expr_start), &tmpchain, exorder);
			TREF(expr_start) = tmpchain.exorder.bl;
			triptr = newtriple(OC_GVRECTARG);
			triptr->operand[0] = put_tref(TREF(expr_start));
		} else
		{
			if (!indirection(&x1))
				return FALSE;
			ref = newtriple(OC_INDGLVN);
		}
		ref->operand[0] = x1;
		*a = put_tref(ref);
		return TRUE;
	default:
		stx_error(ERR_VAREXPECTED);
		return FALSE;
	}
}
