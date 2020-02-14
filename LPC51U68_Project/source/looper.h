

#ifndef LOOPER_H_
#define LOOPER_H_

class CLooper: public IAudioCallback {
public:
	typedef enum {
		ST_UNKNOWN,
		ST_EMPTY,
		ST_INIT_REC,
		ST_STOPPED,
		ST_PLAY,
		ST_OVERDUB
	} STATE;
protected:
	SAMPLE_BLOCK m_last_play_block;
	STATE m_state;
public:
	CLooper() {
		m_state = ST_UNKNOWN;
	}

	void init() {
		m_state = ST_UNKNOWN;
	}

	////////////////////////////////////////////////////////////////////
	void mix_audio(SAMPLE_BLOCK *block, SAMPLE_BLOCK *overdub) {
		for(int i=0; i<	SZ_SAMPLE_BLOCK; ++i) {
			int res = block->data[i] + overdub->data[i];
			if(res < MIN_SAMPLE_VALUE) {
				res = MIN_SAMPLE_VALUE;
			}
			else if(res > MAX_SAMPLE_VALUE) {
				res = MAX_SAMPLE_VALUE;
			}
			block->data[i] = res;
		}
	}

	////////////////////////////////////////////////////////////////////
	// callback from the audio interface when it needs a block of audio
	// to play. This is the point where we track movement through the
	// loop in terms of the actual block being played
	int get_audio_block(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_PLAY:
			if(!g_recording.get_audio(block)) {
				++g_stats.play_buf_empty;
				return 0;
			}
			g_recording.advance_cur_block_no();
			break;
		case ST_OVERDUB:
			if(!g_recording.get_audio(block)) {
				++g_stats.play_buf_empty;
				return 0;
			}
			g_recording.advance_cur_block_no();
			break;
		case ST_INIT_REC:
			g_recording.advance_cur_block_no();
			// fall thru to next case
		default:
			memset(block, 0, sizeof(block));
			break;
		}
		m_last_play_block = *block;
		return 1;
	}
	////////////////////////////////////////////////////////////////////
	// callback from the audio interface when it has a new block of
	// recorded audio to be stored
	int put_audio_block(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_INIT_REC:
			// while making the initial recording, the input audio is
			// written directly to the record track
			if(!g_recording.put_audio(block, 1)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			return 1;
		case ST_PLAY:
			// while playing, the last played block is written back to
			// the record track, bringing it up to date.
			if(!g_recording.put_audio((SAMPLE_BLOCK*)&m_last_play_block, 0)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			return 1;
		case ST_OVERDUB:
			// while overdubbing, the last played block is mixed with the
			// incoming audio and written back to the record track
			mix_audio(block, &m_last_play_block);
			if(!g_recording.put_audio(block, 1)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			return 1;
		default:
			return 0;
		}
	}

	////////////////////////////////////////////////////////////////////
	void run() {
		switch(m_state) {
		case ST_UNKNOWN:
			// at startup need to determine if a loop exists on the SD card
			m_state = g_recording.is_loop_set() ? ST_STOPPED : ST_EMPTY;
			break;
		case ST_INIT_REC:
			if(g_recording.is_loop_overflow()) {
				g_recording.close_initial_rec();
				g_recording.stop_or_reset_playback(1);
				m_state = ST_STOPPED;
			}
			break;
		case ST_EMPTY:
		case ST_STOPPED:
		case ST_PLAY:
		case ST_OVERDUB:
			break;
		}
	}

	void on_undo_redo_button() {

	}


	////////////////////////////////////////////////////////////////////
	// What happens when REC/STOP button is pressed in various states
	void on_rec_stop_button() {
		switch(m_state) {

		//////////////////////////////////////
		// START INITIAL RECORDING
		// Loop length, loop positions already zero
		case ST_EMPTY:
			g_recording.stop_or_reset_playback(0);
			g_recording.open_initial_rec();
			m_state = ST_INIT_REC;
			break;

		//////////////////////////////////////
		// END RECORDING AND STOP
		case ST_INIT_REC:
			g_recording.close_initial_rec();
			g_recording.stop_or_reset_playback(1);
			m_state = ST_STOPPED;
			break;

		//////////////////////////////////////
		// OVERDUB FROM START OF LOOP
		case ST_STOPPED:
			g_recording.set_overdub(1);
			m_state = ST_OVERDUB;
			break;


		//////////////////////////////////////
		// PUNCH IN
		case ST_PLAY:
			g_recording.set_overdub(1);
			m_state = ST_OVERDUB;
			break;

		//////////////////////////////////////
		// PUNCH OUT
		case ST_OVERDUB:
			g_recording.set_overdub(0);
			m_state = ST_PLAY;
			break;

			//////////////////////////////////////
		case ST_UNKNOWN:
			break;
		}
	}

	////////////////////////////////////////////////////////////////////
	// What happens when PLAY/STOP button is pressed in various states
	void on_play_stop_button() {
		switch(m_state) {

		//////////////////////////////////////
		case ST_UNKNOWN:
		case ST_EMPTY:
			// Nothing happens
			break;

		//////////////////////////////////////
		// STOP RECORDING, PLAY FROM START
		case ST_INIT_REC:
			m_state = ST_STOPPED; // in case another audio block comes in
			g_recording.close_initial_rec();
			g_recording.stop_or_reset_playback(1);
			m_state = ST_PLAY;
			break;

		//////////////////////////////////////
		// PLAY FROM START
		case ST_STOPPED:
			m_state = ST_PLAY;
			break;

		//////////////////////////////////////
		// STOP PLAYBACK, RETURN TO START
		case ST_PLAY:
			m_state = ST_STOPPED;
			g_recording.stop_or_reset_playback(1);
			break;

		//////////////////////////////////////
		// STOP OVERDUB, RETURN TO START
		case ST_OVERDUB:
			m_state = ST_STOPPED;
			g_recording.set_overdub(0);
			g_recording.stop_or_reset_playback(1);
			break;
		}
	}

};

class CLooperTest: public CLooper {
public:
	/*
	int get_audio_block(SAMPLE_BLOCK *block) {
		static int q = 0;
		switch(q) {
		case 0: *block = sine::quad0; break;
		case 1: *block = sine::quad1; break;
		case 2: *block = sine::quad2; break;
		case 3: *block = sine::quad3; break;
		}
		if(++q>3) q=0;
		return 1;
	}*/
	void run_till_state_change() {
		int s = m_state;
		while(m_state == s) {
			run();
		}
	}
};

CLooperTest g_looper;


#endif /* LOOPER_H_ */
