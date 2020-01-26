/*
 * How to set up clock using clock driver functions:
 *
 * 1. Setup clock sources.
 *
 * 2. Setup voltage for the fastest of the clock outputs
 *
 * 3. Set up wait states of the flash.
 *
 * 4. Set up all dividers.
 *
 * 5. Set up all selectors to provide selected clocks.
 */

/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Clocks v7.0
processor: LPC51U68
package_id: LPC51U68JBD64
mcu_data: ksdk2_0
processor_version: 7.0.1
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

#include "fsl_power.h"
#include "fsl_clock.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* System clock frequency. */
extern uint32_t SystemCoreClock;

/*******************************************************************************
 ************************ BOARD_InitBootClocks function ************************
 ******************************************************************************/
void BOARD_InitBootClocks(void)
{
    BOARD_BootClockRUN();
}

/*******************************************************************************
 ********************** Configuration BOARD_BootClockRUN ***********************
 ******************************************************************************/
/* TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!Configuration
name: BOARD_BootClockRUN
called_from_default_init: true
outputs:
- {id: FXCOM6_clock.outFreq, value: 33.75/2 MHz}
- {id: FXCOM7_clock.outFreq, value: 33.75/2 MHz}
- {id: Master_clock.outFreq, value: 33.75/2 MHz}
- {id: PLL_clock.outFreq, value: 33.75/2 MHz}
- {id: System_clock.outFreq, value: 48 MHz}
settings:
- {id: SYSCON.FXCOMCLKSEL6.sel, value: SYSCON.PLL_BYPASS}
- {id: SYSCON.FXCOMCLKSEL7.sel, value: SYSCON.PLL_BYPASS}
- {id: SYSCON.MAINCLKSELA.sel, value: SYSCON.fro_hf}
- {id: SYSCON.MCLKCLKSEL.sel, value: SYSCON.PLL_BYPASS}
- {id: SYSCON.M_MULT.scale, value: '92160', locked: true}
- {id: SYSCON.N_DIV.scale, value: '4', locked: true}
- {id: SYSCON.SYSPLLCLKSEL.sel, value: SYSCON.fro_12m}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS **********/

/*******************************************************************************
 * Variables for BOARD_BootClockRUN configuration
 ******************************************************************************/
/*******************************************************************************
 * Code for BOARD_BootClockRUN configuration
 ******************************************************************************/
void BOARD_BootClockRUN(void)
{
    /*!< Set up the clock sources */
    /*!< Set up FRO */
    POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);                   /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);                    /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);                  /*!< Switch to FRO 12MHz first to ensure we can change voltage without accidentally
                                                                being below the voltage for current speed */
    POWER_SetVoltageForFreq(48000000U);             /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(48000000U);   /*!< Set FLASH wait states for core */

    /*!< Set up PLL */
    CLOCK_AttachClk(kFRO12M_to_SYS_PLL);                  /*!< Switch PLL clock source selector to FRO12M */
    const pll_setup_t pllSetup = {
        .syspllctrl = SYSCON_SYSPLLCTRL_UPLIMOFF_MASK,
        .syspllndec = SYSCON_SYSPLLNDEC_NDEC(2U),
        .syspllpdec = SYSCON_SYSPLLPDEC_PDEC(2U),
        .syspllssctrl = {0x0U,(SYSCON_SYSPLLSSCTRL1_MD(46080U) | (uint32_t)(kSS_MF_512) | (uint32_t)(kSS_MR_K0) | (uint32_t)(kSS_MC_NOC))},
        .pllRate = 16875000U,
        .flags =  PLL_SETUPFLAG_POWERUP
    };
    CLOCK_SetPLLFreq(&pllSetup); /*!< Configure PLL to the desired values */

    /* PLL in Fractional/Spread spectrum mode */
    /* SYSTICK is used for waiting for PLL stabilization */

    SysTick->LOAD = 111999UL;                              /*!< Set SysTick count value */
    SysTick->VAL = 0UL;                                           /*!< Reset current count value */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;                      /*!< Enable SYSTICK */
    while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != SysTick_CTRL_COUNTFLAG_Msk){}   /*!< Waiting for PLL stabilization */
    SysTick->CTRL = 0UL;                                          /*!< Stop SYSTICK */

    CLOCK_SetupFROClocking(48000000U);              /*!< Set up high frequency FRO output to selected frequency */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);                  /*!< Set AHBCLKDIV divider to value 1 */
    CLOCK_SetClkDiv(kCLOCK_DivFxI2s0MClk, 0U, true);                  /*!< Reset MCLKDIV divider counter and halt it */
    CLOCK_SetClkDiv(kCLOCK_DivFxI2s0MClk, 1U, false);                  /*!< Set MCLKDIV divider to value 1 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);                  /*!< Switch MAIN_CLK to FRO_HF */
    CLOCK_AttachClk(kSYS_PLL_to_FLEXCOMM6);                  /*!< Switch FLEXCOMM6 to SYS_PLL */
    CLOCK_AttachClk(kSYS_PLL_to_FLEXCOMM7);                  /*!< Switch FLEXCOMM7 to SYS_PLL */
    CLOCK_AttachClk(kSYS_PLL_to_MCLK);                  /*!< Switch MCLK to SYS_PLL */
    /*!< Set SystemCoreClock variable. */
    SystemCoreClock = BOARD_BOOTCLOCKRUN_CORE_CLOCK;
}

