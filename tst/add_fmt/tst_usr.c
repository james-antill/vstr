#include "../tst-main.c"

static const char *rf = __FILE__;

#ifndef HAVE_POSIX_HOST
static int getpid(void) { return (0xdeadbeef); }
#endif

static int tst_errno = 0;

static int tst_usr_vstr_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  Vstr_base *sf    = spec->data_ptr[0];
  size_t sf_flags  = *(unsigned int *)(spec->data_ptr[1]);

  assert(!strcmp(spec->name, "{Vstr:%p%u}"));
  
  if (!vstr_add_vstr(st, pos, sf, 1, sf->len, sf_flags))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_pid_cb(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  assert(!strcmp(spec->name, "{PID}"));
  
  if (!vstr_add_fmt(st, pos, "%ld", (long)getpid()))
    return (FALSE);

  return (TRUE);
}

static int tst_usr_strerror(Vstr_base *st, size_t pos, Vstr_fmt_spec *spec)
{
  assert(!strcmp(spec->name, "m"));
  
  if (!vstr_add_fmt(st, pos, "%s", strerror(tst_errno)))
    return (FALSE);

  return (TRUE);
}

#ifdef HAVE_POSIX_HOST
static int tst_setup_inet_buf(const struct in_addr  *ipv4,
                               const struct in6_addr *ipv6)
{
  size_t len = 0;
  const char *ptr = NULL;
    
  assert(256 >= (INET_ADDRSTRLEN + INET6_ADDRSTRLEN));
  assert(sizeof(buf) > 256);

  memset(buf, ' ', 256);
  buf[256] = 0;
  buf[257] = (char)0xFF;
  
  if (!(ptr = inet_ntop(AF_INET,  ipv4, buf, 128)))
    return (FALSE);
  len = strlen(ptr);
  assert(len < 16);
  
  assert(ptr == buf);
  
  /* turn '\0' into a space */
  assert(!buf[len]);
  buf[len] = ' ';
  
  if (!(ptr = inet_ntop(AF_INET6, ipv6, buf + 128, 128)))
    return (FALSE);
  len = strlen(ptr);
  assert(len < 40);
  
  memmove(buf + (256 - len), buf + 128, len); /* move ipv6 "object" to end */
  memset(buf + 128, ' ', 128 - len);
  
  assert(!buf[256]);
  assert( buf[257] == (char)0xFF);

  return (TRUE);
}

#endif

int tst(void)
{
  int ret = 0;

  sprintf(buf, "%d %d %u %u", INT_MAX, INT_MIN, 0, UINT_MAX);
  VSTR_ADD_CSTR_BUF(s2, s2->len, "$ ");
  VSTR_ADD_CSTR_BUF(s2, s2->len, buf);
  vstr_add_fmt(s2, s2->len, " -- %ld $", (long)getpid());

  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s3->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_cntl_conf(s4->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '%');

  /* setup custom formatters */
  
  vstr_fmt_add(s1->conf, "{Vstr:%p%u}", tst_usr_vstr_cb,
               VSTR_TYPE_FMT_PTR_VOID,
               VSTR_TYPE_FMT_UINT,
               VSTR_TYPE_FMT_END);
  vstr_sc_fmt_add_vstr(s3->conf, "{Vstr:%p%zu%zu%u}");
  vstr_sc_fmt_add_ipv4_ptr(s3->conf, "{ipv4:%p}");
  vstr_sc_fmt_add_ipv6_ptr(s3->conf, "{ipv6:%p}");
  vstr_fmt_add(s3->conf, "{PID}", tst_usr_pid_cb, VSTR_TYPE_FMT_END);  
  vstr_fmt_add(s4->conf, "m", tst_usr_strerror, VSTR_TYPE_FMT_END);  

  /* output */
  
  vstr_add_fmt(s1, 0, "${Vstr:%p%u}", (void *)s2, 0);
  
  TST_B_TST(ret, 1, !VSTR_CMP_EQ(s1, 1, s1->len, s2, 1, s2->len));
  
  vstr_add_fmt(s3, 0, "$$ ${Vstr:%p%zu%zu%u} -- ${PID} $$",
               (void *)s2, 3, strlen(buf), 0);
  
  TST_B_TST(ret, 2, !VSTR_CMP_EQ(s3, 1, s3->len, s2, 1, s2->len));

#ifdef HAVE_POSIX_HOST
  do
  {
    struct in_addr  ipv4;
    struct in6_addr ipv6;
    
    srand(time(NULL) ^ getpid());
    
    ipv4.s_addr = rand();
    
    ipv6.s6_addr32[0] = rand();
    ipv6.s6_addr32[1] = rand();
    ipv6.s6_addr32[2] = rand();
    ipv6.s6_addr32[3] = rand();

    if (!tst_setup_inet_buf(&ipv4, &ipv6)) break;
    
    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "$-128{ipv4:%p}$128{ipv6:%p}", &ipv4, &ipv6);
    
    TST_B_TST(ret, 3, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));

    ipv4.s_addr &= 0xF00F;
    
    ipv6.s6_addr32[0] &= 0xF00F;
    ipv6.s6_addr32[1] &= 0x00FF;
    ipv6.s6_addr32[2] &= 0xFF00;
    ipv6.s6_addr32[3] &= 0x0FF0;

    if (!tst_setup_inet_buf(&ipv4, &ipv6)) break;
    
    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "$-128{ipv4:%p}$128{ipv6:%p}", &ipv4, &ipv6);
    
    TST_B_TST(ret, 4, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));

    ipv4.s_addr = 0xFFFF;
    
    ipv6.s6_addr32[0] = 0xFFFF;
    ipv6.s6_addr32[1] = 0xFFFF;
    ipv6.s6_addr32[2] = 0xFFFF;
    ipv6.s6_addr32[3] = 0xFFFF;

    if (!tst_setup_inet_buf(&ipv4, &ipv6)) break;
    
    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "$-128{ipv4:%p}$128{ipv6:%p}", &ipv4, &ipv6);
    
    TST_B_TST(ret, 5, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));

    ipv4.s_addr = 0;
    
    ipv6.s6_addr32[0] = 0;
    ipv6.s6_addr32[1] = 0;
    ipv6.s6_addr32[2] = 0;
    ipv6.s6_addr32[3] = 0;

    if (!tst_setup_inet_buf(&ipv4, &ipv6)) break;
    
    vstr_del(s3, 1, s3->len);
    vstr_add_fmt(s3, 0, "$-128{ipv4:%p}$128{ipv6:%p}", &ipv4, &ipv6);
    
    TST_B_TST(ret, 6, !VSTR_CMP_CSTR_EQ(s3, 1, s3->len, buf));
    
  } while (FALSE);
#endif

  tst_errno = ERANGE;
  
  vstr_del(s4, 1, s4->len);
  vstr_add_fmt(s4, 0, "%m");

  assert(tst_errno == ERANGE);
  
  TST_B_TST(ret, 7, !VSTR_CMP_CSTR_EQ(s4, 1, s4->len, strerror(tst_errno)));
  
  return (TST_B_RET(ret));
}
