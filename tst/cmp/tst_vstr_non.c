#include "../tst-main.c"

static const char *rf = __FILE__;

int tst(void)
{
  Vstr_base *x_str = vstr_make_base(NULL);
  Vstr_base *y_str = vstr_make_base(NULL);
  
  vstr_add_fmt(x_str, x_str->len, "abcd");
  vstr_add_fmt(y_str, y_str->len, "abcd");
  
  vstr_add_non(x_str, x_str->len, 4);
  vstr_add_non(y_str, y_str->len, 4);
  
  vstr_add_fmt(x_str, x_str->len, "xyz");
  vstr_add_fmt(y_str, y_str->len, "xyz");
  
  if (vstr_cmp_vers(x_str, 1, x_str->len, y_str, 1, y_str->len))
    die();
  if (vstr_cmp_case(x_str, 1, x_str->len, y_str, 1, y_str->len))
    die();
  if (vstr_cmp(x_str, 1, x_str->len, y_str, 1, y_str->len))
    die();
  
  vstr_free_base(x_str);
  vstr_free_base(y_str);
  
  return (0);
}
