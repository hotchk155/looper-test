///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// RECORDING MANAGER
//
// Front end to recorded loop, encapsulating the SD card driver and buffering
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef RECORDING_H_
#define RECORDING_H_


class CRecording {


	// various constants
	enum {
		NUM_TRACKS 			= 2,
		TRACKS_BASE_BLOCK	= 10,
		TRACK_SIZE_BLOCKS   = 320000, // 320000 blocks x 256 samples / 44100 = approx 31 mins
		MAX_READ_AHEAD		= 120,		// furthest to read ahead - keep buffer space for writes
		MAGIC_COOKIE		= 0xAA5500
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

	// This is the buffer which acts as both a read and write cache for the SD card. We access the SD card as writing only
	// (initial recording), reading only (normal play back) and mixed read/write (overdubbing) where on average we read
	// one block for every block we write. This is why we can make the best use of the limited memory by sharign the same
	// buffer for both read and write caches!
	CBlockBuffer m_buf;

	// Accesses to SD card are batched so that we can take advantage of multiple block access transactions. Therefore
	// we toggle between batches of reads and batches of writes. This variable tracks what phase (type of batch) we
	// are doing now
	enum {
		READ_AHEAD_PHASE,
		WRITE_BEHIND_PHASE,
	} m_read_write_phase;

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
	int m_is_overdubbing;		// are we overdubbing?

	int m_loop_overflow_flag;	// flag records if loop overflowed allowed space during initial recording
	int m_loop_cycle_flag;		// flag records if loop cycled from end to start during playback/overdub


	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helper function for toggling the active record and playback tracks while leaving any other
	// tracks (e.g. undo) alone
	int other_track_id(int track_id) {
		return !track_id;
	}

public:
	CRecording() {
		m_rec.active_track_id = 0;
		m_rec.loop_len = 0;

		m_track[0].start_block = TRACKS_BASE_BLOCK;
		m_track[1].start_block = TRACKS_BASE_BLOCK + TRACK_SIZE_BLOCKS;

//TODO... use m_rec
		m_read_write_phase = READ_AHEAD_PHASE;
		m_cycle_status = CYCLE_DIRTY_PASS;
		m_cur_track_id = 0;
		m_cur_block_no = 0;
		m_read_ahead_track_id = 0;
		m_read_ahead_block_no = 0;
		m_write_behind_track_id = 1;
		m_write_behind_block_no = 0;
		m_is_read_ahead = 0;
		m_is_overdubbing = 0;
		m_loop_overflow_flag = 0;
		m_loop_cycle_flag = 0;
	}



	void init() {
		m_buf.clear_play_buffer();
		m_buf.clear_rec_buffer();
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
	void stop_or_reset_playback() {
		m_buf.clear_play_buffer();
		m_buf.clear_rec_buffer();

		m_cur_block_no = 0;
		m_read_ahead_block_no = 0;
		m_write_behind_block_no = 0;
		m_read_ahead_track_id = m_cur_track_id;
		m_write_behind_track_id = other_track_id(m_cur_track_id);

		m_is_read_ahead = 1;
		m_is_overdubbing = 0;
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
	void open_overdub() {
		m_is_overdubbing = 1;
		m_is_read_ahead = 1;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void close_overdub() {
		m_is_overdubbing = 0;
		m_is_read_ahead = 1;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	void run() {

		SAMPLE_BLOCK block;

		// check if the SD card has a block of data ready for us
		if(g_sd_card.read_block_ready(&block)) {
			// do we want it?
			if(m_is_read_ahead) {
				// push it into the read buffer
				m_buf.push_play_block(&block);
				// step to the next block
				if(++m_read_ahead_block_no >= m_rec.loop_len) {
					// prepare to start reading from the other track
					m_read_ahead_block_no = 0;
					m_read_ahead_track_id = other_track_id(m_read_ahead_track_id);
				}
			}
		}

		// when SD card is idle, check for the next action to do. We toggle between "phases" of read
		// ahead and write behind so that SD access can be optimised for multiple block operations
		if(!g_sd_card.busy()) {
			// are we checking for write behind?
			if(WRITE_BEHIND_PHASE == m_read_write_phase) {
				if(!m_buf.get_rec_count()) {
					// nothing to do, so back to read ahead
					m_read_write_phase = READ_AHEAD_PHASE;
				}
				// otherwise get the next block for SD card
				else if(m_buf.pop_rec_block(&block)) {
					// if we are sure that the write track is unchanged from the
					// read track then we can skip the actual write to the SD
					// card since the data will be the same
					if(m_cycle_status != CYCLE_ALL_UNCHANGED) {
						// otherwise write the block to SD card
						int block_no = m_track[m_write_behind_track_id].start_block + m_write_behind_block_no;
						g_sd_card.write_block(block_no, &block);
					}
					// increment the write block location on SD card
					if(++m_write_behind_block_no >= m_rec.loop_len) {
						// next we'll be writing to the other track
						m_write_behind_block_no = 0;
						m_write_behind_track_id = other_track_id(m_write_behind_track_id);
					}
				}
			}
			// we'll be reading from the card, up to the maximum number of blocks to read ahead
			else if(m_is_read_ahead && m_buf.get_play_count() < MAX_READ_AHEAD) {
				// request a block from the card.. this is an asynchronous operation
				int block_no = m_track[m_read_ahead_track_id].start_block + m_read_ahead_block_no;
				g_sd_card.request_read_block(block_no);
			}
			else {
				m_read_write_phase = WRITE_BEHIND_PHASE;
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
				if(m_is_overdubbing) {
					m_cycle_status = CYCLE_DIRTY_PASS;
				}
				else if(CYCLE_CLEAN_PASS == m_cycle_status) {
					m_cycle_status = CYCLE_ALL_UNCHANGED;
				}
				else {
					m_cycle_status = CYCLE_CLEAN_PASS;
				}
			}
		}
		// no loop length, so make sure that we don't exceed the allowable loop size
		else if(m_cur_block_no < TRACK_SIZE_BLOCKS - 1) {
			++m_cur_block_no;
			// initial loop record, so data us always being changed
			m_cycle_status = CYCLE_DIRTY_PASS;
		}
		else {
			// no more space for loop!
			m_loop_overflow_flag = 1;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	inline int get_audio(SAMPLE_BLOCK *block) {
		return m_buf.pop_play_block(block);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	inline int put_audio(SAMPLE_BLOCK *block) {
		return m_buf.push_rec_block(block);
	}


};
extern CRecording g_recording;
#endif /* RECORDING_H_ */
