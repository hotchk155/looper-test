#ifndef BLOCK_BUFFER_H_
#define BLOCK_BUFFER_H_


/*
 The buffer should hold physical information about block numbers on the SD card because otherwise the
 SD card interface needs to know the current play head position for when overdubbing starts - else
 data might get written to wrong location

 tracking the position in the SD card object is a problem



 */

// TODO cache blocks from the start of the loop ready for restart signal
// TODO might be reading ahead from other loop when copying UNDO loop into current loop





	enum {
		NUM_BLOCKS = 128
	};

	// The main RAM block (64K) of the MCU is given over completely
	// to the looper block buffer
	__DATA(SRAM0) SD_BLOCK m_loop[NUM_BLOCKS]; // the big buffer for the looper

class CBlockBuffer :
	public IBlockBuffer
{


	typedef enum {
		ST_EMPTY,			// no takes, not playing
		ST_RECORD,			// starting the first take
		ST_STOP,			// established at least one take, not playing
		ST_PLAY,			// playing current take
		ST_OVERDUB_INIT,	// starting a new overdub, reading audio from previous take
		ST_OVERDUB,			// established overdub, now looping same take
	} STATE_TYPE;
	STATE_TYPE m_state;

	// Both pointers will be set to NULL when there is no audio output (e.g. REC/STOP mode)
	SD_BLOCK *m_read_tail;		// points to the next data block queued up for writing to audio CODEC
	SD_BLOCK *m_read_head;		// points to the location where next data block will written after receiving from CODEC

	// Both pointers will be set to NULL when there is no audio input (e.g. PLAY/STOP mode)
	SD_BLOCK *m_write_tail;		// points to the oldest data block queued up for writing to SD card
	SD_BLOCK *m_write_head;		// points to the location where next data block will be queued for SD card

	SD_BLOCK *const m_buf_end = &m_loop[NUM_BLOCKS]; // points past end of the buffer

	// pointer to the "current" data block location in the buffer - this is the location from where
	// the most recent block was taken for sending to audio CODEC. This will be mixed with incoming audio
	// and placed in the write queue in overdub mode
	SD_BLOCK 	*m_cur_pos;

	// this is the SD card block address corresponding to the buffered block in *m_cur_pos
	SD_BLOCK_NO m_cur_sd_block;

	int m_c

	// the index of the active channel (i.e. "take") within the SD card blocks
	byte m_cur_chan;

	inline SD_BLOCK *inc_block_ptr(SD_BLOCK *ptr) {
		if(++ptr>=m_end) {
			ptr = m_loop;
		}
		return ptr;
	}

	void change_state()


public:
	CBlockBuffer() {
		m_state = ST_STOP;
		m_read_head= m_loop;
		m_write_tail= m_loop;
		m_cur_pos= m_loop;
		m_write_head= NULL;
		//m_cur_block = 0;

	}


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Return the next block of audio to send out to the CODEC. This call is made before the put_audio
	// call when the block has been sent
	void get_audio(SAMPLE_BLOCK *block) {
		switch(m_state) {
		case ST_EMPTY:
		case ST_STOP:
		case ST_RECORD:
			// we output silence in these modes
			memset(block, 0, sizeof(SAMPLE_BLOCK));
			break;


		case ST_OVERDUB_INIT:
			if(m_cur_sd_block == m_overdub_init_sd_block) {
				// after one full cycle through the loop when overdubb
				m_state = ST_OVERDUB;
			}
			// fall thru
		case ST_PLAY:
		case ST_OVERDUB:
			if(m_read_tail == m_read_head) { // could be NULL
				// TODO: no data ready to play.. we'll repeat the last block
				// of audio out
			}
			else {
				m_cur_pos = m_read_tail;
				if(m_state == ST_OVERDUB_INIT) {
					*block = m_cur_pos->chan[m_cur_chan? 0:1];
				}
				else {
					*block = m_cur_pos->chan[m_cur_chan];
				}
				m_read_tail = inc_block_ptr(m_read_tail);
				g_sd_card.inc_block_no(&m_cur_sd_block);
			}
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// This function is called when a block of audio is received from I2S
	// immediately after
	void put_audio(SAMPLE_BLOCK *block) {
		SD_BLOCK *next;

		switch(m_state) {
		case ST_EMPTY:
		case ST_STOP:
		case ST_PLAY:
			// nothing to do
			break;

		case ST_RECORD:
		case ST_OVERDUB_INIT:
		case ST_OVERDUB:
			if(!m_write_head) {
				m_write_head = m_cur_pos;
				m_write_tail = m_cur_pos;
			}
			next = inc_block_ptr(m_write_head)
			if(ST_RECORD == m_state) {
				if(next == m_write_tail) {
					// TODO: record dropped block!
				}
				else {
					m_write_head->chan[m_cur_chan] = *block;
					memset(m_write_head->chan[m_cur_chan? 0:1], 0, sizeof(SAMPLE_BLOCK));
					m_write_head = next;
				}
			}
			else {
				if(next == m_read_tail) {
					// TODO: record dropped block
				}
				else {
					mix_audio(m_cur_pos, block, m_write_head);
					m_write_head = next;
				}
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	int get_sd_block(SD_BLOCK *block) {
		// may have pending writes in any state
		if(m_write_tail == m_write_head) {
			m_write_head = nullptr;
			m_write_tail = nullptr;
			return 0;
		}
		*block = *m_write_tail;
		m_write_tail = inc_block_ptr(m_write_tail);
		return 1;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	void put_sd_block(SD_BLOCK *block) {
		SD_BLOCK *next;
		switch(m_state) {
		case ST_EMPTY:
		case ST_RECORD:
		case ST_STOP:
			break;
		case ST_PLAY:
		case ST_OVERDUB_INIT:
		case ST_OVERDUB:
			if(m_read_head) {// should not be be NULL in these modes...
				next = inc_block_ptr(m_read_head);
				if(next != m_write_tail && next != m_read_head) {
					*m_read_head = *block;
					m_read_head = next;
				}
			}
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	int accept_sd_block() {
		switch(m_state) {
		case ST_EMPTY:
		case ST_RECORD:
		case ST_STOP:
			break;
		case ST_PLAY:
		case ST_OVERDUB_INIT:
		case ST_OVERDUB:
			return (m_read_head && m_read_head != m_read_tail && m_read_head != m_write_tail);
		}
		return 0;
	}


};
CBlockBuffer g_block_buffer;


#endif /* BLOCK_BUFFER_H_ */
