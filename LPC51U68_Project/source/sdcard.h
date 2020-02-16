///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// LOW LEVEL DRIVER FOR DIRECT ACCESS TO SD CARD
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef SDCARD_H_
#define SDCARD_H_

#define SDCARD_SPI_BASE SPI2

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CSDCard {
protected:
	enum {
		STATUS_POLL_RETRIES 	= 2000,
		SAMPLE_BLOCK_SIZE		= 512,
		SD_LOOP_BASE			= 1024,
    	SLOW_SPI_CLOCK_BPS 		= 100000U,
#if 0
    	FAST_SPI_CLOCK_BPS 		= 1000000U
#else
    	FAST_SPI_CLOCK_BPS 		= 20000000U
#endif
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
	};
	enum {
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
	};
	enum {
		DATA_TOKEN				= 0xFE,
		DATA_TOKEN_MULTI_WRITE	= 0xFC,
		DATA_STOP_TRAN			= 0xFD,
	};
	enum {
		DATA_RESP_MASK		= 0x1F,
		DATA_RESP_OK		= 0x05,
		DATA_RESP_ERR_CRC	= 0x0B,
		DATA_RESP_ERR_WRITE	= 0x0D
	};
	enum {
		CMD0	= 0x40, //0,	//None(0)	R1	No	GO_IDLE_STATE	Software reset.
		CMD1	= 0x41, //1,	//None(0)	R1	No	SEND_OP_COND	Initiate initialization process.
		CMD8	= 0x48, //8, //*3	R7	No	SEND_IF_COND	For only SDC V2. Check voltage range.
		CMD12   = 0x4C, //12	// None(0)	R1b	No	STOP_TRANSMISSION	Stop to read data.
		CMD16	= 0x50, //16,	//Block length[31:0]	R1	No	SET_BLOCKLEN	Change R/W block size.
		CMD17	= 0x51, //17, //	Address[31:0]	R1	Yes	READ_SINGLE_BLOCK	Read a block.
		CMD18	= 0x52, //18, //	Address[31:0]	R1	Yes	READ_MULTIPLE_BLOCK	Read multiple blocks.
		CMD24	= 0x58, //24,	//Address[31:0]	R1	Yes	WRITE_BLOCK	Write a block.
		CMD25	= 0x59, //25,	//Address[31:0]	R1	Yes	WRITE_MULTIPLE_BLOCK	Write multiple blocks.
		CMD58	= 0x7A, //58,	//None(0)	R3	No	READ_OCR	Read OCR.
		ACMDXX  = 0x77, //55,
		ACMD41  = 0x69 //41,
	};

#pragma pack(push,1)
	typedef struct {
		byte status;
		SAMPLE_BLOCK data;
		uint16_t crc;
	} SD_DATA_PACKET;
#pragma pack(pop)

	typedef enum {
		ST_READY,
		ST_WRITE_CMD,
		ST_WRITE_CMD_RESP,
		ST_WRITE_DATA_DELAY,
		ST_WRITE_DATA,
		ST_WRITE_DATA_RESP,
		ST_WRITE_UPDATE_PENDING,
		ST_WRITE_READY,
		ST_WRITE_STOP_TRAN,
		ST_WRITE_STOP_TRAN_WAIT,
		ST_READ_CMD,
		ST_READ_CMD_RESP,
		ST_READ_DATA_TOKEN,
		ST_READ_DATA_PACKET,
		ST_READ_READY,
		ST_READ_STOP_TRANSMISSION_WAIT,
		ST_READ_STOP_TRANSMISSION,
		ST_FATAL,
		ST_STOP,
	} STATE;
	STATE m_state;

	enum {
		REQ_NONE,
		REQ_READ,
		REQ_WRITE
	} m_request;

	int m_req_block_no; 	// the block number associated with pending request
	int m_last_block_no;	// the block number associated with the last request

	byte m_data_resp;
	byte m_status;
	byte m_lba_mode;
	uint32_t m_response;

	SD_DATA_PACKET m_data_packet;

	byte m_cmd_buf[6];
	byte m_junk_buf[10];
	int m_retry;
	status_t m_api_result;



	int m_read_block_ready;
	SAMPLE_BLOCK m_read_block;

	int m_next_block_no;

	byte sd_tx(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = data;
		xfer.rxData = NULL;
		xfer.configFlags = 0U;
		LOG2("+SPI_MasterTransferBlocking %ld\r\n", g_clock.millis());
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		LOG2("-SPI_MasterTransferBlocking %ld\r\n", g_clock.millis());
		if(kStatus_Success != m_api_result) {
			LOG2("**** sd_tx SPI_MasterTransferBlocking error %d\r\n", (int)m_api_result);
		}
		return (kStatus_Success == m_api_result);
	}

	byte sd_rx(byte *data, int len) {
		spi_transfer_t xfer;
		xfer.dataSize = len;
		xfer.txData = NULL;
		xfer.rxData = data;
		xfer.configFlags = 0U;
		m_api_result = SPI_MasterTransferBlocking(SDCARD_SPI_BASE, &xfer);
		if(kStatus_Success != m_api_result) {
			LOG2("**** sd_rx SPI_MasterTransferBlocking error %d\r\n", (int)m_api_result);
		}
		return (kStatus_Success == m_api_result);
	}

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
			g_clock.delay(1);
		}
		return 0;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	inline uint32_t block2addr(int block) {
		return ((uint32_t)block)<<9; // * 512
	}

public:
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	CSDCard() {

		m_request = REQ_NONE;
		m_req_block_no = 0;
		m_last_block_no = 0;
		m_data_resp = 0;
		m_status = 0;
		m_lba_mode = 0;
		m_response = 0;
		m_retry = 0;
		m_api_result = 0;
		m_read_block_ready = 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Return nonzero if we are rea
	inline int is_ready() {
		return ((ST_READY == m_state && REQ_NONE == m_request) ||
				(ST_WRITE_READY == m_state) ||
				(ST_READ_READY == m_state));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	void write_block(int block_no, SAMPLE_BLOCK *block) {
		m_request = REQ_WRITE;
		m_req_block_no = block_no;
		m_data_packet.data = *block;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	void request_read_block(int block_no) {
		m_request = REQ_READ;
		m_req_block_no = block_no;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	int read_block_ready(SAMPLE_BLOCK *block) {
		if(m_read_block_ready) {
			m_read_block_ready = 0;
			if(block) {
				*block = m_data_packet.data;
			}
			return 1;
		}
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	byte init() {
		csel(1); // set chip sel high

		// power cycle the SD card
		power_sd_card(0);
		g_clock.delay(100);
	    power_sd_card(1);
		g_clock.delay(100);

		spi_master_config_t config;
		SPI_MasterGetDefaultConfig(&config);
		config.polarity = kSPI_ClockPolarityActiveHigh;
		config.phase = kSPI_ClockPhaseFirstEdge;
	    config.baudRate_Bps = SLOW_SPI_CLOCK_BPS; // slow clock rate
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
		SPI_MasterSetBaud(SDCARD_SPI_BASE, FAST_SPI_CLOCK_BPS, CLOCK_GetFroHfFreq());

		if(!cmd_reset_card()) {
			g_stats.sd_setup_error = 111;
			return 0;
		}
		if(!cmd_check_voltage()) {
			g_stats.sd_setup_error = 222;
			return 0;
		}
		if(!cmd_initialise_card()) {
			g_stats.sd_setup_error = 333;
			return 0;
		}
		if(!cmd_set_block_size(SAMPLE_BLOCK_SIZE)) {
			g_stats.sd_setup_error = 444;
			return 0;
		}
		if(!cmd_read_ocr()) {
			g_stats.sd_setup_error = 555;
			return 0;
		}
		m_lba_mode = !!(m_response & OCR_HIGH_CAPACITY);
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	void deinit() {
	    SPI_Enable(SDCARD_SPI_BASE,0);
		SPI_Deinit(SDCARD_SPI_BASE);
	    power_sd_card(0);
	}

	void PRINT_STATE(STATE state) {
		switch(state) {
		case ST_READY: LOG1("ST_READY"); break;
		case ST_WRITE_CMD: LOG1("ST_WRITE_CMD"); break;
		case ST_WRITE_CMD_RESP: LOG1("ST_WRITE_CMD_RESP"); break;
		case ST_WRITE_DATA_DELAY: LOG1("ST_WRITE_DATA_DELAY"); break;
		case ST_WRITE_DATA: LOG1("ST_WRITE_DATA"); break;
		case ST_WRITE_DATA_RESP: LOG1("ST_WRITE_DATA_RESP"); break;
		case ST_WRITE_UPDATE_PENDING: LOG1("ST_WRITE_UPDATE_PENDING"); break;
		case ST_WRITE_READY: LOG1("ST_WRITE_READY"); break;
		case ST_WRITE_STOP_TRAN: LOG1("ST_WRITE_STOP_TRAN"); break;
		case ST_WRITE_STOP_TRAN_WAIT: LOG1("ST_WRITE_STOP_TRAN_WAIT"); break;
		case ST_READ_CMD: LOG1("ST_READ_CMD"); break;
		case ST_READ_CMD_RESP: LOG1("ST_READ_CMD_RESP"); break;
		case ST_READ_DATA_TOKEN: LOG1("ST_READ_DATA_TOKEN"); break;
		case ST_READ_DATA_PACKET: LOG1("ST_READ_DATA_PACKET"); break;
		case ST_READ_READY: LOG1("ST_READ_READY"); break;
		case ST_READ_STOP_TRANSMISSION_WAIT: LOG1("ST_READ_STOP_TRANSMISSION_WAIT"); break;
		case ST_READ_STOP_TRANSMISSION: LOG1("ST_READ_STOP_TRANSMISSION"); break;
		case ST_FATAL: LOG1("ST_FATAL"); break;
		case ST_STOP: LOG1("ST_STOP"); break;
		default: LOG2("[%d]", state); break;
		}
	}

	inline void set_state(STATE state) {
		LOG2("%ld ", g_clock.millis());
		PRINT_STATE(m_state);
		LOG1("->");
		PRINT_STATE(state);
		LOG1("\r\n");
		m_state = state;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Run the state machine that sequences access to the SD card
	void run() {
		uint32_t sd_addr;
		switch(m_state) {

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// READY TO START A NEW READ OR WRITE TRANSACTION
		case ST_READY:
			g_ui.set_rec(0);
			switch(m_request) {
			case REQ_READ:
				m_request = REQ_NONE;
				set_state(ST_READ_CMD);
				break;
			case REQ_WRITE:
				m_request = REQ_NONE;
				set_state(ST_WRITE_CMD);
				break;
			case REQ_NONE:
				break;
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// START A NEW MULTI BLOCK WRITE
		case ST_WRITE_CMD:
			//PRINTF("write %d\r\n",m_req_block_no);

			g_ui.set_rec(1);
			LOG2("ST_WRITE_CMD %d\r\n", m_req_block_no);

			csel(0); // assert chip select
			sd_addr = block2addr(m_req_block_no);
			m_cmd_buf[0] = CMD25;
			m_cmd_buf[1] = (byte)(sd_addr>>24);
			m_cmd_buf[2] = (byte)(sd_addr>>16);
			m_cmd_buf[3] = (byte)(sd_addr>>8);
			m_cmd_buf[4] = (byte)(sd_addr);
			m_cmd_buf[5] = 0xFF;
			if(!sd_tx(m_cmd_buf, 6)) {
				set_state(ST_FATAL);
			}
			else {
				m_retry = 2000;
				set_state(ST_WRITE_CMD_RESP);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// WAIT FOR THE RESPONSE FOR THE WRITE COMMAND
		case ST_WRITE_CMD_RESP:
			if(!sd_rx(&m_status, 1)) {
				set_state(ST_FATAL); // API failure
			}
			else if(m_status == 0xFF) { // card busy
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			else if(m_status == 0x00) { // expected response
				m_retry = 1;
				set_state(ST_WRITE_DATA_DELAY);
			}
			else { // invalid response
				set_state(ST_FATAL);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// FORCE A DELAY BEFORE WRITING THE DATA PACKET
		case ST_WRITE_DATA_DELAY:
			if(!--m_retry) {
				set_state(ST_WRITE_DATA);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// SEND DATA PACKET
		case ST_WRITE_DATA:
			m_data_packet.status = DATA_TOKEN_MULTI_WRITE;
			m_data_packet.crc = 0xFFFF;
			if(!sd_tx((byte*)&m_data_packet, sizeof(m_data_packet))) {
				set_state(ST_FATAL);	// api error
			}
			else {
				m_retry = 2000;
				set_state(ST_WRITE_DATA_RESP);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// WAIT FOR RESPONSE BYTE
		case ST_WRITE_DATA_RESP:
			if(!sd_rx(&m_status, 1)) {
				set_state(ST_FATAL);	// api error
			}
			else if((m_status & DATA_RESP_MASK) == DATA_RESP_OK) { // expected response
				m_retry = 10000;
				set_state(ST_WRITE_UPDATE_PENDING);
			}
			else if(m_status == 0xFF) { // still busy
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			else {
				LOG2("ST_WRITE_DATA_RESP received %d\r\n", m_status);
				set_state(ST_FATAL); // bad response
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// WAIT FOR THE SD CARD TO UPDATE
		case ST_WRITE_UPDATE_PENDING:
			if(!sd_rx((byte*)&m_status, 1)) {
				set_state(ST_FATAL);	// api error
			}
			else if(m_status == 0xFF) { // DO rises when update is complete
				m_retry = 2000;
				m_last_block_no = m_req_block_no;
				set_state(ST_WRITE_READY);
			}
			else {
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// LAST BLOCK WRITE IS SUCCESSFUL, CHECKING IF WE CAN EXTEND TO ANOTHER BLOCK
		case ST_WRITE_READY:
			g_ui.set_rec(0);
			if((REQ_WRITE == m_request) && (m_req_block_no == m_last_block_no + 1)) {
				// we can continue the write to another block
				m_retry = 10;
				m_request = REQ_NONE;
				set_state(ST_WRITE_DATA_DELAY);
			}
			else if(REQ_NONE == m_request) {
				// we won't hold the write transaction open indefinitely. If no further
				// request is made we'll close the transaction by ourselves
				if(!--m_retry) {
					set_state(ST_WRITE_STOP_TRAN);
				}
			}
			else {
				// a request is pending but we can't add it to the open multi-block write
				// request, so we'll need to close it before we can continue...
				set_state(ST_WRITE_STOP_TRAN);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// CLOSE MULTI BLOCK WRITE TRANSACTION
		case ST_WRITE_STOP_TRAN:
			m_cmd_buf[0] = DATA_STOP_TRAN;
			m_cmd_buf[1] = 0;
			if(!sd_tx(m_cmd_buf, 2)) {
				set_state(ST_FATAL); // api error
			}
			else {
				m_retry = 10000;
				set_state(ST_WRITE_STOP_TRAN_WAIT);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// WAITING FOR SD CARD TO END THE WRITE AND SET DO HIGH
		case ST_WRITE_STOP_TRAN_WAIT:
			if(!sd_rx((byte*)&m_status, 1)) {
				set_state(ST_FATAL);
			}
			else if(m_status == 0xFF) {
				csel(1); // de-assert CSEL
				set_state(ST_READY);
			}
			else {
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// START A MULTI BLOCK READ TRANSACTION
		case ST_READ_CMD:
			LOG2("ST_READ_CMD %d\r\n", m_req_block_no);

			csel(0); // assert csel
			sd_addr = block2addr(m_req_block_no);
			m_cmd_buf[0] = CMD18;
			m_cmd_buf[1] = (byte)(sd_addr>>24);
			m_cmd_buf[2] = (byte)(sd_addr>>16);
			m_cmd_buf[3] = (byte)(sd_addr>>8);
			m_cmd_buf[4] = (byte)(sd_addr);
			m_cmd_buf[5] = 0xFF;
			if(!sd_tx((byte*)&m_cmd_buf, 6)) {
				set_state(ST_FATAL);	// api error
			}
			else {
				m_retry = 2000;
				set_state(ST_READ_CMD_RESP);
			}
			break;
		////////////////////////////////////////////////////////////////////////////////////
		// GET COMMAND RESPONSE FOR MULTI BLOCK READ
		case ST_READ_CMD_RESP:
			if(!sd_rx((byte*)&m_status, 1)) {
				set_state(ST_FATAL); // api error
			}
			else if(m_status == 0xFF) {
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			else if(m_status != 0x00) {
				set_state(ST_FATAL); // unexpected response
			}
			else {
				m_retry = 2000;
				set_state(ST_READ_DATA_TOKEN);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// READ THE DATA TOKEN AT START OF DATA PACKET
		case ST_READ_DATA_TOKEN:
			if(!sd_rx(&m_data_packet.status, 1)) {
				set_state(ST_FATAL);
			}
			else if(m_data_packet.status == 0xFF) {
				if(!--m_retry) {
					set_state(ST_FATAL); // timeout
				}
			}
			else if(m_data_packet.status == DATA_TOKEN) { // the expected response
				set_state(ST_READ_DATA_PACKET);
			}
			else {
				LOG2("ST_READ_DATA_TOKEN received %d\r\n", m_data_packet.status);
				set_state(ST_FATAL);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// READ THE REMAINDER OF THE DATA PACKET
		case ST_READ_DATA_PACKET:
			if(!sd_rx(&((byte*)&m_data_packet)[1], sizeof(m_data_packet) - 1)) {
				set_state(ST_FATAL); // API error
			}
			else {
				m_retry = 2000;
				set_state(ST_READ_READY);
				m_last_block_no = m_req_block_no;
				m_read_block_ready = 1;
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// LAST BLOCK READ IS SUCCESSFUL, CHECKING IF WE CAN EXTEND TO ANOTHER BLOCK
		case ST_READ_READY:
			/*
			if(!sine::is_known(&m_data_packet.data)) {
				PRINTF("OUCH %d\r\n",m_req_block_no);
			}
			else {
				PRINTF("ok.. %d\r\n",m_req_block_no);
			}
			*/
			if((REQ_READ == m_request) && (m_req_block_no == m_last_block_no + 1)) {
				// we can continue the write to another block
				set_state(ST_READ_DATA_TOKEN);
				m_request = REQ_NONE;
			}
			else if(REQ_NONE == m_request) {
				// we won't hold the read transaction open indefinitely. If no further
				// request is made we'll close the transaction by ourselves
				if(!--m_retry) {
					set_state(ST_READ_STOP_TRANSMISSION);
				}
			}
			else {
				// a request is pending but we can't add it to the open multi-block read
				// request, so we'll need to close it before we can continue...
				set_state(ST_READ_STOP_TRANSMISSION);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// STOP TRANSMISSION
		case ST_READ_STOP_TRANSMISSION:
			m_cmd_buf[0] = CMD12;
			m_cmd_buf[1] = 0;
			m_cmd_buf[2] = 0;
			m_cmd_buf[3] = 0;
			m_cmd_buf[4] = 0;
			m_cmd_buf[5] = 0xFF;
			if(!sd_tx((byte*)&m_cmd_buf, 6)) {
				set_state(ST_FATAL);	// api error
			}
			else if(!sd_rx(m_junk_buf, 8)) {
				set_state(ST_FATAL); // api error
			}
			else {
				set_state(ST_READ_STOP_TRANSMISSION_WAIT);
			}
			break;

		////////////////////////////////////////////////////////////////////////////////////
		// WAIT FOR CARD TO BE READY AGAIN AFTER STOPPING READ TRANSMISSION
		case ST_READ_STOP_TRANSMISSION_WAIT:
			if(!sd_rx((byte*)&m_status, 1)) {
				set_state(ST_FATAL); // api error
			}
			else if(0xFF == m_status) {
				csel(1); // de-assert CSEL
				set_state(ST_READY);
			}
			break;

			////////////////////////////////////////////////////////////////////////////////////
		case ST_FATAL:
			++g_stats.sd_access_errors;
			g_ui.set_rec(0);
			csel(1); // de-assert CSEL
			set_state(ST_STOP);
			break;

		////////////////////////////////////////////////////////////////////////////////////
		case ST_STOP:
			break;
		}
	}
};

//CSDCard g_sd_card;

#endif /* SDCARD_H_ */
