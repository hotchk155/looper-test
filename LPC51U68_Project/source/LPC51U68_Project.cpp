
#include <stdbool.h>
#include <cr_section_macros.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_crc.h"
#include "fsl_gpio.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "fsl_spi.h"
#include "fsl_spi_dma.h"
#include "fsl_mrt.h"
#include "fsl_ctimer.h"
#include "defs.h"
#include "ui.h"
#include "clock.h"
#include "pwm.h"
#include "sinewave.h"
#include "audioio.h"
#include "sdcard.h"
#include "block_buffer.h"
#include "recording.h"
#include "looper.h"

CClock g_clock;
CRecording g_recording;
void on_key_event(int key, int value) {
	switch(key) {
	case CUI::KEY_0: g_ui.set_led(0, value? CUI::LED_DUTY_ON : CUI::LED_DUTY_OFF); break;
	case CUI::KEY_1: g_ui.set_led(1, value? CUI::LED_DUTY_ON : CUI::LED_DUTY_OFF); break;
	case CUI::KEY_2: g_ui.set_led(2, value? CUI::LED_DUTY_ON : CUI::LED_DUTY_OFF); break;
	}

}

int main(void) {

	BOARD_InitBootClocks();
    BOARD_InitPins();


    g_clock.init();
	g_audioio.init();
	g_looper.init();
	g_pwm.init();

    g_audioio.set_callback(&g_looper);



	g_audioio.start();
	g_pwm.set_duty_0(50);
	g_pwm.set_duty_1(50);
	for(;;) {
		g_ui.run();
	}
}


#if 0
/*******************************************************************************
 * Definitions

 ******************************************************************************/
static dma_handle_t s_DmaTxHandle;
static dma_handle_t s_DmaRxHandle;
static i2s_dma_handle_t s_TxHandle;
static i2s_dma_handle_t s_RxHandle;

static i2s_config_t s_TxConfig;
//static i2s_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;


static i2s_config_t s_RxConfig;
//static i2s_handle_t s_RxHandle;
static i2s_transfer_t s_RxTransfer;



#if defined(__GNUC__) /* GNU Compiler */
#ifndef __ALIGN_END
#define __ALIGN_END __attribute__((aligned(4)))
#endif
#ifndef __ALIGN_BEGIN
#define __ALIGN_BEGIN
#endif
#else
#ifndef __ALIGN_END
#define __ALIGN_END
#endif
#ifndef __ALIGN_BEGIN
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) /* ARM Compiler */
#define __ALIGN_BEGIN __attribute__((aligned(4)))
#elif defined(__ICCARM__) /* IAR Compiler */
#define __ALIGN_BEGIN
#endif
#endif
#endif

/*!
 * @brief One period of sine wave in 108 32-bit samples.
 * One sample contains two 16-bit channels.
 */
__ALIGN_BEGIN uint8_t g_Tx[108*4] __ALIGN_END = {
    0x00U, 0x00U, 0x00U, 0x00U, 0x71U, 0x07U, 0x71U, 0x07U, 0xDCU, 0x0EU, 0xDCU, 0x0EU, 0x39U, 0x16U, 0x39U, 0x16U,
    0x84U, 0x1DU, 0x84U, 0x1DU, 0xB5U, 0x24U, 0xB5U, 0x24U, 0xC6U, 0x2BU, 0xC6U, 0x2BU, 0xB2U, 0x32U, 0xB2U, 0x32U,
    0x71U, 0x39U, 0x71U, 0x39U, 0xFFU, 0x3FU, 0xFFU, 0x3FU, 0x55U, 0x46U, 0x55U, 0x46U, 0x6FU, 0x4CU, 0x6FU, 0x4CU,
    0x46U, 0x52U, 0x46U, 0x52U, 0xD6U, 0x57U, 0xD6U, 0x57U, 0x19U, 0x5DU, 0x19U, 0x5DU, 0x0CU, 0x62U, 0x0CU, 0x62U,
    0xABU, 0x66U, 0xABU, 0x66U, 0xF0U, 0x6AU, 0xF0U, 0x6AU, 0xD9U, 0x6EU, 0xD9U, 0x6EU, 0x61U, 0x72U, 0x61U, 0x72U,
    0x87U, 0x75U, 0x87U, 0x75U, 0x46U, 0x78U, 0x46U, 0x78U, 0x9EU, 0x7AU, 0x9EU, 0x7AU, 0x8BU, 0x7CU, 0x8BU, 0x7CU,
    0x0DU, 0x7EU, 0x0DU, 0x7EU, 0x21U, 0x7FU, 0x21U, 0x7FU, 0xC7U, 0x7FU, 0xC7U, 0x7FU, 0xFEU, 0x7FU, 0xFEU, 0x7FU,
    0xC7U, 0x7FU, 0xC7U, 0x7FU, 0x21U, 0x7FU, 0x21U, 0x7FU, 0x0DU, 0x7EU, 0x0DU, 0x7EU, 0x8BU, 0x7CU, 0x8BU, 0x7CU,
    0x9EU, 0x7AU, 0x9EU, 0x7AU, 0x46U, 0x78U, 0x46U, 0x78U, 0x87U, 0x75U, 0x87U, 0x75U, 0x61U, 0x72U, 0x61U, 0x72U,
    0xD9U, 0x6EU, 0xD9U, 0x6EU, 0xF0U, 0x6AU, 0xF0U, 0x6AU, 0xABU, 0x66U, 0xABU, 0x66U, 0x0CU, 0x62U, 0x0CU, 0x62U,
    0x19U, 0x5DU, 0x19U, 0x5DU, 0xD6U, 0x57U, 0xD6U, 0x57U, 0x46U, 0x52U, 0x46U, 0x52U, 0x6FU, 0x4CU, 0x6FU, 0x4CU,
    0x55U, 0x46U, 0x55U, 0x46U, 0xFFU, 0x3FU, 0xFFU, 0x3FU, 0x71U, 0x39U, 0x71U, 0x39U, 0xB2U, 0x32U, 0xB2U, 0x32U,
    0xC6U, 0x2BU, 0xC6U, 0x2BU, 0xB5U, 0x24U, 0xB5U, 0x24U, 0x84U, 0x1DU, 0x84U, 0x1DU, 0x39U, 0x16U, 0x39U, 0x16U,
    0xDCU, 0x0EU, 0xDCU, 0x0EU, 0x71U, 0x07U, 0x71U, 0x07U, 0x00U, 0x00U, 0x00U, 0x00U, 0x8FU, 0xF8U, 0x8FU, 0xF8U,
    0x24U, 0xF1U, 0x24U, 0xF1U, 0xC7U, 0xE9U, 0xC7U, 0xE9U, 0x7CU, 0xE2U, 0x7CU, 0xE2U, 0x4BU, 0xDBU, 0x4BU, 0xDBU,
    0x3AU, 0xD4U, 0x3AU, 0xD4U, 0x4EU, 0xCDU, 0x4EU, 0xCDU, 0x8FU, 0xC6U, 0x8FU, 0xC6U, 0x01U, 0xC0U, 0x01U, 0xC0U,
    0xABU, 0xB9U, 0xABU, 0xB9U, 0x91U, 0xB3U, 0x91U, 0xB3U, 0xBAU, 0xADU, 0xBAU, 0xADU, 0x2AU, 0xA8U, 0x2AU, 0xA8U,
    0xE7U, 0xA2U, 0xE7U, 0xA2U, 0xF4U, 0x9DU, 0xF4U, 0x9DU, 0x55U, 0x99U, 0x55U, 0x99U, 0x10U, 0x95U, 0x10U, 0x95U,
    0x27U, 0x91U, 0x27U, 0x91U, 0x9FU, 0x8DU, 0x9FU, 0x8DU, 0x79U, 0x8AU, 0x79U, 0x8AU, 0xBAU, 0x87U, 0xBAU, 0x87U,
    0x62U, 0x85U, 0x62U, 0x85U, 0x75U, 0x83U, 0x75U, 0x83U, 0xF3U, 0x81U, 0xF3U, 0x81U, 0xDFU, 0x80U, 0xDFU, 0x80U,
    0x39U, 0x80U, 0x39U, 0x80U, 0x02U, 0x80U, 0x02U, 0x80U, 0x39U, 0x80U, 0x39U, 0x80U, 0xDFU, 0x80U, 0xDFU, 0x80U,
    0xF3U, 0x81U, 0xF3U, 0x81U, 0x75U, 0x83U, 0x75U, 0x83U, 0x62U, 0x85U, 0x62U, 0x85U, 0xBAU, 0x87U, 0xBAU, 0x87U,
    0x79U, 0x8AU, 0x79U, 0x8AU, 0x9FU, 0x8DU, 0x9FU, 0x8DU, 0x27U, 0x91U, 0x27U, 0x91U, 0x10U, 0x95U, 0x10U, 0x95U,
    0x55U, 0x99U, 0x55U, 0x99U, 0xF4U, 0x9DU, 0xF4U, 0x9DU, 0xE7U, 0xA2U, 0xE7U, 0xA2U, 0x2AU, 0xA8U, 0x2AU, 0xA8U,
    0xBAU, 0xADU, 0xBAU, 0xADU, 0x91U, 0xB3U, 0x91U, 0xB3U, 0xABU, 0xB9U, 0xABU, 0xB9U, 0x01U, 0xC0U, 0x01U, 0xC0U,
    0x8FU, 0xC6U, 0x8FU, 0xC6U, 0x4EU, 0xCDU, 0x4EU, 0xCDU, 0x3AU, 0xD4U, 0x3AU, 0xD4U, 0x4BU, 0xDBU, 0x4BU, 0xDBU,
    0x7CU, 0xE2U, 0x7CU, 0xE2U, 0xC7U, 0xE9U, 0xC7U, 0xE9U, 0x24U, 0xF1U, 0x24U, 0xF1U, 0x8FU, 0xF8U, 0x8FU, 0xF8U,
};

__ALIGN_BEGIN uint8_t g_Rx[108*4] __ALIGN_END = {0};


void copy_rx_to_tx() {
	for(unsigned int i=0; i<sizeof(g_Tx); i+=4) {
		g_Tx[i] = g_Rx[i];
		g_Tx[i+1] = g_Rx[i+1];
	}
}


#define DEMO_I2S_MASTER_CLOCK_FREQUENCY (44100 * 256)
#define DEMO_I2S_TX (I2S0)
#define DEMO_I2S_RX (I2S1)
#define DEMO_I2S_CLOCK_DIVIDER (CLOCK_GetPllOutFreq() / 44100U / 16U / 2U)
#define DEMO_I2S_RX_MODE (kI2S_MasterSlaveNormalSlave)
#define DEMO_I2S_TX_MODE (kI2S_MasterSlaveNormalMaster)
#define DEMO_AUDIO_BIT_WIDTH (16)
#define DEMO_AUDIO_PROTOCOL kCODEC_BusI2S
/*
static void TxCallback(I2S_Type *base, i2s_handle_t *handle, status_t completionStatus, void *userData)
{
    // Enqueue the same original s_Buffer all over again
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferNonBlocking(base, handle, *transfer);
}


static void RxCallback(I2S_Type *base, i2s_handle_t *handle, status_t completionStatus, void *userData)
{
	copy_rx_to_tx();
    // Enqueue the same original s_Buffer all over again
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_RxTransferNonBlocking(base, handle, *transfer);
}
*/

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
    /* Enqueue the same original buffer all over again */
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferSendDMA(base, handle, *transfer);
}

static void RxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	copy_rx_to_tx();
    /* Enqueue the same original buffer all over again */
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_RxTransferReceiveDMA(base, handle, *transfer);
}



int main(void)
{

	BOARD_InitBootClocks();

    RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kFC7_RST_SHIFT_RSTn);

    EnableIRQ(FLEXCOMM6_IRQn);
    EnableIRQ(FLEXCOMM7_IRQn);

    BOARD_InitPins();
//    BOARD_InitDebugConsole();

    g_clock.init();
    g_clock.delay(100);


    mydelay(100000U);

    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 0);
    mydelay(100000U);
    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 1);


    //PRINTF("Configure WM8904 codec\r\n");

    /*
     * masterSlave = kI2S_MasterSlaveNormalMaster;
     * mode = kI2S_ModeI2sClassic;
     * rightLow = false;
     * leftJust = false;
     * pdmData = false;
     * sckPol = false;
     * wsPol = false;
     * divider = 1;
     * oneChannel = false;
     * dataLength = 16;
     * frameLength = 32;
     * position = 0;
     * watermark = 4;
     * txEmptyZero = true;
     * pack48 = false;
     */
    I2S_TxGetDefaultConfig(&s_TxConfig);
    s_TxConfig.dataLength     = 16;
    s_TxConfig.frameLength     = 48;
    s_TxConfig.divider     = 8;
    s_TxConfig.masterSlave = kI2S_MasterSlaveNormalMaster;

    //TODO
    s_TxConfig.leftJust = 1;//?
    s_TxConfig.wsPol=0;//?


    /*
        * masterSlave = kI2S_MasterSlaveNormalSlave;
        * mode = kI2S_ModeI2sClassic;
        * rightLow = false;
        * leftJust = false;
        * pdmData = false;
        * sckPol = false;
        * wsPol = false;
        * divider = 1;
        * oneChannel = false;
        * dataLength = 16;
        * frameLength = 32;
        * position = 0;
        * watermark = 4;
        * txEmptyZero = false;
        * pack48 = false;
   */
   I2S_RxGetDefaultConfig(&s_RxConfig);
   s_RxConfig.dataLength     = 16;
   s_RxConfig.frameLength     = 48;
   s_RxConfig.divider     = 8;
   s_RxConfig.masterSlave = kI2S_MasterSlaveNormalSlave;

   I2S_TxInit(DEMO_I2S_TX, &s_TxConfig);
   I2S_RxInit(DEMO_I2S_RX, &s_RxConfig);


#define DEMO_I2S_TX_CHANNEL (13)
#define DEMO_I2S_RX_CHANNEL (14)

   DMA_Init(DMA0);
   DMA_EnableChannel(DMA0, DEMO_I2S_TX_CHANNEL);
   DMA_EnableChannel(DMA0, DEMO_I2S_RX_CHANNEL);
   DMA_SetChannelPriority(DMA0, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3);
   DMA_SetChannelPriority(DMA0, DEMO_I2S_RX_CHANNEL, kDMA_ChannelPriority2);
   DMA_CreateHandle(&s_DmaTxHandle, DMA0, DEMO_I2S_TX_CHANNEL);
   DMA_CreateHandle(&s_DmaRxHandle, DMA0, DEMO_I2S_RX_CHANNEL);


   I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX, &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
   I2S_RxTransferCreateHandleDMA(DEMO_I2S_RX, &s_RxHandle, &s_DmaRxHandle, RxCallback, (void *)&s_RxTransfer);

   copy_rx_to_tx();

   s_TxTransfer.data     = &g_Tx[0];
   s_TxTransfer.dataSize = sizeof(g_Tx);

   s_RxTransfer.data     = &g_Rx[0];
   s_RxTransfer.dataSize = sizeof(g_Rx);


   /* need to queue two receive buffers so when the first one is filled,
    * the other is immediatelly starts to be filled */
   I2S_RxTransferReceiveDMA(DEMO_I2S_RX, &s_RxHandle, s_RxTransfer);
   I2S_RxTransferReceiveDMA(DEMO_I2S_RX, &s_RxHandle, s_RxTransfer);

   /* need to queue two transmit buffers so when the first one
    * finishes transfer, the other immediatelly starts */
   I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer);
   I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer);


   /*
    copy_rx_to_tx(); // clear L chan

    s_TxTransfer.data     = &g_Tx[0];
    s_TxTransfer.dataSize = sizeof(g_Tx);

    s_RxTransfer.data     = &g_Rx[0];
    s_RxTransfer.dataSize = sizeof(g_Rx);

    I2S_TxTransferCreateHandle(DEMO_I2S_TX, &s_TxHandle, TxCallback, (void *)&s_TxTransfer);
    I2S_RxTransferCreateHandle(DEMO_I2S_RX, &s_RxHandle, RxCallback, (void *)&s_RxTransfer);

   volatile status_t x1 = I2S_TxTransferNonBlocking(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer);
   volatile status_t x2 = I2S_RxTransferNonBlocking(DEMO_I2S_RX, &s_RxHandle, s_RxTransfer);
*/
    CSDCard sd;
    sd.init();
    sd.deinit();
    while (1)
    {
    }
}
#endif
