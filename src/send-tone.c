#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <opus/opus.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <ortp/ortp.h>

OpusEncoder* encoder;
int16_t samples[960 * 2];
uint8_t output[1024];

float pitch = 440.0;


void generate_samples(int amount, int16_t buffer[]) {
	/* static float pitch = 440.0; */
	static const float pi = 3.1415926835;
	static const int sample_rate = 48000;
	static float t = 0.0;

	for (int i=0; i<amount*2; i += 2) {
		t += 2 * pi * pitch / sample_rate;
		t = fmod(t, 2 * pi);
		int16_t sample = sin(t) * 1.0 * 16767.f;

		// write the same sample to both channels
		buffer[i + 0] = sample;
		buffer[i + 1] = sample;
	}
}

void main(int argc, char** argv) {
	int err;

	ortp_init();
	ortp_scheduler_init();
	/* ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR); */

	RtpSession* session = rtp_session_new(RTP_SESSION_SENDONLY);	
	
	rtp_session_set_scheduling_mode(session, 0);
	rtp_session_set_blocking_mode(session, 0);
	rtp_session_set_connected_mode(session, false);

	err = rtp_session_set_remote_addr(session, "127.0.0.1", 8002);

	if (err != 0) return;

	rtp_session_set_payload_type(session,0);

	encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &err);
	if (err < 0) return;

	err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(64000));
	if (err < 0) return;

	/* uint16_t pcm_buffer[960]; */
	/* uint8_t opus_buffer[1024]; */

	uint32_t user_timestamp = 0;

	while (true) {
		generate_samples(960, samples);
		int bytes_encoded = opus_encode(encoder, samples, 960, output, 1024);
		if (bytes_encoded < 0) return;

		/* int bytes_encoded = 50; */

		/* printf("%d  ", bytes_encoded); */

		int bytes_sent = rtp_session_send_with_ts(session, output, bytes_encoded, user_timestamp);
		printf("%d\n", bytes_sent);
		user_timestamp += 160;
	}

}
