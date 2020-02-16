
#include <stdbool.h>
#include <cr_section_macros.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_crc.h"
#include "fsl_gpio.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "fsl_spi.h"
#include "fsl_spi_dma.h"
#include "fsl_mrt.h"
#include "fsl_ctimer.h"
#include "stats.h"
#include "defs.h"
#include "ui.h"
#include "clock.h"
#include "pwm.h"
#include "sinewave.h"
#include "audioio.h"
#include "sdcard.h"
#include "test/sdcardtester.h"
#include "block_buffer.h"
#include "recording.h"
#include "looper.h"



CClock g_clock;
//CRecording g_recording;
void on_key_event(int key, int value) {
	if(value) { // key down
		switch(key) {
		case CUI::KEY_0: g_looper.on_undo_redo_button(); break;
		case CUI::KEY_1: g_looper.on_rec_stop_button(); break;
		case CUI::KEY_2: g_looper.on_play_stop_button(); break;
		}
	}
}

int main(void) {

	BOARD_InitBootClocks();
    BOARD_InitPins();


    //extern SAMPLE_BLOCK g_read_buffer[SZ_BLOCK_BUFFER];
    //CBlockBufferTest t(g_read_buffer, SZ_BLOCK_BUFFER);
    //t.test1();
    memset(&g_stats, 0, sizeof(g_stats));
    g_clock.init();


    //TEST
    //g_sd_card.init();
    //g_sd_card.test2();
    //for(;;);
    //


	g_looper.init();
	g_pwm.init();
	g_pwm.set_duty_0(50);
	g_pwm.set_duty_1(50);

    g_sd_card.init();

    // Fill SD card with 200 sinewave samples
    //g_sd_card.test1A();
    //g_sd_card.test1B(); while(1);

    g_recording.init();

    g_audioio.set_callback(&g_looper);
	g_audioio.init();





    //g_recording.test1();
    //while(1) g_recording.run();

/*
    //g_recording.set_loop_len(200);
    g_recording.stop_or_reset_playback(1);

	for(int i=0; i<1000; ++i) {
		g_looper.run();
		g_recording.run();
		g_sd_card.run();
	}*/

	g_audioio.start();

	//g_looper.test1();

	for(;;) {
		g_ui.run();
		g_looper.run();
		g_recording.run();
		g_sd_card.run();
	}
}

