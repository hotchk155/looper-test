/*
 * codec.h
 *
 *  Created on: 29 Jan 2020
 *      Author: jason
 */

#ifndef AUDIOIO_H_
#define AUDIOIO_H_

#define I2S_TX_MODULE (I2S0)
#define I2S_RX_MODULE (I2S1)
#define I2S_DMA_MODULE (DMA0)
#define I2S_TX_DMA_CHANNEL (13)
#define I2S_RX_DMA_CHANNEL (14)

extern "C"	void I2SDMATxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);
extern "C" void I2SDMARxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

class CAudioIO {

	dma_handle_t m_dma_tx_handle;
	dma_handle_t m_dma_rx_handle;
	i2s_dma_handle_t m_tx_handle;
	i2s_dma_handle_t m_rx_handle;

	i2s_transfer_t m_tx_transfer0;
	i2s_transfer_t m_tx_transfer1;
	i2s_transfer_t m_rx_transfer0;
	i2s_transfer_t m_rx_transfer1;

	enum {
		SZ_DMA_BLOCK = (256 * 2 * 2) // 256 samples x 2 channels x 16 bits
	};

	byte m_tx_buf0[SZ_DMA_BLOCK] __attribute__((aligned(4)));
	byte m_tx_buf1[SZ_DMA_BLOCK] __attribute__((aligned(4)));
	byte m_rx_buf0[SZ_DMA_BLOCK] __attribute__((aligned(4)));
	byte m_rx_buf1[SZ_DMA_BLOCK] __attribute__((aligned(4)));

	byte m_tx_toggle;
	byte m_rx_toggle;


	inline void block2dma(DBLK *block, byte *dma_buf) {
		int j = 0;
		byte *data = block->data;
		for(int i=0; i<512; i+=2) {
			dma_buf[j++] = data[i];
			dma_buf[j++] = data[i+1];
			dma_buf[j++] = data[i];
			dma_buf[j++] = data[i+1];
		}
	}
	inline void dma2block(byte *dma_buf, DBLK *block) {
		int j = 0;
		byte *data = block->data;
		for(int i=0; i<SZ_DMA_BLOCK; i+=4) {
			data[j++] = dma_buf[i];
			data[j++] = dma_buf[i+1];
		}
	}


	void codec_reset() {
	    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 0);
	    g_clock.delay(100);
	    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 1);
	}
public:
	CAudioIO() {
		memset(m_tx_buf0, 0, sizeof m_tx_buf0);
		memset(m_tx_buf1, 0, sizeof m_tx_buf1);
		memset(m_rx_buf0, 0, sizeof m_rx_buf0);
		memset(m_rx_buf1, 0, sizeof m_rx_buf1);
	}



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void init() {
		i2s_config_t config;

	    RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn);
	    RESET_PeripheralReset(kFC7_RST_SHIFT_RSTn);

	    EnableIRQ(FLEXCOMM6_IRQn);
	    EnableIRQ(FLEXCOMM7_IRQn);

	    codec_reset();

	    I2S_TxGetDefaultConfig(&config);
	    config.dataLength  = 16;
	    config.frameLength = 48;
	    config.divider     = 8;
	    config.masterSlave = kI2S_MasterSlaveNormalMaster;

	    //TODO
	    config.leftJust = 1;//?
	    config.wsPol=0;//?
		I2S_TxInit(I2S_TX_MODULE, &config);


	   I2S_RxGetDefaultConfig(&config);
	   config.dataLength	= 16;
	   config.frameLength   = 48;
	   config.divider     	= 8;
	   config.masterSlave = kI2S_MasterSlaveNormalSlave;
	   I2S_RxInit(I2S_RX_MODULE, &config);

	   DMA_Init(I2S_DMA_MODULE);
	   DMA_EnableChannel(I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL);
	   DMA_EnableChannel(I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL);
	   DMA_SetChannelPriority(I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL, kDMA_ChannelPriority3);
	   DMA_SetChannelPriority(I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL, kDMA_ChannelPriority2);
	   DMA_CreateHandle(&m_dma_tx_handle, I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL);
	   DMA_CreateHandle(&m_dma_rx_handle, I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL);

	   m_tx_transfer0.data = m_tx_buf0;
	   m_tx_transfer1.data = m_tx_buf1;
	   m_rx_transfer0.data = m_rx_buf0;
	   m_rx_transfer1.data = m_rx_buf1;
	   m_tx_transfer0.dataSize = sizeof(m_tx_buf0);
	   m_tx_transfer1.dataSize = sizeof(m_tx_buf1);
	   m_rx_transfer0.dataSize = sizeof(m_rx_buf0);
	   m_rx_transfer1.dataSize = sizeof(m_rx_buf1);


	   I2S_TxTransferCreateHandleDMA(I2S_TX_MODULE, &m_tx_handle, &m_dma_tx_handle, I2SDMATxCallback, NULL);
	   I2S_RxTransferCreateHandleDMA(I2S_RX_MODULE, &m_rx_handle, &m_dma_rx_handle, I2SDMARxCallback, NULL);

	}

	void start() {
		m_rx_toggle = 0;
		m_tx_toggle = 0;

		// queue both rx buffers for DMA so that the second can start up while we handle the
		// callback at the end of the first. The receiving I2S module is slaved to the transmitting
		// module, so transfer starts only when transmit module is done
		I2S_RxTransferReceiveDMA(I2S_RX_MODULE, &m_rx_handle, m_rx_transfer0);
		I2S_RxTransferReceiveDMA(I2S_RX_MODULE, &m_rx_handle, m_rx_transfer1);

		// queue both tx buffers for DMA for same reason
		I2S_TxTransferSendDMA(I2S_TX_MODULE, &m_tx_handle, m_tx_transfer0);
		I2S_TxTransferSendDMA(I2S_TX_MODULE, &m_tx_handle, m_tx_transfer1);
	}


	//TODO error checking
	inline void tx_callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
		DBLK block;

		// have we just finished playing buffer #1?
		if(m_tx_toggle) {
			// fill buffer #1 up with new audio and queue it again (if we fail to get more audio we'll keep
			// playing the same buffer content again...)
			if(g_looper.get_audio_out(&block)) {
				block2dma(&block, m_rx_buf1);
			}
			I2S_TxTransferSendDMA(base, handle, m_tx_transfer1);
		}
		else {
			// fill buffer #0 up with new audio and queue it again
			if(g_looper.get_audio_out(&block)) {
				block2dma(&block, m_rx_buf0);
			}
			I2S_TxTransferSendDMA(base, handle, m_tx_transfer0);
		}
		// now we're gonna switch buffers
		m_tx_toggle = !m_tx_toggle;
	}

	// called when receive buffer has been filled by DMA
//TODO error checking
	inline void rx_callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
		DBLK block;

		// are we filling buffer #1 ?
		if(m_rx_toggle) {
			// extract data from buffer #1 and queue it up ready to be filled again
			dma2block(m_rx_buf1, &block);
			I2S_RxTransferReceiveDMA(base, handle, m_rx_transfer1);
		}
		else {
			// extract data from buffer #0 and queue it up ready to be filled again
			dma2block(m_rx_buf0, &block);
			I2S_RxTransferReceiveDMA(base, handle, m_rx_transfer0);
		}
		// now we're gonna switch buffers
		m_rx_toggle = !m_rx_toggle;

		// throw the audio block over to the looper
		g_looper.on_audio_in(&block);
	}

};
CAudioIO g_audioio;

extern "C"	void I2SDMATxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
	g_audioio.tx_callback(base, handle, completionStatus, userData);
}

extern "C" void I2SDMARxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
	g_audioio.rx_callback(base, handle, completionStatus, userData);
}


#endif /* AUDIOIO_H_ */
