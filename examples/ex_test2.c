/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>

#include "ex_utils.h"

#undef assert
#define assert(x) /* nothing */

#if 1 /* does inline do anything ... */

extern void vstr__cache_iovec_add_node_end(Vstr_base *, unsigned int,
                                           unsigned int);

extern int vstr__check_real_nodes(Vstr_base *);
extern int vstr__check_spare_nodes(Vstr_conf *);

static inline int xvstr_add_buf(Vstr_base *base, size_t pos,
                                const void *buffer, size_t len)
{
  Vstr_node *scan = base->end;

  if (scan && (pos == base->len) &&
      (scan->type == VSTR_TYPE_NODE_BUF) && (scan->len < base->conf->buf_sz))
  {
    size_t tmp = (base->conf->buf_sz - scan->len);

    assert(vstr__check_spare_nodes(base->conf));
    assert(vstr__check_real_nodes(base));
  
    if (tmp > len)
      tmp = len;

    memcpy(((Vstr_node_buf *)scan)->buf + scan->len, buffer, tmp);
    scan->len += tmp;
    buffer = ((char *)buffer) + tmp;
    
    vstr__cache_iovec_add_node_end(base, base->num, tmp);
    
    base->len += tmp;
    pos += tmp;
    len -= tmp;
    
    if (!len)
    {
      assert(vstr__check_spare_nodes(base->conf));
      assert(vstr__check_real_nodes(base));
      
      return (TRUE);
    }
  }

  return (vstr_add_buf(base, pos, buffer, len));
}
#else
# define xvstr_add_buf vstr_add_buf
#endif

int main(void)
{
  Vstr_base *str1 = NULL;
  unsigned int count = 0;
  
  if (!vstr_init())
    exit (EXIT_FAILURE);
  
  str1 = vstr_make_base(NULL);
  if (!str1)
    exit (EXIT_FAILURE);
  
  while (count < (10 * 1000 * 1000))
  {
    xvstr_add_buf(str1, str1->len, "x\n", 3);
    
    ++count;
    if (str1->len > 1000)
      vstr_del(str1, 1, str1->len);
  }
  
  if (str1->conf->malloc_bad)
    errno = ENOMEM, DIE("vstr_add_fmt:");
  
  vstr_free_base(str1);
  
  exit (EXIT_SUCCESS);
}
