#ifndef BLOCK_BUFFER_H_
#define BLOCK_BUFFER_H_

typedef enum : uint32_t {
	SD_TAKE_NONE	= 0x00,		// there are no recorded takes
	SD_TAKE_0		= 0x01,		// take 0 is recorded, no take 1
	SD_TAKE_1		= 0x02,		// take 1 is recorded, no take 0
	SD_TAKE_0_UNDO	= 0x11,		// take 0 is active, take 1 is undo
	SD_TAKE_1_UNDO	= 0x12,		// take 0 is active, take 1 is undo
} SD_TAKE_STATUS;

typedef struct {
	SD_BLOCK_NO 	alloc_start_block;	// The base address of SD card space allocated for the take (in blocks)
	SD_BLOCK_NO 	alloc_end_block;	// The end address of SD card space allocated for the take (in blocks)
	SD_BLOCK_NO 	take_start_block;	// The start address of SD card space actually used for the take (in blocks)
	SD_BLOCK_NO 	take_end_block;		// The end address of SD card space actually used for the take (in blocks)
} SD_TAKE_INFO;

typedef struct {
	SD_TAKE_INFO	takes[2];			// The two takes (active and undo)
	SD_TAKE_STATUS 	take_status;				//
	uint32_t 		length;				// The actual length in samples the takes
	uint32_t		magic_cookie;

} SD_LOOP_INFO;


// TODO cache blocks from the start of the loop ready for restart signal
// TODO might be reading ahead from other loop when copying UNDO loop into current loop

typedef uint32_t SD_BLOCK_NO;
typedef uint32_t SD_ADDR;




	enum {
		NUM_BLOCKS = 128
	};

	// The main RAM block (64K) of the MCU is given over completely
	// to the looper block buffer
	__DATA(SRAM0) DBLK m_loop[NUM_BLOCKS]; // the big buffer for the looper

	typedef enum {
		NO_BLOCK,
		NEW_BLOCK,
		NEXT_BLOCK
	} BLOCK_RESULT;

class CBlockBuffer {

	SD_TAKE_INFO m_takes[2];
	SD_TAKE_INFO *m_this_take;		// the
	SD_TAKE_INFO *m_other_take;
	byte m_take_id;

	SD_BLOCK_NO m_read_block;		// absolute block number on SD card where next playback block will be read from
	SD_BLOCK_NO m_write_block;  	// absolute block number on SD card where next recorded/overdub block will be written
	SD_BLOCK_NO m_cur_block;		// the block that we are actually playing, relative to the start of the loop

	typedef enum {
		ST_EMPTY,			// no takes, not playing
		ST_RECORD,			// starting the first take
		ST_STOP,			// established at least one take, not playing
		ST_PLAY,			// playing current take
		ST_OVERDUB_INIT,	// starting a new overdub, reading audio from previous take
		ST_OVERDUB,			// established overdub, now looping same take
	} STATE_TYPE;

	STATE_TYPE m_state;

	DBLK *m_write_tail;
	DBLK *m_write_head;
	DBLK *m_read_head;
	DBLK *m_cur_pos;
	DBLK *const m_end = &m_loop[NUM_BLOCKS];

	uint32_t	m_cur_block;	// the current block number offset into the entire loop as stored on SD card
	uint32_t	m_num_blocks;	// the total number of blocks in the entire loop

	// Data between write tail to write head position will be written out to the SD card
	// in ANY MODE (since we may stop recording but still have recorded data to flush from
	// the buffer)

	// in RECORD / OVERDUB modes, write_head is the same as cur_pos

	// m_read_head is NULL in record mode

	byte undo_take() {

	}

	// In this mode the looper cannot already be recording. Any audio waiting flushing to
	// the SD card (must be from previous recording session) will be lost


	byte start_overdub() {
		// when we start an overdub we need to prepare for UNDO. This means that we need to make a copy
		// of the take that is currently being played. This is done by

		//
		// initially buffer is all read-ahead from take A
		// AAAAAAAAAAAAAAAAAAAAAAAAA*AAAAAAAAAAAAAAAAAAAAA
		//
		// now we start overdubbing.. continue read-ahead from take A but now
		// we are writing mixed audio to take B (at same relative location in take)
		// AAAAAAAAAAAAAAAAAAAAAAAAA##*AAAAAAAAAAAAAAAAAAAA
		//
		// we continue to read-ahead from take A...
		// AAAAAAAAAAAAAAAAAAAAAAAAAAAA######*AAAAAAAAAAAAA
		//
		// when read-ahead has fetched all of take A...
		// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA####*AAAAAAAAAAA
		//
		// read-ahead now starts to pull from take B...
		// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABBBB##*AAAAAAAAA
		//
		// until all read-ahead is from take B and we are in "normal overdub" mode
		// BBBBBBBBBBBBBBBBBBBBBBBBBB#######*BBBBBBBBBBBBBBBBB
		//
		//
		//
		//


		// The new re
		// switch takes
		m_other_take = m_takes[m_take_id];
		m_take_id = m_take_id? 0:1;
		m_this_take = m_takes[m_take_id];

		// map the block read position into the next
		m_read_block = m_this_take->take_start_block + m_cur_block;
		if(m_read_block >= m_this_take->alloc_end_block)
			m_read_block -= m_this_take->alloc_start_block;
		}


		SD_BLOCK_NO m_read_block;		// absolute block number on SD card where next playback block will be read from
		SD_BLOCK_NO m_write_block;  	// absolute block number on SD card where next recorded/overdub block will be written
		SD_BLOCK_NO m_cur_block;		// the block that we are actually playing, relative to the start of the loop



	}



	void event(EV_TYPE event) {
		switch(event) {
		case EV_LOOPER_CLEAR:
			m_write_tail = NULL;
			m_write_head = NULL;
			m_cur_pos = NULL;
			m_read_head = NULL;
			m_state = state;
			break;
		case EV_LOOPER_RECORD:
			if(m_state == LOOPER_EMPTY) {
				m_write_tail = m_loop;
				m_write_head = m_loop;
				m_cur_pos = m_loop;
				m_read_head = NULL;
				m_state = LOOPER_RECORD;
			}
			break;
		case EV_LOOPER_STOP:
			if(m_state == LOOPER_EMPTY) {
				// invalid
				break;
			}
			else if(m_state == LOOPER_RECORD) {
				m_read_head = m_cur_pos;
			}
			m_state = LOOPER_STOP;
			break;
		case EV_LOOPER_PLAY:
			if(m_state == LOOPER_EMPTY) {
				// no can do
			}
			else if(m_state == LOOPER_RECORD) {
				m_read_head = m_cur_pos;
			}
			m_state = LOOPER_PLAY;
			break;
		case EV_LOOPER_OVERDUB:
			if(m_state == LOOPER_STOP || m_state == LOOPER_PLAY) {
				// NOTE: if there is still cached info from previous recording it will be lost
				// when the new recording is started
				m_write_tail = m_loop;
				m_write_head = m_loop;
				m_state = LOOPER_OVERDUB;
				// TODO - SD CARD to set record address = current play address
			}
			break;
		}
	}


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

	inline void inc_loop_ptr(DBLK **ptr) {
		if(++(*ptr)>=m_end) {
			(*ptr) = m_loop;
		}
	}
public:
	CBlockBuffer() {
		m_state = LOOPER_STOPPED;
		m_insert_pos = m_loop;
		m_remove_pos = m_loop;
		m_cur_pos = m_loop;
		m_cur_block = 0;
		m_num_blocks = 0;

	}


	/*
	 MODE

	 rec - SD write from write_tail, audio into write_head. read_head/read_tail not used
	 play - audio out from read_tail, SD card read to read_head. write_head/write_tail not used
	 overdub - SD write from write_tail, audio out from write_head, SD card read to read_head. read_tail not used


	buf_tail
	buf_head
	play_pos


		record
			AUDIO->play_pos
			++play_pos
			sd_write_pos..play_pos -> SD
			sd_read_pos = NULL


		play
			SD->sd_read_pos
			sd_read_pos++
			play_pos -> AUDIO
			++play_pos
			buf_tail = play_pos

		overdub
		 	 SD -> buf_head
		 	 buf_tail..play_pos -> SD
		 	 play_pos->AUDIO
		 	 AUDIO + play_pos -> play_pos


		stop
	 	 	 buf_tail..play_pos -> SD



	}
	 */

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fetch the next block of audio to be played
	byte get_audio_out(DBLK *block) {
		switch(m_looper_state) {
		case LOOPER_STOPPED:
		case LOOPER_RECORD:
			memset(block, 0, sizeof(DBLK));
			break;
		case LOOPER_PLAY:
		case LOOPER_OVERDUB:
			if(m_cur_pos == m_insert_pos) {
				return 0;
			}
			*block = *m_cur_pos;
			break;
		}
		return 1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// Handle the block of audio we recorded at the same time the previous playback block was playing
	byte handle_audio_in(DBLK *block) {
		switch(m_looper_state) {
		case LOOPER_STOPPED:
			return 0;
		case LOOPER_PLAY:
			break;
		case LOOPER_RECORD:
		case LOOPER_OVERDUB:
			if(m_insert_pos == m_remove_pos) {
				return 0;
			}
			if(m_looper_state == LOOPER_OVERDUB) {
				mix_audio(*m_cur_pos, block);
			}
			else {
				*m_cur_pos = block;
				++m_num_blocks;
			}
			break;
		}
		inc_loop_ptr(&m_cur_pos);
		++m_cur_block;

		return 1;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// This method returns the next block from the loop buffer that needs to be written to the SD
	// card. If there is no block available (all data written) then
	BLOCK_RESULT get_next_block_for_sd(DBLK *block, SD_BLOCK_NO *block_no) {
		if(m_rec_tail == m_rec_head) {
			return 0;
		}
		*block = *m_rec_tail;
		inc_loop_ptr(&m_rec_tail);
		return 1;

		NO_BLOCK,
		NEW_BLOCK,
		NEXT_BLOCK

	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	BLOCK_RESULT get_next_block_from_sd(SD_BLOCK_NO *block_no) {
		switch(m_looper_state) {
		case LOOPER_RECORD:
		case LOOPER_STOPPED:
			break;
		case LOOPER_PLAY:
		case LOOPER_OVERDUB:
			break;
		}
		return 0;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	byte put_block_from_sd(DBLK *block) {
		return 0;

	}

};
CBlockBuffer g_block_buffer;


#endif /* BLOCK_BUFFER_H_ */
