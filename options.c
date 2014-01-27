#include		"svc.h"

extern struct svc	config;

static struct argv_opt_s opt_all[] = {

  { /* --connect, -c option  */
    .type = OPT_TYPE_ARG|OPT_TYPE_STR|OPT_TYPE_CON,
    .max_arg_len = 12+1,
    .str = config.ip,
    .int_v = NULL,
    .opt_long = "connect",
    .opt_short = "c",
    .never_with = OPT_TYPE_BIN,
    .ifnotset_need = OPT_TYPE_BIN|OPT_TYPE_HELP,
    .opt_used = 0,
    .desc = "[ip]\tConnect to an other svc who bind"
  },
  
  { /* --bind, -b option */
    .type = OPT_TYPE_ARG|OPT_TYPE_STR|OPT_TYPE_BIN,
    .max_arg_len = 12+1,
    .str = config.ip,
    .int_v = NULL,
    .opt_long = "bind",
    .opt_short = "b",
    .never_with = OPT_TYPE_CON,
    .ifnotset_need = OPT_TYPE_CON|OPT_TYPE_HELP,
    .opt_used = 0,
    .desc = "[ip]\tWait connection from other svc who connect"
  },

  {
    .type = OPT_TYPE_ARG|OPT_TYPE_INT,
    .max_arg_len = 5+1,
    .str = config.str_port,
    .int_v = &config.port,
    .opt_long = "port",
    .opt_short = "p",
    .never_with = 0,
    .ifnotset_need = 0,
    .opt_used = 0,
    .desc = "[port]\tchoose port to bind/connect"
  },

  {
    .type = OPT_TYPE_NOPLAY,
    .max_arg_len = 0,
    .str = NULL,
    .int_v = NULL,
    .opt_long = "noplay",
    .opt_short = "np",
    .never_with = OPT_TYPE_NOREC|OPT_TYPE_BIN,
    .ifnotset_need = 0,
    .opt_used = 0,
    .desc = "don't play the sound recv, impossible to use with --bind option"
  },

  {
    .type = OPT_TYPE_NOREC,
    .max_arg_len = 0,
    .str = NULL,
    .int_v = NULL,
    .opt_long = "norec",
    .opt_short = "nr",
    .never_with = OPT_TYPE_NOPLAY|OPT_TYPE_CON,
    .ifnotset_need = 0,
    .opt_used = 0,
    .desc = "don't send the sound recorded, impossible to use with \
--connect option"
  },

  {
    .type = OPT_TYPE_STDOUT,
    .max_arg_len = 0,
    .str = NULL,
    .int_v = NULL,
    .opt_long = "stdout",
    .opt_short = "o",
    .never_with = OPT_TYPE_NOPLAY,
    .ifnotset_need = 0,
    .opt_used = 0,
    .desc = "Print to stdout PCM stream"
  },

  {
    .type = OPT_TYPE_STDIN,
    .max_arg_len = 0,
    .str = NULL,
    .int_v = NULL,
    .opt_long = "stdin",
    .opt_short = "i",
    .never_with = OPT_TYPE_NOREC,
    .ifnotset_need = 0,
    .opt_used = 0,
    .desc = "Read to stdin PCM stream"
  },

/*   { */
/*     .type = OPT_TYPE_ARG|OPT_TYPE_INT, */
/*     .max_arg_len = 6+1, */
/*     .str = config.str_rate, */
/*     .int_v = &config.rate, */
/*     .opt_long = "rate", */
/*     .opt_short = "r", */
/*     .never_with = 0, */
/*     .ifnotset_need = 0, */
/*     .opt_used = 0, */
/*     .desc = "[RATE] Sampling rate in Hertz. The default rate is 8000 Hertz.\ */
/* !!Warning!! There is no sampling verification, do not use this function\ */
/*  except you know what you do." */
/*   }, */

  {
    .type = OPT_TYPE_HELP,
    .max_arg_len = 0,
    .str = NULL,
    .int_v = NULL,
    .opt_long = "help",
    .opt_short = "h",
    .never_with = UINTMAX_MAX & !OPT_TYPE_HELP,
    .ifnotset_need = 0, 0, "display this help"
  },

  {0, 0, NULL, NULL, NULL, NULL, 0, 0, 0, NULL}
};

int
argv_parser(int argc, char *argv[], char *env[])
{
  int		i;
  int		j;
  bool_t	is_short;

  config.action = 0;
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] != '-')
	return (1); /* error not an option  */
      is_short = argv[i][1] != '-' ? TRUE : FALSE;
      for (j = 0; opt_all[j].opt_long != NULL; j++)
	{
	  if ((is_short && !strcmp(opt_all[j].opt_short, argv[i] + 1)) ||
		!strcmp(opt_all[j].opt_long, argv[i] + 2))
	    {
	      if (opt_all[j].opt_used == 1)
		SVCERR(SVCERR_OPTARDY);
	      opt_all[j].opt_used = 1;
	      if ((opt_all[j].type & OPT_TYPE_ARG))
		{
		  if (++i == argc) {
		    fprintf(stderr,
			    "You must put an argument for option '%s'\n",
			    opt_all[j].opt_long);
		    return (1); /* no arg  */
		  }
		  if (strlen(argv[i]) > opt_all[j].max_arg_len) {
		    fprintf(stderr, "Argument of opt '%s' is too long\n",
			    opt_all[j].opt_long);		    
		    return (1); /* len of opt too big  */
		  }
		  strcpy(opt_all[j].str, argv[i]);
		  if ((opt_all[j].type & OPT_TYPE_INT))
		    *opt_all[j].int_v = atoi(opt_all[j].str);
		}
	      config.action |= opt_all[j].type;
	      break;
	    } /* !strcmp(...) */
	} /* argv[i][0] == '-'  */
      if (opt_all[j].opt_long == NULL) {
	fprintf(stderr, "Unknow '%s' option...\n",
		argv[i]);
	return (1); /* unknow opt  */
      }
    }

  /* test if options ok */
  for (j = 0; opt_all[j].opt_long != NULL; j++)
    {
      if (opt_all[j].opt_used && 
	  (opt_all[j].never_with & config.action)) {
	/* fprintf(stderr, "Error in arguments, used '%d' but never_with '%d'\n", */
	/* 	opt_all[j].opt_used, opt_all[j].never_with & config.action); */
	SVCERR(SVCERR_OPT);
      }
      if (!opt_all[j].opt_used &&
	  opt_all[j].ifnotset_need &&
	  !(opt_all[j].ifnotset_need & config.action))
	{
	  /* fprintf(stderr, "If opt '%s' is not set need: '%d'\n",  */
	  /* 	  opt_all[j].opt_long, opt_all[j].ifnotset_need); */
	  SVCERR(SVCERR_OPT);
	}
    } 
  return (0);
}

void
usage()
{
  int		i;
  int		strlen_biggest_short;
  int		strlen_biggest_long;
  int		tmp;

  fprintf(stderr, "Usage: ./svc [OPTION]...\n\
send or receive audio stream of PCM format (only S16_LE/8kHz rate) compressed\n\
with the speex codec and encapsuled into rtp packet\n\
\n\
Options are:\n");
  strlen_biggest_short = 0;
  strlen_biggest_long = 0;
  for (i = 0; opt_all[i].opt_long; i++) {
    tmp = strlen(opt_all[i].opt_long);
    if (tmp > strlen_biggest_long)
      strlen_biggest_long = tmp;
    tmp = strlen(opt_all[i].opt_short);
    if (tmp > strlen_biggest_short)
      strlen_biggest_short = tmp;
  }

  i = 0;
  while (opt_all[i].opt_long) {
    fprintf(stderr, "  -%s,", opt_all[i].opt_short);
    tmp = strlen_biggest_short - strlen(opt_all[i].opt_short);
    while (tmp--)
      fprintf(stderr, " ");

    fprintf(stderr, " --%s", opt_all[i].opt_long);
    tmp = strlen_biggest_long - strlen(opt_all[i].opt_long);
    while (tmp--)
      fprintf(stderr, " ");

    fprintf(stderr, " %s\n", opt_all[i].desc);
    ++i;
  }
}
