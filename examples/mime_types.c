
#include "mime_types.h"

#include <errno.h>
#include <err.h>

#ifndef FALSE
# define FALSE 0
#endif

#ifndef TRUE
# define TRUE 1
#endif

#ifndef VSTR_AUTOCONF_NDEBUG
# define assert(x) do { if (x) {} else errx(EXIT_FAILURE, "assert(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
# define ASSERT(x) do { if (x) {} else errx(EXIT_FAILURE, "ASSERT(" #x "), FAILED at %s:%u", __FILE__, __LINE__); } while (FALSE)
#else
# define ASSERT(x)
# define assert(x)
#endif
#define ASSERT_NOT_REACHED() assert(FALSE)

static Vstr_base *ent_data = NULL;
static Vstr_sects *ents = NULL;

int mime_types_load_simple(const char *fname)
{
  size_t orig_ent_data_len = ent_data->len;
  unsigned int orig_ents_num = ents->num;
  Vstr_base *data = NULL;
  Vstr_sects *lines = NULL;
  unsigned int num = 0;
  Vstr_sects *sects = NULL;
  size_t data_pos = 0;
  size_t data_len = 0;
  int saved_errno = ENOMEM;
  
  if (!fname || !*fname)
    return (TRUE);
  
  if (!(data = vstr_make_base(NULL)))
    goto fail_data;

  if (!(lines = vstr_sects_make(128)))
    goto fail_sects_lines;

  if (!(sects = vstr_sects_make(4)))
    goto fail_sects_tmp;

  if (!vstr_sc_read_len_file(data, data_pos, fname, 0, 0, NULL))
    goto fail_read_file;

  data_len = data->len - data_pos++;
  vstr_split_cstr_buf(data, data_pos, data_len, "\n", lines, 0, 0);
  if (lines->malloc_bad)
    goto fail_split_file;

  while (++num <= lines->num)
  {
    size_t pos = VSTR_SECTS_NUM(lines, num)->pos;
    size_t len = VSTR_SECTS_NUM(lines, num)->len;
    Vstr_sect_node *sct = NULL;

    if (vstr_export_chr(data, pos) == '#')
      continue;

    sects->num = 0;
    vstr_split_cstr_chrs(data, pos, len, " \t", sects, 0, 0);
    if (sects->malloc_bad)
      goto fail_split_line;
    
    while (sects->num > 1)
    {
      Vstr_sect_node *sext = VSTR_SECTS_NUM(sects, sects->num);
      size_t spos = 0;
      size_t slen = 0;
      
      if (!sct)
      {
        sct = VSTR_SECTS_NUM(sects, 1);
        spos = ent_data->len + 1;
        vstr_add_vstr(ent_data, ent_data->len,
                      data, sct->pos, sct->len, VSTR_TYPE_ADD_DEF);
        sct->pos = spos;
      }
      vstr_sects_add(ents, sct->pos, sct->len);

      if (vstr_export_chr(data, sext->pos) != '.')
      {
        vstr_add_cstr_buf(ent_data, ent_data->len, ".");
        spos = ent_data->len;
        slen = sext->len + 1;
      }
      else
      { /* chop the . off and use everything else */
        sext->len--; sext->pos++;
        spos = ent_data->len + 1;
        slen = sext->len;
      }
      
      vstr_add_vstr(ent_data, ent_data->len,
                    data, sext->pos, sext->len, VSTR_TYPE_ADD_DEF);
      vstr_sects_add(ents, spos, slen);
      sects->num--;
    }
  }

  if (ent_data->conf->malloc_bad || ents->malloc_bad)
    goto fail_end_malloc_check;

  vstr_sects_free(sects);
  vstr_sects_free(lines);
  vstr_free_base(data);

  return (TRUE);
  
 fail_end_malloc_check:
  vstr_sc_reduce(ent_data, 1, ent_data->len, ent_data->len - orig_ent_data_len);
  ents->num = orig_ents_num;
 fail_split_line:
 fail_split_file:
  errno = ENOMEM;
 fail_read_file:
  saved_errno = errno;
  vstr_sects_free(lines);
 fail_sects_tmp:
  vstr_sects_free(sects);
 fail_sects_lines:
  vstr_free_base(data);
 fail_data:
  errno = saved_errno;
  return (FALSE);
}

static size_t mime_types__def_len = 0;
int mime_types_init(const char *def)
{
  ASSERT(def);
  
  if (!(ent_data = vstr_make_base(NULL)))
    return (FALSE);

  vstr_add_cstr_ptr(ent_data, 0, def);
  mime_types__def_len = ent_data->len;
  
  if (!(ents = vstr_sects_make(128)))
    return (FALSE);

  return (TRUE);
}

/* FIXME: way too simple */
int mime_types_match(const Vstr_base *fname, size_t pos, size_t len,
                     Vstr_base **ret_vs1, size_t *ret_pos, size_t *ret_len)
{
  unsigned int num = 0;

  ASSERT(ret_vs1 && ret_pos && ret_len);

  num = ents->num;
  while (num)
  {
    size_t epos  = VSTR_SECTS_NUM(ents, num)->pos;
    size_t elen  = VSTR_SECTS_NUM(ents, num)->len;
    size_t ctpos = 0;
    size_t ctlen = 0;

    --num;
    ctpos = VSTR_SECTS_NUM(ents, num)->pos;
    ctlen = VSTR_SECTS_NUM(ents, num)->len;
    --num;

    if (elen > len)
      continue;

    if (vstr_cmp_eod_eq(fname, pos, len, ent_data, epos, elen))
    {
      *ret_vs1 = ent_data;
      *ret_pos = ctpos;
      *ret_len = ctlen;
      return (TRUE);
    }
  }

  *ret_vs1 = ent_data;
  *ret_pos = 1;
  *ret_len = mime_types__def_len;
  return (FALSE);
}

void mime_types_exit(void)
{
  vstr_free_base(ent_data); ent_data = NULL;
  vstr_sects_free(ents); ents = NULL;
}

