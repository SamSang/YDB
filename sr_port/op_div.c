/****************************************************************
 *								*
 * Copyright 2001, 2011 Fidelity Information Services, Inc	*
 *								*
 * Copyright (c) 2018-2020 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "arit.h"
#include "stringpool.h"
#include "eb_muldiv.h"
#include "promodemo.h"
#include "op.h"

LITREF int4	ten_pwr[];
LITREF mval	literal_zero;
LITREF mval	literal_sqlnull;

error_def(ERR_DIVZERO);
error_def(ERR_NUMOFLOW);

void	op_div (mval *u, mval *v, mval *q)
{
	boolean_t	promo;
	int4		c, exp;
	mval		w, z;
	int		u_mvtype, v_mvtype;

	/* If u or v is $ZYSQLNULL, the result is $ZYSQLNULL */
	if (MV_IS_SQLNULL(u) || MV_IS_SQLNULL(v))
	{
		MV_FORCE_DEFINED(u);
		MV_FORCE_DEFINED(v);
		*q = literal_sqlnull;
		return;
	}
	MV_FORCE_NUM(u);
	MV_FORCE_NUM(v);
	u_mvtype = u->mvtype;
	v_mvtype = v->mvtype;
	assert((v_mvtype & MV_INT) || (0 != v->m[0]) || (0 != v->m[1]));
	if ((v_mvtype & MV_INT) && (0 == v->m[1]))
		rts_error(VARLSTCNT(1) ERR_DIVZERO);
	if (u_mvtype & MV_INT & v_mvtype)
	{
		promo = eb_mvint_div(u->m[1], v->m[1], q->m);
		if (!promo)
		{
			q->mvtype = MV_NM | MV_INT;
			return;
		} else
		{
			w = *u;	     z = *v;
			promote(&w); promote(&z);
			u = &w;	     v = &z;
		}
	} else if (u_mvtype & MV_INT)
	{
		w = *u;
		promote(&w);
		u = &w;
	} else if (v_mvtype & MV_INT)
	{
		w = *v;
		promote(&w);
		v = &w;
	}
	c = eb_div(v->m, u->m, q->m);
	exp = u->e - v->e + c + MV_XBIAS;
	if (EXPHI <= exp)
		rts_error(VARLSTCNT(1) ERR_NUMOFLOW);
	else if (exp < EXPLO)
		*q = literal_zero;
	else if (exp < EXP_INT_OVERF  &&  exp > EXP_INT_UNDERF  &&  q->m[0] == 0  &&  (q->m[1]%ten_pwr[EXP_INT_OVERF-1-exp] == 0))
		demote(q, exp, u->sgn ^ v->sgn);
	else if ((0 == q->m[0]) && (0 == q->m[1]))
		*q = literal_zero;
	else
	{
		q->mvtype = MV_NM;
		q->sgn = u->sgn ^ v->sgn;
		q->e = exp;
	}
	assert(q->m[1] < MANT_HI);
}
