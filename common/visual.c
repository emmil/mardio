#include "libc.h"
#include "cmd.h"
#include "iobuffer.h"
#include "event.h"
#include "platform.h"
#include "snd.h"

#include "kiss_fft.h"

#include "visual.h"

#include <stdlib.h>

#include <math.h>

///////////////////////////////////////

#ifdef	CONFIG_VISUAL_DEBUG
#define	vis_debug(...)	xprintf(__VA_ARGS__)
#else
#define	vis_debug(...)
#endif

#define	VIS_LINE

#define	REFRESH_DELAY	200

#define	SQ(x) (x*x)

#define	FFT_N		1024

struct sample_s {
	int16_t	left;
	int16_t	right;
};

enum vis_state_e {
	VIS_UNKNOWN = 0,
	VIS_OFF,
	VIS_WAVE,
	VIS_FFT,
	VIS_BAR,
};

struct complex16_s {
	int16_t	real;
	int16_t	complx;
};

static struct {
	enum vis_state_e	state;
	int			time_delay;
	uint32_t		time_stamp;
	struct lcd_cfg_s 	lcd_cfg;
	kiss_fft_cfg		kiss_cfg;
} vs;

///////////////////////////////////////

void *kiss_malloc(size_t size) {
	xprintf(" kiss_malloc: %dbytes\n", size);
	return(malloc(size));
}

void kiss_free(void *ptr) {
	xprintf("kiss_free 0x%x\n", ptr);
}

///////////////////////////////////////

void vis_line (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	float		l, p;
	uint16_t	x, y;
	int16_t		vx, vy;

	vx = x2 - x1;
	vy = y2 - y1;
	l = sqrt(SQ(x1 + x2) + SQ(y1 + y2));
	for (p = 0; p < l; p++) {
		x = x1 + vx*(p/l);
		y = y1 + vy*(p/l);

		lcd_putpixel(x, y, 1);
	}
}

void pcm_merge_channels_to_left(struct output_s *out) {
	struct sample_s	*sample;
	uint16_t	count;
	uint16_t	i;

	if (out == NULL)
		return;

	if (out->channels != 2 || out->used == 0 || out->rate == 0)
		return;

	count = (out->used / (out->channels * 2));
	for (i = 0, sample = (struct sample_s *)out->data; i < count; i++, sample++) {
		sample->left = ((sample->left + sample->right)/2);
		sample->right = 0;
	}
}

///////////////////////////////////////

void vis_calc_dft(struct output_s *buff) {
	struct output_s	work;
	struct sample_s	*in = (struct sample_s *)&work.data;
	struct sample_s *out = (struct sample_s *)buff->data;
	float		pi = 3.1415926535;
	float		angle;
	uint16_t	k, n, N;

	uc_memcpy (&work, buff, sizeof(work));

	N = 1024;

	for (k = 0; k < N; k++) {
		out[k].left = 0;
		out[k].right = 0;

		for (n = 0; n < N; n++) {
			angle = ((2*pi*n*k)/N);
			out[k].left += in[n].left*cos(angle) + in[n].right*sin(angle);
			out[k].right += in[n].left*sin(angle) + in[n].right*cos(angle);
		}
	}
}


///////////////////////////////////////

void vis_wave(struct output_s *buff) {
	struct sample_s *psample = (struct sample_s*)buff->data;
	uint16_t height = vs.lcd_cfg.height;
	uint16_t width = vs.lcd_cfg.width;
	uint16_t center = vs.lcd_cfg.height/2;
	int16_t	ratio = (65536/(height));
	int16_t sample;
	int16_t i;

#ifdef	VIS_LINE
	int16_t old_sample, old_i;
#endif

	lcd_clean_screen();
	pcm_merge_channels_to_left (buff);

	for (i = 0; i < width; i+=1, psample++) {
		sample = (psample->left/ratio) + center;

#ifdef	VIS_LINE
		if (i != 0)
			vis_line(old_i, old_sample, i, sample);

		old_sample = sample;
		old_i = i;
#else
		lcd_putpixel(i, sample, 1);
#endif

		lcd_putpixel(i, center, 1);
	}

	lcd_update_screen();
}

void vis_fft_01(struct output_s *buff) {
	struct sample_s *psample;
	uint16_t width = vs.lcd_cfg.width;
	uint16_t height = vs.lcd_cfg.height;
	uint16_t step = 512/width;
	uint16_t sample;
	int16_t ratio = 65536/height;
	int16_t i;

	int16_t	old_i;
	int16_t old_sample;

	if (buff->used < buff->size)
		return;

	pcm_merge_channels_to_left (buff);
	vis_calc_dft(buff);

	lcd_clean_screen();

	psample = (struct sample_s*)buff->data;
	for (i = 0; i < width; i++) {
		sample = (sqrt(SQ(psample[i].left) + SQ(psample[i].right)));
	}

	psample = (struct sample_s*)buff->data;
	for (i = 0; i < width; i+= step) {

		sample = psample[i].left/ratio;
		sample = height - sample;

		if (i != 0)
			vis_line(old_i, old_sample, i, sample);

		old_sample = sample;
		old_i = i;
	}

	lcd_update_screen();
}

void vis_fft(struct output_s *buff) {
	struct sample_s *psample;
	uint16_t width = vs.lcd_cfg.width;
	uint16_t height = vs.lcd_cfg.height;
	int16_t		N = 1024;
//	uint16_t 	ratio = ((65536)/height*128);
	uint16_t	ratio = 1;
	kiss_fft_cpx	in[N];
	kiss_fft_cpx	out[N];
	uint16_t	step = ((N/2)/width);
	uint16_t	y;
	uint16_t	x;
	int16_t		i;
	int16_t		old_x = 0;
	uint16_t	old_y = 0;

	if (vs.kiss_cfg == NULL)
		return;

	pcm_merge_channels_to_left (buff);

//	xprintf ("in + out + cfg: %d\n", sizeof(in) + sizeof(out)); // + sizeof(struct kiss_fft_state));

	psample = (struct sample_s*)buff->data;
	for (i = 0; i < N; i++) {
		in[i].r = psample[i].left;
		in[i].i = 0;
	}

	kiss_fft(vs.kiss_cfg, in, out);

	lcd_clean_screen();
	for (i = 0, x = 0; i < N/2; i+=step, x++) {
		y = height - (sqrt(SQ(out[i].r) + SQ(out[i].i))/ratio);

		vis_line (old_x, old_y, x, y);
//printf ("y: %d\n", y);

		if (x != 0) {
			old_x = x;
			old_y = y;
		}
	}
	lcd_update_screen();
}

void vis_bar(struct output_s *buff) {
}

///////////////////////////////////////

void vis_init(void) {

	vis_debug("%s", __func__);

	uc_memset(&vs, 0, sizeof (vs));

	vs.state = VIS_OFF;
	vs.time_delay = REFRESH_DELAY;

	lcd_get_cfg(&vs.lcd_cfg);

	if (vs.lcd_cfg.bpp == 0) {
		vis_debug(": The is no LCD, no visualisation possible.");
		goto exit;
	}

	vs.kiss_cfg = kiss_fft_alloc (FFT_N, 0, NULL, NULL);
	if (vs.kiss_cfg == NULL) {
		vis_debug(": kiss allocation failed.");
		goto exit;
	}
exit:
	vis_debug("\n");
}

void vis_poll(void) {
	struct output_s		*buff = NULL;

	if (vs.lcd_cfg.bpp == 0)
		return;

	if (vs.state == VIS_OFF || vs.state == VIS_UNKNOWN)
		return;

	if (vs.time_stamp > get_local_time())
		return;

	if (o_used() == 0)
		return;

	buff = o_peek();

	/* Paranoid check */
	if (buff == NULL) {
		xprintf ("%s: Got data and the buffer is null??\n");
		event_push(E_PL_STOP, 0, NULL);
		return;
	}

	/* Bogus frame */
	if (buff->rate == 0 || buff->channels == 0 || buff->size == 0)
		return;

	switch (vs.state) {
	case VIS_WAVE :
		vis_wave(buff);
		break;
	case VIS_FFT :
		vis_fft(buff);
		break;
	case VIS_BAR :
		vis_bar(buff);
		break;
	default :
		xprintf("%s: How did I got here?!\n", __func__);
		event_push(E_PL_STOP, 0, NULL);
	}

	vs.time_stamp = get_local_time() + vs.time_delay;
}

void vis_done(void) {

	if (vs.kiss_cfg != NULL) {
		kiss_free(vs.kiss_cfg);
	}

	vis_debug("%s\n", __func__);
}

///////////////////////////////////////

void cmd_visual(void) {
	int 		count = cmd_Argc();
	int		cmd_len, param_len;
	int		new_delay;
	const char	*cmd;
	const char	*param;

//	xprintf ("count: %d\n", count);

	if (count == 1) {

		if (vs.lcd_cfg.bpp == 0)
			xprintf("There is no LCD to draw to, settings will have no impact.\n\n");

		xprintf("Visualisation ");
		switch(vs.state) {

		case VIS_UNKNOWN :
			xprintf("is not initialized.\n");
			break;

		case VIS_OFF :
			xprintf("is switched off.\n");
			break;

		case VIS_WAVE :
			xprintf("is set to wave.\n");
			break;

		case VIS_FFT :
			xprintf("is set to FFT.\n");
			break;

		case VIS_BAR :
			xprintf("is set to bar.\n");
			break;
		}

		xprintf ("Refresh delay is %d milliseconds.\n", vs.time_delay);

		goto exit;
	}

	cmd = cmd_Argv(1);
	cmd_len = uc_strlen(cmd);

	if (!cmd_len)
		return;

	if (!uc_memcmp(cmd, "help", cmd_len)) {
		xprintf ("Possible options are off, wave, fft, bar, timer, reset.\n");
	} else

	if (!uc_memcmp(cmd, "reset", cmd_len)) {
		vis_init();
	} else

	if (!uc_memcmp(cmd, "off", cmd_len)) {
		vs.state = VIS_OFF;
		xprintf ("Visual switched off.\n");
	} else

	if (!uc_memcmp(cmd, "wave", cmd_len)) {
		vs.state = VIS_WAVE;
		xprintf ("Visual set to wave.\n");
	} else

	if (!uc_memcmp(cmd, "fft", cmd_len)) {
		vs.state = VIS_FFT;
		xprintf ("Visual set to fft.\n");
	} else

	if (!uc_memcmp(cmd, "bar", cmd_len)) {
		vs.state = VIS_BAR;
		xprintf ("Visual set to bar.\n");
	}

	if (!uc_memcmp(cmd, "timer", cmd_len)) {
		if (count == 2) {
			xprintf ("Refresh rate is set to %d milliseconds.\n", vs.time_delay);
		} else {

		param = cmd_Argv(2);
		param_len = uc_strlen(param);

		if (!param_len)
			return;

		new_delay = q3_atoi(param);

		vs.time_delay = new_delay;
		xprintf ("Refresh rate set to %d milliseconds.\n", vs.time_delay);

		}
	}

exit:
	xprintf("\n");
}

