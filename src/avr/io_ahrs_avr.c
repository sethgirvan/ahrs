/**
 * atomic_signal_fence is used in some places to try to get memory
 * synchronization with ISRs
 */
#include <assert.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __STDC_NO_ATOMICS__
#error "stdatomic.h unsupported. If necessary, use of stdatomic can be removed and it can be hacked together with volatile instead."
#endif

#include <stdatomic.h>

#include "io_ahrs.h"
#include "macrodef.h"
#include "dbg.h"


#define NUSART 3
#define BAUD 38400UL // default baud of ahrs


static int (*handler_ahrs_recv)();


static int uart_ahrs_putchar(char c, FILE *stream)
{
	(void)stream;
	while (!(CC_XXX(UCSR, NUSART, A) & (1U << CC_XXX(UDRE, NUSART, ))))
	{
		// wait for transmit buffer to be ready
	}
	CC_XXX(UDR, NUSART, ) = c;
	return 0;
}

static int uart_ahrs_getchar(FILE *stream)
{
	(void)stream;
	while (!(CC_XXX(UCSR, NUSART, A) & (1U << CC_XXX(RXC, NUSART, ))))
	{
		// wait for data to be received
	}
	// UCSRnA needs to be read before UDRn, because reading UDRn changes the
	// read buffer location.
	unsigned char const status = CC_XXX(UCSR, NUSART, A);
	// read buffer asap, so it's free to accept new data
	unsigned char const data = CC_XXX(UDR, NUSART, );

	assert(!(status & (1U << CC_XXX(UPE, NUSART, ))) /* Parity Error. This
	should never happen, since parity is supposed to be disabled?! */);

	if (status & (1U << CC_XXX(FE, NUSART, ))) // was stop bit incorrect (zero)?
	{
		DEBUG("Frame Error on usart " STRINGIFY_X(NUSART) ". Out-of-sync or break condition may have occured.");
		// Return rather than waiting for the next byte because we don't want
		// to block if reading from a Receive Complete Interrupt.
		return _FDEV_EOF;
	}

	if (status & (1U << CC_XXX(DOR, NUSART, ))) // was there a data overrun?
	{
		// At least one frame was lost due to data being received while the
		// receive buffer was full.
		DEBUG("Receive buffer overrun on usart " STRINGIFY_X(NUSART) ". At least one uart frame lost.");
		// No indication of this is made. The caller is expected to be handling
		// synchronization and error checking above this layer.
	}
	return data;
}


FILE *io_ahrs = &(FILE)FDEV_SETUP_STREAM(uart_ahrs_putchar, uart_ahrs_getchar,
		_FDEV_SETUP_RW);


/**
 * Assumes uart NUSART will be used and connected with the ahrs IEA-232
 * interface via a ttl<->IEA-232 converter.
 *
 * Per the TRAX PNI user manual:
 *     Start Bits: 1
 *     Number of Data Bits: 8
 *     Stop Bits: 1
 *     Parity: none
 *     Baud: BAUD
 */
void io_ahrs_init(char const *path)
{
	(void)path;
#include <util/setbaud.h> // uses BAUD macro
	CC_XXX(UBRR, NUSART, ) = UBRR_VALUE; // set baud rate register
	// USE_2X is 1 only if necessary to be within BAUD_TOL
	CC_XXX(UCSR, NUSART, A) |= ((uint8_t)USE_2X << CC_XXX(U2X, NUSART, ));

	// register defaults are already 8 data, no parity, 1 stop bit
	CC_XXX(UCSR, NUSART, B) = (1U << CC_XXX(TXEN, NUSART, )) |
		(1U << CC_XXX(RXEN, NUSART, )); // enable transmitter and receiver
	return;
}

void io_ahrs_clean()
{
	// TODO: disable transmit and receive
	return;
}

ISR(CC_XXX(USART, NUSART, _RX_vect)) // Receive Complete Interrupt
{
	assert(handler_ahrs_recv /* Ahrs usart receive handler is NULL function
								pointer. If it is not initialized, the Receive
								Complete Interrupt should not be enabled. */);

	// This function must read from the UDR (eg, with 'fgetc(io_ahrs)'),
	// clearing the RXC flag, otherwise this interrupt will keep triggering
	// until the flag is cleared.
	handler_ahrs_recv();
}

int io_ahrs_recv_start(int (*handler)())
{
	handler_ahrs_recv = handler;

	/* handler_ahrs_recv must be set before the Receive Complete Interrupt is
	 * enabled, since the ISR calls handler_ahrs_recv. However, we are not
	 * guaranteed memory ordering between the enable and non-volatile
	 * variables. In fact, even 'sei()' does not guarantee this (see
	 * http://www.nongnu.org/avr-libc/user-manual/optimization.html#optim_code_reorder).
	 *
	 * Therefore, we'll try to get our memory ordering with a signal fence. In
	 * theory, handler_ahrs_recv could probably just be declared volatile, but
	 * I want to avoid having to have all ISR shared variables at risk for
	 * order optimizations having to be volatile.
	 */
	atomic_signal_fence(memory_order_acq_rel);

	// enable Receive Complete Interrupt
	CC_XXX(UCSR, NUSART, B) |= (1U << CC_XXX(RXCIE, NUSART, ));
	return 0;
}

void io_ahrs_recv_stop()
{
	CC_XXX(UCSR, NUSART, B) &= ~(1U << CC_XXX(RXCIE, NUSART, )); // disable Receive Complete Interrupt
	// we won't bother setting handler_ahrs_recv to NULL
	return;
}

// new is initialized to 0, so the reader/consumer can know initially when
// there has been any valid data (eg waiting to run PID until a complete data
// set has been received from the ahrs.
//
// clean and new need to be volatile so changes from ISR are visible and so
// changes outside ISR are correctly ordered relative to disabling and
// enabling the ISR.
//
// write, clean, and read may only hold values in range [0, 2]
static struct
{
	unsigned char write          : 2; // Writer/producer writes to buf[write]
	volatile unsigned char clean : 2; // if (new), indexes the newest data
	unsigned char read           : 2; // Reader/consumer reads from buf[read]
	volatile unsigned char new   : 1; /* true if the writer/producer has
										 written a new complete set of data
										 since the last call of
										 io_ahrs_tripbuf_update */
} tripbuf = {0, 1, 2, false};

// Must be atomic relative to io_ahrs_tripbuf_offer
static bool io_ahrs_tripbuf_update_crit()
{
	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));
	if (tripbuf.new)
	{
		tripbuf.new = false;
		// tripbuf.clean must contain a new set of data, so make that the new
		// read, and make read available for clean.
		unsigned char tmp = tripbuf.read;
		tripbuf.read = tripbuf.clean;
		tripbuf.clean = tmp;
		// try to ensure buffer reads will have proper memory ordering
		atomic_signal_fence(memory_order_acquire);
		return true;
	}
	return false;
}

/**
 * May be interrupted by io_ahrs_tripbuf_offer, but cannot interrupt it (ie
 * this can be interrupted by handler_ahrs_recv, but one probably does not want
 * to call it from handler_ahrs_recv).
 *
 * returns whether there has been any new data since last call
 */
bool io_ahrs_tripbuf_update()
{
	if (CC_XXX(UCSR, NUSART, B) & (1 < CC_XXX(RXCIE, NUSART, )))
	{
		// Disable Receive Complete Interrupt, because we assume that if it is
		// enabled, io_ahrs_tripbuf_offer may be run from the interrupt handler
		CC_XXX(UCSR, NUSART, B) &= ~(1 << CC_XXX(RXCIE, NUSART, ));
		bool updated = io_ahrs_tripbuf_update_crit();
		// Reenable Receive Complete Interrupt
		CC_XXX(UCSR, NUSART, B) |= (1 << CC_XXX(RXCIE, NUSART, ));
		return updated;
	}
	return io_ahrs_tripbuf_update_crit();
}

/**
 * May interrupt io_ahrs_tripbuf_update, but may not be interrupted by it
 */
void io_ahrs_tripbuf_offer()
{
	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));
	// Try to ensure buffer writes are ordered correctly
	atomic_signal_fence(memory_order_release);
	unsigned char tmp = tripbuf.write;
	tripbuf.write = tripbuf.clean;
	tripbuf.clean = tmp;
	tripbuf.new = true;
}

unsigned char io_ahrs_tripbuf_write()
{
	return tripbuf.write;
}

unsigned char io_ahrs_tripbuf_read()
{
	return tripbuf.read;
}
