# See if we can cast the automake spell. -*-Makefile-*-
#

#
AUTOMAKE_OPTIONS = no-dependencies 1.4

tst_add = \
    add/tst_add_buf \
    add/tst_add_ptr \
    add/tst_add_ref \
    add/tst_add_vstr \
    add/tst_add_non \
    add/tst_add_rep_chr \
    add/tst_add_iovec_buf \
    add/tst_add_netstr \
    add/tst_add_netstr2

xfail_tst_add = \
    add/xfail_tst_add_buf_pos \
    add/xfail_tst_add_buf_NULL \
    add/xfail_tst_add_ptr_pos \
    add/xfail_tst_add_ptr_NULL \
    add/xfail_tst_add_non_pos \
    add/xfail_tst_add_non_NULL \
    add/xfail_tst_add_ref_pos \
    add/xfail_tst_add_ref_NULL_ref \
    add/xfail_tst_add_ref_NULL_vstr \
    add/xfail_tst_add_vstr_pos_f \
    add/xfail_tst_add_vstr_pos_t \
    add/xfail_tst_add_vstr_NULL_f \
    add/xfail_tst_add_vstr_NULL_t

tst_cache = \
    cache/tst_cache_usr \
    cache/tst_cache_free \
    cache/tst_cache_srch

tst_cmp = \
    cmp/tst_cmp \
    cmp/tst_cmp_case \
    cmp/tst_cmp_vers \
    cmp/tst_cmp_non

tst_cntl = \
    cntl/tst_cntl_conf \
    cntl/tst_cntl_base \
    cntl/tst_cntl_opt

tst_conv = \
    conv/tst_conv_encode \
    conv/tst_conv_decode \
    conv/tst_conv_lowercase \
    conv/tst_conv_uppercase \
    conv/tst_conv_unprintable

tst_del = \
    del/tst_del_empty \
    del/tst_del_all \
    del/tst_del_beg \
    del/tst_del_mid \
    del/tst_del_end

xfail_tst_del = \
    del/xfail_tst_del_oob_pos \
    del/xfail_tst_del_oob_len

tst_dup = \
    dup/tst_dup_buf \
    dup/tst_dup_ptr \
    dup/tst_dup_ref \
    dup/tst_dup_vstr \
    dup/tst_dup_non \
    dup/tst_dup_rep_chr

tst_export = \
    export/tst_export_cstr_buf \
    export/tst_export_cstr_malloc \
    export/tst_export_cstr_ptr \
    export/tst_export_cstr_ref \
    export/tst_export_iovec_ptr_all \
    export/tst_export_iovec_cpy_buf \
    export/tst_export_iovec_cpy_ptr \
    export/tst_export_buf \
    export/tst_export_chr \
    export/tst_export_ref

tst_fmt = \
    fmt/tst_fmt_char \
    fmt/tst_fmt_short \
    fmt/tst_fmt_int \
    fmt/tst_fmt_long \
    fmt/tst_fmt_intmax_t \
    fmt/tst_fmt_double \
    fmt/tst_fmt_long_double \
    fmt/tst_fmt_ptrdiff_t \
    fmt/tst_fmt_size_t \
    fmt/tst_fmt_m \
    fmt/tst_fmt_n \
    fmt/tst_fmt_p \
    fmt/tst_fmt_str_norm \
    fmt/tst_fmt_num \
    fmt/tst_fmt_bkmg \
    fmt/tst_fmt_ip \
    fmt/tst_fmt_i18n \
    fmt/tst_fmt_usr

xfail_tst_fmt = \
    fmt/xfail_tst_fmt_invalid_sysfmt \
    fmt/xfail_tst_fmt_invalid_fmt

tst_loc = \
    loc/tst_loc_num

tst_mov = \
    mov/tst_mov_empty \
    mov/tst_mov_data

tst_parse = \
    parse/tst_parse_ipv4 \
    parse/tst_parse_ipv6 \
    parse/tst_parse_num \
    parse/tst_parse_netstr \
    parse/tst_parse_netstr2

tst_sc = \
    sc/tst_sc_read \
    sc/tst_sc_write \
    sc/tst_sc_mmap \
    sc/tst_sc_basename \
    sc/tst_sc_dirname

tst_sects = \
    sects/tst_sects_add \
    sects/tst_sects_del \
    sects/tst_sects_srch \
    sects/tst_sects_foreach \
    sects/tst_sects_join \
    sects/tst_sects_update

tst_spn = \
    spn/tst_spn_chrs \
    spn/tst_spn_c_chrs

tst_split = \
    split/tst_split_buf \
    split/tst_split_chrs

tst_srch = \
    srch/tst_srch_buf \
    srch/tst_srch_chr \
    srch/tst_srch_case_buf \
    srch/tst_srch_case_chr \
    srch/tst_srch_chrs \
    srch/tst_srch_csrch_chrs \
    srch/tst_srch_vstr \
    srch/tst_srch_case_vstr

tst_sub = \
    sub/tst_sub_buf \
    sub/tst_sub_ptr \
    sub/tst_sub_non \
    sub/tst_sub_ref \
    sub/tst_sub_vstr \
    sub/tst_sub_rep_chr

tst_x = \
    x/tst_x_typedef \
    x/tst_x_inline \
    x/tst_x_include \
    x/tst_x_macros \
    x/tst_x_make \
    x/tst_x_misc \
    x/tst_x_ref \
    x/tst_x_swap_conf \
    x/tst_x_version

xfail_tst_x = \
    x/xfail_tst_x_make_bad_node \
    x/xfail_tst_x_free_bad_node


tst = \
  $(tst_add) \
  $(tst_cache) \
  $(tst_cmp) \
  $(tst_cntl) \
  $(tst_conv) \
  $(tst_del) \
  $(tst_dup) \
  $(tst_export) \
  $(tst_fmt) \
  $(tst_loc) \
  $(tst_mov) \
  $(tst_parse) \
  $(tst_sc) \
  $(tst_sects) \
  $(tst_spn) \
  $(tst_split) \
  $(tst_srch) \
  $(tst_sub) \
  $(tst_x)

xfail_tst = \
  $(xfail_tst_add) \
  $(xfail_tst_del) \
  $(xfail_tst_fmt) \
  $(xfail_tst_x)

check_PROGRAMS = $(tst) $(xfail_tst)

if !USE_ASSERT_LOOP
if !NDEBUG
XF_TST = $(xfail_tst)
endif
endif

TESTS = $(tst) $(XF_TST)

XFAIL_TESTS = $(xfail_tst)

CFLAGS = -I- -I../include -I../../include @CFLAGS@ @WARNS@
LDADD = -L../src/ -lvstr
