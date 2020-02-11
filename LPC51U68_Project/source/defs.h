#ifndef _DEFS_H_
#define _DEFS_H_

#if 0
#define LOG1(a) PRINTF(a)
#define LOG2(a,b) PRINTF(a,b)
#else
#define LOG1(a)
#define LOG2(a,b)
#endif

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

enum {// number of samples that we'll pass per block of audio
	SZ_SAMPLE_BLOCK = 256,
	MAX_SAMPLE_VALUE = 32767,
	MIN_SAMPLE_VALUE = -32767
};

// a block of sample data that is passed to
// and from the audio interface
typedef struct {
	SAMPLE data[SZ_SAMPLE_BLOCK]; // 256 16-bit samples = 512 bytes = one block on the SD card
} SAMPLE_BLOCK;


#endif // _DEFS_H_
