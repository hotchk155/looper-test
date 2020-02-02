typedef uint8_t byte;
void mydelay(volatile uint32_t nof);
void wait_ms();
byte is_ms_tick();

typedef enum {
	EV_NONE,
	EV_LOOPER_CLEAR,
	EV_LOOPER_RECORD,
	EV_LOOPER_STOP,
	EV_LOOPER_PLAY,
	EV_LOOPER_OVERDUB
} EV_TYPE;

// type to hold a single 16-bit audio sample
typedef uint16_t SAMPLE;
typedef uint32_t SD_BLOCK_NO;
typedef uint32_t SD_ADDR;

enum {// number of samples that we'll pass per block of audio
	SZ_SAMPLE_BLOCK = 128
};

// a block of sample data that is passed to
// and from the audio interface
typedef struct {
	SAMPLE data[SZ_SAMPLE_BLOCK];
} SAMPLE_BLOCK;

// the block of data that is passed to and from
// the SD card interface. There are two channels,
// holding the active take and the previous take
// (which we'll switch to for UNDO)
typedef struct {
	SAMPLE_BLOCK chan[2];
} SD_BLOCK;

class IBlockBuffer {
public:
	virtual int get_audio(SAMPLE_BLOCK *block) = 0;
	virtual int put_audio(SAMPLE_BLOCK *block) = 0;
	virtual int get_sd_block(SD_BLOCK *block) = 0;
	virtual int put_sd_block(SD_BLOCK *block) = 0;
	virtual int is_buffer_full() = 0;
};


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
