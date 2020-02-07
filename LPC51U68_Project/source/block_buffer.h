#ifndef BLOCK_BUFFER_H_
#define BLOCK_BUFFER_H_

enum {
	SZ_BLOCK_BUFFER = 128,	// the size of the block buffer, in SAMPLE_BLOCKs
};

// The main RAM block (64K) of the MCU is given over completely
// to the looper block buffer
__DATA(SRAM0) SAMPLE_BLOCK g_buffer[SZ_BLOCK_BUFFER]; // the big buffer for the looper

class CBlockBuffer
{

	SAMPLE_BLOCK *m_play_tail;
	SAMPLE_BLOCK *m_play_head;
	SAMPLE_BLOCK *m_rec_tail;
	SAMPLE_BLOCK *m_rec_head;
	SAMPLE_BLOCK *const m_begin = &g_buffer[0];
	SAMPLE_BLOCK *const m_end = &g_buffer[SZ_BLOCK_BUFFER];
	int m_rec_count;
	int m_play_count;


	///////////////////////////////////////////////////////////////////////////
	inline SAMPLE_BLOCK *inc_ptr(SAMPLE_BLOCK *ptr) {
		if(++ptr>=m_end) {
			ptr = m_begin;
		}
		return ptr;
	}

public:
	///////////////////////////////////////////////////////////////////////////
	CBlockBuffer() {
		m_play_tail = m_begin;
		m_play_head = m_begin;
		m_rec_tail = m_begin;
		m_rec_head = m_begin;
		m_rec_count = 0;
		m_play_count = 0;
	}

	///////////////////////////////////////////////////////////////////////////
	int push_play_block(SAMPLE_BLOCK *block) {
		if(!m_play_count) {
			// if this is the first block to be placed in the play buffer then
			// the buffer pointers first need to be positioned appropriately.
			// If there is an active rec buffer we will place the play buffer one
			// place behind (since we should be one-in-one-out)
			m_play_tail = m_play_head = m_rec_count? inc_ptr(m_rec_tail) : m_begin;
		}
		// establish the limit of the play head pointer, depending if there is also
		// a rec buffer present
		SAMPLE_BLOCK *limit = m_rec_count? m_rec_tail : m_play_tail;
		if(m_rec_head == limit) {
			// no space to insert the new block!
			return 0;
		}
		// store the block in the buffer, move head pointer and update count
		*m_play_head = *block;
		m_play_head = inc_ptr(m_play_head);
		++m_play_count;
		return 1;
	}

	///////////////////////////////////////////////////////////////////////////
	// pull block of data from the read-ahead buffer
	int pop_play_block(SAMPLE_BLOCK *block) {
		// check if any play blocks to pull
		if(m_play_count) {
			// grab the oldest buffered play block, step past it and
			// update count of play blocks in the buffer
			*block = *m_play_tail;
			m_play_tail = inc_ptr(m_play_tail);
			--m_play_count;
			return 1;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	int push_rec_block(SAMPLE_BLOCK *block) {
		if(!m_rec_count) {
			// if this is the first block to be placed in the rec buffer then
			// the rec buffer pointers first need to be positioned appropriately.
			// If there is an active play buffer we will place the rec buffer one
			// place behind (since we should be one-in-one-out)
			m_rec_tail = m_rec_head = m_play_count? inc_ptr(m_play_tail) : m_begin;
		}
		// establish the limit of the rec head pointer, depending if there is also
		// a play buffer present
		SAMPLE_BLOCK *limit = m_play_count? m_play_tail : m_rec_tail;
		if(m_rec_head == limit) {
			// no space to insert the new block!
			return 0;
		}
		// store the block in the buffer, move head pointer and update count
		*m_rec_head = *block;
		m_rec_head = inc_ptr(m_rec_head);
		++m_rec_count;
		return 1;
	}

	///////////////////////////////////////////////////////////////////////////
	// pull block of data from the write behind (rec) buffer
	int pop_rec_block(SAMPLE_BLOCK *block) {
		// check if any rec blocks to pull
		if(m_rec_count) {
			*block = *m_rec_tail;
			m_rec_tail = inc_ptr(m_rec_tail);
			--m_rec_count;
			return 1;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////
	inline int get_play_count() {
		return m_play_count;
	}

	///////////////////////////////////////////////////////////////////////////
	inline int get_rec_count() {
		return m_rec_count;
	}

	///////////////////////////////////////////////////////////////////////////
	void clear_play_buffer() {
		m_play_count = 0;
	}

	///////////////////////////////////////////////////////////////////////////
	void clear_rec_buffer() {
		m_rec_count = 0;
	}
};

#endif /* BLOCK_BUFFER_H_ */
