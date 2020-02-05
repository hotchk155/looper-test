/*
 * looper.h
 *
 *  Created on: 31 Jan 2020
 *      Author: jason
 */

#ifndef LOOPER_H_
#define LOOPER_H_

class CLooper {

	typedef enum {
		ST_EMPTY,			// waiting for intial loop to be determined (no file)
		ST_INITIAL_RECORD,	// initial record, audio input->WRITE
		ST_STOPPED,			// loop is recorded, not playing (no WRITE file)
		ST_PLAY,			// loop is bouncing READ->WRITE, no overdub made on this loop cycle
		ST_PLAY_DIRTY,		// loop is bouncing READ->WRITE, overdub made on this loop cycle
		ST_OVERDUB,			// loop is bouncing READ+[audio input]->WRITE
		ST_OVERDUB_ABORT	// loop is playing READ
	} STATE;

	typedef enum {
		UNDO_NONE,
		UNDO_READY,
		UNDO_DONE
	} UNDO_STATUS;

	SAMPLE_BLOCK m_last_block;
	UNDO_STATUS m_undo_status;
public:


	// The audio buffer calls in to fetch the next block of audio to play,
	// driving the playback at the correct samplerate
	int provide_audio_output(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_EMPTY:
		case ST_INITIAL_RECORD:
		case ST_STOPPED:
			break;
		case ST_PLAY:
		case ST_PLAY_DIRTY:
		case ST_OVERDUB:
		case ST_OVERDUB_ABORT:
			g_sd_card.read_block(&m_last_audio);
			*block = m_last_audio;
		}
	}

	void handle_audio_input(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_EMPTY:
		case ST_INITIAL_RECORD:
		case ST_STOPPED:
		case ST_PLAY:
		case ST_PLAY_DIRTY:
		case ST_OVERDUB:
		case ST_OVERDUB_ABORT:
		}
	}

	void on_clear_loop() {
		m_undo_status = UNDO_NONE;
	}
	void on_record_key() {
		switch(m_state) {
		// Pressing REC when no loop is defined starts
		// the initial recording
		case ST_EMPTY:
			// start initial recording
			g_sd_card.prefetch_from(RECORD_TRACK);
			m_state = ST_INITIAL_RECORD;
			break;
		case ST_INITIAL_RECORD:
			// set the loop
			m_state = ST_STOPPED;
			break;
		case ST_STOPPED:
			// start overdubbing from start of loop
			m_state = ST_OVERDUB;
			break;
		case ST_PLAY:
		case ST_PLAY_DIRTY:
			// start overdubbing from current playback position
			m_state = ST_OVERDUB;
			g_sd_card.prefetch_from(RECORD_TRACK);
			break;
		case ST_OVERDUB:
			// continue playing from current position
			g_sd_card.prefetch_from(RECORD_TRACK);
			m_state = ST_PLAY_DIRTY;
			break;
		case ST_OVERDUB_ABORT:
			// no action
			g_sd_card.prefetch_from(PLAY_TRACK);
			break;
		}
	}


	void on_play_key() {
		switch(m_state) {
		case ST_EMPTY:
			// no action
			break;
		case ST_INITIAL_RECORD:
			// set the loop length
			// set play position to start of loop
			g_sd_card.prefetch_from(RECORD_TRACK);
			m_state = ST_PLAY;
			break;
		case ST_STOPPED:
			// set play position to start of loop
			m_state = ST_PLAY;
			break;
		case ST_OVERDUB:
		case ST_OVERDUB_ABORT:
		case ST_PLAY:
		case ST_PLAY_DIRTY:
			// stop immediately
			// any new recorded info is lost
			m_state = ST_STOPPED;
			g_sd_card.prefetch_from(PLAY_TRACK);
			break;
		}
	}

	void on_undo_key() {
		switch(m_state) {
		case ST_EMPTY:
			// no action
			break;
		case ST_INITIAL_RECORD:
			// abandon recording
			m_state = ST_EMPTY;
			break;
		case ST_STOPPED:
		case ST_PLAY:
			if(m_)
			int read_file_id = g_sd_card.get_read_file_id();
			int undo_file_id = g_sd_card.get_undo_file_id();

			g_sd_card.set_read_file_id(write_file_id);
			g_sd_card.set_undo_file_id(read_file_id);

			m_undo_status = (m_undo_status == UNDO_DONE) ? UNDO_READY : UNDO_DONE;

			// switch undo / play tracks
			// no change to the state
			break;
		case ST_PLAY_DIRTY:
		case ST_OVERDUB:
			m_state = ST_OVERDUB_ABORT;
			// TODO check
			// this take will be erased at the end of the
			// recording - cannot redo
			g_sd_card.prefetch_from(PLAY_TRACK);
			break;
		case ST_OVERDUB_ABORT: // undo overdub abort... we will keep the overdub after all!
			m_state = ST_PLAY_DIRTY;
			g_sd_card.prefetch_from(REC_TRACK);
			break;
		}
	}


	// TODO SHOULD THIS TRIGGER WHEN PRE-FETCHING REACHES THE END OF THE BUFFER?
	void on_end_loop_cycle() {

		// as the pre-fetching of audio data from SD card reaches the end of the
		// loop at the end of the NEXT cycle, we will usually pre-fetch from the
		// record track (which then becomes the play track). This could get changed
		// by any action that invalidates the record track
		g_sd_card.prefetch_from(REC_TRACK);

		int file_id;
		switch(m_state) {
		case ST_EMPTY:
		case ST_INITIAL_RECORD:
		case ST_STOPPED:
			// not applicable
			break;
		case ST_PLAY:
			// no change to write file
			break;
		case ST_OVERDUB_ABORT:
			// return to play mode and do not
			// commit the last overdub track
			m_state = ST_PLAY;
			break;
		case ST_PLAY_DIRTY:
			m_state = ST_PLAY;
		case ST_OVERDUB: {
			// change current "read" file to "undo" file
			// change current "write" file to "read" file
			// change prev "undo" file to "write_file"

			int read_file_id = g_sd_card.get_read_file_id();
			int write_file_id = g_sd_card.get_write_file_id();
			int undo_file_id = g_sd_card.get_undo_file_id();

			g_sd_card.set_read_file_id(write_file_id);
			g_sd_card.set_write_file_id(undo_file_id);
			g_sd_card.set_undo_file_id(read_file_id);

			g_sd_card.restart_tracks();
			m_undo_status = UNDO_READY;

			}
			break;
		}
	}


};
CLooper g_looper;


#endif /* LOOPER_H_ */
