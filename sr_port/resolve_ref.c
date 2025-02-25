/****************************************************************
 *								*
 * Copyright (c) 2001-2020 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
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

#include "gtm_stdio.h"
#include "gtm_string.h"

#include "compiler.h"
#include "mdq.h"
#include "opcode.h"
#include "cmd_qlf.h"
#include "mmemory.h"
#include "resolve_lab.h"
#include "cdbg_dump.h"
#include "gtmdbglvl.h"

GBLREF boolean_t		run_time;
GBLREF command_qualifier	cmd_qlf;
GBLREF mlabel			*mlabtab;
GBLREF triple			t_orig;
GBLREF uint4			ydbDebugLevel;

error_def(ERR_ACTLSTTOOLONG);
error_def(ERR_FMLLSTMISSING);
error_def(ERR_LABELMISSING);
error_def(ERR_LABELNOTFND);
error_def(ERR_LABELUNKNOWN);

int resolve_ref(int errknt)
{	/* simplify operand references where possible and make literals dynamic when in that mode */
	int		actcnt;
	mlabel		*mlbx;
	mline		*mxl;
	oprtype		*opnd;
	tbp		*tripbp;
	triple		*chktrip, *curtrip, *tripref, *looptrip;
#	ifndef i386
	oprtype		*j, *k;
	triple		*ref, *ref1;
#	endif
	DCL_THREADGBL_ACCESS;

	SETUP_THREADGBL_ACCESS;
#	ifdef DEBUG
	if (!run_time && (ydbDebugLevel & GDL_DebugCompiler))
	{	/* ensure that "borrowing" of backpointers has not left any trace */
		dqloop(&t_orig, exorder, curtrip)
		{
			tripbp = &curtrip->backptr;
			assert((tripbp == tripbp->que.fl) && (tripbp == tripbp->que.bl) && (NULL == tripbp->bkptr));
		}
	}
#	endif
	if (errknt && !(cmd_qlf.qlf & CQ_IGNORE))
	{
		assert(!run_time);
		walktree((mvar *)mlabtab, resolve_lab, (char *)&errknt);
	} else
	{
#		ifndef i386
		if (!run_time && (cmd_qlf.qlf & CQ_DYNAMIC_LITERALS))
		{	/* OC_LIT --> OC_LITC wherever OC_LIT is actually used, i.e. not a dead end */
			dqloop(&t_orig, exorder, curtrip)
			{
				switch (curtrip->opcode)
				{	/* Do a few literal optimizations typically done later in alloc_reg. It's convenient to
					 * check for OC_LIT parameters here, before we start sliding OC_LITC opcodes in the way.
					 */
					case OC_NOOP:
					case OC_PARAMETER:
					case OC_LITC:	/* possibly already inserted in bx_boolop */
						continue;
					case OC_STO:	/* see counterpart in alloc_reg.c */
						if ((cmd_qlf.qlf & CQ_INLINE_LITERALS)
						    && (TRIP_REF == curtrip->operand[1].oprclass)
						    && (OC_LIT == curtrip->operand[1].oprval.tref->opcode))
						{
							curtrip->opcode = OC_STOLITC;
							continue;
						}
						break;
					case OC_FNTEXT:
						if (resolve_optimize(curtrip))
						{	/* no need to backup and rescan in this case */
							assert(OC_LITC == curtrip->opcode);
							continue;
						}	/* WARNING fallthrough*/
					default:
						break;
				}
				for (j = curtrip->operand, ref = curtrip; j < ARRAYTOP(ref->operand); )
				{	/* Iterate over all parameters of the current triple */
					k = j;
					while (INDR_REF == k->oprclass)
						k = k->oprval.indr;
					if (TRIP_REF == k->oprclass)
					{
						tripref = k->oprval.tref;
						if (OC_PARAMETER == tripref->opcode)
						{
							ref = tripref;
							j = ref->operand;
							continue;
						}
						if (OC_LIT == tripref->opcode)
						{	/* Insert an OC_LITC to relay the OC_LIT result to curtrip */
							ref1 = maketriple(OC_LITC);
							ref1->src = tripref->src;
							ref1->operand[0] = put_tref(tripref);
							dqrins(curtrip, exorder, ref1);
							*k = put_tref(ref1);
						}
					}
					j++;
				}
			}
		}
#		endif
		COMPDBG(PRINTF("\n\n\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> New Compilation <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");)
		COMPDBG(PRINTF("\n\n\n**************************** Begin resolve_ref scan ****************************\n"););
		dqloop(&t_orig, exorder, curtrip)
		{	/* If the optimization was not executed earlier */
			COMPDBG(PRINTF(" ************************ Triple Start **********************\n"););
			COMPDBG(cdbg_dump_triple(curtrip, 0););
#			ifndef i386
			if (!run_time && !(cmd_qlf.qlf & CQ_DYNAMIC_LITERALS))
			{
#			endif
				if ((OC_FNTEXT == curtrip->opcode) && (resolve_optimize(curtrip)))
				{	/* no need to backup and rescan in this case */
					assert(OC_LIT == curtrip->opcode);
					continue;
				}
#			ifndef i386
			}
#			endif
			for (looptrip = curtrip, opnd = looptrip->operand; opnd < ARRAYTOP(looptrip->operand); )
			{
				switch(looptrip->opcode)
				{
				case OC_BXRELOP:
					if (TRIP_REF == opnd->oprclass)
					{	/* Iterate over all parameters of the current triple */
						tripref = opnd->oprval.tref;
						if (OC_PARAMETER == tripref->opcode)
						{
							looptrip = tripref;
							opnd = looptrip ->operand;
							continue;
						}
					}
				default:
					break;
				}
				if (INDR_REF == opnd->oprclass)
					*opnd = *(opnd->oprval.indr);
				switch (opnd->oprclass)
				{
				case TNXT_REF:
					opnd->oprclass = TJMP_REF;
					opnd->oprval.tref = opnd->oprval.tref->exorder.fl;
					/* caution:  fall through */
				case TJMP_REF:
					tripbp = (tbp *)mcalloc(SIZEOF(tbp));
					tripbp->bkptr = looptrip;
					dqins(&opnd->oprval.tref->jmplist, que, tripbp);
					break;
				case MNXL_REF:	/* external reference to the routine (not within the routine) */
					mxl = opnd->oprval.mlin->child;
					tripref = mxl ? mxl->externalentry : NULL;
					if (!tripref)
					{	/* ignore vacuous DO sp sp */
						(void)resolve_optimize(looptrip);	/* let that do the optimization */
						break;
					}
					opnd->oprclass = TJMP_REF;
					opnd->oprval.tref = tripref;
					break;
				case MLAB_REF:	/* target label should have no parms; this is DO without parens or args */
					assert(!run_time);
					mlbx = opnd->oprval.lab;
					tripref = mlbx->ml ? mlbx->ml->externalentry : NULL;
					if (tripref)
					{
						opnd->oprclass = TJMP_REF;
						opnd->oprval.tref = tripref;
					} else
					{
						errknt++;
						stx_error(ERR_LABELMISSING, 2, mlbx->mvname.len, mlbx->mvname.addr);
						TREF(source_error_found) = 0;
						tripref = newtriple(OC_RTERROR);
						tripref->operand[0]
							= put_ilit((OC_JMP == looptrip->opcode)
								? ERR_LABELNOTFND
								: ERR_LABELUNKNOWN); /* special error for GOTO jmp */
						tripref->operand[1] = put_ilit(TRUE);	/* This is a subroutine/func reference */
						opnd->oprval.tref = tripref;
						opnd->oprclass = TJMP_REF;
					}
					break;
				case MFUN_REF:
					assert(!run_time);
					assert(OC_JMP == looptrip->opcode);
					chktrip = looptrip->exorder.bl;
					assert((OC_EXCAL == chktrip->opcode) || (OC_EXFUN == chktrip->opcode));
					assert(TRIP_REF == chktrip->operand[1].oprclass);
					chktrip = chktrip->operand[1].oprval.tref;
					assert(OC_PARAMETER == chktrip->opcode);
					assert(TRIP_REF == chktrip->operand[0].oprclass);
#					ifdef __i386
					assert(OC_ILIT == chktrip->operand[0].oprval.tref->opcode);
					assert(ILIT_REF == chktrip->operand[0].oprval.tref->operand[0].oprclass);
#					else
					assert(OC_TRIPSIZE == chktrip->operand[0].oprval.tref->opcode);
					assert(TSIZ_REF == chktrip->operand[0].oprval.tref->operand[0].oprclass);
#					endif
					chktrip = chktrip->operand[1].oprval.tref;
					assert(OC_PARAMETER == chktrip->opcode);
					assert(TRIP_REF == chktrip->operand[0].oprclass);
					assert(OC_ILIT == chktrip->operand[0].oprval.tref->opcode);
					assert(ILIT_REF == chktrip->operand[0].oprval.tref->operand[0].oprclass);
#					ifndef __i386
					chktrip = chktrip->operand[1].oprval.tref;
					assert(OC_PARAMETER == chktrip->opcode);
					assert(TRIP_REF == chktrip->operand[0].oprclass);
					assert(OC_ILIT == chktrip->operand[0].oprval.tref->opcode);
					assert(ILIT_REF == chktrip->operand[0].oprval.tref->operand[0].oprclass);
#					endif
					actcnt = chktrip->operand[0].oprval.tref->operand[0].oprval.ilit;
					assert(0 <= actcnt);
					mlbx = opnd->oprval.lab;
					tripref = mlbx->ml ? mlbx->ml->externalentry : 0;
					if (tripref)
					{
						if (NO_FORMALLIST == mlbx->formalcnt)
						{
							errknt++;
							stx_error(ERR_FMLLSTMISSING, 2, mlbx->mvname.len, mlbx->mvname.addr);
							TREF(source_error_found) = 0;
							tripref = newtriple(OC_RTERROR);
							tripref->operand[0] = put_ilit(ERR_FMLLSTMISSING);
							tripref->operand[1] = put_ilit(TRUE);	/* subroutine/func reference */
							opnd->oprval.tref = tripref;
							opnd->oprclass = TJMP_REF;
						} else if (mlbx->formalcnt < actcnt)
						{
							errknt++;
							stx_error(ERR_ACTLSTTOOLONG, 2, mlbx->mvname.len, mlbx->mvname.addr);
							TREF(source_error_found) = 0;
							tripref = newtriple(OC_RTERROR);
							tripref->operand[0] = put_ilit(ERR_ACTLSTTOOLONG);
							tripref->operand[1] = put_ilit(TRUE);	/* subroutine/func reference */
							opnd->oprval.tref = tripref;
							opnd->oprclass = TJMP_REF;
						} else
						{
							opnd->oprclass = TJMP_REF;
							opnd->oprval.tref = tripref;
						}
					} else
					{
						errknt++;
						stx_error(ERR_LABELMISSING, 2, mlbx->mvname.len, mlbx->mvname.addr);
						TREF(source_error_found) = 0;
						tripref = newtriple(OC_RTERROR);
						tripref->operand[0] = put_ilit(ERR_LABELUNKNOWN);
						tripref->operand[1] = put_ilit(TRUE);	/* subroutine/func reference */
						opnd->oprval.tref = tripref;
						opnd->oprclass = TJMP_REF;
					}
					break;
				case TRIP_REF:
					resolve_tref(looptrip, opnd);
					break;
				default:
					break;
				}
				opnd++;
			}
			opnd = &curtrip->destination;
			if (opnd->oprclass == TRIP_REF)
				resolve_tref(curtrip, opnd);
		}
	}
	return errknt;
}

/* insert backpointers from operands to the triples that use them */
void resolve_tref(triple *curtrip, oprtype *opnd)
{
	triple	*tripref;
	tbp	*tripbp;

	while (OC_PASSTHRU == (tripref = opnd->oprval.tref)->opcode)		/* note the assignment */
	{	/* If, for example, there are nested $SELECT() functions feeding a value to a SET $PIECE/$EXTRACT, we need to ensure
		 * no OC_PASSTHRUs remain in the parameter chain as alloc_reg would turn them into OC_NOOPs, which would cause
		 * an assertpro in emit_code; as many OC_PASSTHRUs as are stacked, we devour.
		 */
		COMPDBG(PRINTF(" ** Passthru replacement: Operand at 0x%08lx replaced by operand at 0x%08lx\n",
			       (unsigned long)opnd, (unsigned long)&tripref->operand[0]););
		assert(TRIP_REF == tripref->operand[0].oprclass);
		/* The TRIP_REF check below is needed due to $ZYSQLNULL */
		assert(((NO_REF == tripref->operand[1].oprclass) || (TRIP_REF == tripref->operand[1].oprclass)) &&
			(NO_REF == tripref->destination.oprclass));
		*opnd = tripref->operand[0];
	}
	tripbp = (tbp *)mcalloc(SIZEOF(tbp));
	tripbp->bkptr = curtrip;
	dqins(&opnd->oprval.tref->backptr, que, tripbp);
}
