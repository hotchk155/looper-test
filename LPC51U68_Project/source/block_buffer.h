#ifndef BLOCK_BUFFER_H_
#define BLOCK_BUFFER_H_

class CBlockBuffer
{
protected:
	SAMPLE_BLOCK *m_tail;
	SAMPLE_BLOCK *m_head;
	SAMPLE_BLOCK *m_begin;
	SAMPLE_BLOCK *m_end;
	int m_size;
	int m_count;


	///////////////////////////////////////////////////////////////////////////
	inline SAMPLE_BLOCK *inc_ptr(SAMPLE_BLOCK *ptr) {
		if(++ptr == m_end) {
			return m_begin;
		}
		return ptr;
	}

public:
	///////////////////////////////////////////////////////////////////////////
	CBlockBuffer(SAMPLE_BLOCK *buf, int size) {
		m_size = size;
		m_begin = buf;
		m_end = buf + size;
		clear();
	}

	///////////////////////////////////////////////////////////////////////////
	int push(SAMPLE_BLOCK *block) {
		SAMPLE_BLOCK *next = inc_ptr(m_head);
		if(next == m_tail) {
			return 0;
		}
		*m_head = *block;
		m_head = next;
		++m_count;
		return 1;
	}

	///////////////////////////////////////////////////////////////////////////
	int pop(SAMPLE_BLOCK *block) {
		if(m_head == m_tail) {
			return 0;
		}
		*block = *m_tail;
		m_tail = inc_ptr(m_tail);
		--m_count;
		return 1;
	}

	///////////////////////////////////////////////////////////////////////////
	int get_count() {
		return m_count;
	}

	///////////////////////////////////////////////////////////////////////////
	int get_size() {
		return m_size;
	}


	///////////////////////////////////////////////////////////////////////////
	int is_full() {
		return (m_count >= m_size-1);
	}

	///////////////////////////////////////////////////////////////////////////
	void clear() {
		m_tail = m_begin;
		m_head = m_begin;
		m_count = 0;
	}

};

class CBlockBufferTest : public CBlockBuffer {
public:

	CBlockBufferTest(SAMPLE_BLOCK *buf, int size) :
		CBlockBuffer(buf, size) {}

	void test1() {
		for(int i=0; i<15; ++i) {
			if(!push((SAMPLE_BLOCK*)&sine::quad0)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad1)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad2)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad3)) {
				PRINTF("PUSH ERROR\r\n");
			}
			PRINTF("BUF COUNT %d IS FULL %d\r\n", get_count(), is_full());
		}


		for(int i=0; i<200; ++i) {
			SAMPLE_BLOCK q;
			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad0, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad1, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad2, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad3, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!push((SAMPLE_BLOCK*)&sine::quad0)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad1)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad2)) {
				PRINTF("PUSH ERROR\r\n");
			}
			if(!push((SAMPLE_BLOCK*)&sine::quad3)) {
				PRINTF("PUSH ERROR\r\n");
			}
			PRINTF("BUF COUNT %d IS FULL %d\r\n", get_count(), is_full());

		}


		for(int i=0; i<15; ++i) {
			SAMPLE_BLOCK q;
			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad0, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad1, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad2, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}

			if(!pop(&q)) {
				PRINTF("POP ERROR\r\n");
			}
			if(memcmp(&q, &sine::quad3, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON ERROR\r\n");
			}
			PRINTF("BUF COUNT %d IS FULL %d\r\n", get_count(), is_full());

		}

	}

};


#endif /* BLOCK_BUFFER_H_ */
