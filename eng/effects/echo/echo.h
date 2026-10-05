#ifndef ECHO_H
#define ECHO_H


#define ECHO_MAX_DELAY_MS 2000.0f
/*
delay_ms	80–600	2000		Delays between 1–50 ms sound like a metallic "comb" filtering effect rather than a distinct echo
feedback	0.2–0.5	0.90		Values close to 0.9 cause the repetitions to sustain for a very long time
damp		0.2–0.6	1.0			Higher values make the repetitions darker, creating a more natural sound
wet			0.2–0.5	1.0	1.0 	repetitions are as loud as the original signal
pingpong	0 or 1				Requires stereo (2 channels)
*/
int  echo_init(int sample_rate);
void echo_free(void);

void echo_reset(void);
void echo_set_delay(float ms);
void echo_set_feedback(float feedback);
void echo_set_wet(float wet);
void echo_set_damp(float damp);
void echo_set_pingpong(int enabled);

void echo_process_replace(float *buf, int n_frames, int channels);
int  echo_has_tail(void);

#endif