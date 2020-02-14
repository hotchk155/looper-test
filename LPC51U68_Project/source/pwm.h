/*
 * pwm.h
 *
 *  Created on: 9 Feb 2020
 *      Author: jason
 */

#ifndef PWM_H_
#define PWM_H_

class CPWM {
	#define CTIMER CTIMER0                 /* Timer 0 */
	#define CTIMER_MAT_OUT kCTIMER_Match_1 /* Match output 1 */

	enum {
		CARRIER_FREQ 	= 20000
	};
	uint32_t m_pwm_period;
	uint32_t get_match_value(int duty)
	{
	    if(duty == 0) {
	        return m_pwm_period + 1;
	    }
	    else if(duty >= 100) {
	    	return 0;
	    }
	    else {
	    	return (m_pwm_period * (100 - duty)) / 100;
	    }
	}

public:
	void set_duty_0(int duty) {
	    CTIMER_SetupPwmPeriod(CTIMER0, kCTIMER_Match_0, m_pwm_period, get_match_value(duty), false);
	}
	void set_duty_1(int duty) {//P1_4
	    CTIMER_SetupPwmPeriod(CTIMER0, kCTIMER_Match_1, m_pwm_period, get_match_value(duty), false);
	}


	void init(void)
	{
	    ctimer_config_t config;
	    CTIMER_GetDefaultConfig(&config);
	    uint32_t timer_clock = CLOCK_GetFreq(kCLOCK_BusClk) / (config.prescale + 1);
	    CTIMER_Init(CTIMER, &config);

	    m_pwm_period = (timer_clock / CARRIER_FREQ) - 1;
		set_duty_0(0);
		set_duty_1(0);
	    CTIMER_StartTimer(CTIMER);
	}

};
CPWM g_pwm;


#endif /* PWM_H_ */

