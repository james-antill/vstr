/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>

#include "ex_utils.h"

struct ex12_cache_chr
{
 size_t pos;
 size_t len;
 char srch;
 size_t found_at;
};

static unsigned int ex12_cache_srch_pos = 0;

static void *ex12_cache_cb(const Vstr_base *base __attribute__((unused)),
                           size_t pos, size_t len,
                           unsigned int type, void *passed_data)
{ /* this is a simple version, it could chop the cache when something changes */
  struct ex12_cache_chr *data = passed_data;
  const size_t end_pos = (pos + len - 1);
  const size_t data_end_pos = (data->pos + data->len - 1);

  if (type == VSTR_TYPE_CACHE_FREE)
  {
    free(data);
    return (NULL);
  }

  if (!data->pos)
    return (data);
  
  if (data_end_pos < pos)
    return (data);

  if ((type == VSTR_TYPE_CACHE_ADD) && (data->pos > pos))
  {
    data->pos += len;
    data->found_at += len;
    return (data);
  }
  
  if (type == VSTR_TYPE_CACHE_ADD)
  {
    if (data_end_pos == pos)
      return (data);

    if (data->pos > pos)
    {
      data->pos += len;
      return (data);
    }
  }
  else if (data->pos > end_pos)
  {
    if (type == VSTR_TYPE_CACHE_DEL)
    {
      data->pos -= len;
      data->found_at -= len;
    }
    
    /* do nothing for (type == VSTR_TYPE_CACHE_SUB) */

    return (data);
  }

  data->pos = 0;
  
  return (data);
}

static size_t xvstr_srch_chr_fwd(Vstr_base *base, size_t pos, size_t len,
                                 char srch)
{
  struct ex12_cache_chr *data = vstr_cache_get_data(base, ex12_cache_srch_pos);

  if (data)
  {
    const size_t end_pos = (pos + len - 1);

    /* if the cache applies */
    if (data->pos && (data->srch == srch) &&
        (!data->found_at ||
         ((data->found_at >= pos) && (data->found_at <= end_pos))))
    {
      const size_t data_end_pos = (data->pos + data->len - 1);
      size_t found_at = 0;

      /* if it covers everything... */
      if ((data->pos <= pos) && (data_end_pos >= end_pos))
        return (data->found_at);
      
      /* if it covers some of the start of our search... */
      if ((data->pos <= pos) && (data_end_pos >= pos))
      {
        size_t uc_len = 0;
        
        if (data->found_at)
          return (data->found_at);
        
        uc_len = end_pos - data_end_pos;
        data->len += uc_len;
        
        data->found_at = vstr_srch_chr_fwd(base, data_end_pos, uc_len, srch);
        
        return (data->found_at);
      }
      
      /* it must cover some of the end of our search... */
      assert((data->pos <= end_pos) && (data_end_pos >= end_pos));
      found_at = vstr_srch_chr_fwd(base, pos, data->pos - pos, srch);
      if (found_at)
        data->found_at = found_at;

      data->len += data->pos - pos;
      data->pos = pos;
      
      return (data->found_at);
    }
    
    data->pos = pos;
    data->len = len;
    data->srch = srch;
    
    data->found_at = vstr_srch_chr_fwd(base, pos, len, srch);

    return (data->found_at);
  }

  if (!(data = malloc(sizeof(struct ex12_cache_chr))))
    return (vstr_srch_chr_fwd(base, pos, len, srch));
  
  data->pos = pos;
  data->len = len;
  data->srch = srch;
  
  data->found_at = vstr_srch_chr_fwd(base, pos, len, srch);
  
  if (!vstr_cache_set_data(base, ex12_cache_srch_pos, data))
  {
    size_t found_at = data->found_at;
    
    free(data);
    
    return (found_at);
  }
  
  return (data->found_at);
}

static void do_test(Vstr_base *out)
{
  Vstr_base *base = VSTR_DUP_CSTR_BUF(out->conf, "The tester!retset ehT");

  if (!base)
    errno = ENOMEM, DIE("vstr_make_base:");

  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));

  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  vstr_sc_write_fd(out, 1, out->len, 1, NULL);
  
  VSTR_ADD_CSTR_BUF(base, 0, "abcd-");

  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("abcd-The tester!"),
               xvstr_srch_chr_fwd(base, strlen("abcd-T"), 21, '!'));

  vstr_sc_write_fd(out, 1, out->len, 1, NULL);

  VSTR_SUB_CSTR_BUF(base, 1, strlen("abcd-"), "xyz-");

  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("bcd-The tester!"),
               xvstr_srch_chr_fwd(base, strlen("bcd-T"), 21, '!'));

  vstr_sc_write_fd(out, 1, out->len, 1, NULL);

  vstr_del(base, 1, strlen("xyz-"));

  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  VSTR_ADD_CSTR_BUF(base, base->len, "abcd-");
  
  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  VSTR_SUB_CSTR_BUF(base, base->len - strlen("bcd-"), strlen("abcd-"),
                    "xyz-");
  
  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  vstr_del(base, base->len - strlen("yz-"), strlen("xyz-"));
  
  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  vstr_del(base, base->len - strlen("he tester"), strlen("The tester"));
  
  vstr_add_fmt(out, out->len, " Searching for '!' in: %s\n",
               vstr_export_cstr_ptr(base, 1, base->len));
  
  vstr_add_fmt(out, out->len, "%zu = %zu\n", strlen("The tester!"),
               xvstr_srch_chr_fwd(base, 1, base->len, '!'));

  vstr_sc_write_fd(out, 1, out->len, 1, NULL);

  vstr_free_base(base);
}

int main(void)
{
  Vstr_base *str1 = NULL;

  if (!vstr_init())
      exit (EXIT_FAILURE);

  str1 = vstr_make_base(NULL);
  if (!str1)
    errno = ENOMEM, DIE("vstr_make_base:");

  ex12_cache_srch_pos = vstr_cache_add_cb(str1->conf,
                                          "/ex12/srch char forward",
                                          ex12_cache_cb);

  do_test(str1);
  
  while (str1->len)
    ex_utils_write(str1, 1);
  
  vstr_free_base(str1);

  ex_utils_check();
  
  exit (EXIT_SUCCESS);
}
