#include "opt.h"

int opt_toggle(int val, const char *opt)
{
  if (!opt)
  { val = !val; }
  else if (!strcasecmp("true", optarg))  val = 1;
  else if (!strcasecmp("1", optarg))     val = 1;
  else if (!strcasecmp("false", optarg)) val = 0;
  else if (!strcasecmp("0", optarg))     val = 0;

  return (val);
}

/* get program name ... but ignore "lt-" libtool prefix */
const char *opt_program_name(const char *argv0, const char *def)
{
  if (argv0)
  {
    if ((def = strrchr(argv0, '/')))
      ++def;
    else
      def = argv0;
    
    if ((strlen(def) > strlen("lt-")) && !memcmp("lt-", def, strlen("lt-")))
      def += 3;
  }
  
  return (def);
}

