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

	// The main RAM block (64K) of the MCU is given over completely
	// to the looper block buffer
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

	const DBLK m_sine0 = {{
			0, 0, 	146, 1, 	36, 3, 	182, 4, 	71, 6, 	217, 7, 	106, 9, 	251, 10, 	139, 12, 	27, 14, 	171, 15, 	57, 17, 	199, 18, 	85, 20, 	225, 21, 	109, 23,
			248, 24, 	130, 26, 	11, 28, 	147, 29, 	25, 31, 	159, 32, 	35, 34, 	166, 35, 	39, 37, 	167, 38, 	38, 40, 	163, 41, 	30, 43, 	152, 44, 	16, 46, 	134, 47,
			251, 48, 	109, 50, 	222, 51, 	77, 53, 	185, 54, 	36, 56, 	140, 57, 	242, 58, 	86, 60, 	183, 61, 	22, 63, 	115, 64, 	205, 65, 	37, 67, 	122, 68, 	204, 69,
			28, 71, 	105, 72, 	179, 73, 	250, 74, 	63, 76, 	128, 77, 	191, 78, 	250, 79, 	51, 81, 	104, 82, 	154, 83, 	201, 84, 	244, 85, 	29, 87, 	66, 88, 	99, 89,
			129, 90, 	156, 91, 	179, 92, 	198, 93, 	214, 94, 	226, 95, 	235, 96, 	240, 97, 	241, 98, 	238, 99, 	231, 100, 	221, 101, 	206, 102, 	188, 103, 	165, 104, 	139, 105,
			108, 106, 	74, 107, 	35, 108, 	248, 108, 	201, 109, 	149, 110, 	94, 111, 	34, 112, 	225, 112, 	157, 113, 	84, 114, 	6, 115, 	181, 115, 	94, 116, 	3, 117, 	164, 117,
			64, 118, 	216, 118, 	107, 119, 	249, 119, 	131, 120, 	8, 121, 	137, 121, 	4, 122, 	124, 122, 	238, 122, 	92, 123, 	196, 123, 	41, 124, 	136, 124, 	226, 124, 	56, 125,
			137, 125, 	213, 125, 	28, 126, 	94, 126, 	156, 126, 	212, 126, 	8, 127, 	55, 127, 	97, 127, 	134, 127, 	166, 127, 	193, 127, 	215, 127, 	232, 127, 	245, 127, 	252, 127,
			255, 127, 	252, 127, 	245, 127, 	232, 127, 	215, 127, 	193, 127, 	166, 127, 	134, 127, 	97, 127, 	55, 127, 	8, 127, 	212, 126, 	156, 126, 	94, 126, 	28, 126, 	213, 125,
			137, 125, 	56, 125, 	226, 124, 	136, 124, 	41, 124, 	196, 123, 	92, 123, 	238, 122, 	124, 122, 	4, 122, 	137, 121, 	8, 121, 	131, 120, 	249, 119, 	107, 119, 	216, 118,
			64, 118, 	164, 117, 	3, 117, 	94, 116, 	181, 115, 	6, 115, 	84, 114, 	157, 113, 	225, 112, 	34, 112, 	94, 111, 	149, 110, 	201, 109, 	248, 108, 	35, 108, 	74, 107,
			108, 106, 	139, 105, 	165, 104, 	188, 103, 	206, 102, 	221, 101, 	231, 100, 	238, 99, 	241, 98, 	240, 97, 	235, 96, 	226, 95, 	214, 94, 	198, 93, 	179, 92, 	156, 91,
			129, 90, 	99, 89, 	66, 88, 	29, 87, 	244, 85, 	201, 84, 	154, 83, 	104, 82, 	51, 81, 	250, 79, 	191, 78, 	128, 77, 	63, 76, 	250, 74, 	179, 73, 	105, 72,
			28, 71, 	204, 69, 	122, 68, 	37, 67, 	205, 65, 	115, 64, 	22, 63, 	183, 61, 	86, 60, 	242, 58, 	140, 57, 	36, 56, 	185, 54, 	77, 53, 	222, 51, 	109, 50,
			251, 48, 	134, 47, 	16, 46, 	152, 44, 	30, 43, 	163, 41, 	38, 40, 	167, 38, 	39, 37, 	166, 35, 	35, 34, 	159, 32, 	25, 31, 	147, 29, 	11, 28, 	130, 26,
			248, 24, 	109, 23, 	225, 21, 	85, 20, 	199, 18, 	57, 17, 	171, 15, 	27, 14, 	139, 12, 	251, 10, 	106, 9, 	217, 7, 	71, 6, 	182, 4, 	36, 3, 	146, 1,
	}};

	const DBLK m_sine1 = {{
			0, 0, 	108, 254, 	218, 252, 	72, 251, 	183, 249, 	37, 248, 	148, 246, 	3, 245, 	115, 243, 	227, 241, 	83, 240, 	197, 238, 	55, 237, 	169, 235, 	29, 234, 	145, 232,
			6, 231, 	124, 229, 	243, 227, 	107, 226, 	229, 224, 	95, 223, 	219, 221, 	88, 220, 	215, 218, 	87, 217, 	216, 215, 	91, 214, 	224, 212, 	102, 211, 	238, 209, 	120, 208,
			3, 207, 	145, 205, 	32, 204, 	177, 202, 	69, 201, 	218, 199, 	114, 198, 	12, 197, 	168, 195, 	71, 194, 	232, 192, 	139, 191, 	49, 190, 	217, 188, 	132, 187, 	50, 186,
			226, 184, 	149, 183, 	75, 182, 	4, 181, 	191, 179, 	126, 178, 	63, 177, 	4, 176, 	203, 174, 	150, 173, 	100, 172, 	53, 171, 	10, 170, 	225, 168, 	188, 167, 	155, 166,
			125, 165, 	98, 164, 	75, 163, 	56, 162, 	40, 161, 	28, 160, 	19, 159, 	14, 158, 	13, 157, 	16, 156, 	23, 155, 	33, 154, 	48, 153, 	66, 152, 	89, 151, 	115, 150,
			146, 149, 	180, 148, 	219, 147, 	6, 147, 	53, 146, 	105, 145, 	160, 144, 	220, 143, 	29, 143, 	97, 142, 	170, 141, 	248, 140, 	73, 140, 	160, 139, 	251, 138, 	90, 138,
			190, 137, 	38, 137, 	147, 136, 	5, 136, 	123, 135, 	246, 134, 	117, 134, 	250, 133, 	130, 133, 	16, 133, 	162, 132, 	58, 132, 	213, 131, 	118, 131, 	28, 131, 	198, 130,
			117, 130, 	41, 130, 	226, 129, 	160, 129, 	98, 129, 	42, 129, 	246, 128, 	199, 128, 	157, 128, 	120, 128, 	88, 128, 	61, 128, 	39, 128, 	22, 128, 	9, 128, 	2, 128,
			0, 128, 	2, 128, 	9, 128, 	22, 128, 	39, 128, 	61, 128, 	88, 128, 	120, 128, 	157, 128, 	199, 128, 	246, 128, 	42, 129, 	98, 129, 	160, 129, 	226, 129, 	41, 130,
			117, 130, 	198, 130, 	28, 131, 	118, 131, 	213, 131, 	58, 132, 	162, 132, 	16, 133, 	130, 133, 	250, 133, 	117, 134, 	246, 134, 	123, 135, 	5, 136, 	147, 136, 	38, 137,
			190, 137, 	90, 138, 	251, 138, 	160, 139, 	73, 140, 	248, 140, 	170, 141, 	97, 142, 	29, 143, 	220, 143, 	160, 144, 	105, 145, 	53, 146, 	6, 147, 	219, 147, 	180, 148,
			146, 149, 	115, 150, 	89, 151, 	66, 152, 	48, 153, 	33, 154, 	23, 155, 	16, 156, 	13, 157, 	14, 158, 	19, 159, 	28, 160, 	40, 161, 	56, 162, 	75, 163, 	98, 164,
			125, 165, 	155, 166, 	188, 167, 	225, 168, 	10, 170, 	53, 171, 	100, 172, 	150, 173, 	203, 174, 	4, 176, 	63, 177, 	126, 178, 	191, 179, 	4, 181, 	75, 182, 	149, 183,
			226, 184, 	50, 186, 	132, 187, 	217, 188, 	49, 190, 	139, 191, 	232, 192, 	71, 194, 	168, 195, 	12, 197, 	114, 198, 	218, 199, 	69, 201, 	177, 202, 	32, 204, 	145, 205,
			3, 207, 	120, 208, 	238, 209, 	102, 211, 	224, 212, 	91, 214, 	216, 215, 	87, 217, 	215, 218, 	88, 220, 	219, 221, 	95, 223, 	229, 224, 	107, 226, 	243, 227, 	124, 229,
			6, 231, 	145, 232, 	29, 234, 	169, 235, 	55, 237, 	197, 238, 	83, 240, 	227, 241, 	115, 243, 	3, 245, 	148, 246, 	37, 248, 	183, 249, 	72, 251, 	218, 252, 	108, 254,
	}};


	byte get_audio_out(DBLK *block) {
		static byte which = 0;
		*block = which? m_sine1 : m_sine0;
		which = !which;
		return 1;
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
