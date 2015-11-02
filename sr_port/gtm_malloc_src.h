/****************************************************************
 *								*
 *	Copyright 2001, 2011 Fidelity Information Services, Inc	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* Storage manager for "smaller" pieces of storage. Uses power-of-two
   "buddy" system as described by Knuth. Currently manages pieces of
   size 2K - SIZEOF(header).

   This include file is included in both gtm_malloc.c and gtm_malloc_dbg.c.
   See the headers of those modules for explanations of how the storage
   manager build is actually accomplished.

   Debugging is controlled via the "gtmdbglvl" environment variable in
   the Unix environment and the GTM$DBGLVL logical in the VMS environment.
   If this variable is set to a non-zero value, the debugging environment
   is enabled. The debugging features turned on will correspond to the bit
   values defined gtmdbglvl.h. Note that this mechanism is versatile enough
   that non-storage-managment debugging is also hooked in here. The
   debugging desired is a mask for the features desired. For example, if the
   value 4 is set, then tracing is enabled. If the value is set to 6, then
   both tracing and statistics are enabled. Because the code is expanded
   twice in a "Pro" build, these debugging features are available even
   in a pro build and can thus be enabled in the field without the need for
   a "debug" version to be installed in order to chase a corruption or other
   problem.
*/

#include "mdef.h"

/* We are the redefined versions so use real versions in this module */
#undef malloc
#undef free

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stddef.h>
#include <errno.h>
#if !defined(VMS) && !defined(__MVS__)
#include <malloc.h>
#endif
#include "gtm_stdio.h"
#include "gtm_string.h"
#include "gtm_stdlib.h"

#include "eintr_wrappers.h"
#include "gtmdbglvl.h"
#include "io.h"
#include "iosp.h"
#include "min_max.h"
#include "mdq.h"
#include "error.h"
#include "trans_log_name.h"
#include "gtmmsg.h"
#include "print_exit_stats.h"
#include "mmemory.h"
#include "gtm_logicals.h"
#include "cache.h"
#include "gtm_malloc.h"
#include "have_crit.h"
#ifdef UNIX
#include "deferred_signal_handler.h"
#endif

/* This routine is compiled twice, once as debug and once as pro and put into the same pro build. The alternative
 * memory manager is selected with the debug flags (any non-zero gtmdbglvl setting invokes debug memory manager in
 * a pro build). So the global variables (defined using the STATICD macro) have to be two different fields.
 * One for pro, one for dbg. The fields have different values and different sizes between the two compiles but
 * exist in the same build. They cannot coexist. That is why STATICD is defined to be static for PRO and GBLDEF for DBG.
 * This is the reason why we cannot use the STATICDEF macro here because that is defined to be a GBLDEF for PRO and DBG.
 *
 * To debug this routine effectively, normally static routines are turned into GBLDEFs. Also, for vars that
 * need one copy, define GBLRDEF to GBLDEF for debug and GBLREF for pro. This is because the pro builds always
 * have a debug version in them satisfiying the GBLREF but the debug builds won't have any pro code in them so
 * the define must be in the debug version. Also note that we cannot use the STATICDEF macro (instead of the
 * STATICD below) since that evaluates to a GBLDEF in both PRO and DBG which
 */
#ifdef DEBUG
#  define STATICD GBLDEF
#  define STATICR extern
#  define GBLRDEF GBLDEF
#else
#  define STATICD static
#  define STATICR static
#  define GBLRDEF GBLREF
#endif

#ifdef GTM64
#  define gmaAdr "%016lx"
#  define gmaFill "         "
#  define gmaLine "--------"
#else
#  define gmaAdr "%08lx"
#  define gmaFill " "
#  define gmaLine " "
#endif

#ifdef VMS
/* These routines for VMS are AST-safe */
#  define MALLOC(size, addr)							\
{										\
        int	msize, errnum;							\
	void	*maddr;								\
	msize = size;								\
        errnum = lib$get_vm(&msize, &maddr);					\
	if (SS$_NORMAL != errnum)						\
	{									\
		gtmMallocErrorSize = size;					\
		gtmMallocErrorCallerid = CALLERID;				\
		gtmMallocErrorErrno = errnum;					\
		raise_gtmmemory_error();					\
	}									\
	addr = (void *)maddr;							\
}
#  define FREE(size, addr)							\
{										\
        int	msize, errnum;							\
	void	*maddr;								\
	msize = size;								\
        maddr = addr;								\
        errnum = lib$free_vm(&msize, &maddr);					\
	if (SS$_NORMAL != errnum)						\
	{									\
		--gtmMallocDepth;						\
		assert(FALSE);							\
		rts_error(VARLSTCNT(4) ERR_FREEMEMORY, 1, CALLERID, errnum);	\
	}									\
}
#  define GTM_MALLOC_REENT
#else
/* These routines for Unix are NOT thread-safe */
#  define MALLOC(size, addr) 							\
{										\
	addr = (void *)malloc(size);						\
	if (NULL == (void *)addr)						\
	{									\
		gtmMallocErrorSize = size;					\
		gtmMallocErrorCallerid = CALLERID;				\
		gtmMallocErrorErrno = errno;					\
		raise_gtmmemory_error();					\
	}									\
}
#  define FREE(size, addr) free(addr);
#endif
#ifdef GTM_MALLOC_REENT
#  define GMR_ONLY(statement) statement
#  define NON_GMR_ONLY(statement)
#else
#  define GMR_ONLY(statement)
#  define NON_GMR_ONLY(statement) statement
#endif

#ifdef DEBUG
/* States a storage element may be in. Debug version has values less likely to occur naturally
   although the possibilities are limited with only one byte of information. */
enum ElemState {Allocated = 0x42, Free = 0x24};
#else
enum ElemState {Allocated = 1, Free};	/* 0 is just "too commonly occurring" */
#endif

/* At the end of each block is this header which is used to track when all of the elements that
   a block of real allocated storage was broken into have become free. At that point, we can return
   the chunk to the OS.
*/
typedef struct storExtHdrStruct
{
	struct
	{
		struct storExtHdrStruct	*fl, *bl;	/* In case we need to visit the entire list */
	} links;
	unsigned char	*extentStart;			/* First byte of real extent (not aligned) */
	storElem	*elemStart;			/* Start of array of MAXTWO elements */
	int		elemsAllocd;			/* MAXTWO sized element count. When 0 this block is free */
} storExtHdr;

#define MAXTWO 2048
/* How many "MAXTWO" elements to allocate at one time. This minimizes the waste since our subblocks must
   be aligned on a suitable power of two boundary for the buddy-system to work properly
*/
#define ELEMS_PER_EXTENT 16

#define MAXDEFERQUEUES 10
#ifdef DEBUG
#  define STOR_EXTENTS_KEEP 1 /* Keep only one extent in debug for maximum testing */
#  define MINTWO NON_GTM64_ONLY(64) GTM64_ONLY(128)
#  define MAXINDEX NON_GTM64_ONLY(5) GTM64_ONLY(4)
#  define STE_FP(p) p->fPtr
#  define STE_BP(p) p->bPtr
#else
#  define STOR_EXTENTS_KEEP 5
#  define MINTWO NON_GTM64_ONLY(16) GTM64_ONLY(32)
#  define MAXINDEX NON_GTM64_ONLY(7) GTM64_ONLY(6)
#  define STE_FP(p) p->userStorage.links.fPtr
#  define STE_BP(p) p->userStorage.links.bPtr
#endif
/* Our extent must be aligned on a MAXTWO byte boundary hence we allocate one more extent than
   we actually want so we can be guarranteed usable storage. However if that allocation actually
   starts on a MAXTWO boundary (on guarranteed 8 byte boundary), then we get an extra element.
   Here we define our extent size and provide an initial sanity value for "extent_used". If the
   allocator ever gets this extra block, this field will be increased by the size of one element
   to compensate.
*/
#define EXTENT_SIZE ((MAXTWO * (ELEMS_PER_EXTENT + 1)) + SIZEOF(storExtHdr))
static unsigned int extent_used = (MAXTWO * ELEMS_PER_EXTENT + SIZEOF(storExtHdr));
/* Following are values used in queueIndex in a storage element. Note that both
   values must be less than zero for the current code to function correctly. */
#define QUEUE_ANCHOR		-1
#define REAL_MALLOC		-2

/* Define number of malloc and free calls we will keep track of */
#define MAXSMTRACE 128

/* Structure where malloc and free call trace information is kept */
typedef struct
{
	unsigned char	*smAddr;	/* Addr allocated or released */
	unsigned char	*smCaller;	/* Who called malloc/free */
	gtm_msize_t	smSize;		/* Size allocated or freed */
	gtm_msize_t	smTn;		/* What transaction it was */
} smTraceItem;

#ifdef DEBUG
#  define INCR_CNTR(x) ++x
#  define INCR_SUM(x, y) x += y
#  define DECR_CNTR(x) --x
#  define DECR_SUM(x, y) x -= y
#  define SET_MAX(max, tst) {max = MAX(max, tst);}
#  define SET_ELEM_MAX(qtype, idx) SET_MAX(qtype##ElemMax[idx], qtype##ElemCnt[idx])
#  define TRACE_MALLOC(addr, len, tn) 											\
{															\
	if (GDL_SmTrace & gtmDebugLevel)										\
		DBGFPF((stdout, "Malloc at 0x%lx of %ld bytes from 0x%lx (tn=%ld)\n", addr, len, CALLERID, tn));	\
}
#  define TRACE_FREE(addr, len, tn)											\
{															\
	if (GDL_SmTrace & gtmDebugLevel)										\
		DBGFPF((stdout,"Free at 0x%lx of %d bytes from 0x%lx (tn=%ld)\n", addr, len, CALLERID, tn));		\
}
#else
#  define INCR_CNTR(x)
#  define INCR_SUM(x, y)
#  define DECR_CNTR(x)
#  define DECR_SUM(x, y)
#  define SET_MAX(max, tst)
#  define SET_ELEM_MAX(qtype, idx)
#  define TRACE_MALLOC(addr, len, tn)
#  define TRACE_FREE(addr, len, tn)
#endif
#ifdef DEBUG_SM
#  define DEBUGSM(x) (PRINTF x, fflush(stdout))
# else
#  define DEBUGSM(x)
#endif
/* Note we use unsigned char * instead of caddr_t for all references to caller_id so the caller id
   is always 4 bytes. On Tru64, caddr_t is 8 bytes which will throw off the size of our
   storage header in debug mode */
#ifdef GTM_MALLOC_DEBUG
#  define CALLERID (smCallerId)
#else
#  define CALLERID ((unsigned char *)caller_id())
#endif

/* Define "routines" to enqueue and dequeue storage elements. Use define so we don't
   have to depend on each implementation's compiler inlining to get efficient code here */
#define ENQUEUE_STOR_ELEM(qtype, idx, elem)		\
{							\
	  storElem *qHdr, *fElem;			\
	  qHdr = &qtype##StorElemQs[idx];		\
	  STE_FP(elem) = fElem = STE_FP(qHdr);		\
	  STE_BP(elem) = qHdr;				\
	  STE_FP(qHdr) = STE_BP(fElem) = elem;		\
	  INCR_CNTR(qtype##ElemCnt[idx]);		\
	  SET_ELEM_MAX(qtype, idx);			\
}

#define DEQUEUE_STOR_ELEM(qtype, elem)			\
{ 							\
	  STE_FP(STE_BP(elem)) = STE_FP(elem);		\
	  STE_BP(STE_FP(elem)) = STE_BP(elem);		\
	  DECR_CNTR(qtype##ElemCnt[elem->queueIndex]);	\
}

#define GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr) \
{							\
	qHdr = &freeStorElemQs[sizeIndex];		\
	uStor = STE_FP(qHdr);	      			/* First element on queue */ \
	if (QUEUE_ANCHOR != uStor->queueIndex)		/* Does element exist? (Does queue point to itself?) */ \
	{						\
		DEQUEUE_STOR_ELEM(free, uStor);		/* It exists, dequeue it for use */ \
		if (MAXINDEX == sizeIndex)		\
		{	/* Allocating a MAXTWO block. Increment use counter for this subblock's block */ \
			sEHdr = (storExtHdr *)((char *)uStor + uStor->extHdrOffset); \
			++sEHdr->elemsAllocd;		\
		}					\
	} else						\
		uStor = findStorElem(sizeIndex);	\
	assert(0 == ((unsigned long)uStor & (TwoTable[sizeIndex] - 1)));	/* Verify alignment */ \
}

#ifdef INT8_SUPPORTED
#  define ChunkSize 8
#  define ChunkType gtm_int64_t
#  define ChunkValue 0xdeadbeefdeadbeefLL
#else
#  define ChunkSize 4
#  define ChunkType int4
#  define ChunkValue 0xdeadbeef
#endif
#define AddrMask (ChunkSize - 1)

#ifdef DEBUG
/* For debug builds, keep track of the last MAXSMTRACE mallocs and frees. */
GBLDEF volatile int smLastMallocIndex;			/* Index to entry of last malloc-er */
GBLDEF volatile int smLastFreeIndex;			/* Index to entry of last free-er */
GBLDEF smTraceItem smMallocs[MAXSMTRACE];		/* Array of recent allocators */
GBLDEF smTraceItem smFrees[MAXSMTRACE];			/* Array of recent releasers */
GBLDEF volatile unsigned int smTn;			/* Storage management (wrappable) transaction number */
GBLDEF unsigned int outOfMemorySmTn;			/* smTN when ran out of memory */
#endif

GBLREF	uint4		gtmDebugLevel;			/* Debug level (0 = using default sm module so with
							   a DEBUG build, even level 0 implies basic debugging) */
GBLREF  int		process_exiting;		/* Process is on it's way out */
GBLREF	volatile int4	gtmMallocDepth;			/* Recursion indicator. Volatile so it gets stored immediately */
GBLREF	volatile void	*outOfMemoryMitigation;		/* Reserve that we will freed to help cleanup if run out of memory */
GBLREF	uint4		outOfMemoryMitigateSize;	/* Size of above reserve in Kbytes */
GBLREF	int		mcavail;
GBLREF	mcalloc_hdr	*mcavailptr, *mcavailbase;
GBLREF	void		(*cache_table_relobjs)(void);	/* Function pointer to call cache_table_rebuild() */
UNIX_ONLY(GBLREF ch_ret_type (*ht_rhash_ch)();)		/* Function pointer to hashtab_rehash_ch */
UNIX_ONLY(GBLREF ch_ret_type (*jbxm_dump_ch)();)	/* Function pointer to jobexam_dump_ch */
/* This var allows us to call ourselves but still have callerid info */
GBLREF	unsigned char	*smCallerId;			/* Caller of top level malloc/free */
GBLREF	volatile int4	fast_lock_count;		/* Stop stale/epoch processing while we have our parts exposed */

OS_PAGE_SIZE_DECLARE

#define SIZETABLEDIM MAXTWO/MINTWO
STATICD int size2Index[SIZETABLEDIM];

GBLRDEF	boolean_t	gtmSmInitialized;		/* Initialized indicator */
GBLRDEF	size_t		gtmMallocErrorSize;		/* Size of last failed malloc */
GBLRDEF	unsigned char	*gtmMallocErrorCallerid;	/* Callerid of last failed malloc */
GBLRDEF	int		gtmMallocErrorErrno;		/* Errno at point of last failure */

GBLRDEF readonly struct
{
	unsigned char nullHMark[4];
	unsigned char nullStr[1];
	unsigned char nullTMark[4];
} NullStruct
#ifdef DEBUG
        /* Note, tiz important the first 4 bytes of this are same as markerChar defined below as that is the value both nullHMark
         * and nullTMark are asserted against to validate against corruption.
         */
        = {0xde, 0xad, 0xbe, 0xef, 0x00, 0xde, 0xad, 0xbe, 0xef}
#endif
        ;

#ifdef DEBUG
/* Arrays allocated with size of MAXINDEX + 2 are sized to hold an extra
 * entry for "real malloc" type allocations. Note that the arrays start with
 *  the next larger element with GTM64 due to increased overhead from the
 *  8 byte pointers.
 */

STATICD readonly uint4 TwoTable[MAXINDEX + 2] = {
#  ifndef GTM64
	64,
#  endif
	128, 256, 512, 1024, 2048, 0xFFFFFFFF};	/* Powers of two element sizes */

#  ifdef GTM64
STATICD readonly unsigned char markerChar[8] = {0xde, 0xad, 0xbe, 0xef, 0xef, 0xbe, 0xad, 0xde};
#  else
STATICD readonly unsigned char markerChar[4] = {0xde, 0xad, 0xbe, 0xef};
#  endif
#else
STATICD readonly uint4 TwoTable[MAXINDEX + 2] = {
#  ifndef GTM64
	16,
#  endif
	32, 64, 128, 256, 512, 1024, 2048, 0xFFFFFFFF};
#endif

STATICD storElem	freeStorElemQs[MAXINDEX + 1];	/* Need full element as queue anchor for dbl-linked
							   list since ptrs not at top of element */
STATICD storExtHdr	storExtHdrQ;			/* List of storage blocks we allocate here */
STATICD uint4		curExtents;			/* Number of current extents */

#ifdef GTM_MALLOC_REENT
STATICD storElem *deferFreeQueues[MAXDEFERQUEUES];	/* Where deferred (nested) frees are queued for later processing */
STATICD boolean_t deferFreeExists;			/* A deferred free is pending on a queue */
#endif

#ifdef DEBUG
STATICD storElem allocStorElemQs[MAXINDEX + 2];		/* The extra element is for queueing "real" malloc'd entries */
#  ifdef INT8_SUPPORTED
STATICD readonly unsigned char backfillMarkC[8] = {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef};
#  else
STATICD readonly unsigned char backfillMarkC[4] = {0xde, 0xad, 0xbe, 0xef};
#  endif
#endif

GBLREF	size_t	totalRmalloc;				/* Total storage currently (real) malloc'd (includes extent blocks) */
GBLREF  size_t	totalAlloc;				/* Total allocated (includes allocation overhead but not free space */
GBLREF  size_t	totalUsed;				/* Sum of user allocated portions (totalAlloc - overhead) */

#ifdef DEBUG
/* Define variables used to instrument how our algorithm works */
STATICD	uint4	totalMallocs;				/* Total malloc requests */
STATICD	uint4	totalFrees;				/* Total free requests */
STATICD uint4	totalExtents;				/* Times we allocated more storage */
STATICD uint4	maxExtents;				/* Highwater mark of extents */
STATICD	size_t	rmallocMax;				/* Maximum value of totalRmalloc */
STATICD	uint4	mallocCnt[MAXINDEX + 2];		/* Malloc count satisfied by each queue size */
STATICD	uint4	freeCnt[MAXINDEX + 2];			/* Free count for element in each queue size */
STATICD	uint4	elemSplits[MAXINDEX + 2];		/* Times a given queue size block was split */
STATICD	uint4	elemCombines[MAXINDEX + 2];		/* Times a given queue block was formed by buddies being recombined */
STATICD	uint4	freeElemCnt[MAXINDEX + 2];		/* Current count of elements on the free queue */
STATICD	uint4	allocElemCnt[MAXINDEX + 2];		/* Current count of elements on the allocated queue */
STATICD	uint4	freeElemMax[MAXINDEX + 2];		/* Maximum number of blocks on the free queue */
STATICD	uint4	allocElemMax[MAXINDEX + 2];		/* Maximum number of blocks on the allocated queue */
GMR_ONLY(STATICD	uint4	reentMallocs;)		/* Total number of reentrant mallocs made */
GMR_ONLY(STATICD	uint4	deferFreePending;)	/* Total number of frees that were deferred */
#endif

/* Macro to return an index into the TwoTable for a given size (round up to next power of two)
   Use the size2Index table to get the proper index. This table is indexed by the number of
   storage "blocks" being requested. A storage block is the size of the smallest power of two
   block we can allocate (size MINTWO) */
#ifdef DEBUG
#  define GetSizeIndex(size) (size ? size2Index[(size - 1) / MINTWO] : assert(FALSE))
#else
#  define GetSizeIndex(size) (size2Index[(size - 1) / MINTWO])
#endif

/* Internal prototypes */
void gtmSmInit(void);
storElem *findStorElem(int sizeIndex);
void release_unused_storage(void);
#ifdef DEBUG
void backfill(unsigned char *ptr, gtm_msize_t len);
boolean_t backfillChk(unsigned char *ptr, gtm_msize_t len);
#else
void *gtm_malloc_dbg(size_t);
void gtm_free_dbg(void *);
void raise_gtmmemory_error_dbg(void);
size_t gtm_bestfitsize_dbg(size_t);
#endif

/* Initialize the storage manangement system. Things to initialize:

   - Initialize size2Index table. This table is used to convert a malloc request size
     to a storage queue index.
   - Initialize queue anchor fwd/bkwd pointers to point to queue anchors so we
     build a circular queue. This allows elements to be added and removed without
     end-of-queue special casing. The queue anchor element is easily recognized because
     it's queue index size will be set to a special value.
   - Initialize debug mode. See if gtm_debug_level environment variable is set and
     retrieve it's value if yes.
*/
void gtmSmInit(void)	/* Note renamed to gtmSmInit_dbg when included in gtm_malloc_dbg.c */
{
	char		*ascNum;
	storElem	*uStor;
	int		i, sizeIndex, testSize, blockSize, save_errno;

	error_def(ERR_INVMEMRESRV);

	/* WARNING!! Since this is early initialization, the following asserts are not well behaved if they do
	   indeed trip. The best that can be hoped for is they give a condition handler exhausted error on
	   GTM startup. Unfortunately, more intelligent responses are somewhat elusive since no output devices
	   are setup nor (potentially) most of the GTM runtime.
	*/
	assert(MINTWO == TwoTable[0]);
#	if defined(__linux__) && !defined(__i386)
        /* This will make sure that all the memory allocated using 'malloc' will be in heap and no 'mmap' is used.
         * This is needed to make sure that the offset calculation that we do at places(que_ent, chache_que, etc..)
         * using 2 'malloc'ed memory can be hold in an integer. Though this will work without any problem as the
         * current GT.M will not allocate memory more than 4GB, we should find a permanant solution by migrating those
         * offset fields to long and make sure all other related application logic works fine.
         */
	mallopt(M_MMAP_MAX, 0);
#	endif /* __linux__ && !__i386 */
	/* Check that the storage queue offset in a storage element has sufficient reach
	   to cover an extent.
	*/
	assert(((extent_used - SIZEOF(storExtHdr)) <= ((1 << (SIZEOF(uStor->extHdrOffset) * 8)) - 1)));

	/* Initialize size table used to get a storage queue index */
	sizeIndex = 0;
	testSize = blockSize = MINTWO;
	for (i = 0; i < SIZETABLEDIM; i++, testSize += blockSize)
	{
		if (testSize > TwoTable[sizeIndex])
			++sizeIndex;
		size2Index[i] = sizeIndex;
	}

	/* Need to initialize the fwd/bck ptrs in the anchors to point to themselves */
	for (uStor = &freeStorElemQs[0], i = 0; i <= MAXINDEX; ++i, ++uStor)
	{
		STE_FP(uStor) = STE_BP(uStor) = uStor;
		uStor->queueIndex = QUEUE_ANCHOR;
	}
	DEBUG_ONLY(
		for (uStor = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++uStor)
		{
			STE_FP(uStor) = STE_BP(uStor) = uStor;
			uStor->queueIndex = QUEUE_ANCHOR;
		}
	);
	dqinit(&storExtHdrQ, links);

	/* One last task before we consider ourselves initialized. Allocate the out-of-memory mitigation storage
	   that we will hold onto but not use. If we get an out-of-memory error, this storage will be released back
	   to the OS for it or GTM to use as necessary while we try to go about an orderly shutdown of our process.
	   The term "release" here means a literal release. The thinking is we don't know whether GTM's small storage
	   manager will make use of this storage (32K at a time) or if a larger malloc() will be done by libc for
	   buffers or what not so we will just give this chunk back to the OS to use as it needs it.
	*/
	if (0 < outOfMemoryMitigateSize)
	{
		assert(NULL == outOfMemoryMitigation);
		outOfMemoryMitigation = malloc(outOfMemoryMitigateSize * 1024);
		if (NULL == outOfMemoryMitigation)
		{
			save_errno = errno;
			gtm_putmsg(VARLSTCNT(5) ERR_INVMEMRESRV, 2,
				   RTS_ERROR_LITERAL(UNIX_ONLY("$gtm_memory_reserve")VMS_ONLY("GTM_MEMORY_RESERVE")),
				   save_errno);
			exit(save_errno);
		}
	}
	gtmSmInitialized = TRUE;
}

/* Recursive routine used to obtain an element on a given size queue. If no
   elements of that size are available, we recursively call ourselves to get
   an element of the next larger queue which we will then split in half to
   get the one we need and place the remainder back on the free queue of its
   new smaller size. If we run out of queues, we obtain a fresh new 'hunk' of
   storage, carve it up into the largest block size we handle and process as
   before. */
storElem *findStorElem(int sizeIndex)	/* Note renamed to findStorElem_dbg when included in gtm_malloc_dbg.c */
{
	unsigned char	*uStorAlloc;
	storElem	*uStor, *uStor2, *qHdr;
	storExtHdr	*sEHdr;
	int		hdrSize;
	unsigned int	i;

	UNIX_ONLY(error_def(ERR_SYSCALL);)

	++sizeIndex;
	DEBUG_ONLY(hdrSize = OFFSETOF(storElem, userStorage));	/* Size of storElem header */
	if (MAXINDEX >= sizeIndex)
	{	/* We have more queues to search */
	        GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr);

		/* We have a larger than necessary element now so break it in half and put
		   the second half on the queue one size smaller than us */
		INCR_CNTR(elemSplits[sizeIndex]);
		--sizeIndex;					/* Dealing now with smaller element queue */
		assert(sizeIndex >= 0 && sizeIndex < MAXINDEX);
		uStor2 = (storElem *)((unsigned long)uStor + TwoTable[sizeIndex]);
		uStor2->state = Free;
		uStor2->queueIndex = sizeIndex;
		assert(0 == ((unsigned long)uStor2 & (TwoTable[sizeIndex] - 1)));		/* Verify alignment */
		DEBUG_ONLY(
			memcpy(uStor2->headMarker, markerChar, SIZEOF(uStor2->headMarker));	/* Put header tag in place */
			/* Backfill entire block being freed so usage of it will cause problems */
			if (GDL_SmBackfill & gtmDebugLevel)
				backfill((unsigned char *)uStor2 + hdrSize, TwoTable[sizeIndex] - hdrSize);
		);
		ENQUEUE_STOR_ELEM(free, sizeIndex, uStor2);	/* Place on free queue */
	} else
	{	/* Nothing left to search, [real]malloc a new ALIGNED block of storage and put it on our queues */
		++curExtents;
		SET_MAX(maxExtents, curExtents);
		INCR_CNTR(totalExtents);
		/* Allocate size for one more subblock than we want. This guarrantees us that we can put our subblocks
		   on a power of two boundary necessary for buddy alignment
		*/
		MALLOC(EXTENT_SIZE, uStorAlloc);
		uStor2 = (storElem *)uStorAlloc;
		/* Make addr "MAXTWO" byte aligned */
		uStor = (storElem *)(((unsigned long)(uStor2) + MAXTWO - 1) & (unsigned long) -MAXTWO);
		totalRmalloc += EXTENT_SIZE;
		SET_MAX(rmallocMax, totalRmalloc);
		sEHdr = (storExtHdr *)((char *)uStor + (ELEMS_PER_EXTENT * MAXTWO));
		DEBUGSM(("debugsm: Allocating extent at 0x%08lx\n", uStor));

		/* If the storage given to us was aligned, we have ELEMS_PER_EXTENT+1 blocks, else we have
		   ELEMS_PER_EXTENT blocks. We won't put the first element on the queue since that block is
		   being returned to be split.
		*/
		if (uStor == uStor2)
		{
			i = 0;		/* The storage was suitably aligned, we get an extra block free */
			sEHdr = (storExtHdr *)((char *)sEHdr + MAXTWO);
			extent_used = EXTENT_SIZE; /* New max for sanity checks */
		} else
			i = 1;		/* The storage was not aligned. Have planned number of blocks with some waste */
		assert(((char *)sEHdr + SIZEOF(*sEHdr)) <= ((char *)uStorAlloc + EXTENT_SIZE));
		for (uStor2 = uStor; ELEMS_PER_EXTENT > i; ++i)
		{	/* Place all but first entry on the queue */
			uStor2 = (storElem *)((unsigned long)uStor2 + MAXTWO);
			assert(0 == ((unsigned long)uStor2 & (TwoTable[MAXINDEX] - 1)));	/* Verify alignment */
			uStor2->state = Free;
			uStor2->queueIndex = MAXINDEX;
			uStor2->extHdrOffset = (char *)sEHdr - (char *)uStor2;
			assert(extent_used > uStor2->extHdrOffset);
			DEBUG_ONLY(
				memcpy(uStor2->headMarker, markerChar, SIZEOF(uStor2->headMarker));
				/* Backfill entire block on free queue so we can detect trouble
				 * with premature usage or overflow from something else
				 */
				if (GDL_SmBackfill & gtmDebugLevel)
					backfill((unsigned char *)uStor2 + hdrSize, TwoTable[MAXINDEX] - hdrSize);
			);
			ENQUEUE_STOR_ELEM(free, MAXINDEX, uStor2);	/* Place on free queue */
		}
		uStor->extHdrOffset = (char *)sEHdr - (char *)uStor;
		uStor->state = Free;
		sizeIndex = MAXINDEX;
		/* Set up storage block header */
		sEHdr->extentStart = uStorAlloc;
		sEHdr->elemStart = uStor;
		sEHdr->elemsAllocd = 1;
		dqins(&storExtHdrQ, links, sEHdr);
	}
	assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
	uStor->queueIndex = sizeIndex;		/* This is now a smaller block */
	return uStor;
}

#ifdef GTM_MALLOC_REENT
/* Routine to process deferred frees in the deferred free queues */
void processDeferredFrees()	/* Note renamed to processDeferredFrees_dbg when included in gtm_malloc_dbg.c */
{
	int		dqIndex;
	storElem	*uStor, *uStorNext;
	VMS_ONLY(error_def(ERR_FREEMEMORY);)

	assert(0 == gtmMallocDepth);
	do
	{
		deferFreeExists = FALSE;
		/* Run queue in reverse order so we can process the highest index queues first freeing them
		   up that much sooner. This eliminates the problem of index creep. */
		for (dqIndex = MAXDEFERQUEUES - 1; 0 <= dqIndex; --dqIndex)
		{
			/* Check if queue is empty or not once outside of the gtmMallocDepth lock 'cause
			   we don't want to get the lock unless we really need to */
			if (deferFreeQueues[dqIndex])
			{
				gtmMallocDepth = dqIndex + 2;
				uStor = deferFreeQueues[dqIndex];	/* Dequeue entire chain at this location */
				deferFreeQueues[dqIndex] = NULL;
				gtmMallocDepth = 0;
				for (; uStor; uStor = uStorNext)	/* Release all elements on this queue */
				{
					uStorNext = uStor->userStorage.deferFreeNext;
					gtm_free(&uStor->userStorage.userStart);
				}
			}
		}
	} while (deferFreeExists);
}
#endif


/* Note, if the below declaration changes, corresponding changes in gtmxc_types.h needs to be done. */
/* Obtain free storage of the given size */
void *gtm_malloc(size_t size)	/* Note renamed to gtm_malloc_dbg when included in gtm_malloc_dbg.c */
{
	unsigned char	*retVal;
	storElem 	*uStor, *qHdr;
	storExtHdr	*sEHdr;
	gtm_msize_t	tSize;
	int		sizeIndex, i, hdrSize;
	unsigned char	*trailerMarker;
	boolean_t	reentered;

	UNIX_ONLY(error_def(ERR_SYSCALL);)
	error_def(ERR_MEMORYRECURSIVE);

#	ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (default/optimized) case we will fall into the code
	   and handle the rerouting at the end. */
	if (GDL_None == gtmDebugLevel)
	{
#	endif
		/* Note that this if is also structured for maximum fallthru. The else will
		   be near the end of this entry point */
		if (gtmSmInitialized)
		{
			hdrSize = OFFSETOF(storElem, userStorage);		/* Size of storElem header */
			NON_GTM64_ONLY(if ((size + hdrSize) < size) GTMASSERT); /* Check for wrap in 32 bit platforms */
			assert((hdrSize + SIZEOF(markerChar)) < MINTWO);

			NON_GMR_ONLY(fast_lock_count++);
			++gtmMallocDepth;				/* Nesting depth of memory calls */
			reentered = (1 < gtmMallocDepth);
			NON_GMR_ONLY(
				if (reentered)
				{
					--gtmMallocDepth;
					assert(FALSE);
					rts_error(VARLSTCNT(1) ERR_MEMORYRECURSIVE);
				}
			);
			INCR_CNTR(totalMallocs);
			INCR_CNTR(smTn);
			/* Validate null string not overwritten */
			assert(0 == memcmp(&NullStruct.nullHMark[0], markerChar, SIZEOF(NullStruct.nullHMark)));
			assert(0 == memcmp(&NullStruct.nullTMark[0], markerChar, SIZEOF(NullStruct.nullHMark)));
			DEBUG_ONLY(
				GMR_ONLY(if (!reentered))
				{	/* Verify the storage chains before we play */
					VERIFY_STORAGE_CHAINS;
				}
			);
			if (0 != size)
			{
				GMR_ONLY(size = MAX(SIZEOF(char *), size);)	/* Need room for deferred free next pointer */
				tSize = size + hdrSize;				/* Add in header size */
				DEBUG_ONLY(
					tSize += SIZEOF(markerChar);			/* Add in room for trailer label */
					/* If being a storage hog, we want to make sure we have plenty of room for
					 * filler. For strings up to MAXTWO in length, we pad with an additional 50%
					 *  of storage with a minimum of 32 bytes and a maximum of 256 bytes. For larger
					 *  strings, we pad with 256 bytes. Since selecting GDL_SmStorHog also turns on
					 *  GDL_SmBackfill and GDL_SmChkAllocBackfill, this padding will be backfilled and
					 *  checked during allocate storage validation calls.
					 */
					if (GDL_SmStorHog & gtmDebugLevel)
					{
						if (MAXTWO >= size)
							tSize += (MIN(MAX(size / 2, 32), 256));
						else
							tSize += 256;
					}
				);
				/* The difference between $ZALLOCSTOR and $ZUSEDSTOR (totalAlloc and totalUsed global vars)  is
				 * that when you allocate, say 16 bytes, that comes out of a 32 byte chunk (with the pro storage
				 * mgr) with the rest being unusable. In a debug build (or a pro build with $gtmdbglvl set to
				 * something non-zero), $ZUSEDSTOR is incremented by 16 bytes (the requested allocation) while
				 * $ZALLOCSTOR is incremented by 32 bytes (the actual allocation). But, in a pro build using
				 * the pro memory manager, we do not track the user-allocated size anywhere. We know it when
				 * we do the allocation of course, but when it comes time to free it, we no longer know what
				 * the user requested size was. We only know that it came out of a 32 byte block. In order for
				 * the free to be consistent with the allocation, we have to use the one value we know at both
				 * malloc and free times - 32 bytes. The net result is that $ZALLOCSTOR and $ZUSEDSTOR report
				 * the same value in a pro build with the pro stmgr while they will be quite different in a
				 * debug build or a pro build with $gtmdbglvl engaged. The difference between them shows the
				 * allocation overhead of gtm_malloc itself.
				 */
				if (MAXTWO >= tSize GMR_ONLY(&& !reentered))
				{	/* Use our memory manager for smaller pieces */
					sizeIndex = GetSizeIndex(tSize);		/* Get index to size we need */
					assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
					GET_QUEUED_ELEMENT(sizeIndex, uStor, qHdr, sEHdr);
					tSize = TwoTable[sizeIndex];
					uStor->realLen = tSize;
				} else
				{	/* Use regular malloc to obtain the piece */
					MALLOC(tSize, uStor);
					totalRmalloc += tSize;
					SET_MAX(rmallocMax, totalRmalloc);
					uStor->queueIndex = REAL_MALLOC;
					uStor->realLen = tSize;
					DEBUG_ONLY(sizeIndex = MAXINDEX + 1);	/* Just so the ENQUEUE below has a queue since
										 * we use -1 as the "real" queueindex  for
										 * malloc'd storage and we don't record allocated
										 * storage in other than debug mode. */
				}
				totalUsed += DEBUG_ONLY(size) PRO_ONLY(tSize);
				totalAlloc += tSize;
				INCR_CNTR(mallocCnt[sizeIndex]);
				uStor->state = Allocated;
#				ifdef DEBUG
				/* Fill in extra debugging fields in header */
				uStor->allocatedBy = CALLERID;				/* Who allocated us */
				uStor->allocLen = size;					/* User requested size */
				memcpy(uStor->headMarker, markerChar, SIZEOF(uStor->headMarker));
				trailerMarker = (unsigned char *)&uStor->userStorage.userStart + size;	/* Where to put trailer */
				memcpy(trailerMarker, markerChar, SIZEOF(markerChar));	/* Small trailer */
				if (GDL_SmInitAlloc & gtmDebugLevel)
					/* Initialize the space we are allocating */
					backfill((unsigned char *)&uStor->userStorage.userStart, size);
				if (GDL_SmBackfill & gtmDebugLevel)
				{	/* Use backfill method of after-allocation metadata */
					backfill(trailerMarker + SIZEOF(markerChar),
						 (uStor->realLen - size - hdrSize - SIZEOF(markerChar)));
				}

				uStor->smTn = smTn;					/* transaction number */
				GMR_ONLY(if (!reentered))
				{
					ENQUEUE_STOR_ELEM(alloc, sizeIndex, uStor);
				}
#				ifdef GTM_MALLOC_REENT
				else
				{	/* Reentrant allocates cannot be put on our allocated queue -- sorry too dangerous */
					uStor->fPtr = uStor->bPtr = NULL;
					INCR_CNTR(allocElemCnt[sizeIndex]);
					INCR_CNTR(reentMallocs);
				}
#				endif
#				endif
				retVal = &uStor->userStorage.userStart;
				assert(((long)retVal & (long)-8) == (long)retVal);	/* Assert we have an 8 byte boundary */
			} else	/* size was 0 */
				retVal = &NullStruct.nullStr[0];
			DEBUG_ONLY(
				/* Record this transaction in debugging history */
				++smLastMallocIndex;
				if (MAXSMTRACE <= smLastMallocIndex)
					smLastMallocIndex = 0;
				smMallocs[smLastMallocIndex].smAddr = retVal;
				smMallocs[smLastMallocIndex].smSize = size;
				smMallocs[smLastMallocIndex].smCaller = CALLERID;
				smMallocs[smLastMallocIndex].smTn = smTn;
			);
			TRACE_MALLOC(retVal, size, smTn);

			--gtmMallocDepth;
			GMR_ONLY(
				/* Check on deferred frees */
				if (0 == gtmMallocDepth && deferFreeExists)
					processDeferredFrees();
			);
			NON_GMR_ONLY(--fast_lock_count);
			DEFERRED_EXIT_HANDLING_CHECK;
			return retVal;
		} else  /* Storage mgmt has not been initialized */
		{
			gtmSmInit();
			/* Reinvoke gtm_malloc now that we are initialized. Note that this one time (the first
			   call to malloc), we will not record the proper caller id in the storage header or in
			   the traceback table. The caller will show up as gtm_malloc(). However, all subsequent
			   calls will be correct.
			*/
			return (void *)gtm_malloc(size);
		}
#	ifndef DEBUG
	} else
	{	/* We have a non-DEBUG module but debugging is turned on so redirect the call to the appropriate module */
		smCallerId = (unsigned char *)caller_id();
		return (void *)gtm_malloc_dbg(size);
	}
#	endif
}

/* Note, if the below declaration changes, corresponding changes in gtmxc_types.h needs to be done. */
/* Release the free storage at the given address */
void gtm_free(void *addr)	/* Note renamed to gtm_free_dbg when included in gtm_malloc_dbg.c */
{
	storElem 	*uStor, *buddyElem;
	storExtHdr	*sEHdr;
	unsigned char	*trailerMarker;
	int 		sizeIndex, hdrSize, saveIndex, dqIndex, freedElemCnt;
	gtm_msize_t	saveSize, allocSize;

	error_def(ERR_MEMORYRECURSIVE);
	UNIX_ONLY(error_def(ERR_MEMORY);)
	VMS_ONLY(error_def(ERR_FREEMEMORY);)
	VMS_ONLY(error_def(ERR_VMSMEMORY);)

#	ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (optimized) case we will fall into the code and
	   handle the rerouting at the end. */
	if (GDL_None == gtmDebugLevel)
	{
#	endif
		if (!gtmSmInitialized)	/* Storage must be init'd before can free anything */
			GTMASSERT;
		/* If we are exiting, don't bother with frees. Process destruction can do it *UNLESS* we are handling an
		   out of memory condition with the proviso that we can't return memory if we are already nested */
		if (process_exiting && (0 != gtmMallocDepth || error_condition != UNIX_ONLY(ERR_MEMORY) VMS_ONLY(ERR_VMSMEMORY)))
			return;
		NON_GMR_ONLY(++fast_lock_count);
		++gtmMallocDepth;	/* Recursion indicator */

#		ifdef GTM_MALLOC_REENT
		/* If we are attempting to do a reentrant free, we will instead put the free on a queue to be released
		   at a later time. Ironically, since we cannot be sure of any queues of available blocks, we have to
		   malloc a small block to carry this info which we will free with the main storage */
		if (1 < gtmMallocDepth)
		{
			if ((unsigned char *)addr != &NullStruct.nullStr[0])
			{
				dqIndex = gtmMallocDepth - 2;		/* 0 origin index into defer queues */
				if (MAXDEFERQUEUES <= dqIndex)		/* Can't run out of queues */
					GTMASSERT;
				hdrSize = offsetof(storElem, userStorage);
				uStor = (storElem *)((unsigned long)addr - hdrSize);		/* Backup ptr to element header */
				uStor->userStorage.deferFreeNext = deferFreeQueues[dqIndex];
				deferFreeQueues[dqIndex] = uStor;
				deferFreeExists = TRUE;
				INCR_CNTR(deferFreePending);
			}
			--gtmMallocDepth;
			return;
		}
#		else
		if (1 < gtmMallocDepth)
		{
			--gtmMallocDepth;
			assert(FALSE);
			rts_error(VARLSTCNT(1) ERR_MEMORYRECURSIVE);
		}
#		endif
		INCR_CNTR(smTn);	/* Bump the transaction number */

		/* Validate null string not overwritten */
		assert(0 == memcmp(&NullStruct.nullHMark[0], markerChar, SIZEOF(NullStruct.nullHMark)));
		assert(0 == memcmp(&NullStruct.nullTMark[0], markerChar, SIZEOF(NullStruct.nullHMark)));
		/* verify chains before we attempt dequeue */
		DEBUG_ONLY(VERIFY_STORAGE_CHAINS);
		INCR_CNTR(totalFrees);
		if ((unsigned char *)addr != &NullStruct.nullStr[0])
		{
			hdrSize = OFFSETOF(storElem, userStorage);
			uStor = (storElem *)((unsigned long)addr - hdrSize);		/* Backup ptr to element header */
			sizeIndex = uStor->queueIndex;
#			ifdef DEBUG
			if (GDL_SmInitAlloc & gtmDebugLevel)
				/* Initialize the space we are de-allocating */
				backfill((unsigned char *)&uStor->userStorage.userStart, uStor->allocLen);
			TRACE_FREE(addr, uStor->allocLen, smTn);
			saveSize = uStor->allocLen;
			/* Extra checking for debugging. Note that these sanity checks are only done in debug
			   mode. The thinking is that we will bypass the checks in the general case for speed but
			   if we really need to chase a storage related problem, we should switch to the debug version
			   in the field to turn on these and other checks.
			*/
			assert(Allocated == uStor->state);
			assert(0 == memcmp(uStor->headMarker, markerChar, SIZEOF(uStor->headMarker)));
			trailerMarker = (unsigned char *)&uStor->userStorage.userStart + uStor->allocLen;/* Where trailer was put */
			assert(0 == memcmp(trailerMarker, markerChar, SIZEOF(markerChar)));
			if (GDL_SmChkAllocBackfill & gtmDebugLevel)
			{	/* Use backfill check method for after-allocation metadata */
				assert(backfillChk(trailerMarker + SIZEOF(markerChar),
						   (uStor->realLen - uStor->allocLen - hdrSize - SIZEOF(markerChar))));
			}

			/* Remove element from allocated queue unless element is from a reentered malloc call. In that case, just
			 * manipulate the counters.
			 */
			if (NULL != uStor->fPtr)
			{
				if (0 <= uStor->queueIndex)
				{
					DEQUEUE_STOR_ELEM(alloc, uStor);
				} else
				{	/* shenanigans so that counts are maintained properly in debug mode */
					saveIndex = uStor->queueIndex;
					uStor->queueIndex = MAXINDEX + 1;
					DEQUEUE_STOR_ELEM(alloc, uStor);
					uStor->queueIndex = saveIndex;
				}
			} else
				DECR_CNTR(allocElemCnt[((0 <= uStor->queueIndex) ? uStor->queueIndex : MAXINDEX + 1)]);
#			endif
			totalUsed -= DEBUG_ONLY(uStor->allocLen) PRO_ONLY(uStor->realLen);
			if (sizeIndex >= 0)
			{	/* We can put the storage back on one of our simple queues */
				assert(0 == ((unsigned long)uStor & (TwoTable[sizeIndex] - 1)));	/* Verify alignment */
				assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
				uStor->state = Free;
				INCR_CNTR(freeCnt[sizeIndex]);
				assert(uStor->realLen == TwoTable[sizeIndex]);
				totalAlloc -= TwoTable[sizeIndex];
			        /* First, if there are larger queues than this one, see if it has a buddy that it can
				   combine with */
				while (sizeIndex < MAXINDEX)
				{
					buddyElem = (storElem *)((unsigned long)uStor ^ TwoTable[sizeIndex]);/* Address of buddy */
					assert(0 == ((unsigned long)buddyElem & (TwoTable[sizeIndex] - 1)));/* Verify alignment */
					assert(buddyElem->state == Allocated || buddyElem->state == Free);
					assert(buddyElem->queueIndex >= 0 && buddyElem->queueIndex <= sizeIndex);
					if (buddyElem->state == Allocated || buddyElem->queueIndex != sizeIndex)
						/* All possible combines done */
						break;

					/* Remove buddy from its queue and make a larger element for a larger queue */
					DEQUEUE_STOR_ELEM(free, buddyElem);
					if (buddyElem < uStor)		/* Pick lower address buddy for top of new bigger block */
						uStor = buddyElem;
					++sizeIndex;
					assert(sizeIndex >= 0 && sizeIndex <= MAXINDEX);
					INCR_CNTR(elemCombines[sizeIndex]);
					uStor->queueIndex = sizeIndex;
				}
				DEBUG_ONLY(
					/* Backfill entire block being freed so usage of it will cause problems */
					if (GDL_SmBackfill & gtmDebugLevel)
						backfill((unsigned char *)uStor + hdrSize, TwoTable[sizeIndex] - hdrSize);
				);
				ENQUEUE_STOR_ELEM(free, sizeIndex, uStor);
				if (MAXINDEX == sizeIndex)
				{	/* Freeing/Coagulating a MAXTWO block. Decrement use counter for this element's block */
					sEHdr = (storExtHdr *)((char *)uStor + uStor->extHdrOffset);
					--sEHdr->elemsAllocd;
					assert(0 <= sEHdr->elemsAllocd);
					/* Check for an extent being ripe for return to the system. Requirements are:
					     1) All subblocks must be free (elemsAllocd == 0).
					     2) There must be more than STOR_EXTENTS_KEEP extents already allocated.
					   If these conditions are met, we will dequeue each individual element from
					   it's queue and release the entire extent in a (real) free.
					*/
					if (STOR_EXTENTS_KEEP < curExtents && 0 == sEHdr->elemsAllocd)
					{	/* Release this extent */
						DEBUGSM(("debugsm: Extent being freed from 0x%08lx\n", sEHdr->elemStart));
						DEBUG_ONLY(freedElemCnt = 0);
						for (uStor = sEHdr->elemStart;
						     (char *)uStor < (char *)sEHdr;
						     uStor = (storElem *)((char *)uStor + MAXTWO))
						{
							DEBUG_ONLY(++freedElemCnt);
							assert(Free == uStor->state);
							assert(MAXINDEX == uStor->queueIndex);
							DEQUEUE_STOR_ELEM(free, uStor);
							DEBUGSM(("debugsm: ... element removed from free q 0x%08lx\n", uStor));
						}
						assert(ELEMS_PER_EXTENT <= freedElemCnt);	/* one loop to free them all */
						assert((char *)uStor == (char *)sEHdr);
						dqdel(sEHdr, links);
						FREE(EXTENT_SIZE, sEHdr->extentStart);
						totalRmalloc -= EXTENT_SIZE;
						--curExtents;
						assert(curExtents);
					}
				}

			} else
			{
				assert(REAL_MALLOC == sizeIndex);		/* Better be a real malloc type block */
				INCR_CNTR(freeCnt[MAXINDEX + 1]);		/* Count free of malloc */
				allocSize = saveSize = uStor->realLen;
				DEBUG_ONLY(
					/* Backfill entire block being freed so usage of it will cause problems */
					if (GDL_SmBackfill & gtmDebugLevel)
						backfill((unsigned char *)uStor, allocSize);
				);
				FREE(allocSize, uStor);
				totalRmalloc -= allocSize;
				totalAlloc -= allocSize;
			}
		}
		DEBUG_ONLY(
			/* Make trace entry for this free */
			++smLastFreeIndex;
			if (MAXSMTRACE <= smLastFreeIndex)
				smLastFreeIndex = 0;
			smFrees[smLastFreeIndex].smAddr = addr;
			smFrees[smLastFreeIndex].smSize = saveSize;
			smFrees[smLastFreeIndex].smCaller = CALLERID;
			smFrees[smLastFreeIndex].smTn = smTn;
		);
		--gtmMallocDepth;
		GMR_ONLY(
			/* Check on deferred frees */
			if (0 == gtmMallocDepth && deferFreeExists)
				processDeferredFrees();
		);
		NON_GMR_ONLY(--fast_lock_count);
#	ifndef DEBUG
	} else
	{	/* If not a debug module and debugging is enabled, reroute call to
		   the debugging version.
		*/
		smCallerId = (unsigned char *)caller_id();
		gtm_free_dbg(addr);
	}
#	endif
	DEFERRED_EXIT_HANDLING_CHECK;
}


/* When an out-of-storage type error is encountered, besides releasing our memory reserve, we also
   want to release as much unused storage within various GTM queues that we can find.
*/
void release_unused_storage(void)	/* Note renamed to release_unused_storage_dbg when included in gtm_malloc_dbg.c */
{
	mcalloc_hdr	*curhdr, *nxthdr;

	/* Release compiler storage if we aren't in the compiling business currently */
	if (NULL != mcavailbase && mcavailptr == mcavailbase && mcavail == mcavailptr->size)
	{	/* Buffers are unused and subject to release */
		for (curhdr = mcavailbase; curhdr; curhdr = nxthdr)
		{
			nxthdr = curhdr->link;
			gtm_free(curhdr);
		}
		mcavail = 0;
		mcavailptr = mcavailbase = NULL;
	}
	/* If the cache_table_rebuild() routine is available in this executable, call it through its
	   function pointer. */
	if (NULL != cache_table_relobjs)
		(*cache_table_relobjs)();	/* Release object code in indirect cache */
}


/* Raise ERR_MEMORY or ERR_VMSMEMORY. Separate routine since is called from hashtable logic in place of the
   previous HTEXPFAIL error message. As such, it checks and properly deals with which flavor is running
   (debug or non-debug).
*/
void raise_gtmmemory_error(void)	/* Note renamed to raise_gtmmemory_error_dbg when included in gtm_malloc_dbg.c */
{
	void	*addr;

	VMS_ONLY(error_def(ERR_VMSMEMORY);)
	UNIX_ONLY(error_def(ERR_MEMORY);)

#	ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (optimized) case we will fall into the code and
	   handle the rerouting at the end.

	   Note: The DEBUG expansion of this code in a pro build actually has a different
	   entry point name (raise_gtmmeory_error_dbg) and if malloc debugging options are
	   on in pro, we need to call that version, but since efficiency in pro trumps
	   clarity, we put the redirecting call at the bottom of the if-else block to
	   avoid disrupting the instruction pipeline.
	*/
        if (GDL_None == gtmDebugLevel)
        {
#	endif
		if (NULL != (addr = (void *)outOfMemoryMitigation)
		    UNIX_ONLY(&& !(ht_rhash_ch == active_ch->ch || jbxm_dump_ch == active_ch->ch || stp_gcol_ch == active_ch->ch)))
		{       /* Free our reserve only if not in certain condition handlers (on UNIX) since it is */
			/* going to unwind this error and ignore it. On VMS the error will not be trapped   */
			outOfMemoryMitigation = NULL;
			UNIX_ONLY(free(addr));
			VMS_ONLY(lib$free_vm(addr));
			DEBUG_ONLY(if (0 == outOfMemorySmTn) outOfMemorySmTn = smTn);
			/* Must decr gtmMallocDepth after release above but before the  */
			/* call to release_unused_storage() below.                      */
			--gtmMallocDepth;
			release_unused_storage();
		} else
			--gtmMallocDepth;
		UNIX_ONLY(--fast_lock_count);
		DEFERRED_EXIT_HANDLING_CHECK;
		UNIX_ONLY(rts_error(VARLSTCNT(5) ERR_MEMORY, 2, gtmMallocErrorSize, gtmMallocErrorCallerid, gtmMallocErrorErrno));
		VMS_ONLY(rts_error(VARLSTCNT(4) ERR_VMSMEMORY, 2, gtmMallocErrorSize, gtmMallocErrorCallerid));
#	ifndef DEBUG
	} else
               /* If not a debug module and debugging is enabled, reroute call to
                   the debugging version.
	       */
                raise_gtmmemory_error_dbg();
#	endif
}


/* Return the maximum size that would fully utilize a storage block given the input size. If the size will not
   fit in one of the buddy list queue elems, it is returned unchanged. Otherwise, the size of the buddy list queue
   element minus the overhead will be returned as the best fit size.
*/
size_t gtm_bestfitsize(size_t size)
{
	size_t	tSize;
	int	hdrSize, sizeIndex;

#	ifndef DEBUG
	/* If we are not expanding for DEBUG, check now if DEBUG has been turned on.
	   If it has, we are in the wrong module Jack. This IF is structured so that
	   if this is the normal (optimized) case we will fall into the code and
	   handle the rerouting at the end.

	   Note: The DEBUG expansion of this code in a pro build actually has a different
	   entry point name (gtm_bestfitsize_dbg) and if malloc debugging options are
	   on in pro, we need to call that version, but since efficiency in pro trumps
	   clarity, we put the redirecting call at the bottom of the if-else block to
	   avoid disrupting the instruction pipeline.
	*/
        if (GDL_None == gtmDebugLevel)
        {
#	endif
		hdrSize = OFFSETOF(storElem, userStorage);		/* Size of storElem header */
		tSize = size + hdrSize DEBUG_ONLY(+ SIZEOF(markerChar));
		if (MAXTWO >= tSize)
		{	/* Allocation would fit in a buddy list queue */
			sizeIndex = GetSizeIndex(tSize);
			tSize = TwoTable[sizeIndex];
			return (tSize - hdrSize DEBUG_ONLY(- SIZEOF(markerChar)));
		}
		return size;
#	ifndef DEBUG
	}
	/* If not a debug module and debugging is enabled, reroute call to
	   the debugging version.
	*/
	return gtm_bestfitsize_dbg(size);
#	endif
}


/* Note that the DEBUG define takes on an additional meaning in this module. Not only are routines defined within
   intended for DEBUG builds but they are also only generated ONCE rather than twice like most of the routines are
   in this module.
*/
#ifdef DEBUG
/* Backfill the requested area with marker text. We do this by doing single byte
   stores up to the point where we can do aligned stores of the native register
   length. Then fill the area as much as possible and finish up potentially with
   a few single byte unaligned bytes at the end.
*/
void backfill(unsigned char *ptr, gtm_msize_t len)
{
	unsigned char	*c;
	ChunkType	*chunkPtr;
	gtm_msize_t	unalgnLen, chunkCnt;

	if (0 != len)
	{
		/* Process unaligned portion first */
		unalgnLen = (gtm_msize_t)ptr & AddrMask;	/* Past an alignment point */
		if (unalgnLen)
		{
			unalgnLen = ChunkSize - unalgnLen;	/* How far to go to get to alignment point */
			unalgnLen = MIN(unalgnLen, len);	/* Make sure not going too far */
			c = backfillMarkC;
			len -= unalgnLen;
			do
			{
				*ptr++ = *c++;
				--unalgnLen;
			} while(unalgnLen);
		}

		/* Now, do aligned portion */
		assert(0 == ((gtm_msize_t)ptr & AddrMask));	/* Verify aligned */
		chunkCnt = len / ChunkSize;
		chunkPtr = (ChunkType *)ptr;
		while (chunkCnt--)
		{
			*chunkPtr++ = ChunkValue;
			len -= SIZEOF(ChunkType);
		}

		/* Do remaining unaligned portion if any */
		if (len)
		{
			ptr = (unsigned char *)chunkPtr;
			c = backfillMarkC;
			do
			{
				*ptr++ = *c++;
				--len;
			} while(len);
		}
	}
}

/*  ** still under ifdef DEBUG ** */
/* Check the given backfilled area that it was filled in exactly as
   the above backfill routine would have filled it in. Again, do any
   unaligned single chars first, then aligned native length areas,
   then any stragler unaligned chars */
boolean_t backfillChk(unsigned char *ptr, gtm_msize_t len)
{
	unsigned char	*c;
	ChunkType	*chunkPtr;
	gtm_msize_t	unalgnLen, chunkCnt;

	if (0 != len)
	{
		/* Process unaligned portion first */
		unalgnLen = (gtm_msize_t)ptr & AddrMask;	/* Past an alignment point */
		if (unalgnLen)
		{
			unalgnLen = ChunkSize - unalgnLen;	/* How far to go to get to alignment point */
			unalgnLen = MIN(unalgnLen, len);	/* Make sure not going too far */
			c = backfillMarkC;
			len -= unalgnLen;
			do
			{
				if (*ptr++ == *c++)
					--unalgnLen;
				else
					return FALSE;
			} while(unalgnLen);
		}

		/* Now, do aligned portion */
		assert(0 == ((gtm_msize_t)ptr & AddrMask));	/* Verify aligned */
		chunkCnt = len / ChunkSize;
		chunkPtr = (ChunkType *)ptr;
		while (chunkCnt--)
		{
			if (*chunkPtr++ == ChunkValue)
				len -= SIZEOF(ChunkType);
			else
				return FALSE;
		}

		/* Do remaining unaligned portion if any */
		if (len)
		{
			ptr = (unsigned char *)chunkPtr;
			c = backfillMarkC;
			do
			{
				if (*ptr++ == *c++)
					--len;
				else
					return FALSE;
			} while(len);
		}
	}
	return TRUE;
}


/*  ** still under ifdef DEBUG ** */
/* Routine to run the free storage chains to verify that everything is in the correct place */
void verifyFreeStorage(void)
{
	storElem	*eHdr, *uStor;
	uint4		i;
	int		hdrSize;

	hdrSize = OFFSETOF(storElem, userStorage);
	/* Looping for each free queue */
	for (eHdr = &freeStorElemQs[0], i = 0; i <= MAXINDEX; ++i, ++eHdr)
	{
		for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
		{
			assert(((MAXINDEX + 1) >= i));							/* Verify loop limits */
			assert(((i == uStor->queueIndex) && (MAXINDEX <= MAXINDEX))
			       || (((MAXINDEX + 1) == i) && (REAL_MALLOC == uStor->queueIndex)));	/* Verify queue index */
			assert(0 == ((unsigned long)uStor & (TwoTable[i] - 1)));			/* Verify alignment */
			assert(Free == uStor->state);							/* Verify state */
			assert(0 == memcmp(uStor->headMarker, markerChar, SIZEOF(uStor->headMarker)));	/* Vfy metadata marker */
			assert(MAXINDEX != i || extent_used > uStor->extHdrOffset);
			if (GDL_SmChkFreeBackfill & gtmDebugLevel)
			{	/* Use backfill check method for verifying freed storage is untouched */
				assert(backfillChk((unsigned char *)uStor + hdrSize, TwoTable[i] - hdrSize));
			}
		}
	}
}


/*  ** still under ifdef DEBUG ** */
/* Routine to run the allocated chains to verify that the markers are all still in place */
void verifyAllocatedStorage(void)
{
	storElem	*eHdr, *uStor;
	unsigned char	*trailerMarker;
	uint4		i;
	int		hdrSize;

	hdrSize = OFFSETOF(storElem, userStorage);
	/* Looping for MAXINDEX+1 will check the real-malloc'd chains too */
	for (eHdr = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++eHdr)
	{
		for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
		{
			assert(((MAXINDEX + 1) >= i));						/* Verify loop not going nutz */
			assert(((i == uStor->queueIndex) && (MAXINDEX <= MAXINDEX))
			       || (((MAXINDEX + 1) == i) && (REAL_MALLOC == uStor->queueIndex)));	/* Verify queue index */
			if (i != MAXINDEX + 1)							/* If not verifying real mallocs,*/
				assert(0 == ((unsigned long)uStor & (TwoTable[i] - 1)));	/* .. verify alignment */
			assert(Allocated == uStor->state);					/* Verify state */
			assert(0 == memcmp(uStor->headMarker, markerChar, SIZEOF(uStor->headMarker)));	/* Vfy metadata markers */
			trailerMarker = (unsigned char *)&uStor->userStorage.userStart+uStor->allocLen;/* Where  trailer was put */
			assert(0 == memcmp(trailerMarker, markerChar, SIZEOF(markerChar)));
			assert(MAXINDEX != i || extent_used > uStor->extHdrOffset);
			if (GDL_SmChkAllocBackfill & gtmDebugLevel)
			{	/* Use backfill check method for after-allocation metadata */
				assert(backfillChk(trailerMarker + SIZEOF(markerChar),
					(uStor->realLen - uStor->allocLen - hdrSize - SIZEOF(markerChar))));
			}
		}
	}
}


/*  ** still under ifdef DEBUG ** */
/* Routine to print the end-of-process info -- either allocation statistics or malloc trace dump.
   Note that the use of FPRINTF here instead of util_out_print is historical. The output was at one
   time going to stdout and util_out_print goes to stderr. If necessary or desired, these could easily
   be changed to use util_out_print instead of FPRINTF
*/
void printMallocInfo(void)
{
	int 		i, j;

	if (GDL_SmStats & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc small storage performance:\n");
		FPRINTF(stderr,
			"Total mallocs: %d, total frees: %d, total extents: %d, total rmalloc bytes: %ld,"
			" max rmalloc bytes: %ld\n",
			totalMallocs, totalFrees, totalExtents, totalRmalloc, rmallocMax);
		FPRINTF(stderr,
			"Total (currently) allocated (includes overhead): %ld, Total (currently) used (no overhead): %ld\n",
			totalAlloc, totalUsed);
		FPRINTF(stderr,
			"Maximum extents: %d, Current extents: %d, Released extents: %d\n", maxExtents, curExtents,
			(totalExtents - curExtents));
		GMR_ONLY(FPRINTF(stderr,"Total reentrant mallocs: %d, total deferred frees: %d\n", reentMallocs, deferFreePending);)
		FPRINTF(stderr,"\nQueueSize   Mallocs     Frees    Splits  Combines    CurCnt    MaxCnt    CurCnt    MaxCnt\n");
		FPRINTF(stderr,  "                                                      Free      Free      Alloc     Alloc\n");
		FPRINTF(stderr,  "-----------------------------------------------------------------------------------------\n");
		{
			for (i = 0; i <= MAXINDEX + 1; ++i)
			{
				FPRINTF(stderr,
					"%9d %9d %9d %9d %9d %9d %9d %9d %9d\n", TwoTable[i], mallocCnt[i], freeCnt[i],
					elemSplits[i], elemCombines[i], freeElemCnt[i], freeElemMax[i],
					allocElemCnt[i], allocElemMax[i]);
			}
		}
	}
	if (GDL_SmDumpTrace & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc Storage Traceback:   gtm_malloc() addr: 0x"gmaAdr"\n", &gtm_malloc);
		FPRINTF(stderr,"TransNumber "gmaFill" AllocAddr        Size "gmaFill" CallerAddr\n");
		FPRINTF(stderr,"------------------------------------------------"gmaLine gmaLine"\n");
		for (i = 0,j = smLastMallocIndex; i < MAXSMTRACE; ++i,--j)/* Loop through entire table, start with last elem used */
		{
			if (0 > j)					   /* Wrap as necessary */
				j = MAXSMTRACE - 1;
			if (0 != smMallocs[j].smTn)
				FPRINTF(stderr,"%9d    0x"gmaAdr"  %10d   0x"gmaAdr"\n",
					smMallocs[j].smTn, smMallocs[j].smAddr,	smMallocs[j].smSize, smMallocs[j].smCaller);
		}
		FPRINTF(stderr,"\n\nFree Storage Traceback:\n");
		FPRINTF(stderr,"TransNumber  "gmaFill" FreeAddr        Size "gmaFill" CallerAddr\n");
		FPRINTF(stderr,"------------------------------------------------"gmaLine gmaLine"\n");
		for (i = 0, j = smLastFreeIndex; i < MAXSMTRACE; ++i, --j)/* Loop through entire table, start with last elem used */
		{
			if (0 > j)					  /* Wrap as necessary */
				j = MAXSMTRACE - 1;
			if (0 != smFrees[j].smTn)
				FPRINTF(stderr,"%9d    0x"gmaAdr"  %10d   0x"gmaAdr"\n",
					smFrees[j].smTn, smFrees[j].smAddr, smFrees[j].smSize, smFrees[j].smCaller);
		}
		FPRINTF(stderr,"\n");
		fflush(stderr);
	}
	printMallocDump();
}


/* Routine to print storage dump. This is called as part of print_malloc_info but is also potentially separately called from
   op_view so is a separate routine.
*/
void printMallocDump(void)
{
	storElem	*eHdr, *uStor;
	int		i;

	if (GDL_SmDump & gtmDebugLevel)
	{
		FPRINTF(stderr,"\nMalloc Storage Dump:   gtm_malloc() addr: 0x"gmaAdr"\n", &gtm_malloc);
		FPRINTF(stderr,gmaFill"Malloc Addr  "gmaFill"   Alloc From     Malloc Size   Trans Number\n");
		FPRINTF(stderr," ----------------------------------------------------------"gmaLine gmaLine"\n");
		/* Looping for each allocated queue */
		for (eHdr = &allocStorElemQs[0], i = 0; i <= (MAXINDEX + 1); ++i, ++eHdr)
		{
			for (uStor = STE_FP(eHdr); uStor->queueIndex != QUEUE_ANCHOR; uStor = STE_FP(uStor))
			{
				FPRINTF(stderr, "  0x"gmaAdr"      0x"gmaAdr"      %10d     %10d\n",
					&uStor->userStorage.userStart, uStor->allocatedBy, uStor->allocLen, uStor->smTn);
			}
		}
		fflush(stderr);
	}
}
#endif
