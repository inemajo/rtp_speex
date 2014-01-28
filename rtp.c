#include	"svc.h"

static int		rtp_started = 0;

/**
 * Callback of rtp_session_signal_connect()
 * ...See svc_ortp_bind() or svc_ortp_connect()
 */
static int
svc_ortp_neterror(RtpSession *session, char *desc, int value)
{
  fprintf(stderr, "ortp network error (from session:%p) %d: %s\n", 
	  (void *)session, value, desc);
  return (0);
}

/**
 * Same as svc_ortp_neterror()
 */
static int
rtpsig_ssrc_changed(RtpSession *session)
{
  fprintf(stderr, "ssrc changed session=%p\n", (void *)session);
  return (0);
}




/**
 * ortp lib need global init
 * 
 **/
int
svc_ortp_init()
{
  if (rtp_started)
    return (1);

  fprintf(stderr, "init ortp lib...\n");
  rtp_started = 1;

  ortp_init(); /* init ortp lib */

  ortp_scheduler_init(); /* Need for enable rtp_session_set_blocking_mode() */

  /* It's possible to add flags "ORTP_MESSAGE,ORTP_DEBUG" */
  ortp_set_log_level_mask(ORTP_WARNING|ORTP_ERROR);

  return (0);
}

/**
 * ortp lib may be gracefully uninitialized
 */
int
svc_ortp_exit()
{
  if (!rtp_started)
    return (1);

  rtp_started = 0;

  ortp_exit(); /* This function uninitialize ortplib and ortp_schelduer. */
  return (0);
}

/**
 * destroy rtp session
 */
int
svc_ortp_destroy(svc_rtp_t *rtp)
{
  rtp_session_destroy(rtp->session);

  free(rtp->ip);
  free(rtp);
  return (0);
}

/**
 * Create rtp session and bind ip:port
 */
svc_rtp_t *
svc_ortp_bind(char *ip, unsigned short port)
{
  svc_rtp_t	*rtp;
  RtpSession	*session;

  /* You may use RTP_SESSION_SENDONLY or RTP_SESSION_RECVONLY 
     instead of RTP_SESSION_SENDRECV */
  if (!(session=rtp_session_new(RTP_SESSION_SENDRECV)))
    return (NULL);

  rtp = malloc(sizeof(svc_rtp_t));
  rtp->session = session;
  rtp->ip = strdup(ip);
  rtp->port = port;  
  rtp->timestamp = 0;

  /* Need for blocking mode, you must call ortp_scheduler_init() before. */
  rtp_session_set_scheduling_mode(rtp->session, 1);

  /* Block rtp_session_recv_with_ts/rtp_session_send_with_ts
     until timestamp timeout.
   */
  rtp_session_set_blocking_mode(rtp->session, TRUE);

  fprintf(stderr, "binding to %s port %d\n", ip, port);
  rtp_session_set_local_addr(rtp->session, ip, port); /* Set bind ip:port */

  /* Reject all receive packet from other address than the setted with
     rtp_session_set_remote_addr() function */
  rtp_session_set_connected_mode(rtp->session, TRUE);
  /* The ip:port of the first rtp packet receive is setted with
    rtp_session_set_remote_addr() function */
  rtp_session_set_symmetric_rtp(rtp->session,TRUE);

  /* jitter is used for stream fluctuation  */
  rtp_session_enable_adaptive_jitter_compensation(rtp->session, TRUE);
  rtp_session_set_jitter_compensation(rtp->session, 20);

  /* payload 97 is speex codec with 8000Hz rate  */
  /* BUG: If i put 97 in playload it doesn't work?(0=pcm 16LE/8Khz) */
  rtp_session_set_payload_type(rtp->session, 0);

  rtp_session_signal_connect(rtp->session, "ssrc_changed", 
			     (RtpCallback)rtpsig_ssrc_changed, 0);
  rtp_session_signal_connect(rtp->session, "ssrc_changed", 
			     (RtpCallback)rtp_session_reset, 0);
  rtp_session_signal_connect(rtp->session, "network_error",
			     (RtpCallback)svc_ortp_neterror, 0);

  return (rtp);
}

svc_rtp_t *
svc_ortp_connect(char *ip, unsigned short port)
{

  svc_rtp_t	*rtp;
  RtpSession	*session;
    
  if (!(session = rtp_session_new(RTP_SESSION_SENDRECV)))
      return (NULL);

  rtp = malloc(sizeof(svc_rtp_t));
  rtp->session = session;
  rtp->ip = strdup(ip);
  rtp->port = port;
  rtp->timestamp = 0;

  rtp_session_set_scheduling_mode(rtp->session, 1);
  rtp_session_set_blocking_mode(rtp->session, TRUE);

  rtp_session_set_connected_mode(rtp->session, TRUE);

  fprintf(stderr, "connecting to %s port %d\n", rtp->ip, rtp->port);
  fprintf(stderr, "rtp_session_set_remote_addr:%d\n",
	  rtp_session_set_remote_addr(rtp->session, rtp->ip, rtp->port));

  rtp_session_set_payload_type(rtp->session, 0);
  return (rtp);
}

int
svc_ortp_recv(svc_rtp_t *rtp, buff_t *buff, int *have_more)
{
  int err;
  int in_have_more;

  err = rtp_session_recv_with_ts(rtp->session, buff->s, buff->size, 
				 rtp->timestamp,
				 have_more ? have_more : &in_have_more);

  /* printf("%d = rtp_session_recv_with_ts(%p, %p, %d, %d)\n", err, (void *)rtp->session, buff->s, (int)buff->size, rtp->timestamp); */
  return (err);
}

int
svc_ortp_send(svc_rtp_t *rtp, void *data, size_t count)
{
  int	err;

  err = rtp_session_send_with_ts(rtp->session, data, count, 
				 rtp->timestamp);
  /* printf("%d = rtp_session_send_with_ts(%p, %p, %d, %d)\n", err, (void *)rtp->session, data, (int)count, rtp->timestamp); */
  return (err);
}
