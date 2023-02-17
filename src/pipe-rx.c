#include <math.h>
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <opus/opus.h>
#include <ortp/ortp.h>

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
 
const float pi = 3.14159265358;

#define DEFAULT_RATE			48000
#define DEFAULT_CHANNELS		2

void ssrc_cb(RtpSession* session) {}

OpusDecoder* decoder;
uint8_t input[1024] = {0};
int16_t decoded[2 * 5760];

RtpSession* session;

uint32_t timestamp = 0;
 
typedef struct {
	struct pw_main_loop *loop;
	struct pw_stream *stream;
	double t;
} Data;

static void on_process(void* userdata) {
	Data* data = userdata;
	struct pw_buffer* pipewire_buffer = pw_stream_dequeue_buffer(data -> stream);
	if (pipewire_buffer == NULL) return;

	struct spa_buffer* buffer = pipewire_buffer -> buffer;
	if (buffer == NULL) return;

	int16_t* dest = buffer -> datas[0].data;
	int stride = sizeof(int16_t) * DEFAULT_CHANNELS;

	int have_more = 1;

	int nbytes = rtp_session_recv_with_ts(session, input, 1024, timestamp, &have_more);
	if (nbytes <= 0) goto end;

	int samples = opus_decode(decoder, input, nbytes, dest, 5760, 0);

	fprintf(stderr, "decoded:\n");
	if (samples < 0) { fprintf(stderr, "failed to decode: %d\n", samples); return; };

	buffer -> datas[0].chunk->offset = 0;
	buffer -> datas[0].chunk->stride = stride;
	buffer -> datas[0].chunk->size = samples * stride;

end:
	timestamp += 160;
	pw_stream_queue_buffer(data->stream, pipewire_buffer);
}

RtpSession* init_rtp(char* address, int port) {
	ortp_init();
	ortp_scheduler_init();

	RtpSession* session = rtp_session_new(RTP_SESSION_RECVONLY);	
	rtp_session_set_scheduling_mode(session, false);
	rtp_session_set_blocking_mode(session, false);

	rtp_session_set_local_addr(session, address, port, -1);
	rtp_session_set_connected_mode(session, false);
	rtp_session_enable_adaptive_jitter_compensation(session, true);
	rtp_session_set_jitter_compensation(session, 40);
	rtp_session_set_time_jump_limit(session, 40 * 16);

	rtp_session_set_payload_type(session, 0);
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)ssrc_cb,0);
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)rtp_session_reset,0);

	rtp_session_enable_rtcp(session, false);

	return session;
}


void init_pipewire() {

	pw_init(NULL, NULL);

	// define properties for the stream
	struct pw_properties* stream_properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio");
	pw_properties_set(stream_properties, PW_KEY_MEDIA_CATEGORY, "Playback");
	pw_properties_set(stream_properties, PW_KEY_MEDIA_ROLE, "Music");

	static const struct pw_stream_events stream_events = {
		PW_VERSION_STREAM_EVENTS,
		.process = on_process,
	};

	static Data data = {0};
	struct pw_main_loop* loop = pw_main_loop_new(NULL);
	struct pw_stream* stream = pw_stream_new_simple(pw_main_loop_get_loop(loop), "audio-src", stream_properties, &stream_events, &data);
	data = (Data){ .loop = loop, .stream = stream };

	static uint8_t buffer[5760];
	
	struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
	struct spa_pod* parameter = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_S16, .channels = 2, .rate = 48000));
	struct spa_pod* params[] = { parameter };

	pw_stream_connect(data.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY, PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS, params, 1);

	// runs for the lifetime of the program
	pw_main_loop_run(data.loop);

	// free memory after loop is finished
	pw_stream_destroy(data.stream);
	pw_main_loop_destroy(data.loop);
}
 
 
int main(int argc, char *argv[]) {
	printf("a\n");
	int err;
	decoder = opus_decoder_create(48000, 2, &err);
	if (err < 0) return -1;

	session = init_rtp("127.0.0.1", 8002);
	if (session == NULL) return;

	/* printf("b\n"); */

	/* uint8_t buffer[160]; */

	/* while(true) { */
	/* 	int have_more = 1; */
	/* 	while (have_more) { */
	/* 		printf("c\n"); */
	/* 		int bytes = rtp_session_recv_with_ts(session, buffer, 160, timestamp, &have_more); */
	/* 		printf("d\n"); */
	/* 		printf("%b\n", have_more); */
	/* 		if (bytes > 0) { */
	/* 			printf("received %d bytes\n", bytes); */
	/* 			for (int i=0;i<bytes;i++) printf("%02x ", buffer[i]); */
	/* 			printf("\n"); */
	/* 		} else { */

	/* 		} */

	/* 	} */
	/* 	timestamp += 160; */
	/* } */

	init_pipewire();

}
