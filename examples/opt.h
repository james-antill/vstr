#ifndef OPT_H
#define OPT_H

#include <getopt.h>
#include <string.h>

static int opt_toggle(int val, const char *opt)
{
  if (!opt)
  { val = !val; }
  else if (!strcasecmp("true", optarg))  val = 1;
  else if (!strcasecmp("1", optarg))     val = 1;
  else if (!strcasecmp("false", optarg)) val = 0;
  else if (!strcasecmp("0", optarg))     val = 0;

  return (val);
}

#define OPT_TOGGLE_ARG(val) (val = opt_toggle(val, optarg))

#endif
