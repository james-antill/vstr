/* This is a _simple_ program to lookup a hostname via. gethostbyname().
 *
 * This shows how easy it is to use custom format specifiers, to make your code
 * easier to write and maintain.
 *
 * This file is more commented than normal code, so as to make it easy to follow
 * while knowning almost nothing about Vstr or Linux IO programming.
 */

#define VSTR_COMPILE_INCLUDE 1 /* make Vstr include it's own system headers */
#include <vstr.h>

#include <errno.h>

#include <sys/socket.h>
#include <netdb.h>

/* Creates a Vstr string, prints an error message to it and dies
 * don't really need to bother free'ing things as it's only called on a
 * major error -- but it's a good habit */
static void DIE(const char *msg, ...)
{
  Vstr_base *err = vstr_make_base(NULL);

  if (err)
  {
    va_list ap;
    
    va_start(ap, msg);
    vstr_add_vsysfmt(err, 0, msg, ap);
    va_end(ap);

    while (err->len)
      if (!vstr_sc_write_fd(err, 1, err->len, 2, NULL))
      {
        if ((errno != EAGAIN) && (errno != EINTR))
          break; /* don't recurse */
      }
  }
  vstr_free_base(err);

  exit (EXIT_FAILURE);
}




int main(int argc, char *argv[])
{
  Vstr_base *s1 = NULL;
  struct hostent *hp = NULL; /* data from the resolver library */
  
  /* init the Vstr string library, note that if this fails we can't call DIE
   * or the program will crash */
  if (!vstr_init())
    exit (EXIT_FAILURE);

  /* create a Vstr string for doing the IO with,
   * use the default configuration */  
  s1 = vstr_make_base(NULL);
  if (!s1)
    errno = ENOMEM, DIE("vstr_make_base: %m\n");

  
  /* setup the pre-written custom format specifier for IPv4 addresses,
   */
  vstr_cntl_conf(s1->conf, VSTR_CNTL_CONF_SET_FMT_CHAR_ESC, '$');
  vstr_sc_fmt_add_ipv4_ptr(s1->conf, "{IPv4:%p}");


  if (argc != 2) /* if a filename isn't given output an message to stderr */
  {
    size_t pos = 0;
    size_t len = 0;
    
    /* add another format specifier for printing Vstr strings */
    vstr_sc_fmt_add_vstr(s1->conf, "{Vstr:%p%zu%zu%u}");

    /* find the program name ...
     * putting it at the begining of the Vstr string */
    VSTR_ADD_CSTR_PTR(s1, 0, argc ? argv[0] : "lookup_ip");
    vstr_sc_basename(s1, 1, s1->len, &pos, &len);

    /* add a format line to the Vstr string, including the program name
     * which is at the begining of this Vstr string itself */
    len = vstr_add_fmt(s1, s1->len, " %s ${Vstr:%p%zu%zu%u} %s\n",
                       "Format:",
                       s1, pos, len, 0,
                       "<hostname>");
    
    vstr_del(s1, 1, s1->len - len); /* delete the original program name */
    
    /* loop until all data is output */
    while (s1->len)
      if (!vstr_sc_write_fd(s1, 1, s1->len, 2, NULL))
      {
        if ((errno != EAGAIN) && (errno != EINTR))
          DIE("write: %m\n");
      }
    
    exit (EXIT_FAILURE);
  }


  /* lookup the hostname */
  hp = gethostbyname(argv[1]);

  
  /* just print the relevant data.... Note that nothing complicated needs to
   * be done to print the IPv4 address, the customer formatter takes care of
   * it */
  if (!hp)
    vstr_add_fmt(s1, 0, " The hostname '%s' couldn't be found, because: %s.\n",
                 argv[1], strerror(h_errno));
  else if (hp->h_addrtype == AF_INET)
    vstr_add_fmt(s1, 0, " The hostname '%s' has an "
                 "IPv4 address of \"${IPv4:%p}\".\n", hp->h_name,
                 hp->h_addr_list[0]);
  else
    vstr_add_fmt(s1, 0, " The hostname '%s' has an address type that "
                 "isn't IPv4.\n",
                 hp->h_name);
    

  
  /* loop until all data is output */
  while (s1->len)
    if (!vstr_sc_write_fd(s1, 1, s1->len, 1, NULL))
    {
      if ((errno != EAGAIN) && (errno != EINTR))
        DIE("write: %m\n");
    }
  
  
  /* These next two calls are only really needed to make valgrind happy,
   * but that does help in that any memory leaks are easier to see.
   */
  
  /* free s1, this also free's out custom Vstr configuration */
  vstr_free_base(s1);
  
  /* "exit" Vstr, this free's all internal data and no library calls apart from
   * vstr_init() should be called after this.
   */
  vstr_exit();
  
  exit (EXIT_SUCCESS);
}
