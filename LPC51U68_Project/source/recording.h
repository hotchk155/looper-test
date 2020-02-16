/*

 TODO
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RECORDING MANAGER
//
// Front end to recorded loop, encapsulating the SD card driver and buffering
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef RECORDING_H_
#define RECORDING_H_

enum {
	SZ_BLOCK_BUFFER = 64,	// the size of the block buffer, in SAMPLE_BLOCKs
};

// The main RAM block (64K) of the MCU is given over completely
// to the looper block buffer
__DATA(SRAM0) SAMPLE_BLOCK g_read_buffer[SZ_BLOCK_BUFFER]; // the big buffer for the looper
__DATA(SRAM0) SAMPLE_BLOCK g_write_buffer[SZ_BLOCK_BUFFER]; // the big buffer for the looper



class CRecording {
friend class CLooperTest;
public:
	typedef enum {
		REC_NONE,
		REC_INIT,
		REC_PLAY,
		REC_OVERDUB
	} REC_MODE;
protected:
	// various constants
	enum {
		NUM_TRACKS 			 = 2,
		TRACKS_BASE_BLOCK	 = 10,
		TRACK_SIZE_BLOCKS    = 320000, // 320000 blocks x 256 samples / 44100 = approx 31 mins
//		TRACK_SIZE_BLOCKS    = 5000, // 320000 blocks x 256 samples / 44100 = approx 31 mins
		MAGIC_COOKIE		 = 0xAA5500,
		READ_BATCH			 = 10,
		WRITE_BATCH			 = 10,
		INIT_REC_TRACK_ID    = 0
	};

	// This is the information that gets written to the SD card so that we can still use
	// a loop after power off/on
//TODO write only when changed
//Write following a STOP
	typedef struct {
		int loop_len;			// the length of the loop
		int active_track_id;	// the track ID of the "play" track
		int magic_cookie;		// this is set to a special value so that we can identify when SD card has valid data
	} RECORDING_INFO;

	// This is the information that describes the layout of each track on the SD card
	typedef struct {
		int start_block;	// SD card block where the trackl starts
	} TRACK_INFO;

	typedef struct _TRACK_POSITION {
		int track_id;
		int block_no;

		inline void to_track(int track) {
			track_id = track;
			block_no = 0;
		}
		inline void to_track(struct _TRACK_POSITION &pos) {
			to_track(pos.track_id);
		}
		inline void to_other_track(struct _TRACK_POSITION &pos) {
			to_track(!pos.track_id);
		}
		inline void to_other_track() {
			to_other_track(*this);
		}
	} TRACK_POSITION;

	// This is our cache of the recording status info from the SD card
	RECORDING_INFO m_rec;

	// This is a look up of track layout info
	TRACK_INFO m_track[NUM_TRACKS];

	CBlockBuffer m_rec_buf;
	CBlockBuffer m_play_buf;

	// Accesses to SD card are batched so that we can take advantage of multiple block access transactions. Therefore
	// we toggle between batches of reads and batches of writes. This variable tracks what phase (type of batch) we
	// are doing now
	enum {
		READ_AHEAD_PHASE,
		WRITE_BEHIND_PHASE,
	} m_read_write_phase;
	int m_blocks_to_transfer;

	// While a loop is being played, with or without overdubs, we continually copy one track to another, mixing in any
	// overdub content as we go. If we've already made at least one complete pass without overdubs then both of these
	// tracks have identical content so we can skip making any actual writes to the SD card. This variable tracks whether
	// any overdubs were made during a loop cycle
	enum {
		CYCLE_DIRTY_PASS,
		CYCLE_CLEAN_PASS,
		CYCLE_ALL_UNCHANGED
	} m_cycle_status;

	TRACK_POSITION m_cur_pos;
	TRACK_POSITION m_read_ahead_pos;
	TRACK_POSITION m_write_behind_pos;

	int m_is_read_ahead; 		// are we reading ahead (not needed during initial recording)
	int m_loop_overflow_flag;	// flag records if loop overflowed allowed space during initial recording
	int m_loop_cycle_flag;		// flag records if loop cycled from end to start during playback/overdub

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	void reset_to_start() {
		m_read_ahead_track_id = m_cur_track_id;
		m_write_behind_track_id = other_track_id(m_cur_track_id);
		m_cur_block_no = 0;
		m_read_ahead_block_no = 0;
		m_write_behind_block_no = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}
	*/

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void clear_sd_buffers() {
		m_rec_buf.clear();
		m_play_buf.clear();
		g_sd_card.read_block_ready(NULL);

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Advance the play/rec position (corresponds to the sample that is actually being played/overdubbed)
	void advance_cur_block_no() {

		// has a loop been established?
		if(m_rec.loop_len) {

			// track the block number corresponding to the position in the loop that
			// is actually being recorded/played at this moment in time
			if(++m_cur_pos.block_no >= m_rec.loop_len) {

				++g_stats.loop_cycles;

				// loop has cycled past end and back to start
				m_loop_cycle_flag = 1;

				// move to other track
				m_cur_pos.to_other_track();

				// track two loop cycles without any overdubs.. at this point both tracks
				// have identical content so no need to do real write behind to SD card as
				// data has not changed
				switch(m_cycle_status) {
				case CYCLE_DIRTY_PASS:
					m_cycle_status = CYCLE_CLEAN_PASS;
					break;
				case CYCLE_CLEAN_PASS:
					m_cycle_status = CYCLE_ALL_UNCHANGED;
					break;
				case CYCLE_ALL_UNCHANGED:
					// no change
					break;
				}
			}
		}
		// no loop length, so make sure that we don't exceed the allowable loop size
		else if(m_cur_pos.block_no < TRACK_SIZE_BLOCKS - 1) {
			++m_cur_pos.block_no;
		}
		else {
			// no more space for loop!
			m_loop_overflow_flag = 1;
		}
	}

public:
	CRecording() :
		m_rec_buf(g_write_buffer, SZ_BLOCK_BUFFER),
		m_play_buf(g_read_buffer, SZ_BLOCK_BUFFER)
	{
		m_rec.active_track_id = 0;
		m_rec.loop_len = 0;

		m_track[0].start_block = TRACKS_BASE_BLOCK;
		m_track[1].start_block = TRACKS_BASE_BLOCK + TRACK_SIZE_BLOCKS;

//TODO... use m_rec
		m_read_write_phase = READ_AHEAD_PHASE;
		m_blocks_to_transfer = READ_BATCH;
		m_cycle_status = CYCLE_CLEAN_PASS;
		m_cur_pos.track_id = INIT_REC_TRACK_ID;
		m_is_read_ahead = 0;

		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}


	void init() {

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Determine whether a loop is recorded
	int is_loop_set() {
		return !!m_rec.loop_len;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Signal whether the loop reached size limit during initial recording
	int is_loop_overflow() {
		int loop_overflow_flag = m_loop_overflow_flag;
		m_loop_overflow_flag = 0;
		return loop_overflow_flag;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Signal whether the loop has cycled around during playback/overdubbing
	int is_loop_cycle() {
		int loop_cycle_flag = m_loop_cycle_flag;
		m_loop_cycle_flag = 0;
		return loop_cycle_flag;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// called to fetch the next block of audio to be played
	int get_audio(SAMPLE_BLOCK *block) {
		return m_play_buf.pop(block);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// called when we receive a block of audio.
	int put_audio(SAMPLE_BLOCK *block, REC_MODE mode) {
		switch(mode) {
		case REC_INIT:
			m_cycle_status = CYCLE_DIRTY_PASS;
			advance_cur_block_no();
			if(!m_play_buf.is_full()) {
				m_play_buf.push(block); // pre-populate play buffer with the first samples
			}
			return m_rec_buf.push(block);
		case REC_PLAY:
			advance_cur_block_no();
			return m_rec_buf.push(block);
		case REC_OVERDUB:
			m_cycle_status = CYCLE_DIRTY_PASS;
			advance_cur_block_no();
			return m_rec_buf.push(block);
		case REC_NONE:
			break;
		}
		return 0;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void erase_recording() {
		clear_sd_buffers();
		m_rec.loop_len = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
		m_is_read_ahead = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// prepare for the initial recording of the loop
	void open_initial_rec() {
		clear_sd_buffers();
		m_cur_pos.to_track(INIT_REC_TRACK_ID);
		m_write_behind_pos.to_track(m_cur_pos);
		m_read_ahead_pos.to_other_track(m_cur_pos);
		m_is_read_ahead = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// close out initial recording, enabling playback to commence from start
	void close_initial_rec() {

		// store the length of the loop
		m_rec.loop_len = m_cur_pos.block_no;

		// set up tracks so that we will play the newly recorded track
		m_cur_pos.to_track(INIT_REC_TRACK_ID);
		m_read_ahead_pos.to_track(m_cur_pos);
		m_write_behind_pos.to_other_track(m_cur_pos);

		// during recording we pre-buffered blocks from the start, so
		// position the read ahead block accordingly
		m_read_ahead_pos.block_no = m_play_buf.get_count();

		// set flags
		m_is_read_ahead = 1;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void reset_playback() {

		// clear any buffered blocks
		clear_sd_buffers();

		// return to the start of the current tracks
		m_write_behind_pos.to_other_track(m_cur_pos);
		m_read_ahead_pos.to_track(m_cur_pos);

		// set flags
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
		m_is_read_ahead = 1;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void run() {

		SAMPLE_BLOCK block;

		g_pwm.set_duty_0(100 * m_play_buf.get_count()/m_play_buf.get_size());
		g_pwm.set_duty_1(100 * m_rec_buf.get_count()/m_rec_buf.get_size());


		// check if the SD card has a block of data ready for us
		if(g_sd_card.read_block_ready(&block)) {
			// do we want it?
			if(m_is_read_ahead) {
				// push it into the read buffer
				m_play_buf.push(&block);
				++g_stats.block_reads;

				// step to the next block
				if(++m_read_ahead_pos.block_no >= m_rec.loop_len) {
					// prepare to start reading from the other track
					m_read_ahead_pos.to_other_track();
				}
			}
			else {
				++g_stats.block_read_ignores;
			}
		}

		// when SD card is ready, check for the next action to do. We toggle between "phases" of read
		// ahead and write behind so that SD access can be optimised for multiple block operations
		if(g_sd_card.is_ready()) {

			switch(m_read_write_phase) {
			case READ_AHEAD_PHASE:
				if(m_play_buf.is_full() || !m_blocks_to_transfer) {
					if(m_rec_buf.get_count()) {
						m_read_write_phase = WRITE_BEHIND_PHASE;
						m_blocks_to_transfer = WRITE_BATCH;
					}
				}
				else {
					--m_blocks_to_transfer;
				}
				break;
			case WRITE_BEHIND_PHASE:
				if(!m_rec_buf.get_count() || !m_blocks_to_transfer) {
					m_read_write_phase = READ_AHEAD_PHASE;
					m_blocks_to_transfer = READ_BATCH;
				}
				else {
					--m_blocks_to_transfer;
				}
			}

			// are we checking for write behind?
			if(WRITE_BEHIND_PHASE == m_read_write_phase) {

				// anything waiting to be written
				if(m_rec_buf.get_count()) {
					m_rec_buf.pop(&block);

					// if we are sure that the write track is unchanged from the
					// read track then we can skip the actual write to the SD
					// card since the data will be the same
					if(m_cycle_status == CYCLE_ALL_UNCHANGED) {
						++g_stats.dummy_writes;
					}
					else {
						// otherwise write the block to SD card
						int block_no = m_track[m_write_behind_pos.track_id].start_block + m_write_behind_pos.block_no;
						g_sd_card.write_block(block_no, &block);
						++g_stats.block_writes;
					}
					// increment the write block location on SD card
					++m_write_behind_pos.block_no;
					if(m_rec.loop_len && m_write_behind_pos.block_no >= m_rec.loop_len) {
						// next we'll be writing to the other track
						m_write_behind_pos.to_other_track();
					}
				}
			}
			else if (!m_play_buf.is_full()) {
				// request a block from the card.. this is an asynchronous operation
				int block_no = m_track[m_read_ahead_pos.track_id].start_block + m_read_ahead_pos.block_no;
				g_sd_card.request_read_block(block_no);
			}
		}
	}

};

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CRecordingTest: public CRecording {
public:
	void set_loop_len(int len) {
		m_rec.loop_len = len;
	}

	void test1() {
		// set up two tracks worth of
		clear_sd_buffers();
		for(int i=0; i<15; ++i) {
			m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad0);
			m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad1);
			m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad2);
			m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad3);
		}
		set_loop_len(60);
	}



};
CRecordingTest g_recording;
#endif /* RECORDING_H_ */
