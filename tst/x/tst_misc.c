#define VSTR_COMPILE_ATTRIBUTES 0 /* ignore "pure" attribute or
                                   * the below might go away */

#include "../tst-main.c"

extern VSTR_SECTS_EXTERN_DECL(blahfoo, 4);

static const char *rf = __FILE__;

static void tst_v(const char *msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  vstr_add_vfmt(s1, 0, msg, ap);
  va_end(ap);
  
  va_start(ap, msg);
  vstr_add_vsysfmt(s1, 0, msg, ap);
  va_end(ap);  
}


/* misc crap for tst_covorage -- it's called by other stuff */
int tst(void)
{
  Vstr_ref *ref = NULL;
  VSTR_SECTS_DECL(abcd, 4);
  typeof(blahfoo[0]) *tmp = abcd;
  
  assert(sizeof(blahfoo) == sizeof(abcd));
  assert(tmp->sz == 4);
  
  vstr_make_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 8);
  
  tst_v("%s", "123456789 123456789 123456789 123456789 123456789 123456789 ");
  assert(s1->len == 120);

  vstr_free_spare_nodes(s1->conf, VSTR_TYPE_NODE_BUF, 1);

  ref = vstr_ref_make_malloc(4);
  vstr_ref_add(ref);
  vstr_ref_del(ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(NULL, vstr_ref_cb_free_ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr_ref);
  vstr_ref_del(ref);

  ref = vstr_ref_make_ptr(malloc(1), vstr_ref_cb_free_ptr);
  vstr_ref_del(ref);
  free(ref);
  
  return (!VSTR_FLAG31(CONV_UNPRINTABLE_ALLOW,
                       NUL, BEL, BS, HT, LF, VT, FF, CR, SP, COMMA,
                       NUL, BEL, BS, HT, LF, VT, FF, CR, SP, COMMA,
                       NUL, BEL, BS, HT, LF,
                       DOT, _, ESC, DEL, HSP, HIGH));
}

/* Crap for tst_coverage constants....
 *
 * VSTR_COMPILE_INCLUDE
 *
 * VSTR_MAX_NODE_ALL
 *
 * VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NEG
 * VSTR_FLAG_SC_FMT_CB_BEG_OBJ_NUM
 * VSTR_FLAG_SC_FMT_CB_BEG_OBJ_STR
 *
 * VSTR_TYPE_FMT_PTR_CHAR
 * VSTR_TYPE_FMT_SIZE_T
 *
 * VSTR_SECTS_INIT()
 */
