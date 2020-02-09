///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LOW LEVEL AUDIO INTERFACE
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef AUDIOIO_H_
#define AUDIOIO_H_

//
// DEFINITIONS
//

class IAudioCallback {
public:
	virtual int get_audio_block(SAMPLE_BLOCK *block) = 0;
	virtual int put_audio_block(SAMPLE_BLOCK *block) = 0;
};

#define I2S_TX_MODULE (I2S0)
#define I2S_RX_MODULE (I2S1)
#define I2S_DMA_MODULE (DMA0)
#define I2S_TX_DMA_CHANNEL (13)
#define I2S_RX_DMA_CHANNEL (14)

extern "C"	void I2SDMATxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);
extern "C" void I2SDMARxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This object communicates with the I2S CODEC and drives the looper by consuming
// playback data and providing recording data at the samplerate defined by the
// FLEXCOMM clocks (from PLL clock) and dividers (44.1kHz, 16bit)
class CAudioIO {

	enum {
		SZ_DMA_SAMPLE_BLOCK = (SZ_SAMPLE_BLOCK * 2) // need L + R channels for I2S
	};

	// API data
	status_t m_api_status;
	dma_handle_t m_dma_tx_handle;
	dma_handle_t m_dma_rx_handle;
	i2s_dma_handle_t m_tx_handle;
	i2s_dma_handle_t m_rx_handle;
	i2s_transfer_t m_tx_transfer0;
	i2s_transfer_t m_tx_transfer1;
	i2s_transfer_t m_rx_transfer0;
	i2s_transfer_t m_rx_transfer1;

	// pairs of ping-pong buffers used to feed the DMA controller for audio out and in.
	// Each is large enough to contain the 256 x 16 bit samples in a single 512 data block
	// from the looper, however for the I2S peripheral left and right channels are needed,
	// so the data blocks are expanded when copied into these buffers
	SAMPLE m_tx_buf0[SZ_DMA_SAMPLE_BLOCK] __attribute__((aligned(4)));
	SAMPLE m_tx_buf1[SZ_DMA_SAMPLE_BLOCK] __attribute__((aligned(4)));
	SAMPLE m_rx_buf0[SZ_DMA_SAMPLE_BLOCK] __attribute__((aligned(4)));
	SAMPLE m_rx_buf1[SZ_DMA_SAMPLE_BLOCK] __attribute__((aligned(4)));

	// flags record which ping-pong buffer is in use for audio out and in
	byte m_tx_toggle;
	byte m_rx_toggle;

	IAudioCallback *m_callback;

	//////////////////////////////////////////////////////////////////////////////////////
	// double up the samples for stereo by duplicating data to both channels
	inline void mono2stereo(SAMPLE_BLOCK *block, SAMPLE *dma_buf) {
		auto count = 0;
		SAMPLE *data = block->data;
		while(count++ < SZ_SAMPLE_BLOCK) {
			*dma_buf++ = *data;
			*dma_buf++ = *data++;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// shrink stereo to mono samples by losing one channel
	inline void stereo2mono(SAMPLE *dma_buf, SAMPLE_BLOCK *block) {
		auto count = 0;
		SAMPLE *data = block->data;
		while(count++ < SZ_SAMPLE_BLOCK) {
			*data++ = *dma_buf;
			dma_buf+=2;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// reset the CODEC
	void codec_reset() {
	    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 0);
	    g_clock.delay(10);
	    GPIO_PinWrite(BOARD_INITPINS_CODEC_RESET_GPIO, BOARD_INITPINS_CODEC_RESET_PORT, BOARD_INITPINS_CODEC_RESET_PIN, 1);
	}
public:
	CAudioIO() {
		m_callback = NULL;
	}
	void set_callback(IAudioCallback *callback) {
		m_callback = callback;
	}



	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void init() {
		i2s_config_t config;

		// prepare buffers and API objects to reference them
		memset(m_tx_buf0, 0, sizeof m_tx_buf0);
		memset(m_tx_buf1, 0, sizeof m_tx_buf1);
		memset(m_rx_buf0, 0, sizeof m_rx_buf0);
		memset(m_rx_buf1, 0, sizeof m_rx_buf1);
		m_tx_transfer0.data = (byte*)m_tx_buf0;
		m_tx_transfer1.data = (byte*)m_tx_buf1;
		m_rx_transfer0.data = (byte*)m_rx_buf0;
		m_rx_transfer1.data = (byte*)m_rx_buf1;
		m_tx_transfer0.dataSize = sizeof(m_tx_buf0);
		m_tx_transfer1.dataSize = sizeof(m_tx_buf1);
		m_rx_transfer0.dataSize = sizeof(m_rx_buf0);
		m_rx_transfer1.dataSize = sizeof(m_rx_buf1);

		// reset the I2S peripherals
	    RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn);
	    RESET_PeripheralReset(kFC7_RST_SHIFT_RSTn);

		// enable IRQ on I2S
	    EnableIRQ(FLEXCOMM6_IRQn);
	    EnableIRQ(FLEXCOMM7_IRQn);

	    // reset the CODEC
	    codec_reset();

	    // configure transmitting peripheral (I2S master)
	    I2S_TxGetDefaultConfig(&config);
	    config.dataLength  	= 16;
	    config.frameLength 	= 48;
	    config.divider     	= 8;
	    config.leftJust		= 1;
	    config.masterSlave = kI2S_MasterSlaveNormalMaster;
	    I2S_TxInit(I2S_TX_MODULE, &config);

	    // configure receiving peripheral (Slaved to Tx)
	    I2S_RxGetDefaultConfig(&config);
	    config.dataLength	= 16;
	    config.frameLength  = 48;
	    config.divider     	= 8;
	    config.leftJust		= 1;
	    config.masterSlave 	= kI2S_MasterSlaveNormalSlave;
	    I2S_RxInit(I2S_RX_MODULE, &config);

	    // configure the DMA controller
		DMA_Init(I2S_DMA_MODULE);
		DMA_EnableChannel(I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL);
		DMA_EnableChannel(I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL);
		DMA_SetChannelPriority(I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL, kDMA_ChannelPriority3);
		DMA_SetChannelPriority(I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL, kDMA_ChannelPriority2);
		DMA_CreateHandle(&m_dma_tx_handle, I2S_DMA_MODULE, I2S_TX_DMA_CHANNEL);
		DMA_CreateHandle(&m_dma_rx_handle, I2S_DMA_MODULE, I2S_RX_DMA_CHANNEL);
		I2S_TxTransferCreateHandleDMA(I2S_TX_MODULE, &m_tx_handle, &m_dma_tx_handle, I2SDMATxCallback, NULL);
		I2S_RxTransferCreateHandleDMA(I2S_RX_MODULE, &m_rx_handle, &m_dma_rx_handle, I2SDMARxCallback, NULL);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Start up the DMA transfers. Once started they will run continually as more transfers are queued on completion
	void start() {

		// reset ping-pong buffer status
		m_rx_toggle = 0;
		m_tx_toggle = 0;

		// queue both rx buffers for DMA so that the second can start up while we handle the
		// callback at the end of the first. The receiving I2S module is slaved to the transmitting
		// module, so transfer starts only when transmit module is done
		m_api_status = I2S_RxTransferReceiveDMA(I2S_RX_MODULE, &m_rx_handle, m_rx_transfer0);
		m_api_status = I2S_RxTransferReceiveDMA(I2S_RX_MODULE, &m_rx_handle, m_rx_transfer1);

		// queue both tx buffers for DMA for same reason
		m_api_status = I2S_TxTransferSendDMA(I2S_TX_MODULE, &m_tx_handle, m_tx_transfer0);
		m_api_status = I2S_TxTransferSendDMA(I2S_TX_MODULE, &m_tx_handle, m_tx_transfer1);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// called when transmit buffer has been sent by DMA
	//TODO error checking
	inline void tx_callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
		SAMPLE_BLOCK block;

		// have we just finished playing buffer #1?
		if(m_tx_toggle) {
			// fill buffer #1 up with new audio and queue it again (if we fail to get more audio we'll keep
			// playing the same buffer content again...)
			if(m_callback->get_audio_block(&block)) {
				mono2stereo(&block, m_tx_buf1);
			}
			I2S_TxTransferSendDMA(base, handle, m_tx_transfer1);
		}
		else {
			// fill buffer #0 up with new audio and queue it again
			if(m_callback->get_audio_block(&block)) {
				mono2stereo(&block, m_tx_buf0);
			}
			I2S_TxTransferSendDMA(base, handle, m_tx_transfer0);
		}
		// now we're gonna switch buffers
		m_tx_toggle = !m_tx_toggle;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// called when receive buffer has been filled by DMA
//TODO error checking
	inline void rx_callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
		SAMPLE_BLOCK block;

		// are we filling buffer #1 ?
		if(m_rx_toggle) {
			// extract data from buffer #1 and queue it up ready to be filled again
			stereo2mono(m_rx_buf1, &block);
			I2S_RxTransferReceiveDMA(base, handle, m_rx_transfer1);
		}
		else {
			// extract data from buffer #0 and queue it up ready to be filled again
			stereo2mono(m_rx_buf0, &block);
			I2S_RxTransferReceiveDMA(base, handle, m_rx_transfer0);
		}
		// now we're gonna switch buffers
		m_rx_toggle = !m_rx_toggle;

		// throw the audio block over to the looper
		m_callback->put_audio_block(&block);
	}

};

// global instance of the audio interface
CAudioIO g_audioio;

// Define the callback functions for DMA transfrer completion
extern "C"	void I2SDMATxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
	g_audioio.tx_callback(base, handle, completionStatus, userData);
}
extern "C" void I2SDMARxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
	g_audioio.rx_callback(base, handle, completionStatus, userData);
}


#endif /* AUDIOIO_H_ */
