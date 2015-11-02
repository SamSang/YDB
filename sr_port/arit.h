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

/*
	arit.h : defines constants for arithmetic operations.
		gdsroot.h has a definition for the MAX_NUM_SUBSC_LEN that
		depends on the number of significant digits available.  If
		it changes from 18, the define must be updated
*/
#define NUM_DEC_DG_1L		   9			/* Number of decimal digits in a int4 		*/
#define NUM_DEC_DG_2L		  18			/* Number of decimal digits in two longs 	*/

#define EXP_IDX_BIAL		  (9+MV_XBIAS)		/* Max. biased exponent index within a int4 	*/
#define EXP_IDX_BIAQ		 (18+MV_XBIAS)		/* Max. biased exponent index within two longs 	*/
#define EXPLO			(-42+MV_XBIAS)		/* Min. biased exponent  			*/
#define EXPHI			 (48+MV_XBIAS)		/* Max. biased exponent  			*/
/* Note: The above values for EXPLO and EXPHI seem to set up a range of 20 thru 110 which still leaves room for expansion
 * in the 7-bit exponent whose full range is 0 thru 127. Not clear what the reason for leaving some gap on either side is.
 * One thought is maybe the gap of 19 in the lower side & 17 in the upper side has got something to do with the max
 * precision we suport (18 decimal digits). But it is still not clear to me why. --- nars -- 2011/01/14.
 */
#define EXP_INT_OVERF		  (7+MV_XBIAS)		/* Upper bound on MV_INT numbers 		*/
#define EXP_INT_UNDERF		 (-3+MV_XBIAS)		/* Lower bound on MV_INT numbers (includes
							 *   integers & fractions upto 3 decimal places */
#define MANT_LO			100000000		/* mantissa.1 >= MANT_LO 			*/
#define MANT_HI			1000000000		/* mantissa.1 <  MANT_HI , mantissa.0 < MANT_HI */
#define INT_HI			1000000			/* -INT_HI < mv_int < INT_HI 			*/
