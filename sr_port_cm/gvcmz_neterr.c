/****************************************************************
 *								*
 * Copyright (c) 2001-2017 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 * Copyright (c) 2017-2021 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "cmidef.h"
#include "hashtab_mname.h"	/* needed for cmmdef.h */
#include "cmmdef.h"
#include "locklits.h"
#include "iotimer.h"
#include "gtm_string.h"
#include "gvcmy_close.h"
#include "gvcmz.h"
#include "op.h"
#include "dpgbldir.h"
#include "lv_val.h"		/* needed for "callg.h" */
#include "callg.h"

GBLREF	struct NTD	*ntd_root;
GBLREF	bool		neterr_pending;

LITREF	mval		literal_notimeout;

error_def(ERR_LCKSCANCELLED);

void	gvcmz_neterr(gparam_list *err_plist)
{
	struct CLB	*p, *pn, *p1;
	unsigned char	*temp, buff[512];
	gd_addr		*gdptr;
	gd_region	*region, *r_top;
	uint4		count, lck_info;
	boolean_t	locks = FALSE;

	ASSERT_IS_LIBGNPCLIENT;
	neterr_pending = FALSE;
	assertpro(NULL != ntd_root);
	for (p = (struct CLB *)RELQUE2PTR(ntd_root->cqh.fl);  p != (struct CLB *)ntd_root;  p = pn)
	{
		/* Get the forward link, in case a close removes the current entry */
		pn = (struct CLB *)RELQUE2PTR(p->cqe.fl);
		if (0 != ((link_info *)p->usr)->neterr)
		{
			p->ast = NULL;
			if (locks)
				gvcmy_close(p);
			else
			{
				locks = ((link_info *)p->usr)->lck_info & REMOTE_CLR_MASK;
				gvcmy_close(p);
				if (locks)
				{
					buff[0] = CMMS_L_LKCANALL;
					for (p1 = (struct CLB *)RELQUE2PTR(ntd_root->cqh.fl);
					     p1 != (struct CLB *)ntd_root;
					     p1 = (struct CLB *)RELQUE2PTR(p1->cqe.fl))
					{
						p1->ast = NULL;
						/* The following line effectively clears REQUEST_PENDING */
						lck_info = ((link_info *)p1->usr)->lck_info &= REMOTE_CLR_MASK;
						if (lck_info)
						{
							temp = p1->mbf;
							p1->mbf = buff;
							p1->cbl = S_HDRSIZE + S_LAFLAGSIZE;
							if (lck_info & (REMOTE_LOCKS | LREQUEST_SENT))
							{
								buff[1] = CM_LOCKS;
								cmi_write(p1);
							}
							if (lck_info & (REMOTE_ZALLOCATES | ZAREQUEST_SENT))
							{
								buff[1] = CM_ZALLOCATES;
								cmi_write(p1);
							}
							p1->mbf = temp;
						}
					}
					op_unlock();
					op_zdeallocate((mval *)&literal_notimeout);
				}
			}
			/* Cycle through all active global directories */
			for (gdptr = get_next_gdr(NULL);  NULL != gdptr;  gdptr = get_next_gdr(gdptr))
				for (region = gdptr->regions, r_top = region + gdptr->n_regions;  region < r_top;  ++region)
					if ((dba_cm == region->dyn.addr->acc_meth) && (p == region->dyn.addr->cm_blk))
					{
						/* If it's a CM-accessed region via the current (error-generating) link: */
						region->open = FALSE;
						region->dyn.addr->acc_meth = dba_bg;
					}
		}
	}
	if (locks)
	{
		if (NULL != err_plist)
		{
			count = err_plist->n;
			err_plist->arg[count++] = (void *)0;
			err_plist->arg[count++] = (void *)ERR_LCKSCANCELLED;
			err_plist->arg[count++] = (void *)0;
			err_plist->n = count;
			callg_signal(err_plist);
		} else
			rts_error_csa(NULL, VARLSTCNT(1) ERR_LCKSCANCELLED);
	} else  if (NULL != err_plist)
		callg_signal(err_plist);

}
