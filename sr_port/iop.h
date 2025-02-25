/****************************************************************
 *								*
 * Copyright (c) 2001-2019 Fidelity National Information	*
 * Services, Inc. and/or its subsidiaries. All rights reserved.	*
 *								*
 * Copyright (c) 2022 YottaDB LLC and/or its subsidiaries.	*
 * All rights reserved.						*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* iop.h, to be included when io parameters need enumeration */
/* last entry = n_iops is the size of the table */
/* Must add entries at the end in order to avoid a recompile */
/* structure:
enum_value,   enum_mnemonic,      data_element_size,      legal_operations,                    source_value_type */
IOP_DESC(0,   iop_eol,            0,                      0,                                   0),
IOP_DESC(1,   iop_canctlo,        0,                      IOP_USE_OK,                          0),
IOP_DESC(2,   iop_cenable,        0,                      IOP_USE_OK,                          0),
IOP_DESC(3,   iop_nocenable,      0,                      IOP_USE_OK,                          0),
IOP_DESC(4,   iop_clearscreen,    0,                      IOP_USE_OK,                          0),
IOP_DESC(5,   iop_convert,        0,                      IOP_USE_OK,                          0),
IOP_DESC(6,   iop_noconvert,      0,                      IOP_USE_OK,                          0),
IOP_DESC(7,   iop_downscroll,     0,                      IOP_USE_OK,                          0),
IOP_DESC(8,   iop_echo,           0,                      IOP_USE_OK,                          0),
IOP_DESC(9,   iop_noecho,         0,                      IOP_USE_OK,                          0),
IOP_DESC(10,  iop_eraseline,      0,                      IOP_USE_OK,                          0),
IOP_DESC(11,  iop_field,          SIZEOF(short),          IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(12,  iop_terminator,     SIZEOF(int4)*8,         IOP_USE_OK,                          IOP_SRC_LNGMSK),
IOP_DESC(13,  iop_upscroll,       0,                      IOP_USE_OK,                          0),
IOP_DESC(14,  iop_width,          SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(15,  iop_blocksize,      SIZEOF(int4),           IOP_OPEN_OK,                         IOP_SRC_INT),
IOP_DESC(16,  iop_ctrap,          SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_MSK),
IOP_DESC(17,  iop_x,              SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(18,  iop_y,              SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(19,  iop_escape,         0,                      IOP_USE_OK,                          0),
IOP_DESC(20,  iop_noescape,       0,                      IOP_USE_OK,                          0),
IOP_DESC(21,  iop_allocation,     SIZEOF(int4),           IOP_OPEN_OK,                         IOP_SRC_INT),
IOP_DESC(22,  iop_contiguous,     0,                      IOP_OPEN_OK,                         0),
IOP_DESC(23,  iop_delete,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(24,  iop_extension,      SIZEOF(unsigned short), IOP_OPEN_OK,                         IOP_SRC_INT),
IOP_DESC(25,  iop_newversion,     0,                      IOP_OPEN_OK,                         0),
IOP_DESC(26,  iop_nosequential,   0,                      IOP_OPEN_OK,                         0),
IOP_DESC(27,  iop_s_protection,   SIZEOF(char),           IOP_OPEN_OK|IOP_CLOSE_OK,            IOP_SRC_PRO),
IOP_DESC(28,  iop_w_protection,   SIZEOF(char),           IOP_OPEN_OK|IOP_CLOSE_OK,            IOP_SRC_PRO),
IOP_DESC(29,  iop_g_protection,   SIZEOF(char),           IOP_OPEN_OK|IOP_CLOSE_OK,            IOP_SRC_PRO),
IOP_DESC(30,  iop_o_protection,   SIZEOF(char),           IOP_OPEN_OK|IOP_CLOSE_OK,            IOP_SRC_PRO),
IOP_DESC(31,  iop_readonly,       0,                      IOP_OPEN_OK,                         0),
IOP_DESC(32,  iop_recordsize,     SIZEOF(int4),           IOP_OPEN_OK,                         IOP_SRC_INT),
IOP_DESC(33,  iop_shared,         0,                      IOP_OPEN_OK,                         0),
IOP_DESC(34,  iop_spool,          0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(35,  iop_submit,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(36,  iop_rfa,            0,                      IOP_USE_OK,                          0),
IOP_DESC(37,  iop_space,          SIZEOF(int4),           IOP_USE_OK|IOP_CLOSE_OK,             IOP_SRC_INT),
IOP_DESC(38,  iop_queue,          IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(39,  iop_rename,         IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(40,  iop_uic,            IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_CLOSE_OK,            IOP_SRC_STR),
IOP_DESC(41,  iop_wrap,           0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(42,  iop_nowrap,         0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(43,  iop_rewind,         0,                      IOP_OPEN_OK|IOP_USE_OK|IOP_CLOSE_OK, 0),
IOP_DESC(44,  iop_exception,      IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK|IOP_CLOSE_OK, IOP_SRC_STR),
IOP_DESC(45,  iop_ebcdic,         0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(46,  iop_label,          IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(47,  iop_nolabel,        0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(48,  iop_newtape,        0,                      0,                                   0),
IOP_DESC(49,  iop_mount,          0,                      IOP_OPEN_OK,                         0),
IOP_DESC(50,  iop_fixed,          0,                      IOP_OPEN_OK,                         0),
IOP_DESC(51,  iop_erasetape,      0,                      IOP_OPEN_OK|IOP_USE_OK|IOP_CLOSE_OK, 0),
IOP_DESC(52,  iop_next,           0,                      IOP_USE_OK,                          0),
IOP_DESC(53,  iop_writeof,        0,                      IOP_USE_OK|IOP_CLOSE_OK,             0),
IOP_DESC(54,  iop_noreadonly,     0,                      IOP_OPEN_OK,                         0),
IOP_DESC(55,  iop_rdcheckdata,    0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(56,  iop_nordcheckdata,  0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(57,  iop_wtcheckdata,    0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(58,  iop_nowtcheckdata,  0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(59,  iop_inhretry,       0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(60,  iop_inhextgap,      0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(61,  iop_unload,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(62,  iop_skipfile,       SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(63,  iop_writeonly,      0,                      IOP_OPEN_OK,                         0),
IOP_DESC(64,  iop_nowriteonly,    0,                      IOP_OPEN_OK,                         0),
IOP_DESC(65,  iop_wait,           0,                      IOP_USE_OK,                          0),
IOP_DESC(66,  iop_tmpmbx,         0,                      IOP_OPEN_OK,                         0),
IOP_DESC(67,  iop_prmmbx,         0,                      IOP_OPEN_OK,                         0),
IOP_DESC(68,  iop_append,         0,                      IOP_OPEN_OK,                         0),
IOP_DESC(69,  iop_nowait,         0,                      IOP_USE_OK,                          0),
IOP_DESC(70,  iop_nofixed,        0,                      IOP_OPEN_OK,                         0),
IOP_DESC(71,  iop_stream,         0,                      IOP_OPEN_OK,                         0),
IOP_DESC(72,  iop_nostream,       0,                      IOP_OPEN_OK,                         0),
IOP_DESC(73,  iop_flush,          0,                      IOP_USE_OK,                          0),
IOP_DESC(74,  iop_length,         SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(75,  iop_typeahead,      0,                      IOP_USE_OK,                          0),
IOP_DESC(76,  iop_notypeahead,    0,                      IOP_USE_OK,                          0),
IOP_DESC(77,  iop_editing,        0,                      IOP_USE_OK,                          0),
IOP_DESC(78,  iop_noediting,      0,                      IOP_USE_OK,                          0),
IOP_DESC(79,  iop_hostsync,       0,                      IOP_USE_OK,                          0),
IOP_DESC(80,  iop_nohostsync,     0,                      IOP_USE_OK,                          0),
IOP_DESC(81,  iop_insert,         0,                      IOP_USE_OK,                          0),
IOP_DESC(82,  iop_noinsert,       0,                      IOP_USE_OK,                          0),
IOP_DESC(83,  iop_pasthru,        0,                      IOP_USE_OK,                          0),
IOP_DESC(84,  iop_nopasthru,      0,                      IOP_USE_OK,                          0),
IOP_DESC(85,  iop_readsync,       0,                      IOP_USE_OK,                          0),
IOP_DESC(86,  iop_noreadsync,     0,                      IOP_USE_OK,                          0),
IOP_DESC(87,  iop_ttsync,         0,                      IOP_USE_OK,                          0),
IOP_DESC(88,  iop_nottsync,       0,                      IOP_USE_OK,                          0),
IOP_DESC(89,  iop_after,          SIZEOF(int4)*2,         0,                                   0),
IOP_DESC(90,  iop_burst,          0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(91,  iop_characteristic, SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(92,  iop_copies,         SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(93,  iop_cli,            IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(94,  iop_flag,           0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(95,  iop_form,           SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(96,  iop_header,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(97,  iop_hold,           0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(98,  iop_lowercase,      0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(99,  iop_name,           IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(100, iop_cpulimit,       SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(101, iop_noburst,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(102, iop_print,          0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(103, iop_noprint,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(104, iop_noflag,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(105, iop_noheader,       0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(106, iop_nohold,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(107, iop_nolowercase,    0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(108, iop_nonotify,       0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(109, iop_nopassall,      0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(110, iop_norestart,      0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(111, iop_noused_1,       0,                      0,                                   0),
IOP_DESC(112, iop_note,           IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(113, iop_notify,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(114, iop_notrailer,      0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(115, iop_operator,       IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(116, iop_firstpage,      SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(117, iop_passall,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(118, iop_priority,       SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(119, iop_remote,         IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(120, iop_restart,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(121, iop_setup,          IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(122, iop_trailer,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(123, iop_user,           IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(124, iop_logfile,        IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(125, iop_logqueue,       IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(126, iop_lastpage,       SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(127, iop_page,           0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(128, iop_nopage,         0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(129, iop_doublespace,    0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(130, iop_nodoublespace,  0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(131, iop_p1,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(132, iop_p2,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(133, iop_p3,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(134, iop_p4,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(135, iop_p5,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(136, iop_p6,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(137, iop_p7,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(138, iop_p8,             IOP_VAR_SIZE,           IOP_CLOSE_OK,                        IOP_SRC_STR),
IOP_DESC(139, iop_filter,         IOP_VAR_SIZE,           IOP_USE_OK,                          IOP_SRC_STR),
IOP_DESC(140, iop_nofilter,       0,                      IOP_USE_OK,                          0),
IOP_DESC(141, iop_writetm,        0,                      IOP_USE_OK|IOP_CLOSE_OK,             0),
IOP_DESC(142, iop_writelb,        IOP_VAR_SIZE,           IOP_USE_OK,                          IOP_SRC_STR),
IOP_DESC(143, iop_truncate,       0,                      IOP_USE_OK|IOP_OPEN_OK,              0),
IOP_DESC(144, iop_notruncate,     0,                      IOP_USE_OK|IOP_OPEN_OK,              0),
IOP_DESC(145, iop_extgap,         0,                      IOP_USE_OK|IOP_OPEN_OK,              0),
IOP_DESC(146, iop_noebcdic,       0,                      IOP_USE_OK|IOP_OPEN_OK,              0),
IOP_DESC(147, iop_retry,          0,                      IOP_USE_OK|IOP_OPEN_OK,              0),
IOP_DESC(148, iop_nl,             0,                      IOP_OPEN_OK,                         0),
IOP_DESC(149, iop_noterminator,   0,                      IOP_USE_OK,                          0),
IOP_DESC(150, iop_sequential,     0,                      IOP_OPEN_OK,                         0),
IOP_DESC(151, iop_fifo,           0,                      IOP_OPEN_OK,                         0),
IOP_DESC(152, iop_canonical,      0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(153, iop_nocanonical,    0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(154, iop_socket,         IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_CLOSE_OK|IOP_USE_OK, IOP_SRC_STR),
IOP_DESC(155, iop_listen,         0,                      0,                                   0),            /* no longer used - see iop_zlisten */
IOP_DESC(156, iop_urgent,         0,                      IOP_USE_OK,                          0),
IOP_DESC(157, iop_nourgent,       0,                      IOP_USE_OK,                          0),
IOP_DESC(158, iop_delimiter,      IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(159, iop_connect,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(160, iop_ioerror,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(161, iop_attach,         IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(162, iop_detach,         IOP_VAR_SIZE,           IOP_USE_OK,                          IOP_SRC_STR),
IOP_DESC(163, iop_zlisten,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(164, iop_ipchset,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(165, iop_opchset,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(166, iop_nodelimiter,    0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(167, iop_zdelay,         0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(168, iop_znodelay,       0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(169, iop_zbfsize,        SIZEOF(int4),           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_INT),
IOP_DESC(170, iop_zibfsize,       SIZEOF(int4),           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_INT),
IOP_DESC(171, iop_zff,            IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(172, iop_znoff,          0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(173, iop_zlength,        SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(174, iop_zwidth,         SIZEOF(int4),           IOP_USE_OK,                          IOP_SRC_INT),
IOP_DESC(175, iop_zwrap,          0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(176, iop_znowrap,        0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(177, iop_bigrecord,      0,                      IOP_OPEN_OK,                         0),
IOP_DESC(178, iop_nobigrecord,    0,                      IOP_OPEN_OK,                         0),
IOP_DESC(179, iop_rfm,            IOP_VAR_SIZE,           IOP_OPEN_OK,                         IOP_SRC_STR),
IOP_DESC(180, iop_m,              0,                      IOP_OPEN_OK,                         0),
IOP_DESC(181, iop_utf8,           0,                      IOP_OPEN_OK,                         0),
IOP_DESC(182, iop_utf16,          0,                      IOP_OPEN_OK,                         0),
IOP_DESC(183, iop_utf16be,        0,                      IOP_OPEN_OK,                         0),
IOP_DESC(184, iop_utf16le,        0,                      IOP_OPEN_OK,                         0),
IOP_DESC(185, iop_pad,            SIZEOF(int4),           IOP_OPEN_OK,                         IOP_SRC_INT),
IOP_DESC(186, iop_chset,          IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(187, iop_morereadtime,   SIZEOF(int4),           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_INT),
IOP_DESC(188, iop_shell,          IOP_VAR_SIZE,           IOP_OPEN_OK,                         IOP_SRC_STR),
IOP_DESC(189, iop_command,        IOP_VAR_SIZE_4BYTE,     IOP_OPEN_OK,                         IOP_SRC_STR),
IOP_DESC(190, iop_stderr,         IOP_VAR_SIZE,           IOP_OPEN_OK,                         IOP_SRC_STR),
IOP_DESC(191, iop_independent,    0,                      IOP_OPEN_OK,                         0),
IOP_DESC(192, iop_parse,          0,                      IOP_OPEN_OK,                         0),
IOP_DESC(193, iop_destroy,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(194, iop_nodestroy,      0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(195, iop_rundown,        0,                      IOP_CLOSE_OK,                        0),
IOP_DESC(196, iop_follow,         0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(197, iop_nofollow,       0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(198, iop_empterm,        0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(199, iop_noempterm,      0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(200, iop_timeout,        SIZEOF(int4),           IOP_CLOSE_OK,                        IOP_SRC_INT),
IOP_DESC(201, iop_seek,           IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(202, iop_key,            IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(203, iop_input_key,      IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(204, iop_output_key,     IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(205, iop_inseek,         IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(206, iop_outseek,        IOP_VAR_SIZE,           IOP_OPEN_OK|IOP_USE_OK,              IOP_SRC_STR),
IOP_DESC(207, iop_inrewind,       0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(208, iop_outrewind,      0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(209, iop_buffered,       0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(210, iop_fflf,           0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(211, iop_nofflf,         0,                      IOP_OPEN_OK|IOP_USE_OK,              0),
IOP_DESC(212, iop_hupenable,      0,                      IOP_USE_OK,                          0),
IOP_DESC(213, iop_nohupenable,    0,                      IOP_USE_OK,                          0),
IOP_DESC(214, n_iops,             0,                      0,                                   0),
