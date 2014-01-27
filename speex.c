#include		"svc.h"

int
svc_speex_encodeframe(spx_coder_t *coder, spx_frame_t *frame, buff_t *buff)
{
  int			nbbytes;

  speex_bits_reset(&coder->bits);
  if (!speex_encode_int(coder->state, frame->s, &coder->bits))
    return (0);
  nbbytes = speex_bits_write(&coder->bits, (char *)buff->s, buff->size);
  return (nbbytes);
}

int
svc_speex_decodeframe(spx_coder_t *coder, char *cbits, size_t nb_bytes, 
		      spx_frame_t *frame)
{
  int	result;

  speex_bits_reset(&coder->bits);
  speex_bits_read_from(&coder->bits, cbits, nb_bytes);
  if ((result = speex_decode_int(coder->state, &coder->bits, frame->s)))
    return (result);
  return (0);
}

int
svc_speex_setquality(spx_coder_t *coder, int quality) {
  if ((coder->type & SPX_ENCODER)) {
    return (speex_encoder_ctl(coder->state, SPEEX_SET_QUALITY, &quality));
  }

  return (speex_decoder_ctl(coder->state, SPEEX_SET_QUALITY, &quality));
}

int
svc_speex_setsamplingrate(spx_coder_t *coder, int rate) {
  if ((coder->type & SPX_ENCODER)) {
    return (speex_encoder_ctl(coder->state, SPEEX_SET_SAMPLING_RATE, &rate));
  }

  return (speex_decoder_ctl(coder->state, SPEEX_SET_SAMPLING_RATE, &rate));
}


int
svc_speex_setenhancement(spx_coder_t *coder, int enhancement) {
  if ((coder->type & SPX_ENCODER)) {
    return (speex_encoder_ctl(coder->state, SPEEX_SET_ENH, &enhancement));
  }

  return (speex_decoder_ctl(coder->state, SPEEX_SET_ENH, &enhancement));
}

spx_coder_t *
svc_speex_new_coder(int type, int quality, int enhancement) {

  spx_coder_t	*coder;

  coder = malloc(sizeof(spx_coder_t));
  coder->type = type;

  if ((type & SPX_ENCODER))
    coder->state = speex_encoder_init(&speex_nb_mode);
  else
    coder->state = speex_decoder_init(&speex_nb_mode);

  speex_bits_init(&coder->bits);

  svc_speex_setquality(coder, quality);
  svc_speex_setenhancement(coder, enhancement);

  return (coder);
}

void
svc_speex_del_coder(spx_coder_t *coder)
{
  speex_bits_destroy(&coder->bits);
  if ((coder->type & SPX_ENCODER))
    speex_encoder_destroy(coder->state);
  else
    speex_decoder_destroy(coder->state);

  free(coder);
  return ;
}
