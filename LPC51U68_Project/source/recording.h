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

protected:
	// various constants
	enum {
		NUM_TRACKS 			= 2,
		TRACKS_BASE_BLOCK	= 10,
		TRACK_SIZE_BLOCKS   = 320000, // 320000 blocks x 256 samples / 44100 = approx 31 mins
		MAGIC_COOKIE		= 0xAA5500,
		READ_BATCH			= 10,
		WRITE_BATCH			= 10,
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
		IDLE_PHASE,
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


	int m_cur_track_id;
	int m_cur_block_no;

	// member variables track where read-ahead data is coming from... this might have
	// already wrapped over into a different track to that which is playing
	int m_read_ahead_track_id;
	int m_read_ahead_block_no;

	int m_write_behind_track_id;
	int m_write_behind_block_no;

	int m_is_read_ahead; 		// are we reading ahead (not needed during initial recording)
	int m_is_overdub;		// are we overdubbing?

	int m_loop_overflow_flag;	// flag records if loop overflowed allowed space during initial recording
	int m_loop_cycle_flag;		// flag records if loop cycled from end to start during playback/overdub


	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helper function for toggling the active record and playback tracks while leaving any other
	// tracks (e.g. undo) alone
	int other_track_id(int track_id) {
		return !track_id;
	}

public:
	CRecording() :
		m_rec_buf(g_read_buffer, SZ_BLOCK_BUFFER),
		m_play_buf(g_write_buffer, SZ_BLOCK_BUFFER)
	{
		m_rec.active_track_id = 0;
		m_rec.loop_len = 0;

		m_track[0].start_block = TRACKS_BASE_BLOCK;
		m_track[1].start_block = TRACKS_BASE_BLOCK + TRACK_SIZE_BLOCKS;

//TODO... use m_rec
		m_read_write_phase = READ_AHEAD_PHASE;
		m_blocks_to_transfer = READ_BATCH;
		m_cycle_status = CYCLE_CLEAN_PASS;
		m_cur_track_id = 0;
		m_cur_block_no = 0;
		m_read_ahead_track_id = 0;
		m_read_ahead_block_no = 0;
		m_write_behind_track_id = 1;
		m_write_behind_block_no = 0;
		m_is_read_ahead = 0;
		m_is_overdub = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}


	void init() {
		clear_sd_buffers();
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
	void erase_recording() {
		m_rec.loop_len = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void clear_sd_buffers() {
		m_rec_buf.clear();
		m_play_buf.clear();
		g_sd_card.read_block_ready(NULL);

	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void stop_or_reset_playback(int read_ahead) {
		clear_sd_buffers();

		m_cur_block_no = 0;
		m_read_ahead_block_no = 0;
		m_write_behind_block_no = 0;
		m_read_ahead_track_id = m_cur_track_id;
		m_write_behind_track_id = other_track_id(m_cur_track_id);

		m_is_read_ahead = read_ahead;
		m_is_overdub = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;

	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// prepare for the initial recording of the loop
	void open_initial_rec() {
		m_is_read_ahead = 0;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void close_initial_rec() {
		m_rec.loop_len = m_cur_block_no;
		m_is_read_ahead = 1;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void set_overdub(int overdub) {
		m_is_overdub = overdub;
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
				if(++m_read_ahead_block_no >= m_rec.loop_len) {
					// prepare to start reading from the other track
					m_read_ahead_block_no = 0;
					m_read_ahead_track_id = other_track_id(m_read_ahead_track_id);
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
						int block_no = m_track[m_write_behind_track_id].start_block + m_write_behind_block_no;
						g_sd_card.write_block(block_no, &block);
						++g_stats.block_writes;
					}
					// increment the write block location on SD card
					if(++m_write_behind_block_no >= m_rec.loop_len) {
						// next we'll be writing to the other track
						m_write_behind_block_no = 0;
						m_write_behind_track_id = other_track_id(m_write_behind_track_id);
					}
				}
			}
			else if (!m_play_buf.is_full()) {
				// request a block from the card.. this is an asynchronous operation
				int block_no = m_track[m_read_ahead_track_id].start_block + m_read_ahead_block_no;
				g_sd_card.request_read_block(block_no);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Advance the play/rec position (corresponds to the sample that is actually being played/overdubbed)
	void advance_cur_block_no() {

		// has a loop been established?
		if(m_rec.loop_len) {

			// track the block number corresponding to the position in the loop that
			// is actually being recorded/played at this moment in time
			if(++m_cur_block_no >= m_rec.loop_len) {

				// loop has cycled past end and back to start
				m_loop_cycle_flag = 1;

				// move to other track
				m_cur_block_no = 0;
				m_cur_track_id = other_track_id(m_cur_track_id);

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
		else if(m_cur_block_no < TRACK_SIZE_BLOCKS - 1) {
			++m_cur_block_no;
		}
		else {
			// no more space for loop!
			m_loop_overflow_flag = 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	inline int get_audio(SAMPLE_BLOCK *block) {
		return m_play_buf.pop(block);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	inline int put_audio(SAMPLE_BLOCK *block, int is_new) {
		if(is_new) {
			m_cycle_status = CYCLE_DIRTY_PASS;
		}
		return m_rec_buf.push(block);
	}

};

class CRecordingTest: public CRecording {
public:
	void set_loop_len(int len) {
		m_rec.loop_len = len;
	}

	void test1() {
		// set up two tracks worth of
		set_loop_len(4);
		clear_sd_buffers();
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad0);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad1);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad2);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad3);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad0);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad1);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad2);
		m_rec_buf.push((SAMPLE_BLOCK*)&sine::quad3);
	}



};
CRecordingTest g_recording;
#endif /* RECORDING_H_ */
