/*
 * clock.h
 *
 *  Created on: 29 Jan 2020
 *      Author: jason
 */

#ifndef CLOCK_H_
#define CLOCK_H_

//////////////////////////////////////////////////////////////////////////////////////////////////
class CClock {
	volatile uint32_t m_millis;
	volatile byte m_ticked;
public:

	///////////////////////////////////////////////////////////////////////////////
	CClock() {
		m_millis = 0;
		m_ticked = 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	void init() {
	  mrt_config_t mrtConfig;
	  MRT_GetDefaultConfig(&mrtConfig);
	  MRT_Init(MRT0, &mrtConfig);
	  MRT_SetupChannelMode(MRT0, kMRT_Channel_0, kMRT_RepeatMode);
	  MRT_EnableInterrupts(MRT0, kMRT_Channel_0, kMRT_TimerInterruptEnable);
	  EnableIRQ(MRT0_IRQn);
	  MRT_StartTimer(MRT0, kMRT_Channel_0, USEC_TO_COUNT(1000U, CLOCK_GetFreq(kCLOCK_BusClk)));
	}

	///////////////////////////////////////////////////////////////////////////////
	void wait_for_tick() {
		m_ticked = 0;
		while(!m_ticked);
	}

	///////////////////////////////////////////////////////////////////////////////
	void delay(int ms) {
		while(ms) {
			m_ticked = 0;
			while(!m_ticked);
			--ms;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	byte is_ticked() {
		if(m_ticked) {
			m_ticked = 0;
			return 1;
		}
		return 0;
	}

	///////////////////////////////////////////////////////////////////////////////
	volatile uint32_t millis() {
		return m_millis;
	}

	///////////////////////////////////////////////////////////////////////////////
	inline void tick_isr() {
	    ++m_millis;
	    m_ticked = 1;

	}
};

extern CClock g_clock;

///////////////////////////////////////////////////////////////////////////////
extern "C" void MRT0_IRQHandler(void){
    MRT_ClearStatusFlags(MRT0, kMRT_Channel_0, kMRT_TimerInterruptFlag);
    g_clock.tick_isr();
    g_ui.tick_isr();
 }


#endif /* CLOCK_H_ */
