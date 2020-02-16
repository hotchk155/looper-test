

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

	//////////////////////////////////////////////////////////////////
	void set_state(STATE state) {
		switch(state) {
		case ST_INIT_REC:
			g_ui.set_led(CUI::LED_UNDO, CUI::LED_DUTY_OFF);
			g_ui.set_led(CUI::LED_REC, CUI::LED_DUTY_ON);
			g_ui.set_led(CUI::LED_PLAY, CUI::LED_DUTY_OFF);
			break;
		case ST_STOPPED:
			g_ui.set_led(CUI::LED_UNDO, CUI::LED_DUTY_ON);
			g_ui.set_led(CUI::LED_REC, CUI::LED_DUTY_OFF);
			g_ui.set_led(CUI::LED_PLAY, CUI::LED_DUTY_OFF);
			break;
		case ST_PLAY:
			g_ui.set_led(CUI::LED_UNDO, CUI::LED_DUTY_ON);
			g_ui.set_led(CUI::LED_REC, CUI::LED_DUTY_OFF);
			g_ui.set_led(CUI::LED_PLAY, CUI::LED_DUTY_ON);
			break;
		case ST_OVERDUB:
			g_ui.set_led(CUI::LED_UNDO, CUI::LED_DUTY_ON);
			g_ui.set_led(CUI::LED_REC, CUI::LED_DUTY_ON);
			g_ui.set_led(CUI::LED_PLAY, CUI::LED_DUTY_ON);
			break;
		case ST_UNKNOWN:
		case ST_EMPTY:
			g_ui.set_led(CUI::LED_UNDO, CUI::LED_DUTY_OFF);
			g_ui.set_led(CUI::LED_REC, CUI::LED_DUTY_OFF);
			g_ui.set_led(CUI::LED_PLAY, CUI::LED_DUTY_OFF);
			break;
		}
		m_state = state;
	}

	////////////////////////////////////////////////////////////////////
	void mix_audio(SAMPLE_BLOCK *block, SAMPLE_BLOCK *overdub) {
		for(int i=0; i<	SZ_SAMPLE_BLOCK; ++i) {
			int res = (int16_t)block->data[i] + (int16_t)overdub->data[i];
			if(res < MIN_SAMPLE_VALUE) {
				res = MIN_SAMPLE_VALUE;
			}
			else if(res > MAX_SAMPLE_VALUE) {
				res = MAX_SAMPLE_VALUE;
			}
			block->data[i] = res;
		}
	}

public:

	//////////////////////////////////////////////////////////////////
	CLooper() {
		m_state = ST_UNKNOWN;
	}

	//////////////////////////////////////////////////////////////////
	void init() {
		set_state(ST_UNKNOWN);
	}


	////////////////////////////////////////////////////////////////////
	// callback from the audio interface when it needs a block of audio
	int get_audio_block(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_UNKNOWN:
		case ST_INIT_REC:
		case ST_EMPTY:
		case ST_STOPPED:
			memset(block, 0, sizeof(SAMPLE_BLOCK));
			break;
		case ST_PLAY:
		case ST_OVERDUB:
			if(!g_recording.get_audio(block)) {
				memset(&m_last_play_block, 0, sizeof(m_last_play_block));
				++g_stats.play_buf_empty;
				return 0;
			}
			m_last_play_block = *block;
		}
		return 1;
	}

	////////////////////////////////////////////////////////////////////
	// callback from the audio interface when it has a new block of
	// recorded audio to be stored
	int put_audio_block(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_UNKNOWN:
		case ST_EMPTY:
		case ST_STOPPED:
			break;
		case ST_INIT_REC:
			if(!g_recording.put_audio(block, CRecording::REC_INIT)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			break;
		case ST_PLAY:
			if(!g_recording.put_audio(block, CRecording::REC_PLAY)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			break;
		case ST_OVERDUB:
			mix_audio(block, &m_last_play_block);
			if(!g_recording.put_audio(block, CRecording::REC_OVERDUB)) {
				++g_stats.rec_buf_full;
				return 0;
			}
			break;
		}
		return 1;
	}


	////////////////////////////////////////////////////////////////////
	void run() {
		switch(m_state) {
		case ST_UNKNOWN:
			// at startup need to determine if a loop exists on the SD card
			set_state(g_recording.is_loop_set() ? ST_STOPPED : ST_EMPTY);
			break;
		case ST_INIT_REC:
			if(g_recording.is_loop_overflow()) {
				g_recording.close_initial_rec();
				set_state(ST_STOPPED);
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
		g_recording.erase_recording();
		set_state(ST_EMPTY);
	}


	////////////////////////////////////////////////////////////////////
	// What happens when REC/STOP button is pressed in various states
	void on_rec_stop_button() {
		switch(m_state) {

		//////////////////////////////////////
		// START INITIAL RECORDING
		// Loop length, loop positions already zero
		case ST_EMPTY:
			g_recording.open_initial_rec();
			set_state(ST_INIT_REC);
			break;

		//////////////////////////////////////
		// END RECORDING AND STOP
		case ST_INIT_REC:
			g_recording.close_initial_rec();
			g_recording.reset_playback();
			set_state(ST_STOPPED);
			break;

		//////////////////////////////////////
		// OVERDUB FROM START OF LOOP
		case ST_STOPPED:
			g_recording.reset_playback();
			set_state(ST_OVERDUB);
			break;


		//////////////////////////////////////
		// PUNCH IN
		case ST_PLAY:
			set_state(ST_OVERDUB);
			break;

		//////////////////////////////////////
		// PUNCH OUT
		case ST_OVERDUB:
			set_state(ST_PLAY);
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
			set_state(ST_STOPPED); // in case another audio block comes in
			g_recording.close_initial_rec();
			set_state(ST_PLAY);
			break;

		//////////////////////////////////////
		// PLAY FROM START
		case ST_STOPPED:
			g_recording.reset_playback();
			set_state(ST_PLAY);
			break;

		//////////////////////////////////////
		// STOP PLAYBACK, RETURN TO START
		case ST_PLAY:
		case ST_OVERDUB:
			set_state(ST_STOPPED);
			break;
		}
	}

};

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
class CLooperTest: public CLooper {
	SAMPLE_BLOCK m_block;
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
++	}*/
	void run_till_state_change() {
		int s = m_state;
		while(m_state == s) {
			run();
		}
	}

// LOOPBACK OF CODEC INPUT TO CODEC OUTPUT
#if 0
	int get_audio_block(SAMPLE_BLOCK *block) {
		*block = m_block;
		return 1;
	}
	int put_audio_block(SAMPLE_BLOCK *block) {
		m_block = *block;
		return 1;
	}
#endif


#if 0
	// FORCE SINEWAVE SIGNAL AT AUDIO IN
	int put_audio_block(SAMPLE_BLOCK *block) {
		static int count = 0;
		switch(count) {
		case 0:
		default:
			m_block = sine::quad0;
			break;
		case 1:
			m_block = sine::quad1;
			break;
		case 2:
			m_block = sine::quad2;
			break;
		case 3:
			m_block = sine::quad3;
			break;
		}
		if(++count>3) {
			count = 0;
		}
		if(!CLooper::put_audio_block(&m_block)) {
			//PRINTF("PUT AUDIO BLOCK FAIL");
			return 0;
		}
		return 1;
	}
#endif

#if 0
	int qqq = 0;
	int put_audio_block(SAMPLE_BLOCK *block) {
		if(qqq) {
			return g_recording.put_audio(block);
		}
		return 1;
	}
	int get_audio_block(SAMPLE_BLOCK *block) {
		if(qqq) {
			return g_recording.get_audio(block);
		}
		else {
			memset(block, 0, sizeof(SAMPLE_BLOCK));
		}
		return 1;
	}


	void test1() {
		int i;
		g_recording.erase_recording();
		g_recording.open_initial_rec();
		// stick 60 samples in the buffer
		for(i=0; i<15; ++i) {
			if(	!g_recording.put_audio((SAMPLE_BLOCK*)&sine::quad0) ||
				!g_recording.put_audio((SAMPLE_BLOCK*)&sine::quad1) ||
				!g_recording.put_audio((SAMPLE_BLOCK*)&sine::quad2) ||
				!g_recording.put_audio((SAMPLE_BLOCK*)&sine::quad3)) {
				PRINTF("PUT AUDIO BLOCK FAIL\r\n");
			}
		}

		// wait for all the data to be saved
		for(i=0; i<100; ++i) {
			if(g_recording.m_rec_buf.get_count()) {
				i=0;
			}
			g_sd_card.run();
			g_recording.run();
		}
		g_recording.close_initial_rec();

		qqq = 1; // enable playback
		for(;;) {
			g_sd_card.run();
			g_recording.run();
		}



	}

#endif


};

CLooperTest g_looper;


#endif /* LOOPER_H_ */
