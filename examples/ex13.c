/* for easy of use in certain places it's possible to do this to include the
 * system headers needed */
#define VSTR_COMPILE_INCLUDE 1
#include <vstr.h>

#include <string.h>
#include <errno.h>

#include "ex_utils.h"

static Vstr_base *out = NULL;  
static Vstr_base *str1 = NULL;
static unsigned char ips[4];
static unsigned int cidr = -1;
static size_t num_len = -1;
static unsigned int err = -1;

#define DO_TST(x) do_tst(x, #x)

static void do_tst(unsigned int flags, const char *str_flags)
{
  int ret = 0;
  
  memset(ips, -1, 4);
  
  ret = vstr_parse_ipv4(str1, 1, str1->len, ips, &cidr, flags, &num_len, &err);

  vstr_add_fmt(out, out->len,
               "-------------------------------------------------------\n"
               "Str   = %s\n"
               "Flags = %s\n"
               "-------------------------------------------------------\n"
               " ret = %d"
               " Ip: %u.%u.%u.%u/%u\n"
               " num_len=%zu\n"
               " err=%u\n",
               vstr_export_cstr_ptr(str1, 1, str1->len),
               str_flags,
               ret,
               ips[0], ips[1], ips[2], ips[3], cidr,
               num_len, err);  
}

int main(void)
{
  if (!vstr_init())
      exit (EXIT_FAILURE);
  
  str1 = VSTR_DUP_CSTR_BUF(NULL, "127.0.0.1/8");
  if (!str1)
    exit (EXIT_FAILURE);

  out = VSTR_DUP_CSTR_BUF(NULL, "");
  if (!out)
    exit (EXIT_FAILURE);

  DO_TST(VSTR_FLAG_PARSE_IPV4_DEF);
  DO_TST(VSTR_FLAG_PARSE_IPV4_FULL);
  DO_TST(VSTR_FLAG_PARSE_IPV4_CIDR);
  DO_TST(VSTR_FLAG_PARSE_IPV4_CIDR | VSTR_FLAG_PARSE_IPV4_CIDR_FULL);
  DO_TST(VSTR_FLAG_PARSE_IPV4_CIDR | VSTR_FLAG_PARSE_IPV4_NETMASK);
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK);
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK | VSTR_FLAG_PARSE_IPV4_NETMASK_FULL);
  VSTR_SUB_CSTR_PTR(str1, 1, str1->len, "127.0.0.1/255.255.128.0");
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK);
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK | VSTR_FLAG_PARSE_IPV4_NETMASK_FULL);
  DO_TST(VSTR_FLAG_PARSE_IPV4_CIDR | VSTR_FLAG_PARSE_IPV4_NETMASK);
  VSTR_SUB_CSTR_PTR(str1, 1, str1->len, "127.0.0.1/255.255.255.255");
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK);
  DO_TST(VSTR_FLAG_PARSE_IPV4_NETMASK | VSTR_FLAG_PARSE_IPV4_NETMASK_FULL);
  DO_TST(VSTR_FLAG_PARSE_IPV4_CIDR | VSTR_FLAG_PARSE_IPV4_NETMASK);
  DO_TST(VSTR_FLAG_PARSE_IPV4_ONLY);
  VSTR_SUB_CSTR_PTR(str1, 1, str1->len, "127.0. 0.1 *");
  DO_TST(VSTR_FLAG_PARSE_IPV4_DEF);
  DO_TST(VSTR_FLAG_PARSE_IPV4_FULL);
  DO_TST(VSTR_FLAG_PARSE_IPV4_ONLY);
  
  while (out->len)
    ex_utils_write(out, 1);

  vstr_free_base(str1);
  vstr_free_base(out);
  
  exit (EXIT_SUCCESS);
}
