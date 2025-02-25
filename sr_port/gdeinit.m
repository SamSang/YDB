;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;								;
; Copyright (c) 2001-2020 Fidelity National Information		;
; Services, Inc. and/or its subsidiaries. All rights reserved.	;
;								;
; Copyright (c) 2017-2022 YottaDB LLC and/or its subsidiaries.	;
; All rights reserved.						;
;								;
; Copyright (c) 2017-2018 Stephen L Johnson.			;
; All rights reserved.						;
;								;
;	This source code contains the intellectual property	;
;	of its copyright holder(s), and is made available	;
;	under a license.  If you do not know the terms of	;
;	the license, please stop and do not read further.	;
;								;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
gdeinit: ;set up local variables and arrays
GDEINIT
	view "BADCHAR"
	s renpref=""
	s log=0,logfile="GDELOG.LOG",BOL=""
	s ONE=$c(1),TRUE=ONE,ZERO=$c(0),FALSE=ZERO,TAB=$c(9)
	s endian("x86","SCO")=FALSE,endian("x86","UWIN")=FALSE,endian("x86","Linux")=FALSE,endian("x86","CYGWIN")=FALSE
	s endian("x86_64","Linux")=FALSE
	s glo("SCO")=384,glo("UWIN")=1024,glo("Linux")=1024,glo("CYGWIN")=1024
	s endian("IA64","HP-UX")=TRUE,glo("HP-UX")=1024
	s endian("IA64","Linux")=FALSE,glo("Linux")=1024
	s endian("MIPS","A25")=TRUE,glo("A25")=1024
	s endian("RS6000","AIX")=TRUE,glo("AIX")=1024
	s endian("S390","OS390")=TRUE,endian("S390X","Linux")=TRUE,glo("OS390")=1024
	s endian("armv6l","Linux")=FALSE,glo("Linux")=1024
	s endian("armv7l","Linux")=FALSE,glo("Linux")=1024
	s endian("aarch64","Linux")=FALSE,glo("Linux")=1024
	s HEX(0)=1
	s gtm64=$p($zver," ",4)
	i "/RS6000/x86_64/x86/S390/S390X/aarch64"[("/"_gtm64) s encsupportedplat=TRUE,gtm64=$s("x86"=gtm64:FALSE,1:TRUE)
	e  s (encsupportedplat,gtm64)=FALSE
	i (gtm64=TRUE) d
	. f x=1:1:16 s HEX(x)=HEX(x-1)*16 i x#2=0 s TWO(x*4)=HEX(x)
	e  f x=1:1:8 s HEX(x)=HEX(x-1)*16 i x#2=0 s TWO(x*4)=HEX(x)
	f i=25:1:30 s TWO(i)=TWO(i-1)*2
	s TWO(31)=TWO(32)*.5
	s lower="abcdefghijklmnopqrstuvwxyz",upper="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	s endian=endian($p($zver," ",4),$p($zver," ",3))
	s ver=$p($zver," ",3)
	s defglo=glo(ver)
	s comline=$zcmdline
	s typevalue("STR2NUM","TNULLSUB","A")=1
	s typevalue("STR2NUM","TNULLSUB","AL")=1
	s typevalue("STR2NUM","TNULLSUB","ALW")=1
	s typevalue("STR2NUM","TNULLSUB","ALWA")=1
	s typevalue("STR2NUM","TNULLSUB","ALWAY")=1
	s typevalue("STR2NUM","TNULLSUB","ALWAYS")=1
	s typevalue("NUM2STR","TNULLSUB",1)="ALWAYS"
	s typevalue("STR2NUM","TNULLSUB","E")=2
	s typevalue("STR2NUM","TNULLSUB","EX")=2
	s typevalue("STR2NUM","TNULLSUB","EXI")=2
	s typevalue("STR2NUM","TNULLSUB","EXIS")=2
	s typevalue("STR2NUM","TNULLSUB","EXIST")=2
	s typevalue("STR2NUM","TNULLSUB","EXISTI")=2
	s typevalue("STR2NUM","TNULLSUB","EXISTIN")=2
	s typevalue("STR2NUM","TNULLSUB","EXISTING")=2
	s typevalue("NUM2STR","TNULLSUB",2)="EXISTING"
	s typevalue("STR2NUM","TNULLSUB","N")=0
	s typevalue("STR2NUM","TNULLSUB","NE")=0
	s typevalue("STR2NUM","TNULLSUB","NEV")=0
	s typevalue("STR2NUM","TNULLSUB","NEVE")=0
	s typevalue("STR2NUM","TNULLSUB","NEVER")=0
	s typevalue("NUM2STR","TNULLSUB",0)="NEVER"
	s typevalue("STR2NUM","TNULLSUB","F")=0
	s typevalue("STR2NUM","TNULLSUB","FA")=0
	s typevalue("STR2NUM","TNULLSUB","FAL")=0
	s typevalue("STR2NUM","TNULLSUB","FALS")=0
	s typevalue("STR2NUM","TNULLSUB","FALSE")=0
	s typevalue("STR2NUM","TNULLSUB","T")=1
	s typevalue("STR2NUM","TNULLSUB","TR")=1
	s typevalue("STR2NUM","TNULLSUB","TRU")=1
	s typevalue("STR2NUM","TNULLSUB","TRUE")=1
	s typevalue("STR2NUM","TACCMETH","BG")=0
	s typevalue("STR2NUM","TACCMETH","MM")=1
	s nommbi=1              ; this is used in GDEVERIF and should be removed along with the code when support is added
	d UNIX
	d syntabi
;
	; Explanation of some of the SIZEOF local variable subscripts.
	;	SIZEOF("gd_segment")		; --> size of the "gd_segment" structure (defined in gdsfhead.h)
	;
	i (gtm64=FALSE) d
	. s SIZEOF("am_offset")=336		; --> offset of "acc_meth" field in the "gd_segment" structure
	. s SIZEOF("file_spec")=256		; --> maximum size (in bytes) of a file name specified in gde command line
	. s SIZEOF("gd_contents")=80		; --> size of the "gd_addr" structure (defined in gdsfhead.h)
	. s SIZEOF("gd_header")=16		; --> 16-byte header structure at offset 0 of .gld (12 byte label, 4-byte filesize)
	. s SIZEOF("gd_map")=16			; --> size of the "gd_binding" structure (defined in gdsfhead.h)
	. s SIZEOF("gd_region")=416		; --> size of the "gd_region"  structure (defined in gdsfhead.h)
	. s SIZEOF("gd_region_padding")=0	; --> padding at end of "gd_region" structure (4-bytes for 64-bit platforms)
	. s SIZEOF("gd_segment")=372		; --> size of the "gd_segment" structure (defined in gdsfhead.h)
	e  d
	. s SIZEOF("am_offset")=340		; --> offset of "acc_meth" field in the "gd_segment" structure
	. s SIZEOF("file_spec")=256		; --> maximum size (in bytes) of a file name specified in gde command line
	. s SIZEOF("gd_contents")=120		; --> size of the "gd_addr" structure (defined in gdsfhead.h)
	. s SIZEOF("gd_header")=16		; --> 16-byte header structure at offset 0 of .gld (12 byte label, 4-byte filesize)
	. s SIZEOF("gd_map")=24			; --> size of the "gd_binding" structure (defined in gdsfhead.h)
	. s SIZEOF("gd_region")=432		; --> size of the "gd_region"  structure (defined in gdsfhead.h)
	. s SIZEOF("gd_region_padding")=8	; --> padding at end of "gd_region" structure (4-bytes for 64-bit platforms)
	. s SIZEOF("gd_segment")=384		; --> size of the "gd_segment" structure (defined in gdsfhead.h)
	d gvstats
	s SIZEOF("blk_hdr")=16
	s SIZEOF("dsk_blk")=512
	s SIZEOF("gd_gblname")=40
	s SIZEOF("gd_inst_info")=SIZEOF("file_spec")	; --> size of the "gd_inst_info" structure (defined in gdsfhead.h)
	s SIZEOF("max_str")=1048576
	s SIZEOF("mident")=32
	s SIZEOF("reg_jnl_deq")=4
	s SIZEOF("rec_hdr")=4	;GTM-6941
	s MAXGVSUBS=31						; needs to be equal to (MAX_GVSUBSCRIPTS) in mdef.h at all times
	s MAXNAMLN=SIZEOF("mident")-1,MAXREGLN=32,MAXSEGLN=32	; maximum name length allowed is 31 characters
	s MAXSTRLEN=(2**20)					; needs to be equal to MAX_STRLEN in mdef.h at all times
	s (PARNAMLN,PARREGLN,PARSEGLN)=MAXNAMLN
;
; tokens are used for parsing and error reporting
	s tokens("TKIDENT")="identifier"
	s tokens("TKEOL")="end-of-line"
	s tokens("""")="TKSTRLIT",tokens("TKSTRLIT")="string literal"
	s tokens("@")="TKAT",tokens("TKAT")="at sign"
	s tokens(",")="TKCOMMA",tokens("TKCOMMA")="comma"
	s tokens("=")="TKEQUAL",tokens("TKEQUAL")="equal sign"
	s tokens("(")="TKLPAREN",tokens("TKLPAREN")="left parenthesis"
	s tokens(")")="TKRPAREN",tokens("TKRPAREN")="right parenthesis"
	s tokens("!")="TKEXCLAM",tokens("TKEXCLAM")="exclamation point"
	s tokens("-")="TKDASH",tokens("TKDASH")="dash"
	; Check gdequiet to determine if null tokens should be added. gdequiet indicates that parsing of tokens
	; isn't going to be performed as it is non-interactive
	s:'$g(gdequiet) tokens("")="TKEOL"	; for parsing purposes
; tokendelim is used for parsing; it is defined for all characters that can terminate a valid token during parsing
	i '$g(gdequiet) for c=" ",TAB,"=",",",")","" s tokendelim(c)=""
	; In the Unix shell command line, space is by default the separator for multiple qualifiers.
	; GDE handles separators similarly.
; spacedelim is used for parsing when we want to terminate token parsing only if we see a white space (a subset of "tokendelim")
; this is needed in case we are parsing say a filename and we don't want a "-" or "=" in the file name to terminate the parse.
	i '$g(gdequiet) for c=" ",TAB,"" s spacedelim(c)=""
	k c
; maximums and mimimums
; gblname
	s mingnam("COLLATION")=0
	s maxgnam("COLLATION")=255
; instance
	s maxinst("FILE")=SIZEOF("file_spec")
; region
	s minreg("ALIGNSIZE")=4096,maxreg("ALIGNSIZE")=4194304			; geq RECORD_SIZE
	s minreg("ALLOCATION")=2048,maxreg("ALLOCATION")=8388607
	s minreg("AUTODB")=0,maxreg("AUTODB")=1
	s minreg("AUTOSWITCHLIMIT")=16384,maxreg("AUTOSWITCHLIMIT")=8388607
	s minreg("BEFORE_IMAGE")=0,maxreg("BEFORE_IMAGE")=1
	s minreg("BUFFER_SIZE")=2307,maxreg("BUFFER_SIZE")=1048576
	s minreg("COLLATION_DEFAULT")=0,maxreg("COLLATION_DEFAULT")=255
	s minreg("EPOCH_INTERVAL")=1,maxreg("EPOCH_INTERVAL")=32767
	s minreg("EPOCHTAPER")=0,maxreg("EPOCHTAPER")=1
	s minreg("EXTENSION")=0,maxreg("EXTENSION")=1073741823
	s minreg("INST_FREEZE_ON_ERROR")=0,maxreg("INST_FREEZE_ON_ERROR")=1
	s minreg("JOURNAL")=0,maxreg("JOURNAL")=1
	s minreg("KEY_SIZE")=3
	s maxreg("KEY_SIZE")=1019	; = max value of KEY->end that returns TRUE for CAN_APPEND_HIDDEN_SUBS(KEY) in gdsfhead.h
	s minreg("LOCK_CRIT_SEPARATE")=0,maxreg("LOCK_CRIT_SEPARATE")=1
	s minreg("NULL_SUBSCRIPTS")=0,maxreg("NULL_SUBSCRIPTS")=2
	s minreg("QDBRUNDOWN")=0,maxreg("QDBRUNDOWN")=1
	s minreg("RECORD_SIZE")=0,maxreg("RECORD_SIZE")=SIZEOF("max_str")
	s minreg("STATS")=0,maxreg("STATS")=1
	s minreg("STDNULLCOLL")=0,maxreg("STDNULLCOLL")=1
	s minreg("SYNC_IO")=0,maxreg("SYNC_IO")=1
	s minreg("YIELD_LIMIT")=0,maxreg("YIELD_LIMIT")=2048
	; Store defaults in "dflreg". "tmpreg" serves this purpose for the most part but it is user-editable (using the
	; TEMPLATE command) but for things where we want to use the GT.M-default value (not user-edited default value)
	; we use "dflreg". Currently the only parameter we care about is KEY_SIZE which is used for setting statsdb characteristics
	s dflreg("KEY_SIZE")=64	; should be maintained in parallel with STATSDB_KEY_SIZE in gdsfhead.h
; segments
	; First define segment characteristics (minimum and maximum) that are identical to BG and MM access methods
	; Then define overrides specific to BG and MM
	n minsegcommon,maxsegcommon
	s minsegcommon("ALLOCATION")=10,maxsegcommon("ALLOCATION")=TWO(30)-TWO(25) ; supports 992M blocks
	s minsegcommon("BLOCK_SIZE")=SIZEOF("dsk_blk"),maxsegcommon("BLOCK_SIZE")=HEX(4)-SIZEOF("dsk_blk")
	s minsegcommon("EXTENSION_COUNT")=0,maxsegcommon("EXTENSION_COUNT")=HEX(5)-1
	s minsegcommon("LOCK_SPACE")=10,maxsegcommon("LOCK_SPACE")=262144
	s minsegcommon("MUTEX_SLOTS")=64	; keep this in sync with MIN_CRIT_ENTRY in gdsbt.h
	s maxsegcommon("MUTEX_SLOTS")=32768	; keep this in sync with MAX_CRIT_ENTRY in gdsbt.h
	s minsegcommon("RESERVED_BYTES")=0,maxsegcommon("RESERVED_BYTES")=HEX(4)-SIZEOF("dsk_blk")
	s minsegcommon("FULLBLKWRT")=0,maxsegcommon("FULLBLKWRT")=2
; bg
	m minseg("BG")=minsegcommon,maxseg("BG")=maxsegcommon	; copy over all common stuff into BG access method first
	; now add BG specific overrides; GTM64_WC_MAX_BUFFS defined in gdsbt.h
	s minseg("BG","GLOBAL_BUFFER_COUNT")=64,maxseg("BG","GLOBAL_BUFFER_COUNT")=$select(gtm64=TRUE:2097151,1:65536)
; mm
	m minseg("MM")=minsegcommon,maxseg("MM")=maxsegcommon	; copy over all common stuff into MM access method first
	; now add MM specific overrides
	s minseg("MM","DEFER")=0,maxseg("MM","DEFER")=86400
	; Now define default segment characteristics
	; This is particularly needed for fields that are only available in more recent .gld formats
	; So if we are reading an older format .gld file, we can use these as the default values.
	s defseg("ALLOCATION")=100
	s defseg("ASYNCIO")=0
	s defseg("FULLBLKWRT")=0
	s defseg("BLOCK_SIZE")=4096
	s defseg("BUCKET_SIZE")=""
	s defseg("DEFER_ALLOCATE")=1
	s defseg("ENCRYPTION_FLAG")=0
	s defseg("EXTENSION_COUNT")=100
	s defseg("FILE_TYPE")="DYNAMIC"
	s defseg("LOCK_SPACE")=220	; If this changes, need to also change DEF_LOCK_SIZE macro in mlkdef.h
	s defseg("MUTEX_SLOTS")=1024 ; keep this in sync with DEFAULT_NUM_CRIT_ENTRY in gdsbt.h
	s defseg("RESERVED_BYTES")=0
	s defseg("WINDOW_SIZE")=""
	q

;-----------------------------------------------------------------------------------------------------------------------------------

; gde command language syntax table
syntabi:
	s syntab("ADD","GBLNAME")=""
	s syntab("ADD","GBLNAME","COLLATION")="REQUIRED"
	s syntab("ADD","GBLNAME","COLLATION","TYPE")="TNUMBER"
	s syntab("ADD","NAME")=""
	s syntab("ADD","NAME","REGION")="REQUIRED"
	s syntab("ADD","NAME","REGION","TYPE")="TREGION"
	s syntab("ADD","REGION")=""
	s syntab("ADD","REGION","AUTODB")="NEGATABLE"
	s syntab("ADD","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("ADD","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("ADD","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("ADD","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("ADD","REGION","EPOCHTAPER")="NEGATABLE"
	s syntab("ADD","REGION","INST_FREEZE_ON_ERROR")="NEGATABLE"
	s syntab("ADD","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("ADD","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","AUTOSWITCHLIMIT")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","AUTOSWITCHLIMIT","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("ADD","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("ADD","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("ADD","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	s syntab("ADD","REGION","KEY_SIZE")="REQUIRED"
	s syntab("ADD","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","REGION","LOCK_CRIT_SEPARATE")="NEGATABLE"
	s syntab("ADD","REGION","NULL_SUBSCRIPTS")="NEGATABLE,REQUIRED"
	s syntab("ADD","REGION","NULL_SUBSCRIPTS","TYPE")="TNULLSUB"
	s syntab("ADD","REGION","QDBRUNDOWN")="NEGATABLE"
	s syntab("ADD","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("ADD","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","REGION","STATS")="NEGATABLE"
	s syntab("ADD","REGION","STDNULLCOLL")="NEGATABLE"
	s syntab("ADD","SEGMENT")=""
	s syntab("ADD","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("ADD","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("ADD","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("ADD","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","ASYNCIO")="NEGATABLE"
	s syntab("ADD","SEGMENT","FULLBLKWRT")="REQUIRED"
	s syntab("ADD","SEGMENT","FULLBLKWRT","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","DEFER")="NEGATABLE"
	s syntab("ADD","SEGMENT","DEFER_ALLOCATE")="NEGATABLE"
 	s syntab("ADD","SEGMENT","ENCRYPTION_FLAG")="NEGATABLE"
	s syntab("ADD","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("ADD","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("ADD","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("ADD","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("ADD","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("ADD","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","MUTEX_SLOTS")="REQUIRED"
	s syntab("ADD","SEGMENT","MUTEX_SLOTS","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("ADD","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("ADD","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("ADD","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","GBLNAME")=""
	s syntab("CHANGE","GBLNAME","COLLATION")="REQUIRED"
	s syntab("CHANGE","GBLNAME","COLLATION","TYPE")="TNUMBER"
	s syntab("CHANGE","INSTANCE")=""
	s syntab("CHANGE","INSTANCE","FILE_NAME")="REQUIRED"
	s syntab("CHANGE","INSTANCE","FILE_NAME","TYPE")="TFSPEC"
	s syntab("CHANGE","NAME")=""
	s syntab("CHANGE","NAME","REGION")="REQUIRED"
	s syntab("CHANGE","NAME","REGION","TYPE")="TREGION"
	s syntab("CHANGE","REGION")=""
	s syntab("CHANGE","REGION","AUTODB")="NEGATABLE"
	s syntab("CHANGE","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("CHANGE","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("CHANGE","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("CHANGE","REGION","EPOCHTAPER")="NEGATABLE"
	s syntab("CHANGE","REGION","INST_FREEZE_ON_ERROR")="NEGATABLE"
	s syntab("CHANGE","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("CHANGE","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","AUTOSWITCHLIMIT")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","AUTOSWITCHLIMIT","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("CHANGE","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("CHANGE","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	s syntab("CHANGE","REGION","KEY_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","LOCK_CRIT_SEPARATE")="NEGATABLE"
	s syntab("CHANGE","REGION","NULL_SUBSCRIPTS")="NEGATABLE,REQUIRED"
	s syntab("CHANGE","REGION","NULL_SUBSCRIPTS","TYPE")="TNULLSUB"
	s syntab("CHANGE","REGION","QDBRUNDOWN")="NEGATABLE"
	s syntab("CHANGE","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("CHANGE","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","REGION","STATS")="NEGATABLE"
	s syntab("CHANGE","REGION","STDNULLCOLL")="NEGATABLE"
	s syntab("CHANGE","SEGMENT")=""
	s syntab("CHANGE","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("CHANGE","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("CHANGE","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("CHANGE","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","ASYNCIO")="NEGATABLE"
	s syntab("CHANGE","SEGMENT","FULLBLKWRT")="REQUIRED"
	s syntab("CHANGE","SEGMENT","FULLBLKWRT","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","DEFER")="NEGATABLE"
	s syntab("CHANGE","SEGMENT","DEFER_ALLOCATE")="NEGATABLE"
	s syntab("CHANGE","SEGMENT","ENCRYPTION_FLAG")="NEGATABLE"
	s syntab("CHANGE","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("CHANGE","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("CHANGE","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("CHANGE","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("CHANGE","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","MUTEX_SLOTS")="REQUIRED"
	s syntab("CHANGE","SEGMENT","MUTEX_SLOTS","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("CHANGE","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("CHANGE","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("CHANGE","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION")=""
	s syntab("TEMPLATE","REGION","AUTODB")="NEGATABLE"
	s syntab("TEMPLATE","REGION","COLLATION_DEFAULT")="REQUIRED"
	s syntab("TEMPLATE","REGION","COLLATION_DEFAULT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","DYNAMIC_SEGMENT")="REQUIRED"
	s syntab("TEMPLATE","REGION","DYNAMIC_SEGMENT","TYPE")="TSEGMENT"
	s syntab("TEMPLATE","REGION","EPOCHTAPER")="NEGATABLE"
	s syntab("TEMPLATE","REGION","INST_FREEZE_ON_ERROR")="NEGATABLE"
	s syntab("TEMPLATE","REGION","JOURNAL")="NEGATABLE,REQUIRED,LIST"
	s syntab("TEMPLATE","REGION","JOURNAL","ALLOCATION")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","ALLOCATION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","AUTOSWITCHLIMIT")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","AUTOSWITCHLIMIT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","BEFORE_IMAGE")="NEGATABLE"
	s syntab("TEMPLATE","REGION","JOURNAL","BUFFER_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","BUFFER_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","EXTENSION")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","EXTENSION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","JOURNAL","FILE_NAME")="REQUIRED"
	s syntab("TEMPLATE","REGION","JOURNAL","FILE_NAME","TYPE")="TFSPEC"
	s syntab("TEMPLATE","REGION","KEY_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","KEY_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","LOCK_CRIT_SEPARATE")="NEGATABLE"
	s syntab("TEMPLATE","REGION","NULL_SUBSCRIPTS")="NEGATABLE,REQUIRED"
	s syntab("TEMPLATE","REGION","NULL_SUBSCRIPTS","TYPE")="TNULLSUB"
	s syntab("TEMPLATE","REGION","QDBRUNDOWN")="NEGATABLE"
	s syntab("TEMPLATE","REGION","RECORD_SIZE")="REQUIRED"
	s syntab("TEMPLATE","REGION","RECORD_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","REGION","STATS")="NEGATABLE"
	s syntab("TEMPLATE","REGION","STDNULLCOLL")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT")=""
	s syntab("TEMPLATE","SEGMENT","ACCESS_METHOD")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","ACCESS_METHOD","TYPE")="TACCMETH"
	s syntab("TEMPLATE","SEGMENT","ALLOCATION")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","ALLOCATION","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","ASYNCIO")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT","FULLBLKWRT")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","FULLBLKWRT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","BLOCK_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","BLOCK_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","BUCKET_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","BUCKET_SIZE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","DEFER")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT","DEFER_ALLOCATE")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT","ENCRYPTION_FLAG")="NEGATABLE"
	s syntab("TEMPLATE","SEGMENT","EXTENSION_COUNT")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","EXTENSION_COUNT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","FILE_NAME")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","FILE_NAME","TYPE")="TFSPEC"
	s syntab("TEMPLATE","SEGMENT","GLOBAL_BUFFER_COUNT")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","GLOBAL_BUFFER_COUNT","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","LOCK_SPACE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","LOCK_SPACE","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","MUTEX_SLOTS")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","MUTEX_SLOTS","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","RESERVED_BYTES")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","RESERVED_BYTES","TYPE")="TNUMBER"
	s syntab("TEMPLATE","SEGMENT","WINDOW_SIZE")="REQUIRED"
	s syntab("TEMPLATE","SEGMENT","WINDOW_SIZE","TYPE")="TNUMBER"
	s syntab("DELETE","GBLNAME")=""
	s syntab("DELETE","NAME")=""
	s syntab("DELETE","REGION")=""
	s syntab("DELETE","SEGMENT")=""
	s syntab("EXIT")=""
	s syntab("HELP")=""
	s syntab("LOCKS","REGION")="REQUIRED"
	s syntab("LOCKS","REGION","TYPE")="TREGION"
	s syntab("LOG","OFF")=""
	s syntab("LOG","ON")="OPTIONAL"
	s syntab("LOG","ON","TYPE")="TFSPEC"
	s syntab("QUIT")=""
	s syntab("RENAME","GBLNAME")=""
	s syntab("RENAME","NAME")=""
	s syntab("RENAME","REGION")=""
	s syntab("RENAME","SEGMENT")=""
	s syntab("SETGD","FILE")="REQUIRED"
	s syntab("SETGD","FILE","TYPE")="TFSPEC"
	s syntab("SETGD","QUIT")=""
	s syntab("SHOW")=""
	s syntab("SHOW","ALL")=""
	s syntab("SHOW","COMMANDS")=""
	s syntab("SHOW","COMMANDS","FILE")="OPTIONAL"
	s syntab("SHOW","COMMANDS","FILE","TYPE")="TFSPEC"
	s syntab("SHOW","GBLNAME")=""
	s syntab("SHOW","INSTANCE")=""
	s syntab("SHOW","MAP")=""
	s syntab("SHOW","MAP","REGION")="REQUIRED"
	s syntab("SHOW","MAP","REGION","TYPE")="TREGION"
	s syntab("SHOW","NAME")=""
	s syntab("SHOW","REGION")=""
	s syntab("SHOW","SEGMENT")=""
	s syntab("SHOW","TEMPLATE")=""
	s syntab("SPAWN")=""
	s syntab("VERIFY","ALL")=""
	s syntab("VERIFY","GBLNAME")=""
	s syntab("VERIFY","MAP")=""
	s syntab("VERIFY","NAME")=""
	s syntab("VERIFY","REGION")=""
	s syntab("VERIFY","SEGMENT")=""
	s syntab("VERIFY","TEMPLATE")=""
	q
UNIX:
	s hdrlab="GTCGBDUNX015"         ; must be concurrently maintained in gbldirnam.h!!!
	i (gtm64=TRUE) s hdrlab="GTCGBDUNX115" ; the high order digit is a 64-bit flag
	s tfile=$view("GBLDIRXLATE",$ztrnlnm("ydb_gbldir"))
	s accmeth="\BG\MM"
	s helpfile="$ydb_dist/gdehelp.gld"
	s defdb="mumps.dat"
	s defgld="mumps.gld",defgldext="*.gld"
	s defreg="DEFAULT"
	s defseg="DEFAULT"
	s dbfilpar="1E"
	s filexfm="filespec"
	s sep="TKDASH"
	q
gvstats		; Obtain the gvstats size from the output of ZSHOW "G"
	n zshow
	zsh "G":zshow
	s SIZEOF("gvstats")=($length(zshow("G",0),",")-2)*8	; --> Field count, minus GLD/REG fields, times 8 bytes per stat.
	q
