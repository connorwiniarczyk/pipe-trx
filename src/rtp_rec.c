#include <stdio.h>
#include <stdbool.h>

#include <ortp/ortp.h>

void ssrc_cb(RtpSession* session) {
	//pass
}

int main() {

	ortp_init();
	ortp_scheduler_init();
	/* ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR); */

	/* signal(SIGINT, stop_handler); */

	RtpSession* session = rtp_session_new(RTP_SESSION_RECVONLY);	
	rtp_session_set_scheduling_mode(session, false);
	rtp_session_set_blocking_mode(session, false);
	/* rtp_session_set_local_addr(session,"0.0.0.0", atoi(argv[2]), -1); */

	rtp_session_set_local_addr(session,"0.0.0.0", 8002, -1);
	rtp_session_set_connected_mode(session, false);
	/* rtp_session_set_symmetric_rtp(session, false); */
	rtp_session_enable_adaptive_jitter_compensation(session, true);
	rtp_session_set_jitter_compensation(session, 40);

	rtp_session_set_payload_type(session, 0);
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)ssrc_cb,0);
	rtp_session_signal_connect(session,"ssrc_changed",(RtpCallback)rtp_session_reset,0);

	rtp_session_enable_rtcp(session, false);

	uint32_t timestamp = 0;
	uint8_t buffer[160];

	int loop_condition = 1;
	while (loop_condition) {
		int have_more = 1;
		while (have_more) {
			int bytes = rtp_session_recv_with_ts(session, buffer, 160, timestamp, &have_more);
			printf("%b\n", have_more);
			if (bytes > 0) {
				printf("received %d bytes\n", bytes);
				for (int i=0;i<bytes;i++) printf("%02x ", buffer[i]);
				printf("\n");
			} else {

			}

		}
		timestamp += 160;
	}
}
