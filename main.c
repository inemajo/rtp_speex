#include	"svc.h"

static FILE	*arecord;
static FILE	*aplay;
static bool_t	run = FALSE;
struct svc	config;

#define	SVC_RUNNING() (run == TRUE)
void
stophandler(int sig)
{
  run = FALSE;
}

void
svcerr(const char *func, const char *file, int line, int err)
{
  if (err == SVCERR_OPT)
    usage();
  else {
    fprintf(stderr, "svc ERROR: at %s() %s:%d err=%d\n",
	    func, file, line, err);
  }

  exit(EXIT_FAILURE);
}


FILE *
svc_popen(const char *rw, const char *format, ...)
{
  FILE		*result;
  char		popbuff[64]; /* The chain musn't be superior than 46 */
  va_list	ap;

  va_start(ap, format);

  if (vsnprintf(popbuff, sizeof(popbuff), format, ap) >= sizeof(popbuff)) {
    fprintf(stderr, "ERROR: %s(%s:%d): string is to long\n",
	    __FUNCTION__, __FILE__, __LINE__);
    return (NULL);
  }

  fprintf(stderr, "Popen: %s\n", popbuff);
  if ((result = popen(popbuff, rw)) == NULL) {
    return (NULL);
  }

  return (result);
}


/**
 * Record PCM from arecord (FILE *) and send to rtp
 */
int
svc_rec_send(svc_rtp_t *rtp, buff_t *buff, spx_frame_t *frame, spx_coder_t *coder)
{
  int	count;

  count = fread(frame->s, sizeof(spx_int16_t), frame->size, arecord);
  if (count == 0) /* End of stream exit  */
    kill(getpid(), SIGTERM);
  
  count = svc_speex_encodeframe(coder, frame, buff);
  return (svc_ortp_send(rtp, buff->s, count));
}

int
svc_recv_play(svc_rtp_t *rtp, buff_t *buff, spx_frame_t *frame, spx_coder_t *coder)
{
  int	count;
  int	have_more;

  have_more = 0;
  count = svc_ortp_recv(rtp, buff, &have_more);
  /* if have_more ... */
  if (count > 0) {
    if (!svc_speex_decodeframe(coder, (char *)buff->s, count, frame)) {
      fwrite(frame->s, sizeof(spx_int16_t), frame->size, aplay);
      fflush(aplay);
      return (160);
    }
    else
      fprintf(stderr, "speex_decode ERROR\n");
  }
  else if (count == -1)
    return (-1);

  /* else /\* count == 0 *\/ { */
  /*   usleep(16000); */
  /* } */
  return (0);
}

void
loop()
{
  spx_frame_t	frame;
  svc_rtp_t	*rtp;
  buff_t	buff;
  spx_coder_t	*encoder = NULL;
  spx_coder_t	*decoder = NULL;

  frame.size = FRAME_SIZE * (config.rate / 2000);
  frame.s = malloc(frame.size * sizeof(spx_int16_t));

  buff.size = frame.size; /* In the worst time speex no compress PCM */
  buff.s = malloc(sizeof(uint8_t) * buff.size);

  if (!frame.s || !buff.s) {
    fprintf(stderr, "Memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  if (!(config.action & OPT_TYPE_NOREC)) {
    encoder = svc_speex_new_coder(SPX_ENCODER, 8, 40);
    svc_speex_setsamplingrate(encoder, config.rate);
  }
  if (!(config.action & OPT_TYPE_NOPLAY)) {
    decoder = svc_speex_new_coder(SPX_DECODER, 8, 40);
    svc_speex_setsamplingrate(decoder, config.rate);
  }

  svc_ortp_init(); /* initialize ortp lib */

  if ((config.action & OPT_TYPE_BIN)) /* if action == bind */ {
    rtp = svc_ortp_bind(config.ip, config.port);
    while (svc_ortp_recv(rtp, &buff, NULL) <= 0) {
      rtp->timestamp += 160;
      fprintf(stderr, "Wait client...\n");
    }
  }
  else if ((config.action & OPT_TYPE_CON)) /* else if action == connect */
    rtp = svc_ortp_connect(config.ip, config.port);
  else
    exit(EXIT_FAILURE); /* no action specified exit failure */

  signal(SIGINT,stophandler); /* prevent ctrl+c  */
  signal(SIGTERM,stophandler); /* prevent program Terminating */
  run = TRUE; /* Stop handler replace run = TRUE by run = False */
  while (SVC_RUNNING()) { /* SVC_RUNNING is just a define for (run == TRUE) */

    if (!(config.action & OPT_TYPE_NOREC)) { /* if norec is disable */
      if (svc_rec_send(rtp, &buff, &frame, encoder) != -1)
	rtp->timestamp += 0;
    }
    else {
      rtp->timestamp += 0;
    }
      rtp->timestamp += 160;

    if (!(config.action & OPT_TYPE_NOPLAY)) { /* if noplay is disable */
      svc_recv_play(rtp, &buff, &frame, decoder);
    }

  }

  free(frame.s);

  if (arecord)
    fprintf(stderr, "close arecord: %d\n", pclose(arecord));
  if (aplay)
    fprintf(stderr, "close aplay: %d\n", pclose(aplay));

  if (encoder)
    svc_speex_del_coder(encoder);
  if (decoder)
    svc_speex_del_coder(decoder);

  svc_ortp_destroy(rtp);
  svc_ortp_exit();

  fprintf(stderr, "[done]!\n");
  exit(EXIT_SUCCESS);
}

int
main(int argc, char *argv[], char *env[])
{

  /* Default values  */
  config.rate = 8000;
  config.port = 40000;

  if (argv_parser(argc, argv, env))
    exit(EXIT_FAILURE);
  printf("Using sampling rate: %d\n", config.rate);

  if ((config.action & OPT_TYPE_HELP)) {
    usage();
    exit(EXIT_SUCCESS);
  }

  if (!(config.action & OPT_TYPE_NOREC)) {
    if ((config.action & OPT_TYPE_STDIN))
      arecord = stdin;
    else {
      if ((arecord = svc_popen("r", "arecord -f S16_LE -c1 -r%d -t raw --quiet",
			       config.rate)) == NULL)
	exit(EXIT_FAILURE);
    }
  }

  if (!(config.action & OPT_TYPE_NOPLAY)) {
    if ((config.action & OPT_TYPE_STDOUT))
      aplay = stdout;
    else {
      if ((aplay = svc_popen("w", "aplay -f S16_LE -c1 -r%d -t raw --quiet",
			     config.rate)) == NULL)
	exit(EXIT_FAILURE);
    }
  }

  loop();
  exit(EXIT_SUCCESS);
  return (0);
}
