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


	// Write 4 block sinewave sample into both track slots
	int test1() {
		write_block_and_wait(10, (SAMPLE_BLOCK*)&sine::quad0);
		write_block_and_wait(11, (SAMPLE_BLOCK*)&sine::quad1);
		write_block_and_wait(12, (SAMPLE_BLOCK*)&sine::quad2);
		write_block_and_wait(13, (SAMPLE_BLOCK*)&sine::quad3);
		write_block_and_wait(320010, (SAMPLE_BLOCK*)&sine::quad0);
		write_block_and_wait(320011, (SAMPLE_BLOCK*)&sine::quad1);
		write_block_and_wait(320012, (SAMPLE_BLOCK*)&sine::quad2);
		write_block_and_wait(320013, (SAMPLE_BLOCK*)&sine::quad3);

		while(1) {
			run();
		}
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
