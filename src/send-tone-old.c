#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <opus/opus.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

OpusEncoder* encoder;
int16_t samples[960 * 2];
uint8_t output[1024];

float pitch = 440.0;


void generate_samples(int amount, int16_t buffer[]) {
	/* static float pitch = 440.0; */
	static const float pi = 3.1415926835;
	static const int sample_rate = 48000;
	static float t = 0.0;

	/* static float seconds = 0.0; */
	/* seconds += 0.02; */
	/* pitch = 440.0 + sin(seconds * 10) * 50; */
	/* fprintf(stderr, "%f\n", pitch); */

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

	char* address = argv[1];
	int port = atoi(argv[2]);
	pitch = (float)atoi(argv[3]);
	fprintf(stderr, "%f\n", pitch);

	int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in server_address = {0};
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	/* server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); */
	server_address.sin_addr.s_addr = inet_addr(address);

	encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &err);
	if (err < 0) return;

	err = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(64000));
	if (err < 0) return;

	while(true) {
		generate_samples(960, samples);
		int bytes_encoded = opus_encode(encoder, samples, 960, output, 1024);
		if (bytes_encoded < 0) return;
		sendto(udp_socket, output, bytes_encoded, 0, (struct sockaddr*)&server_address, sizeof(server_address));

		usleep(19000);
	}


	while(true) {
		generate_samples(960, samples);
		int bytes_encoded = opus_encode(encoder, samples, 960, output, 1024);
		/* fprintf(stderr, "input: %d\n", opus_packet_get_nb_frames(output, bytes_encoded)); */

		int size = write(1, output, bytes_encoded);
		fprintf(stderr, "wrote %d / %d\n", size, bytes_encoded);
	}
}
