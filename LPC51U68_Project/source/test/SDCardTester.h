class CSDCardTester: public CSDCard {
public:
	int write_block_and_wait(int block_no, SAMPLE_BLOCK *block) {
		LOG2("TEST: write_block_and_wait %d\r\n", block_no);
		if(!is_ready()) {
			LOG1("*** NOT READY\r\n");
		}
		CSDCard::write_block(block_no, block);
		run();
		while(!is_ready()) {
			run();
		}
		LOG1("TEST: write_block_and_wait DONE\r\n");
		return 1;
	}

	int read_block_and_wait(int block_no, SAMPLE_BLOCK *block) {
		LOG2("TEST: read_block_and_wait %d\r\n", block_no);
		if(!is_ready()) {
			LOG1("*** NOT READY\r\n");
		}
		request_read_block(block_no);
		run();
		while(!read_block_ready(block)) {
			run();
		}
		LOG1("TEST: read_block_and_wait DONE\r\n");
		return 1;
	}

	// write 200 sample blocks (50 sine wave cycles)
	int test1A() {
		int addr = 10;
		for(int i=0; i<50; ++i) {
			write_block_and_wait(addr++, (SAMPLE_BLOCK*)&sine::quad0);
			write_block_and_wait(addr++, (SAMPLE_BLOCK*)&sine::quad1);
			write_block_and_wait(addr++, (SAMPLE_BLOCK*)&sine::quad2);
			write_block_and_wait(addr++, (SAMPLE_BLOCK*)&sine::quad3);
		}
		while(1) {
			run();
		}
		return 1;
	}

	int test1B() {
		int addr = 10;
		SAMPLE_BLOCK block = {0};
		for(int i=0; i<50; ++i) {
			PRINTF("Checking %d...\r\n", i);
			read_block_and_wait(addr++, (SAMPLE_BLOCK*)&block);
			if(memcmp(&block, &sine::quad0, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON FAIL BLOCK 1 %d\r\n", i);
			}
			read_block_and_wait(addr++, (SAMPLE_BLOCK*)&block);
			if(memcmp(&block, &sine::quad1, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON FAIL BLOCK 2 %d\r\n", i);
			}
			read_block_and_wait(addr++, (SAMPLE_BLOCK*)&block);
			if(memcmp(&block, &sine::quad2, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON FAIL BLOCK 3 %d\r\n", i);
			}
			read_block_and_wait(addr++, (SAMPLE_BLOCK*)&block);
			if(memcmp(&block, &sine::quad3, sizeof(SAMPLE_BLOCK))) {
				PRINTF("COMPARISON FAIL BLOCK 4 %d\r\n", i);
			}
		}
		return 1;
	}


	int test2() {
		SAMPLE_BLOCK block = {0};
		read_block_and_wait(10, &block);
		read_block_and_wait(11, &block);
		read_block_and_wait(12, &block);
		read_block_and_wait(13, &block);
		while(1) {
			run();
		}
		return 1;
	}


	int test3() {
		SAMPLE_BLOCK block = {0};
		read_block_and_wait(10, &block);
		read_block_and_wait(11, &block);
		read_block_and_wait(12, &block);
		read_block_and_wait(13, &block);
		while(1) {
			read_block_and_wait(10, &block);
			read_block_and_wait(11, &block);
			read_block_and_wait(12, &block);
			read_block_and_wait(13, &block);
		}
		return 1;
	}


};
CSDCardTester g_sd_card;
