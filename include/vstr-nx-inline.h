/* not external inline functions memrchr() isn't std. */
#ifdef VSTR_AUTOCONF_USE_WRAP_MEMRCHR
extern inline void *vstr_nx_wrap_memrchr(const void *passed_s1, int c, size_t n)
{
  const unsigned char *s1 = passed_s1;
  const void *ret = 0;
  int tmp = 0;

  switch (n)
  {
    case 7:  tmp = s1[6] == c; if (tmp) { ret = s1 + 6; break; }
    case 6:  tmp = s1[5] == c; if (tmp) { ret = s1 + 5; break; }
    case 5:  tmp = s1[4] == c; if (tmp) { ret = s1 + 4; break; }
    case 4:  tmp = s1[3] == c; if (tmp) { ret = s1 + 3; break; }
    case 3:  tmp = s1[2] == c; if (tmp) { ret = s1 + 2; break; }
    case 2:  tmp = s1[1] == c; if (tmp) { ret = s1 + 1; break; }
    case 1:  tmp = s1[0] == c; if (tmp) { ret = s1 + 0; break; }
      break;
    default: ret = memrchr(s1, c, n);
      break;
  }
  
  return ((void *)ret);
}
#else
# define vstr_nx_wrap_memrchr(x, y, z) memrchr(x, y, z)
#endif
