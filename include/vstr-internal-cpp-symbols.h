#define vstr_nx_ref_cb_free_nothing vstr_ref_cb_free_nothing
#define vstr_nx_ref_cb_free_ref vstr_ref_cb_free_ref
#define vstr_nx_ref_cb_free_ptr vstr_ref_cb_free_ptr
#define vstr_nx_ref_cb_free_ptr_ref vstr_ref_cb_free_ptr_ref
#define vstr_nx_ref_make_ptr vstr_ref_make_ptr
#define vstr_nx_ref_make_malloc vstr_ref_make_malloc
#define vstr_nx_init vstr_init
#define vstr_nx_exit vstr_exit
#define vstr_nx_make_conf vstr_make_conf
#define vstr_nx_free_conf vstr_free_conf
#define vstr_nx_make_base vstr_make_base
#define vstr_nx_free_base vstr_free_base
#define vstr_nx_make_spare_nodes vstr_make_spare_nodes
#define vstr_nx_free_spare_nodes vstr_free_spare_nodes
#define vstr_nx_add_ptr vstr_add_ptr
#define vstr_nx_add_non vstr_add_non
#define vstr_nx_add_ref vstr_add_ref
#define vstr_nx_add_vstr vstr_add_vstr
#define vstr_nx_dup_buf vstr_dup_buf
#define vstr_nx_dup_ptr vstr_dup_ptr
#define vstr_nx_dup_non vstr_dup_non
#define vstr_nx_dup_vstr vstr_dup_vstr
#define vstr_nx_add_vfmt vstr_add_vfmt
#define vstr_nx_add_fmt vstr_add_fmt
#define vstr_nx_add_iovec_buf_beg vstr_add_iovec_buf_beg
#define vstr_nx_add_iovec_buf_end vstr_add_iovec_buf_end
#define vstr_nx_add_netstr_beg vstr_add_netstr_beg
#define vstr_nx_add_netstr_end vstr_add_netstr_end
#define vstr_nx_add_netstr2_beg vstr_add_netstr2_beg
#define vstr_nx_add_netstr2_end vstr_add_netstr2_end
#define vstr_nx_sub_buf vstr_sub_buf
#define vstr_nx_sub_ptr vstr_sub_ptr
#define vstr_nx_sub_non vstr_sub_non
#define vstr_nx_sub_ref vstr_sub_ref
#define vstr_nx_sub_vstr vstr_sub_vstr
#define vstr_nx_conv_lowercase vstr_conv_lowercase
#define vstr_nx_conv_uppercase vstr_conv_uppercase
#define vstr_nx_conv_unprintable_chr vstr_conv_unprintable_chr
#define vstr_nx_conv_unprintable_del vstr_conv_unprintable_del
#define vstr_nx_conv_encode_uri vstr_conv_encode_uri
#define vstr_nx_conv_decode_uri vstr_conv_decode_uri
#define vstr_nx_mov vstr_mov
#define vstr_nx_cntl_opt vstr_cntl_opt
#define vstr_nx_cntl_base vstr_cntl_base
#define vstr_nx_cntl_conf vstr_cntl_conf
#define vstr_nx_num vstr_num
#define vstr_nx_cmp vstr_cmp
#define vstr_nx_cmp_buf vstr_cmp_buf
#define vstr_nx_cmp_case vstr_cmp_case
#define vstr_nx_cmp_case_buf vstr_cmp_case_buf
#define vstr_nx_cmp_vers vstr_cmp_vers
#define vstr_nx_srch_chr_fwd vstr_srch_chr_fwd
#define vstr_nx_srch_chr_rev vstr_srch_chr_rev
#define vstr_nx_srch_chrs_fwd vstr_srch_chrs_fwd
#define vstr_nx_srch_chrs_rev vstr_srch_chrs_rev
#define vstr_nx_csrch_chrs_fwd vstr_csrch_chrs_fwd
#define vstr_nx_csrch_chrs_rev vstr_csrch_chrs_rev
#define vstr_nx_srch_buf_fwd vstr_srch_buf_fwd
#define vstr_nx_srch_buf_rev vstr_srch_buf_rev
#define vstr_nx_srch_vstr_fwd vstr_srch_vstr_fwd
#define vstr_nx_srch_vstr_rev vstr_srch_vstr_rev
#define vstr_nx_srch_sects_add_buf_fwd vstr_srch_sects_add_buf_fwd
#define vstr_nx_spn_buf_fwd vstr_spn_buf_fwd
#define vstr_nx_spn_buf_rev vstr_spn_buf_rev
#define vstr_nx_cspn_buf_fwd vstr_cspn_buf_fwd
#define vstr_nx_cspn_buf_rev vstr_cspn_buf_rev
#define vstr_nx_export_iovec_ptr_all vstr_export_iovec_ptr_all
#define vstr_nx_export_iovec_cpy_buf vstr_export_iovec_cpy_buf
#define vstr_nx_export_iovec_cpy_ptr vstr_export_iovec_cpy_ptr
#define vstr_nx_export_buf vstr_export_buf
#define vstr_nx_export_chr vstr_export_chr
#define vstr_nx_export_ref vstr_export_ref
#define vstr_nx_export_cstr_ptr vstr_export_cstr_ptr
#define vstr_nx_export_cstr_buf vstr_export_cstr_buf
#define vstr_nx_export_cstr_ref vstr_export_cstr_ref
#define vstr_nx_parse_short vstr_parse_short
#define vstr_nx_parse_ushort vstr_parse_ushort
#define vstr_nx_parse_int vstr_parse_int
#define vstr_nx_parse_uint vstr_parse_uint
#define vstr_nx_parse_long vstr_parse_long
#define vstr_nx_parse_ulong vstr_parse_ulong
#define vstr_nx_parse_intmax vstr_parse_intmax
#define vstr_nx_parse_uintmax vstr_parse_uintmax
#define vstr_nx_parse_ipv4 vstr_parse_ipv4
#define vstr_nx_parse_netstr vstr_parse_netstr
#define vstr_nx_parse_netstr2 vstr_parse_netstr2
#define vstr_nx_sects_make vstr_sects_make
#define vstr_nx_sects_free vstr_sects_free
#define vstr_nx_sects_del vstr_sects_del
#define vstr_nx_sects_srch vstr_sects_srch
#define vstr_nx_sects_foreach vstr_sects_foreach
#define vstr_nx_split_buf vstr_split_buf
#define vstr_nx_split_chrs vstr_split_chrs
#define vstr_nx_join_buf vstr_join_buf
#define vstr_nx_cache_add_cb vstr_cache_add_cb
#define vstr_nx_cache_srch vstr_cache_srch
#define vstr_nx_cache_set_data vstr_cache_set_data
#define vstr_nx_cache_sub vstr_cache_sub
#define vstr_nx_sc_add_fd vstr_sc_add_fd
#define vstr_nx_sc_add_file vstr_sc_add_file
#define vstr_nx_sc_read_fd vstr_sc_read_fd
#define vstr_nx_sc_write_fd vstr_sc_write_fd
#define vstr_nx_sc_write_file vstr_sc_write_file
#define vstr_nx_extern_inline_add_buf vstr_extern_inline_add_buf
#define vstr_nx_extern_inline_del vstr_extern_inline_del
#define vstr_nx_extern_inline_sects_add vstr_extern_inline_sects_add