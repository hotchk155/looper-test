///////////////////////////////////////////////////////////////////////////////////////////////////////
// DRIVER FOR DIRECT ACCESS TO SD CARD
#ifndef SDCARD_H_
#define SDCARD_H_

#define SDCARD_SPI_BASE SPI2
class CSDCard {
	enum {
		STATUS_POLL_RETRIES = 2000
	};
	enum {
		R1_IDLE				= 0x01,
		R1_ERASE_RESET		= 0x02,
		R1_COMMAND_ERR		= 0x04,
		R1_CRC_ERR			= 0x08,
		R1_ERASE_SEQ_ERR	= 0x10,
		R1_ADDRESS_ERR		= 0x20,
		R1_PARAMETER_ERR	= 0x40,

		R1_ERR_MASK			= 0x7E,

		OCR_2V7_2V8			= (1U<<15),
		OCR_2V8_2V9			= (1U<<16),
		OCR_2V9_3V0			= (1U<<17),
		OCR_3V0_3V1			= (1U<<18),
		OCR_3V1_3V2			= (1U<<19),
		OCR_3V2_3V3			= (1U<<20),
		OCR_3V3_3V4			= (1U<<21),
		OCR_3V4_3V5			= (1U<<22),
		OCR_3V5_3V6			= (1U<<23),
		OCR_HIGH_CAPACITY	= (1U<<30),
		OCR_POWER_STATUS	= (1U<<31),

		DATA_SINGLE_BLOCK	= 0xFE,
		DATA_MULTI_BLOCK	= 0xFC,
		DATA_STOP_TRAN		= 0xFD
	};
	enum {
		CMD0	= 0x40, //0,	//None(0)	R1	No	GO_IDLE_STATE	Software reset.
		CMD1	= 0x41, //1,	//None(0)	R1	No	SEND_OP_COND	Initiate initialization process.
		CMD8	= 0x48, //8, //*3	R7	No	SEND_IF_COND	For only SDC V2. Check voltage range.
		CMD16	= 0x50, //16,	//Block length[31:0]	R1	No	SET_BLOCKLEN	Change R/W block size.
		CMD17	= 0x51, //17, //	Address[31:0]	R1	Yes	READ_SINGLE_BLOCK	Read a block.
		CMD18	= 0x52, //18, //	Address[31:0]	R1	Yes	READ_MULTIPLE_BLOCK	Read multiple blocks.
		CMD24	= 0x58, //24,	//Address[31:0]	R1	Yes	WRITE_BLOCK	Write a block.
		CMD25	= 0x59, //25,	//Address[31:0]	R1	Yes	WRITE_MULTIPLE_BLOCK	Write multiple blocks.
		CMD58	= 0x7A, //58,	//None(0)	R3	No	READ_OCR	Read OCR.
		ACMDXX  = 0x77, //55,
		ACMD41  = 0x69 //41,
	};

	enum {
		ST_READY,
		ST_WRITE0,
		ST_WRITE1,
		ST_WRITE2,
		ST_WRITE3,
		ST_WRITE4,
		ST_READ0,
		ST_READ1,
		ST_READ2,
		ST_READ3,
		ST_FATAL,
		ST_STOP,

	};

	typedef struct {
		byte status;
		DBLK data;
		byte crc_hi;
		byte crc_lo;
	} SD_DATA_PACKET;

	byte m_data_resp;
	byte m_status;
	byte m_lba_mode;
	uint32_t m_response;

	SD_DATA_PACKET m_data_packet;
	byte m_state = ST_READY;
	byte m_cmd_buf[6];
	int m_retry;
	status_t m_api_result;




	//////////////////////////////////////////////
	//
	//
	//       AAAAAAAAAAAAAAAA
	//  +-----------------------------+
	//
	//  AAAAA               AAAAAAAAAAA
	//  +-----------------------------+

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Control chip select line for the card (active low)
	void csel(int val) {
	    GPIO_PinWrite(BOARD_INITPINS_SD_CSEL_GPIO, BOARD_INITPINS_SD_CSEL_PORT, BOARD_INITPINS_SD_CSEL_PIN, val);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Control 3V3 power for the card
	void power_sd_card(int val) {
		GPIO_PinWrite(BOARD_INITPINS_SD_POWER_GPIO, BOARD_INITPINS_SD_POWER_PORT, BOARD_INITPINS_SD_POWER_PIN, val);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send data to SD card in blocking mode (used during initialisation)
	byte tx_blocking(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = data;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		return(kStatus_Success == m_api_result);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Receive data from SD card in blocking mode (used during initialisation)
	byte rx_blocking(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = NULL;
		xfer.rxData = data;
		xfer.configFlags = 0U;
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		return(kStatus_Success == m_api_result);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Poll SD card until a status byte is returned, API error occurs or time out
	byte poll_for_status() {
	    m_status = 0xFF;
		int timeout = STATUS_POLL_RETRIES;
		while(timeout) {
			if(!rx_blocking(&m_status,1)) {
				return 0;
			}
			else if(m_status != 0xFF) {
				return 1;
			}
			--timeout;
		}
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Write a command, with 32-bit argument and CRC to SD card
	byte write_command(byte cmd,  uint32_t arg, byte crc) {
		byte msg[6] = {(byte)(cmd), (byte)(arg>>24), (byte)(arg>>16), (byte)(arg>>8), (byte)arg, crc};
		return tx_blocking(msg,6);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Execute a full command, in blocking mode, where R1 response type is expected (status only)
	byte do_cmd_R1(byte cmd,  uint32_t arg = 0, byte crc = 0xFF) {
		byte result = 0;
		csel(0);
		if(write_command(cmd, arg, crc)) {
			if(poll_for_status()) {
				result = 1;
			}
		}
		csel(1);
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Execute a full command, in blocking mode, where R3/R7 response type is expected (status with 32-bit payload)
	byte do_cmd_R3R7(byte cmd,  uint32_t arg = 0, byte crc = 0xFF) {
		byte result = 0;
		csel(0);
		if(write_command(cmd, arg, crc)) {
			if(poll_for_status()) {
				if(!(m_status & R1_ERR_MASK)) {
					byte data[4];
					spi_transfer_t xfer;
					xfer.dataSize = sizeof(data);
					xfer.txData = NULL;
					xfer.rxData = data;
					xfer.configFlags = 0U;
					m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
					if(kStatus_Success == m_api_result) {
						m_response =((uint32_t)data[0])<<24
								| 	((uint32_t)data[1])<<16
								| 	((uint32_t)data[2])<<8
								| 	((uint32_t)data[3]);
						result = 1;
					}
				}
			}
		}
		csel(1);
		return result;
	}

public:
	CSDCard() {
		m_status = 0;
		m_response = 0;
		m_data_resp = 0;
		m_lba_mode = 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_reset_card() {
		if(!do_cmd_R1(CMD0, 0, 0x95)) {
			return 0;
		}
		return (m_status == R1_IDLE);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_set_block_size(uint32_t block_size) {
		if(!do_cmd_R1(CMD16, block_size)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_read_ocr() {
		if(!do_cmd_R3R7(CMD58)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_check_voltage() {
		if(!do_cmd_R3R7(CMD8, 0x000001AA, 0x87)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_initialise_card() {
		for(int i=0; i<1000; ++i) {
			if(!do_cmd_R1(ACMDXX)) {
				return 0;
			}
			if(m_status != 0x01) {
				return 0;
			}
			if(!do_cmd_R1(ACMD41, 0x40000000, 1)) {
				return 0;
			}
			if(m_status == 0x00) {
				return 1;
			}
			if(m_status != 0x01) {
				return 0;
			}
			wait_ms();
		}
		return 0;
	}

	/*
	void send_data(	uint32_t addr, byte *data) {
		m_addr = addr;
		m_tx_buf[0] = 0xFE;
		memcpy(&m_tx_buf[1], data, 512);
		m_tx_buf[513] = 0xFF;
		m_tx_buf[514] = 0xFF;
		m_state = ST_WRITE0;
		while(m_state != ST_READY) {
			run();
		}
	}
	void read_data(	uint32_t addr, byte *data) {
		m_addr = addr;
		m_state = ST_READ0;
		while(m_state != ST_READY) {
			run();
		}
	}
*/


	byte check_for_write_block() {
		if(g_block_buffer.get_next_block_for_sd(&m_data_packet.data, &m_block_addr)) {
			m_data_packet.status = DATA_SINGLE_BLOCK;
			m_data_packet.crc_hi = 0xFF;
			m_data_packet.crc_lo = 0xFF;
			return 1;
		}
		return 0;
	}
	byte check_for_read_block() {
		return g_block_buffer.get_next_block_from_sd(&m_block_addr);
	}



	byte sd_tx(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = data;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		return (kStatus_Success == m_api_result);
	}

	byte sd_rx(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = NULL;
		xfer.rxData = data;
		xfer.configFlags = 0U;
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		return (kStatus_Success == m_api_result);
	}

	void run() {
		SD_ADDR sd_addr;
		switch(m_state) {
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_READY:
			if(check_for_read_block()) {
				m_state = ST_READ0;
			}
			else if(check_for_write_block()) {
				m_state = ST_WRITE0;
			}
			break;
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Start a new write
		case ST_WRITE0: // Send the CMD24
			sd_addr = block2addr(m_block_addr);
			csel(0);
			m_cmd_buf[0] = CMD24;
			m_cmd_buf[1] = (byte)(sd_addr>>24);
			m_cmd_buf[2] = (byte)(sd_addr>>16);
			m_cmd_buf[3] = (byte)(sd_addr>>8);
			m_cmd_buf[4] = (byte)(sd_addr);
			m_cmd_buf[5] = 0xFF;
			if(!sd_tx(m_cmd_buf, 6)) {
				m_state = ST_FATAL;
			}
			else {
				m_retry = 2000;
				m_state = ST_WRITE1;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE1: // Wait for response to CMD24
			if(!sd_rx(&m_status, 1)) {
				m_state = ST_FATAL;
			}
			else if(m_status == 0xFF) {
				if(!--m_retry) {
					m_state = ST_FATAL; // timeout
				}
			}
			else if(m_status != 0x00) {
				m_state = ST_FATAL; // timeout
			}
			else {
				m_retry = 2000;
				m_state = ST_WRITE2;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE2: // Delay in between response to CMD24 and the data packet
			if(!--m_retry) {
				m_state = ST_WRITE3;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE3: // data packet
			if(!sd_tx((byte*)&m_data_packet, sizeof(m_data_packet))) {
				m_state = ST_FATAL;
			}
			else {
				m_state = ST_WRITE4;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE4: // wait for card to drive DO high
			if(!sd_rx((byte*)&m_status, 1)) {
				m_state = ST_FATAL;
			}
			else if(m_status == 0xFF) { // ready for next

				csel(1); // de-assert CSEL

				if(check_for_write_block()) {
					m_state = ST_WRITE0;
				}
				else if(check_for_read_block()) {
					m_state = ST_READ0;
				}
				else {
					m_state = ST_READY;
				}
			}
			break;


		case ST_READ0:
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			csel(0);
			sd_addr = block2addr(m_block_addr);
			m_cmd_buf[0] = CMD17;
			m_cmd_buf[1] = (byte)(sd_addr>>24);
			m_cmd_buf[2] = (byte)(sd_addr>>16);
			m_cmd_buf[3] = (byte)(sd_addr>>8);
			m_cmd_buf[4] = (byte)(sd_addr);
			m_cmd_buf[5] = 0xFF;
			if(!sd_tx((byte*)&m_cmd_buf, 6)) {
				m_state = ST_FATAL;
			}
			else {
				m_retry = 2000;
				m_state = ST_READ1;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_READ1:
			if(!sd_rx((byte*)&m_status, 1)) {
				m_state = ST_FATAL;
			}
			else if(m_status == 0xFF) {
				if(!--m_retry) {
					m_state = ST_FATAL; // timeout
				}
			}
			else if(m_status != 0x00) {
				m_state = ST_FATAL; // timeout
			}
			else {
				m_retry = 2000;
				m_state = ST_READ2;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_READ2:
			if(!sd_rx(&m_status, 1)) {
				m_state = ST_FATAL;
			}
			else if(m_status == 0xFF) {
				if(!--m_retry) {
					m_state = ST_FATAL; // timeout
				}
			}
			else if(m_status == 0xFE) {
				m_state = ST_READ3;
			}
			else {
				m_state = ST_FATAL;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_READ3:
			if(!sd_rx(&m_data_packet[1], sizeof(m_data_packet) - 1)) {
				m_state = ST_FATAL;
			}
			else {
				csel(1); // de-assert CSEL

				g_block_buffer.put_block_from_sd(&m_data_packet.data);
				if(check_for_read_block()) {
					m_state = ST_READ0;
				}
				else if(check_for_write_block()) {
					m_state = ST_WRITE0;
				}
				else {
					m_state = ST_READY;
				}
			}
			break;

		case ST_FATAL:
			csel(1); // de-assert CSEL
			m_state = ST_STOP;
			break;
		case ST_STOP:
			break;
		}
	}




	byte init() {
		spi_master_config_t config;
		/*
		config->enableLoopback            = false;
	    config->enableMaster              = true;
	    config->polarity                  = kSPI_ClockPolarityActiveHigh;
	    config->phase                     = kSPI_ClockPhaseFirstEdge;
	    config->direction                 = kSPI_MsbFirst;
	    config->baudRate_Bps              = 500000U;
	    config->dataWidth                 = kSPI_Data8Bits;
	    config->sselNum                   = kSPI_Ssel0;
	    config->txWatermark               = (uint8_t)kSPI_TxFifo0;
	    config->rxWatermark               = (uint8_t)kSPI_RxFifo1;
	    config->sselPol                   = kSPI_SpolActiveAllLow;
	    config->delayConfig.preDelay      = 0U;
	    config->delayConfig.postDelay     = 0U;
	    config->delayConfig.frameDelay    = 0U;
	    config->delayConfig.transferDelay = 0U;
		 */

		csel(1); // set chip sel high

		// power cycle the SD card
		power_sd_card(0);
	    mydelay(10000U);
	    power_sd_card(1);
	    mydelay(100000U);

		SPI_MasterGetDefaultConfig(&config);
		config.polarity = kSPI_ClockPolarityActiveHigh;
		config.phase = kSPI_ClockPhaseFirstEdge;
	    config.baudRate_Bps              = 100000U; // slow clock rate
	    SPI_MasterInit(SDCARD_SPI_BASE, &config, CLOCK_GetFroHfFreq());
	    SPI_Enable(SDCARD_SPI_BASE,1);

	    SPI_SetDummyData(SDCARD_SPI_BASE,0xFF);

		spi_transfer_t xfer;
		byte data[10];
		xfer.dataSize = sizeof(data);
		xfer.txData = NULL;
		xfer.rxData = data;
		xfer.configFlags = 0U;
	    SPI_SetDummyData(SDCARD_SPI_BASE,0xFF);
		SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		SPI_MasterSetBaud(SDCARD_SPI_BASE, 1000000U, CLOCK_GetFroHfFreq());

		if(!cmd_reset_card()) {
			return 0;
		}
		if(!cmd_check_voltage()) {
			return 0;
		}
		if(!cmd_initialise_card()) {
			return 0;
		}
		if(!cmd_set_block_size(SZ_DBLK)) {
			return 0;
		}
		if(!cmd_read_ocr()) {
			return 0;
		}
		m_lba_mode = !!(m_response & OCR_HIGH_CAPACITY);


		//byte block[512] = {0};
		//for(int i=0; i<(int)sizeof(block); ++i) {
			//block[i]=(byte)i;
		//}
		//send_data(1024, block);
		//read_data(1024, block);
		return 1;
	}

	void deinit() {
	    SPI_Enable(SDCARD_SPI_BASE,0);
		SPI_Deinit(SDCARD_SPI_BASE);
	    power_sd_card(0);
	}

};


#endif /* SDCARD_H_ */
