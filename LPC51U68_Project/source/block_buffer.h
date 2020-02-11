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

	enum {
		MAX_BUF_LEN = SZ_BLOCK_BUFFER - 1
	};
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
		if(ptr == m_end - 1) {
			ptr = m_begin;
		}
		return ptr + 1;
	}
	///////////////////////////////////////////////////////////////////////////
	inline SAMPLE_BLOCK *dec_ptr(SAMPLE_BLOCK *ptr) {
		if(ptr ==  m_begin) {
			ptr = m_end;
		}
		return ptr - 1;
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

	int len() {
		return SZ_BLOCK_BUFFER;
	}

	// ......................... empty buffer
	// ^ play head
	// ^ play tail

	// ......................... full buffer
	// ^ play head
	//  ^ play tail

	//                     v rec head
	//         v rec tail
	// ........RRRRRRRRRRRR......
	// ^ play head
	// ^ play tail


	///////////////////////////////////////////////////////////////////////////
	int push_play_block(SAMPLE_BLOCK *block) {
		if(is_full()) {
			return 0;
		}

		if(!m_play_count) { // first play block in the buffer
			if(m_rec_count) {
				// If there are rec blocks in the buffer already then we will
				// will aim for the following location of first data block
				// note that buffer should be 1 in - 1 out
				// ..RRRRRRRRRRRRRR.X......................
				//   ^ rec tail    ^ rec head
				//                  ^ play tail
				//                   ^ play head
				m_play_tail = inc_ptr(m_rec_head);
				m_play_head = m_play_tail;
			}
			else {
				// else simply start at the beginning of the buffer
				m_play_head = m_begin;
				m_play_tail = m_begin;
			}
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

		if(is_full()) {
			return 0;
		}
		if(!m_rec_count) { // first rec block in the buffer
			if(m_play_count) {
				// If there are play blocks in the buffer already then we will
				// will aim for the following location of first data block
				// note that buffer should be 1 in - 1 out
				// .......X.PPPPPPPPPPPPPP..........
				//          ^ play tail   ^ playhead
				//         ^ rec head
				//        ^ rec tail
				m_rec_head = dec_ptr(m_play_tail);
				m_rec_head = dec_ptr(m_rec_head);
				m_rec_tail = m_rec_head;
			}
			else {
				// else simply start at the beginning of the buffer
				m_rec_head = m_begin;
				m_rec_tail = m_begin;
			}
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
	inline int is_full() {
		return (m_play_count + m_rec_count) >= (MAX_BUF_LEN - 1);
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
