/*
 * ui.h
 *
 *  Created on: 9 Feb 2020
 *      Author: jason
 */

#ifndef UI_H_
#define UI_H_
extern void on_key_event(int key, int value);
class CUI {
	int m_scan_state;
	int m_scan_chan;
	int m_debounce;
	byte m_led[3];
	int m_keys;
	int m_last_keys;
	int m_key_scan;
	byte m_cycle;
public:
	enum : byte {
		LED_DUTY_ON 	= 0xFF,
		LED_DUTY_OFF 	= 0x00,
		LED_DUTY_BLINK 	= 0x80, // 50% duty
	};
	enum {
		KEY_0 			= (1<<0),
		KEY_1 			= (1<<1),
		KEY_2 			= (1<<2),
		LED_UNDO 		= 0,
		LED_REC 		= 1,
		LED_PLAY 		= 2,
	};
	CUI() {
		m_scan_state = 0;
		m_scan_chan = 0;
		m_cycle = 0;
		m_led[0] = LED_DUTY_OFF;
		m_led[1] = LED_DUTY_OFF;
		m_led[2] = LED_DUTY_OFF;
		m_keys = 0;
		m_debounce = 0;
	}
	inline void set_rec(byte mode) {
		GPIO_PinWrite(BOARD_INITPINS_REC_LED_GPIO, BOARD_INITPINS_REC_LED_PORT, BOARD_INITPINS_REC_LED_PIN, !!mode);
	}
	void set_led(int led, byte mode) {
		m_led[led] = mode;
	}
	void run() {
		if(!m_debounce) {
			int delta = m_last_keys ^ m_keys;
			if(delta) {
				if(delta & KEY_0) on_key_event(KEY_0, !!(m_keys & KEY_0));
				if(delta & KEY_1) on_key_event(KEY_1, !!(m_keys & KEY_1));
				if(delta & KEY_2) on_key_event(KEY_2, !!(m_keys & KEY_2));
				m_last_keys = m_keys;
				m_debounce = 20;
			}
		}
	}
	void tick_isr() {
		if(!m_scan_state) {
			GPIO_PinWrite(BOARD_INITPINS_UI_LED_SINK_GPIO, BOARD_INITPINS_UI_LED_SINK_PORT, BOARD_INITPINS_UI_LED_SINK_PIN, 1);
			GPIO_PinWrite(BOARD_INITPINS_UI_0_IO_GPIO, BOARD_INITPINS_UI_0_IO_PORT, BOARD_INITPINS_UI_0_IO_PIN, (m_scan_chan==0));
			GPIO_PinWrite(BOARD_INITPINS_UI_1_IO_GPIO, BOARD_INITPINS_UI_1_IO_PORT, BOARD_INITPINS_UI_1_IO_PIN, (m_scan_chan==1));
			GPIO_PinWrite(BOARD_INITPINS_UI_2_IO_GPIO, BOARD_INITPINS_UI_2_IO_PORT, BOARD_INITPINS_UI_2_IO_PIN, (m_scan_chan==2));
			GPIO_PinWrite(BOARD_INITPINS_UI_LED_SINK_GPIO, BOARD_INITPINS_UI_LED_SINK_PORT, BOARD_INITPINS_UI_LED_SINK_PIN, !(m_led[m_scan_chan] & m_cycle));
		}

		if(++m_scan_state >= 2) {
			m_scan_state = 0;
			if(GPIO_PinRead(BOARD_INITPINS_UI_SW_READ_GPIO, BOARD_INITPINS_UI_SW_READ_PORT, BOARD_INITPINS_UI_SW_READ_PIN)) {
				m_key_scan |= (1<<m_scan_chan);
			}
			if(++m_scan_chan>2) {
				m_scan_chan = 0;
				m_keys = m_key_scan;
				m_key_scan = 0;
			}
			++m_cycle;
			if(m_debounce) {
				--m_debounce;
			}
		}
	}
};
CUI g_ui;

#endif /* UI_H_ */
