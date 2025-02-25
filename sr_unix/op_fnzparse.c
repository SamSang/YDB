/****************************************************************
 *								*
 * Copyright (c) 2001-2018 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 * Copyright (c) 2020-2022 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"

#include "parse_file.h"
#include "gtm_caseconv.h"
#include "stringpool.h"
#include "op.h"
#include "min_max.h"

#define ZP_DEVICE	1
#define ZP_DIRECTORY	2
#define ZP_NAME		3
#define ZP_NODE		4
#define ZP_TYPE		5
#define ZP_VERSION	6
#define ZP_FULL		7

#define DEV_LEN		6
#define DIR_LEN		9
#define VER_LEN		7
#define ZP_LEN		4
#define NCON_LEN	10
#define SYN_LEN		11

error_def(ERR_ZPARSETYPE);
error_def(ERR_ZPARSFLDBAD);
error_def(ERR_INVSTRLEN);

void	op_fnzparse (mval *file, mval *field, mval *def1, mval *def2, mval *type, mval *ret)
{
	char 		field_type;
	int4 		status;
	char 		field_buf[DIR_LEN], type_buf[SYN_LEN], result[MAX_FN_LEN + 1];
	parse_blk	pblk;

	MV_FORCE_STR(field);
	MV_FORCE_STR(def1);
	MV_FORCE_STR(def2);
	MV_FORCE_STR(file);
	MV_FORCE_STR(type);

	if (def1->str.len > MAX_FN_LEN)
		rts_error_csa(CSA_ARG(NULL) VARLSTCNT(4) ERR_INVSTRLEN, 2, def1->str.len, MAX_FN_LEN);
	if (def2->str.len > MAX_FN_LEN)
		rts_error_csa(CSA_ARG(NULL) VARLSTCNT(4) ERR_INVSTRLEN, 2, def2->str.len, MAX_FN_LEN);

	if (field->str.len == 0)
	{
		field_type = ZP_FULL;
	}
	else
	{
		field_type = 0;
		if (field->str.len <= DIR_LEN)
		{
			lower_to_upper((uchar_ptr_t)&field_buf[0], (uchar_ptr_t)field->str.addr, field->str.len);
			switch( field_buf[0] )
			{
			case 'D':
				if (field->str.len <= DEV_LEN  &&  memcmp(&field_buf[0], "DEVICE", field->str.len) == 0)
					field_type = ZP_DEVICE;
				else if (field->str.len <= DIR_LEN  &&  memcmp(&field_buf[0], "DIRECTORY", field->str.len) == 0)
					field_type = ZP_DIRECTORY;
				break;

			case 'N':
				if (field->str.len <= ZP_LEN)
				{
					if (memcmp(&field_buf[0], "NAME", field->str.len) == 0)
						field_type = ZP_NAME;
					else if (memcmp(&field_buf[0], "NODE", field->str.len) == 0)
						field_type = ZP_NODE;
				}
				break;

			case 'T':
				if (field->str.len <= ZP_LEN  &&  memcmp(&field_buf[0], "TYPE", field->str.len) == 0)
					field_type = ZP_TYPE;
				break;

			case 'V':
				if (field->str.len <= VER_LEN  &&  memcmp(&field_buf[0], "VERSION", field->str.len) == 0)
					field_type = ZP_VERSION;
				break;

			default:
				break;

			}
		}
		if (field_type == 0)
			rts_error_csa(CSA_ARG(NULL) VARLSTCNT(4) ERR_ZPARSFLDBAD, 2, field->str.len, field->str.addr);
	}

	memset(&pblk, 0, SIZEOF(pblk));

	if (type->str.len != 0)
	{
		lower_to_upper((uchar_ptr_t)&type_buf[0], (uchar_ptr_t)type->str.addr, MIN(SIZEOF(type_buf), type->str.len));
		if (type->str.len <= SYN_LEN  &&  memcmp(&type_buf[0], "SYNTAX_ONLY", type->str.len) == 0)
			pblk.fop |= F_SYNTAXO;
		else if (type->str.len <= NCON_LEN  &&  memcmp(&type_buf[0], "NO_CONCEAL", type->str.len) == 0)
		{
			/* no meaning on unix */
		} else if (type->str.len <= STR_LIT_LEN("SYMLINK") && !memcmp(&type_buf[0], "SYMLINK" , type->str.len))
			pblk.symlink = TRUE;
		else
			rts_error_csa(CSA_ARG(NULL) VARLSTCNT(4) ERR_ZPARSETYPE, 2, type->str.len, type->str.addr);
	}

	pblk.buffer = result;
	pblk.buff_size = MAX_FN_LEN;
	pblk.def1_size = def1->str.len;
	pblk.def1_buf = def1->str.addr;
	pblk.def2_size = def2->str.len;
	pblk.def2_buf = def2->str.addr;

	status = parse_file(&file->str, &pblk);
	if (ERR_FILEPATHTOOLONG == status)
		rts_error_csa(CSA_ARG(NULL) VARLSTCNT(5) ERR_FILEPARSE, 2, file->str.len, file->str.addr, status);
	if (0 == (status & 1))
	{
		assert(ERR_FILENOTFND == status);
		ret->mvtype = MV_STR;
		ret->str.len = 0;
		return;
	}
	assert(ERR_PARNORMAL == status);
	ret->mvtype = MV_STR;
	switch( field_type )
	{
	case ZP_DIRECTORY:
		ret->str.addr = pblk.l_dir;
		ret->str.len = pblk.b_dir;
		break;

	case ZP_NAME:
		ret->str.addr = pblk.l_name;
		ret->str.len = pblk.b_name;
		break;

	case ZP_TYPE:
		ret->str.addr = pblk.l_ext;
		ret->str.len = pblk.b_ext;
		break;

	default:
		ret->str.addr = pblk.buffer;
		ret->str.len = pblk.b_esl;
		break;

	}
	s2pool(&ret->str);
	return;
}
