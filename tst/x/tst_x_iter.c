#include "tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  unsigned int scan = 1;
  size_t pos = 1;
  
  while (scan <= 111)
    vstr_add_rep_chr(s1, s1->len, scan++, 1);
  vstr_add_non(s1, s1->len, 11);
  scan += 11;
  while (scan <= 255)
    vstr_add_rep_chr(s1, s1->len, scan++, 1);

  vstr_add_rep_chr(s1, s1->len, 0, 11);
  
  ASSERT(s1->len == 266);

  while (pos <= 266)
  {
    Vstr_iter iter[1];
    unsigned int ern;
    size_t len = vstr_sc_posdiff(pos, 266);
    
    ASSERT(vstr_iter_fwd_beg(s1, pos, len, iter));
    
    ASSERT(vstr_iter_pos(iter, pos, len) == pos);
    ASSERT(vstr_iter_len(iter)           == len);

    scan = pos;
    while (scan <= 266)
    {
      unsigned char val = 0;

      ASSERT(vstr_iter_pos(iter, pos, len) == scan);
      ASSERT(vstr_iter_len(iter)           == vstr_sc_posdiff(scan, 266));
      
      val = vstr_iter_fwd_chr(iter, &ern);

      ASSERT(ern != VSTR_TYPE_ITER_END);
      ASSERT((ern == VSTR_TYPE_ITER_NON) || (ern == VSTR_TYPE_ITER_DEF));
      
      if (ern != VSTR_TYPE_ITER_NON)
        ASSERT((val == scan) || ((val == 0) && (scan >= 256) && (scan <= 266)));
      else
        ASSERT((scan > 111) && (scan <= 123) && (val == 0));
      
      ++scan;
    }
    ASSERT(!vstr_iter_len(iter));
    
    ++pos;
  }
  
  return (EXIT_SUCCESS);
}

/*
 */
