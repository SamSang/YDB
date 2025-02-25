/****************************************************************
 *								*
 * Copyright (c) 2001-2018 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 * Copyright (c) 2017-2022 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#ifndef FGNCALSP_H_INCLUDED
#define FGNCALSP_H_INCLUDED

#include "hashtab.h"
#include "hashtab_str.h"

/* fgncalsp.h - UNIX foreign calls (d &package.label) */

#define MAX_NAME_LENGTH		255	/* Maximum length of file name */
#define MAX_ERRSTR_LEN		1024	/* Maximum length of the error string returned
					 * by dlerror(). Couldn't find any system
					 * defined length, 1024 is just arbitrary */
#define MAX_TABLINE_LEN		1024	/* Maximum length of a line estimated to be sufficient
					 * to specify MAX_ACTUALS parameters in the
					 * callin/xcall table */

#define	UNKNOWN_SYSERR 		"unknown system error"
#define COPY_DLLERR_MSG(err_ptr, err_buf)					\
{										\
	int len = 0;								\
										\
	if ((err_ptr = dlerror()) != NULL)					\
	{									\
		len = real_len(SIZEOF(err_buf) - 1, (uchar_ptr_t)err_ptr);	\
		strncpy(err_buf, err_ptr, len);					\
		err_buf[len] = '\0';						\
	} else									\
	{	/* Ensure we will not overrun err_buf limits */			\
		assert(ARRAYSIZE(UNKNOWN_SYSERR) < ARRAYSIZE(err_buf));		\
		STRCPY(err_buf, UNKNOWN_SYSERR);				\
	}									\
}

typedef int4	(*fgnfnc)();
typedef void	(*clnupfptr)();

/* A chain of packages, each package has a list of "entries", that is external routine entry points.  */

struct extcall_package_list
{
	struct extcall_package_list	*next_package;
	struct extcall_entry_list	*first_entry;
	void_ptr_t			package_handle;
	mstr				package_name;
	clnupfptr			package_clnup_rtn;
};

/* Any additions to the below enum would need additions to the "parm_space_needed[]" array in "sr_unix/exttab_parse.c" */
enum ydb_types
{
	ydb_notfound,
	ydb_void,
	ydb_status,
	ydb_int,
	ydb_uint,
	ydb_long,
	ydb_ulong,
#	ifdef GTM64
	ydb_int64,
	ydb_uint64,
#	endif
	ydb_float,
	ydb_double,
	ydb_int_star,
	ydb_uint_star,
	ydb_long_star,
	ydb_ulong_star,
#	ifdef GTM64
	ydb_int64_star,
	ydb_uint64_star,
#	endif
	ydb_string_star,
	ydb_float_star,
	ydb_char_star,
	ydb_char_starstar,
	ydb_double_star,
	ydb_pointertofunc,
	ydb_pointertofunc_star,
	ydb_jboolean,
	ydb_jint,
	ydb_jlong,
	ydb_jfloat,
	ydb_jdouble,
	ydb_jstring,
	ydb_jbyte_array,
	ydb_jbig_decimal,
	ydb_buffer_star,
};

typedef enum {SIGSAFE, XCBEHAVIORTAILENTRY} gtm_ext_call_behavior;

enum callintogtm_fncs
{
	gtmfunc_hiber_start,
	gtmfunc_hiber_start_any,
	gtmfunc_start_timer,
	gtmfunc_cancel_timer,
	gtmfunc_gtm_malloc,
	gtmfunc_gtm_free,
	gtmfunc_unknown_function
};

/* There is one of these for each external routine.  Each is owned by a package.  */
struct extcall_entry_list
{
	struct extcall_entry_list	*next_entry;
	enum ydb_types			return_type;	/* function return value */
	int				ret_pre_alloc_val; /* amount of space to be pre-allocated for the return type */
	uint4				input_mask;	/* is it an input parameter lsb = 1st parm */
	uint4				output_mask;	/* is it an output parameter lsb = 1st parm */
	int				parmblk_size;	/* size in bytes of parameter block to be allocated for call*/
	int				argcnt;		/* number of arguments */
	enum ydb_types			*parms;		/* pointer to parameter array */
	int				*param_pre_alloc_size; /* amount of space to be pre-allocated for the parameters */
	fgnfnc				fcn;		/* address of runtime routine */
	mstr				entry_name;	/* name of M entryref */
	mstr				call_name;	/* corresponding name of C function */
	boolean_t			ext_call_behaviors[XCBEHAVIORTAILENTRY]; /* Array of set behaviors*/
};

/* A list of entries in the call-in table each indicating the signature of
   a call-in routine */
typedef struct callin_entry_list
{
	mstr			label_ref;	/* labelref name of M routine */
	mstr			call_name;	/* corresponding name of C function */
	uint4			input_mask;	/* input parameter? LSB = 1st parm */
	uint4			output_mask;	/* output parameter? LSB = 1st parm */
	unsigned short		argcnt;		/* number of arguments */
	enum ydb_types		return_type;
	enum ydb_types		*parms;		/* parameter types */
	struct callin_entry_list	*next_entry;
} callin_entry_list;

/* parameter block that ci_restart uses to pass arguments to M routine */
typedef struct parmblk_struct
{
	void    *retaddr;
	int4    argcnt;
	int4    mask;
	lv_val	*args[MAX_ACTUALS];
} parmblk_struct;

typedef struct ci_tab_entry {
	callin_entry_list	*cname_list;
	hash_table_str		*hashtab;
	struct ci_tab_entry	*next;
	int			fname_len;	/* length of "fname" */
	char			fname[1];	/* null terminated file name that stored this call-in table (hung buffer) */
} ci_tab_entry_t;

#define	INTERNAL_USE_FALSE	FALSE
#define	INTERNAL_USE_TRUE	TRUE
#define FGN_OK_IF_NOT_FOUND	TRUE
#define FGN_ERROR_IF_NOT_FOUND	FALSE

/* function prototypes */
void_ptr_t	fgn_getpak(char *pak_name, int msgtype);
fgnfnc 		fgn_getrtn(void_ptr_t pak_handle, mstr *sym_name, int msgtype, boolean_t ok_not_found);
void		fgn_closepak(void_ptr_t pak_handle, int msgtype);
int 		fgncal_getint(mstr *inp);
int 		fgncal_read_args(mstr *inp);
void 		fgncal_getstr(mstr *inp, mstr *str);
void 		fgncal_lkbind(mstr *inp);
void 		fgn_glopref(mval *v);

struct extcall_package_list 	*exttab_parse (mval *package);
callin_entry_list		*citab_parse(boolean_t internal_use, const char *fname);
ci_tab_entry_t			*ci_tab_entry_open(boolean_t internal_use, const char *fname);
callin_entry_list		*ci_find_rtn_entry(ci_tab_entry_t *ci_tab, const char *call_name);
callin_entry_list		*ci_load_table_rtn_entry(const char *crtnname, ci_tab_entry_t **citab);

#endif
