/*
 * sdcard.h
 *
 *  Created on: 26 Jan 2020
 *      Author: jason
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#define SDCARD_SPI_BASE SPI2
class CSDCard {
	byte m_data_resp;
	byte m_status;
	byte m_lba_mode;
	uint32_t m_response;

public:
	CSDCard() {
		m_status = 0;
		m_response = 0;
		m_data_resp = 0;
		m_lba_mode = 0;




		crc_config_t config;
		config.polynomial    = kCRC_Polynomial_CRC_16;
		config.reverseIn     = true;
		config.complementIn  = false;
		config.reverseOut    = true;
		config.complementOut = false;
		config.seed          = 0;
		CRC_Init(CRC_ENGINE, &config);
	}
	enum {
		SD_BLOCK_SIZE 			= 512
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
		OCR_POWER_STATUS	= (1U<<31)

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


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	uint16_t crc16(byte *data, int len) {
	    CRC_WriteData(CRC_ENGINE, data, len);
	    return CRC_Get16bitResult(CRC_ENGINE);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_set_block_size(uint32_t block_size) {
		if(!do_cmd(CMD16, block_size)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_read_ocr() {
		if(!do_cmd(CMD58, 0, 1)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_reset_card() {
		if(!do_cmd(CMD0, 0, 0, 0x95)) {
			return 0;
		}
		return (m_status == R1_IDLE);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_check_voltage() {
		if(!do_cmd(CMD8, 0x000001AA, 1, 0x87)) {
			return 0;
		}
		return !(m_status & R1_ERR_MASK);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_initialise_card() {
		for(int i=0; i<1000; ++i) {
			if(!do_cmd(ACMDXX)) {
				return 0;
			}
			if(m_status != 0x01) {
				return 0;
			}
			if(!do_cmd(ACMD41, 0x40000000, 1)) {
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_read_block(uint32_t address, byte *data) {

		byte result = 0;
		csel(0); // set chip sel low
		if(write_command(CMD17, address, 0xFF)) {
			if(read_response_block(data)) {
				result = 1;
			}
		}
		csel(1); // set chip sel low
	}




	void csel(int val) {
	    GPIO_PinWrite(BOARD_INITPINS_SD_CSEL_GPIO, BOARD_INITPINS_SD_CSEL_PORT, BOARD_INITPINS_SD_CSEL_PIN, val);
	}


	byte do_cmd(byte cmd,  uint32_t arg = 0, byte is_resp_data = 0, byte crc = 0xFF) {
		byte result = 0;
		csel(0); // set chip sel low
		if(write_command(cmd, arg, crc)) {
			for(int i=0; i<2000; ++i) {
				if(is_resp_data) {
					result = read_R3R7();
					if(!result) {
						break;
					}
				}
				else {
					result = read_R1();
					if(!result) {
						break;
					}
				}
				if(m_status != 0xFF) {
					break;
				}
			}
		}
		csel(1); // set chip sel low
		return result;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Write a command to SD card
	byte write_command(byte cmd,  uint32_t arg, byte crc) {
		byte msg[6] = {(byte)(cmd), (byte)(arg>>24), (byte)(arg>>16), (byte)(arg>>8), (byte)arg, crc};
		spi_transfer_t xfer;
		xfer.dataSize = sizeof(msg);
		xfer.txData = msg;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		return (kStatus_Success == SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Read an R1 response (no data payload)
	byte read_R1() {
	    spi_transfer_t xfer;
	    m_status = 0xFF;
	    m_response = 0;
		xfer.dataSize = 1;
		xfer.txData = NULL;
		xfer.rxData = &m_status;
		xfer.configFlags = 0U;
		return (kStatus_Success == SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Read an R3 or R7 response (4-byte data payload)
	byte read_R3R7() {

	    m_status = 0xFF;
	    m_response = 0;

	    spi_transfer_t xfer;
		xfer.dataSize = 1;
		xfer.txData = NULL;
		xfer.rxData = &m_status;
		xfer.configFlags = 0U;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}
		if(m_status == 0xFF) {
			return 1;
		}

		byte resp[4];
		xfer.dataSize = 4;
		xfer.rxData = resp;
		if(kStatus_Success == SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			m_response =((uint32_t)resp[1])<<24
					| 	((uint32_t)resp[2])<<16
					| 	((uint32_t)resp[3])<<8
					| 	((uint32_t)resp[4]);
			return 1;
		}
		else {
			return 0;
		}

	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte read_data_resp() {
	    spi_transfer_t xfer;
	    m_data_resp = 0xFF;
		xfer.dataSize = 1;
		xfer.txData = NULL;
		xfer.rxData = &m_data_resp;
		xfer.configFlags = 0U;
		for(int i=0; i<1000; ++i) {
			if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
				return 0;
			}
			if(m_data_resp != 0xFF) {
				break;
			}
		}
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte wait_for_write() {
	    spi_transfer_t xfer;
	    byte result = 0x00;
		xfer.dataSize = 1;
		xfer.txData = NULL;
		xfer.rxData = &result;
		xfer.configFlags = 0U;
		for(int i=0; i<1000; ++i) {
			if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
				return 0;
			}
			if(result) {
				break;
			}
		}
		return 1;
	}



	enum {
		ST_READY,
		ST_WRITE0,
		ST_WRITE1,
		ST_WRITE2,
		ST_WRITE3,
		ST_WRITE4,
		ST_FATAL,
		ST_STOP,

	};

	enum {
		SZ_DATA_PACKET = 1 + 512 + 2
	};
	byte m_state = ST_READY;
	byte m_cmd_buf[6];
	byte m_tx_buf[1+512+2];
	uint32_t m_addr;
	int m_retry;
	status_t m_api_result;
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
	void run() {

	    spi_transfer_t xfer;



		switch(m_state) {
		case ST_WRITE0: // Send the CMD24
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			csel(0);
			m_cmd_buf[0] = CMD24;
			m_cmd_buf[1] = (byte)(m_addr>>24);
			m_cmd_buf[2] = (byte)(m_addr>>16);
			m_cmd_buf[3] = (byte)(m_addr>>8);
			m_cmd_buf[4] = (byte)(m_addr);
			m_cmd_buf[5] = 0xFF;
			xfer.dataSize = 6;
			xfer.txData = m_cmd_buf;
			xfer.rxData = NULL;
			xfer.configFlags = 0U;
			m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
			if(kStatus_Success != m_api_result) {
				m_state = ST_FATAL;
			}
			else {
				m_retry = 2000;
				m_state = ST_WRITE1;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE1: // Wait for response to CMD24
			xfer.dataSize = 1;
			xfer.txData = NULL;
			xfer.rxData = &m_status;
			xfer.configFlags = 0U;
			m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
			if(kStatus_Success != m_api_result) {
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
			xfer.dataSize = sizeof(m_tx_buf);
			xfer.txData = m_tx_buf;
			xfer.rxData = NULL;
			xfer.configFlags = 0U;
			m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
			if(kStatus_Success != m_api_result) {
				m_state = ST_FATAL;
			}
			else {
				m_state = ST_WRITE4;
			}
			break;
			////////////////////////////////////////////////////////////////////////////////////////////////////////
		case ST_WRITE4: // wait for card to drive DO high
			xfer.dataSize = 1;
			xfer.txData = NULL;
			xfer.rxData = &m_status;
			xfer.configFlags = 0U;
			m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
			if(kStatus_Success != m_api_result) {
				m_state = ST_FATAL;
			}
			else if(m_status == 0xFF) {
				m_state = ST_READY;
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


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte cmd_write_block(uint32_t address, byte *data) {

		byte result = 0;
		csel(0); // set chip sel low
		if(write_command(CMD24, address, 0xFF)) {
			if(read_R1()) {
				if(m_status == 0x00) {

					for(int j=0; j<10000; ++j) {
						 __asm volatile ("NOP");
					}
					if(write_data_block(data)) {
						if(read_data_resp()) {
							if((m_data_resp & 0x1F) == 0x05) {
								if(wait_for_write()) {
									result = 1;
								}
							}
						}
					}
				}
			}
		}
		csel(1); // set chip sel low
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte write_data_block(byte *data) {
	    spi_transfer_t xfer;

		csel(0); // set chip sel low

		byte token = 0xFE; // need to change for multiple block write
		xfer.dataSize = 1;
		xfer.txData = &token;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}

		xfer.dataSize = SD_BLOCK_SIZE;
		xfer.txData = data;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}
		uint16_t crc = 0xFFFF;
		xfer.dataSize = 2;
		xfer.txData = (byte*)&crc;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}
		return 1;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte read_response_block(byte * buf) {

	    m_status = 0xFF;
	    m_response = 0;

	    spi_transfer_t xfer;
		xfer.dataSize = 1;
		xfer.txData = NULL;
		xfer.rxData = &m_status;
		xfer.configFlags = 0U;

		int i;
		for(i=0; i<1000; ++i) {
			if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
				return 0;
			}
			if(m_status != 0xFF) {
				break;
			}
		}
		if(0x00 != m_status) {
			return 0;
		}
		for(i=0; i<1000; ++i) {
			if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
				return 0;
			}
			if(m_status != 0xFF) {
				break;
			}
		}
		if(0xFC != (m_status & 0xFC)) {
			return 0;
		}



		xfer.dataSize = SD_BLOCK_SIZE;
		xfer.rxData = buf;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}

		uint16_t crc = 0;
		xfer.dataSize = 2;
		xfer.rxData = (byte*)&crc;
		if(kStatus_Success != SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer)) {
			return 0;
		}

		return 1;
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
	    GPIO_PinWrite(BOARD_INITPINS_SD_POWER_GPIO, BOARD_INITPINS_SD_POWER_PORT, BOARD_INITPINS_SD_POWER_PIN, 0);
	    mydelay(10000U);
	    GPIO_PinWrite(BOARD_INITPINS_SD_POWER_GPIO, BOARD_INITPINS_SD_POWER_PORT, BOARD_INITPINS_SD_POWER_PIN, 1);
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


		volatile status_t k = SPI_MasterSetBaud(SDCARD_SPI_BASE, 1000000U, CLOCK_GetFroHfFreq());

	    //SPI_Enable(SDCARD_SPI_BASE,0);
		//SPI_Deinit(SDCARD_SPI_BASE);


//		SPI_MasterGetDefaultConfig(&config);
//	    config.baudRate_Bps = 12000000U; // fast clock rate
//	    SPI_MasterInit(SDCARD_SPI_BASE, &config, CLOCK_GetFroHfFreq());
//	    SPI_Enable(SDCARD_SPI_BASE,1);

		if(!cmd_reset_card()) {
			return 0;
		}
		if(!cmd_check_voltage()) {
			return 0;
		}
		if(!cmd_initialise_card()) {
			return 0;
		}
		if(!cmd_set_block_size(SD_BLOCK_SIZE)) {
			return 0;
		}
		if(!cmd_read_ocr()) {
			return 0;
		}
		m_lba_mode = !!(m_response & OCR_HIGH_CAPACITY);


		byte block[512] = {0};
		for(int i=0; i<sizeof(block); ++i) {
			block[i]=(byte)i;
		}
		send_data(1024, block);
		return 1;
	}

	void deinit() {
	    GPIO_PinWrite(BOARD_INITPINS_SD_POWER_GPIO, BOARD_INITPINS_SD_POWER_PORT, BOARD_INITPINS_SD_POWER_PIN, 0);
	    SPI_Enable(SDCARD_SPI_BASE,0);
		SPI_Deinit(SDCARD_SPI_BASE);
	}


};


#endif /* SDCARD_H_ */
