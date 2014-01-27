#ifndef		__SVC_H__
#define		__SVC_H__

/* The value 2 or greater additionally exposes definitions for POSIX.2-1992. */
#define _POSIX_C_SOURCE 2
#define _BSD_SOURCE

/**
 * *********************
 * common (*.c)
 *
 * *********************
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SVCERR(errtype) svcerr(__FUNCTION__, __FILE__, __LINE__, errtype)
#define SVCERR_NOERR	0
#define SVCERR_OPTARDY	1
#define SVCERR_OPT	2

struct svc {

  unsigned int	action;
  char	ip[12+1]; /* what the size in ipv6 ? */
  int	rate;
  char	str_rate[6+1]; /* max=192Khz */
  int	port;
  char	str_port[5+1];
};

void
svcerr(const char *func, const char *file, int line, int err);
void
usage();

typedef struct
{
  uint8_t	*s;
  size_t	size;
}		buff_t;

/**
 * *********************
 * speex lib (speex.c)
 *
 * *********************
 */

#include <speex/speex.h>
#define		SPX_ENCODER	1
#define		SPX_DECODER	2
typedef struct
{
  void		*state;
  SpeexBits	bits;
  unsigned int	type;
}		spx_coder_t;

typedef struct
{
  spx_int16_t	*s;
  size_t	size;
}		spx_frame_t;

/* FRAME_SIZE corresponding to the minimum size of a sample of PCM in
   spx_int16_t (2bytes). The minimum size is equivalent to 2Khz rate 
   (the minimum rate of alsa-utils) */

/* ref: http://lists.xiph.org/pipermail/speex-dev/2003-October/001932.html */
#define FRAME_SIZE 40 

int
svc_speex_encodeframe(spx_coder_t *coder, spx_frame_t *frame, buff_t *buff);
int
svc_speex_decodeframe(spx_coder_t *coder, char *cbits, size_t nb_bytes, 
		      spx_frame_t *frame);
int
svc_speex_setquality(spx_coder_t *coder, int quality);
int
svc_speex_setenhancement(spx_coder_t *coder, int enhancement);
int
svc_speex_setsamplingrate(spx_coder_t *coder, int rate);
spx_coder_t *
svc_speex_new_coder(int type, int quality, int enhancement);
void
svc_speex_del_coder(spx_coder_t *coder);


/**
 * *********************
 * ortp lib (rtp.c)
 *
 * *********************
 */

#include <ortp/ortp.h>

typedef struct
{
  RtpSession	*session;
  char		*ip;
  uint16_t	port;
  int		timestamp;
}		svc_rtp_t;

int
svc_ortp_init();
int
svc_ortp_exit();
int
svc_ortp_destroy(svc_rtp_t *rtp);
svc_rtp_t *
svc_ortp_bind(char *ip, unsigned short port);
svc_rtp_t *
svc_ortp_connect(char *ip, unsigned short port);
int
svc_ortp_recv(svc_rtp_t *rtp, buff_t *buff, int *have_more);
int
svc_ortp_send(svc_rtp_t *rtp, void *data, size_t count);



/**
 * *********************
 * options (options.c)
 *
 * *********************
 */

struct argv_opt_s {
  
  int	type;
  int	max_arg_len;
  char	*str;
  int	*int_v;
  char	*opt_long;
  char	*opt_short; /* char * pour utf8  */
  
  int	never_with;
  int   ifnotset_need;
  int	opt_used;

  char	*desc;
};

#define OPT_TYPE_ARG	1<<0
#define OPT_TYPE_STR	1<<1
#define OPT_TYPE_INT	1<<2
#define OPT_TYPE_CON	1<<3
#define OPT_TYPE_BIN	1<<4
#define OPT_TYPE_NOPLAY	1<<5
#define OPT_TYPE_NOREC	1<<6
#define OPT_TYPE_STDIN	1<<7
#define OPT_TYPE_STDOUT	1<<8
#define OPT_TYPE_HELP	1<<9

int
argv_parser(int argc, char *argv[], char *env[]);

#endif
