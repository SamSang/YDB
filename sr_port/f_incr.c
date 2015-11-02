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
#include "indir_enum.h"
#include "toktyp.h"
#include "mdq.h"
#include "advancewindow.h"
#include "fullbool.h"
#include "show_source_line.h"

GBLREF triple		*curtchain;
GBLREF char		window_token;

int f_incr(oprtype *a, opctype op)
{
	boolean_t	ok, save_shift;
	char		source_line_buff[MAX_SRCLINE + SIZEOF(ARROW)];
	oprtype		*increment;
	triple		incrchain, *oldchain, *r, *savptr, targchain, tmpexpr, *triptr;
	error_def(ERR_VAREXPECTED);
	DCL_THREADGBL_ACCESS;

	SETUP_THREADGBL_ACCESS;
	r = maketriple(op);
	/* may need to evaluate the increment (2nd arg) early and use result later: prepare to juggle triple chains */
	save_shift = TREF(shift_side_effects);
	if (GTM_BOOL != TREF(gtm_fullbool))
		TREF(shift_side_effects) = FALSE;	/* if no short circuit, only the outermost should juggle */
	dqinit(&targchain, exorder);	/* a place for the operation and the target */
	dqinit(&incrchain, exorder);	/* a place for the increment */
	dqinit(&tmpexpr, exorder);	/* a place to juggle the shifted chain in case it's active */
	triptr = TREF(expr_start);
	savptr = TREF(expr_start_orig);	/* but make sure expr_start_orig == expr_start since this is a new chain */
	TREF(expr_start_orig) = TREF(expr_start) = &tmpexpr;
	oldchain = setcurtchain(&targchain);	/* save the result of the first argument 'cause it evaluates 2nd */
	switch (window_token)
	{
	case TK_IDENT:
		/* $INCREMENT() performs an implicit $GET() on a first argument lvn so we use OC_PUTINDX because
		 * we know only at runtime whether to signal an UNDEF error (depending on whether we have
		 * VIEW "NOUNDEF" or "UNDEF" state; op_putindx creates the local variable unconditionally, even if
		 * we have "UNDEF" state, in which case any error in op_fnincr causes an op_kill of that local variable
		 */
		ok = (lvn(&(r->operand[0]), OC_PUTINDX, 0));
		break;
	case TK_CIRCUMFLEX:
		ok = gvn();
		r->opcode = OC_GVINCR;
		r->operand[0] = put_ilit(0);	/* dummy fill since emit_code does not like empty operand[0] */
		break;
	case TK_ATSIGN:
		ok = indirection(&r->operand[0]);
		r->opcode = OC_INDINCR;
		break;
	default:
		ok = FALSE;
		break;
	}
	if (!ok)
	{
		setcurtchain(oldchain);
		return FALSE;
	}
	assert(TREF(expr_start) == tmpexpr.exorder.bl);	/* maks sure nothing else did something fancy */
	TREF(expr_start) = triptr;				/* restore original shift chain */
	TREF(expr_start_orig) = savptr;
	increment = &r->operand[1];
	setcurtchain(&incrchain);	/* now to the increment expr, which must evaluate before the glvn in $INCR(glvn,expr) */
	if (window_token != TK_COMMA)
		*increment = put_ilit(1);	/* default optional increment to 1 */
	else
	{
		advancewindow();
		if (!strexpr(increment))
		{
			setcurtchain(oldchain);
			return FALSE;
		}
	}
	triptr = incrchain.exorder.bl;			/* prepare to park the target after the increment */
	dqadd(triptr, &targchain, exorder);		/* this is a violation of info hiding */
	coerce(increment, OCT_MVAL);
	ins_triple(r);
	if (&tmpexpr != tmpexpr.exorder.bl)
	{	/* one or more OC_GVNAME may have shifted so add to the end of the shift chain */
		assert(TREF(shift_side_effects));
		dqadd(TREF(expr_start), &tmpexpr, exorder);	/* this is a violation of info hiding */
		TREF(expr_start) = tmpexpr.exorder.bl;
		triptr = newtriple(OC_GVRECTARG);	/* restore the result of the last gvn to preserve $referece (the naked) */
		triptr->operand[0] = put_tref(TREF(expr_start));
	}
	TREF(shift_side_effects) = save_shift;
	if (!save_shift || ((GTM_BOOL == TREF(gtm_fullbool)) && (OC_INDINCR != r->opcode)))
	{	/* put it on the end of the main chain as there's no reason to play more with the ordering */
		setcurtchain(oldchain);
		triptr = curtchain->exorder.bl;
		dqadd(triptr, &incrchain, exorder);	/* this is a violation of info hiding */
	} else	/* need full side effects or indirect 1st argument so put everything on the shift chain */
	{	/* add the chain after "expr_start" which may be much before "curtchain" */
		newtriple(OC_GVSAVTARG);
		setcurtchain(oldchain);
		assert(NULL != TREF(expr_start));
		dqadd(TREF(expr_start), &incrchain, exorder);	/* this is a violation of info hiding */
		TREF(expr_start) = incrchain.exorder.bl;
		triptr = newtriple(OC_GVRECTARG);
		triptr->operand[0] = put_tref(TREF(expr_start));
	}
	*a = put_tref(r);
	return TRUE;
}
