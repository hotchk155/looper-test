/*
 * looper.h
 *
 *  Created on: 28 Jan 2020
 *      Author: jason
 */

#ifndef LOOPER_H_
#define LOOPER_H_

	enum {
		NUM_BLOCKS = 128,
	};

	__DATA(SRAM0) DBLK m_loop[NUM_BLOCKS]; // the big buffer for the looper


class CLooper {

	enum {
		INIT_MODE,
		REC_MODE,
		PLAY_MODE,
		OVERDUB_MODE,
	};

	byte m_looper_mode;


	DBLK *m_write_head;
	DBLK *m_write_tail;
	DBLK *m_read_head;
	DBLK *m_read_tail;
	DBLK * const m_end = &m_loop[NUM_BLOCKS];

	int m_sd_loop_address; 	// start address of the loop on the SD card
	int m_sd_loop_length; 	// total length of the loop on the SD card
	int m_sd_read_address; 	// address on SD card from where the next block will be read (on a block boundary)
	int m_sd_write_address;	// address on SD card from where the next block will be written (on a block boundary)
	int m_sd_storage_mode;	// what action we're currently trying to get the SD card to do

	// The circular buffer is made up of 512 byte blocks, each of
	// which contains 256 * 16 bit mono samples (one block is ~5ms of audio)
	//

	// The play/rec position advances through the buffer one block at a time...

	// - As each fresh block is played while in REC/OVERDUB mode is is replaced
	//

	// RR  WWWWWWRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
	// ................................................
	//  |  |     ^ play/rec position

	// In REC/OVERDUB modes, the loop buffer object tries to minimize the
	// number of blocks that are waiting to be written out the SD card

	inline DBLK *inc(DBLK *ptr) {
		if(++ptr>=m_end) {
			ptr = m_loop;
		}
		return ptr;
	}
public:
	CLooper() {
		m_write_head = m_loop;
		m_write_tail = m_loop;
		m_read_head = m_loop;
		m_read_tail = m_loop;
	}

	byte get_audio_out(DBLK *block) {
		return 0;
	}
	void on_audio_in(DBLK *block) {
	}



#if 0
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	void write_push(DBLK *block) {
		if(m_write_head == m_read_tail) {
			// no space to queue the write block
			++m_write_drop_errors;
		}
		else {
			++m_write_count;
			*m_write_head = *block;
			m_write_head = inc(m_write_head);
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	byte write_pop(DBLK *block) {
		if(m_write_tail == m_write_head) {
			// nothing to write
			return 0;
		}
		else {
			--m_write_count;
			*block = *m_write_tail;
			m_write_tail = inc(m_write_tail);
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	void read_push(DBLK *block) {
		if(m_read_head == m_write_tail) {
			// full of read stuff
		}
		else {
			*m_write_head = *block;
			m_write_head = inc(m_write_head);
		}
	}

	byte get_audio_out(DBLK *block) {
		return 0;
	}
	void on_audio_in(DBLK *block) {
	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// fetch the next block
	void pop_for_play(BLOCK_PTR block) {

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// This is the method called by the audio interface when it has finished playing/recording a block of
	// data. Each parameter points to a block of 512 bytes of data (256 x 16bit samples)
	// last_rec is data that has been recorded or overdubbed
	// next_play is the next data to be played out
	void process_block(byte *last_rec, byte *next_play) {
		switch(m_looper_mode) {
		case REC_MODE:
			push_for_write(last_rec);
			break;
		case PLAY_MODE:
			pop_for_play(next_play);
			break;
		case OVERDUB_MODE:
			push_for_write(last_rec);
			pop_for_play(next_play);
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// In initial record mode
	int sd_card_req_rec(uint32_t *addr, byte **data, int *len) {

	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// The SD card interface will repeatedly ask us what to do next, and if anything to be done we
	// will provide it with an action, an SD card address and a data buffer
	int sd_card_req_callback(uint32_t *addr, byte **data, int *len) {

		switch(m_looper_mode) {
		case
		INIT_MODE,
		REC_MODE,
		PLAY_MODE,
		OVERDUB_MODE,

		}


		if(m_read_dest)
		int m_play_pos; 	// play/record position
		int m_write_src;	// source position for next block to write out to card
		int m_read_dest;    // destination position for next block to read from card


		switch(m_sd_storage_mode) {

		}


		if(m_sd_storage_mode == IStorageController::IDLE)


		if(m_sd_storage_mode == IStorageController::READ_BLOCKS)
		return IStorageController::IDLE;
	}

#endif
};
CLooper g_looper;


#endif /* LOOPER_H_ */
