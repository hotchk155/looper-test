/*
 * stats.h
 *
 *  Created on: 12 Feb 2020
 *      Author: jason
 */

#ifndef STATS_H_
#define STATS_H_

struct {
	int rec_buf_full;
	int play_buf_empty;
	int block_reads;
	int block_read_ignores;
	int block_writes;
	int dummy_writes;
	int audio_out;
	int audio_in;
	int loop_cycles;
	int sd_setup_error;
	int sd_access_errors;
} g_stats;




#endif /* STATS_H_ */
