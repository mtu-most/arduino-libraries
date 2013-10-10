// vim: set foldmethod=marker filetype=cpp:

#include <Arduino.h>

#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

extern volatile unsigned long timer0_overflow_count;
extern volatile unsigned long timer0_millis;

void readFreq (uint32_t ms, uint32_t &result0, uint32_t &result1)	// {{{
// This function reads the frequency of the signal on pins 4 (timer 0 input t0) and 5 (timer 1 input t1).
// It waits for ms, then returns the frequencies in Hz on its value arguments.
{
	uint16_t counter = (F_CPU >> 8) * ms / 1000;	// 16000000 Hz, 256 cycles per overflow: 31250 overflows for 500 ms.
	// This is the number of overflows we need to wait extra after the measurement is done; must be at least 2.
	// However, the number must be 1 lower, because we get 1 from overhead.
	uint8_t counter_correction = ((0x3e - counter) & 0x3f) + 1;
	uint8_t low_result0;
	uint16_t overflow0 (0), overflow1 (0), low_result1 (0), tmpreg (0);
	// The number of overflows we will be skipping by the code below.
	uint16_t overflow_skips = (counter + counter_correction + 1) >> 6;
	uint16_t fract_extra = overflow_skips * FRACT_INC;
	uint16_t millis_extra = overflow_skips * MILLIS_INC;
	cli ();
	// I'd have liked to update timer0_fract as well, but it's defined static in wiring.c, so I can't.
	unsigned long new_millis = timer0_millis + millis_extra;
	timer0_millis = new_millis + fract_extra / FRACT_MAX;
	timer0_overflow_count += overflow_skips;
	sei ();
	// use timer2 for keeping time, timer0 and timer1 for counting.
	asm volatile (
		// Wait for recent overflow, to avoid races.
	"nooverflow_loop:\n"
	"	ldd __tmp_reg__, %a[counter_base] + %[TCNT0_]\n"
	"	sbrc __tmp_reg__, 7\n"
	"	rjmp nooverflow_loop\n"
		// Disable interrupts.
	"	cli\n"
		// Store timer0 settings, so it can be restored; use result1, because we can't have more operands.
	"	in %A[result1], %[TCCR0A_]\n"
	"	in %B[result1], %[TCCR0B_]\n"
		// Wait for an overflow; it makes things easier.
	"overflow_loop:\n"
	"	sbis %[TIFR0_], 0\n"
	"	rjmp overflow_loop\n"
		// have a "1"-register.
	"	mov __tmp_reg__, __zero_reg__\n"
	"	inc __tmp_reg__\n"
		// Store counter0.
	"	ldd %A[cnt_backup], %a[counter_base] + %[TCNT0_]\n"				// -0x12
		// Set timer0 to count pulses from t0.
	"	out %[TCCR0A_], __zero_reg__\n"							// -0x11
	"	out %[TCCR0B_], %[_7]\n"	// 6 for falling edge, 7 for rising edge.	// -0x10
		// Set timer1 to count pulses from t1.
	"	std %a[counter_base] + %[TCCR1A_], __zero_reg__\n"				// -0xe
	"	std %a[counter_base] + %[TCCR1B_], %[_7]\n"	// 6 for falling edge, 7 for rising edge.	// -0xc
		// Set timer2 to count the system clock.
	"	std %a[counter_base] + %[ASSR_], __zero_reg__\n"				// -0xa
	"	std %a[counter_base] + %[TCCR2A_], __zero_reg__\n"
	"	std %a[counter_base] + %[TCCR2B_], __tmp_reg__\n"				// -8
		// Wait a constant time.
	"	std %a[counter_base] + %[TCNT2_], %[_2]\n"					// -6
		// Set timer1 to 0.
	"	std %a[counter_base] + %[TCNT1H_], __zero_reg__\n"	// Set timer0 to 0.	// -4
	"	std %a[counter_base] + %[TCNT1L_], __zero_reg__\n"	// Set timer0 to 0.	// -2
		// Set timer0 to 0.
	"	out %[TCNT0_], __zero_reg__\n"							// 0
		// Clear overflow flags.
	"	out %[TIFR0_], __tmp_reg__\n"							// 1
	"	out %[TIFR1_], __tmp_reg__\n"							// 2
	"	out %[TIFR2_], __tmp_reg__\n"							// 3
	"countloop:\n"
		// If an overflow happened, record and clear it.
	"	sbis %[TIFR0_], 0\n"
	"	rjmp no_overflow0\n"
	"	adiw %[overflow0], 1\n"
	"	out %[TIFR0_], __tmp_reg__\n"	// Clear overflow flag.
	"no_overflow0:\n"
	"	sbis %[TIFR1_], 0\n"
	"	rjmp no_overflow1\n"
	"	adiw %[overflow1], 1\n"
	"	out %[TIFR1_], __tmp_reg__\n"	// Clear overflow flag.
	"no_overflow1:\n"
	"wait_for_overflow:\n"
	"	sbis %[TIFR2_], 0\n"
	"	rjmp wait_for_overflow\n"							// i * 0x100 - 6 - x + 2
	"	out %[TIFR2_], __tmp_reg__\n"	// Clear timer 2 overflow flag.				
		// End loop.
	"	sbiw %[counter], 1\n"								// i * 0x100 - 6 - x + 4
	"	brne countloop\n"								// N * 0x100 - 6 - x + 5
		// Stop timers 1 and 0. (use this order so they record an equal time.)
	"	std %a[counter_base] + %[TCCR1B_], __zero_reg__\n"				// N * 0x100 - 6 - x + 7
	"	out %[TCCR0B_], __zero_reg__\n"							// N * 0x100 - 6 - x + 8
		// Ok, so the timers have run for N * 0x100 - 6 - x + 8 cycles.
		// This must be equal to N * 0x100, so -6 - x + 8 must be 0,
		// in other words, x must be 8 - 6 = 2.						// N * 0x100
	
		// For restoring the timer, we must wait until the timer is at k * 0x100 - 0x12 again, plus counter_correction * 0x100.
		// That'll be (N + M + 1) * 0x100 - 0x12, or (N + M) * 0x100 + 0xee.
		// So let's do some constant time stuff and wait.

		// Read value of timer 0.
	"	in %[result0], %[TCNT0_]\n"							// N * 0x100 + 1
		// Read timer 0 overflow.
	"	in __tmp_reg__, %[TIFR0_]\n"							// N * 0x100 + 2
		// Restore timer 0 values.
	"	out %[TCCR0A_], %A[result1]\n"							// N * 0x100 + 3
	"	out %[TCCR0B_], %B[result1]\n"							// N * 0x100 + 4
		// Read value of timer 1.
	"	ldd %A[result1], %a[counter_base] + %[TCNT1L_]\n"				// N * 0x100 + 6
	"	ldd %B[result1], %a[counter_base] + %[TCNT1H_]\n"				// N * 0x100 + 8
	"	nop\n"										// N * 0x100 + 9

	"	ldi %B[cnt_backup], 76\n"							// N * 0x100 + 0xa
	"correct_loop1:\n"
	"	dec %B[cnt_backup]\n"								// N * 0x100 + 0xa + i * 1
	"	brne correct_loop1\n"								// N * 0x100 + 9 + k * 3 = N * 0x100 + 0xed
	"correct_loop3:\n"
	"	ldi %B[cnt_backup], 0x3f\n"							// N * 0x100 + 0xed + i * 1
	"correct_loop2:\n"
	"	nop\n"										// These three lines take 0xfb clock pulses.
	"	dec %B[cnt_backup]\n"								// 
	"	brne correct_loop2\n"								// N * 0x100 + 0xed + i * 0xfc
	"	nop\n"										// N * 0x100 + 0xed + i * 0xfd
	"	dec %[cnt_correct]\n"							// N * 0x100 + 0xed + i * 0xfe
	"	brne correct_loop3\n"								// N * 0x100 + 0xec + M * 0x100 = (N + M) * 0x100 + 0xec
	"	std %a[counter_base] + %[TCNT0_], %A[cnt_backup]\n"				// (N + M) * 0x100 + 0xee

		// Handle one last overflow for each counter.
	"	sbic %[TIFR1_], 0\n"
	"	adiw %[overflow1], 1\n"
	"	sbrc __tmp_reg__, 0\n"
	"	adiw %[overflow0], 1\n"
		// Enable interrupts again.
	"	sei"
	:
		[counter] "+w" (counter),
		[overflow0] "+w" (overflow0),
		[overflow1] "+w" (overflow1),
		[cnt_backup] "+a" (tmpreg),
		[cnt_correct] "+a" (counter_correction),
		[result0] "=r" (low_result0),
		[result1] "+r" (low_result1)
	:
		[_2] "r" ((uint8_t)2),
		[_7] "r" ((uint8_t)7),
		[TCCR0A_] "I" (_SFR_IO_ADDR(TCCR0A)),
		[TCCR0B_] "I" (_SFR_IO_ADDR(TCCR0B)),
		[TCNT0_] "I" (_SFR_IO_ADDR(TCNT0)),
		[TIFR0_] "I" (_SFR_IO_ADDR(TIFR0)),
		[TIFR1_] "I" (_SFR_IO_ADDR(TIFR1)),
		[TIFR2_] "I" (_SFR_IO_ADDR(TIFR2)),
		[TCCR1A_] "I" (_SFR_MEM_ADDR(TCCR1A) - 0x80), // not directly addressable
		[TCCR1B_] "I" (_SFR_MEM_ADDR(TCCR1B) - 0x80), // not directly addressable
		[TCNT1H_] "I" (_SFR_MEM_ADDR(TCNT1H) - 0x80), // not directly addressable
		[TCNT1L_] "I" (_SFR_MEM_ADDR(TCNT1L) - 0x80), // not directly addressable
		[TCCR2A_] "I" (_SFR_MEM_ADDR(TCCR2A) - 0x80), // not directly addressable
		[TCCR2B_] "I" (_SFR_MEM_ADDR(TCCR2B) - 0x80), // not directly addressable
		[TCNT2_] "I" (_SFR_MEM_ADDR(TCNT2) - 0x80), // not directly addressable
		[ASSR_] "I" (_SFR_MEM_ADDR(ASSR) - 0x80), // not directly addressable
		[counter_base] "b" (0x80)
	);
	result0 = ((uint32_t)overflow0 << 8 | (uint32_t)low_result0) * 1000 / ms;
	result1 = ((uint32_t)overflow1 << 16 | (uint32_t)low_result1) * 1000 / ms;
}	// }}}
