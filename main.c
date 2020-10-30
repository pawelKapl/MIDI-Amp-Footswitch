
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/atomic.h>

#include "config.h"

/* Debounce time as a floating point number */
#define DEB_TIME_DBL	((double) CFG_DEBOUNCE_TIME_MS)

/* Current state of a single footswitch */
struct fs {
	bool is_on;
};

extern const struct cfg_fs g_cfg[];
extern const struct init_ch ch_i;
static struct fs g_fs[CFG_NB_FS];
static uint8_t MIDI_CH = 1;


//=======================================INIT PART

static void set_midi_channel(void)
{
	/* Set channel selector ports as high inputs */
	uint8_t i;
	for (i = 0; i < 4; ++i)
	{
		*ch_i.ddr &= ~_BV(ch_i.pos[i]);
		*ch_i.port |= _BV(ch_i.pos[i]);
	}
	
	/* Read channel from ports */
	uint8_t channel = 0;
	for (i = 0; i < 4; ++i)
	{
		if (*ch_i.pin & _BV(ch_i.pos[i]))
		{
			channel |= (1 << i);
		}
	}
	MIDI_CH = channel;
}

static void init_uart(void)
{
	/* Set MIDI baud rate */
	UBRRH = (unsigned char)(CFG_UART_BAUD_RATE_VAL >> 8);
	UBRRL = (unsigned char)(CFG_UART_BAUD_RATE_VAL);
	
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); 
	UCSRC =  (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	
	set_midi_channel();
	
}

static void init_io(void)
{
	/* Set Inputs and Outputs */
	uint8_t i;
	for (i = 0; i < CFG_NB_FS; ++i)
	{
		const struct cfg_fs * const cfg = &g_cfg[i];

		/* Switch: input */
		*cfg->sw.ddr &= ~_BV(cfg->sw.pos);
		PORTB |= _BV(cfg->sw.pos);

		/* Channels: output */
		if (cfg->led.is_enabled) 
		{
			*cfg->led.ddr |= _BV(cfg->led.pos);
		}
	}
}


//==============================================Switching

static inline void turnOffAllMomentaryPins(const struct cfg_fs * const cfg)
{
	if (cfg->sw.is_momentary)
	{
		uint8_t i;
		for (i = 0; i < CFG_NB_FS; ++i)
		{
			if (g_cfg[i].sw.is_momentary)
			{
				PORTC &= ~_BV(g_cfg[i].led.pos);
			}
		}
	}
}

static inline void channel_change(const struct cfg_fs * const cfg, const bool is_on)
{
	if (!cfg->led.is_enabled) 
	{
		return;
	}
	
	/* Cleaning whole port (momentary)   */
	turnOffAllMomentaryPins(cfg);
	
	if (is_on) 
	{
		*cfg->led.port |= _BV(cfg->led.pos);
	} 
	else 
	{
		*cfg->led.port &= ~_BV(cfg->led.pos);
	}
}

static void sync_fs_outputs(const struct cfg_fs * const cfg,
const struct fs * const fs)
{
	channel_change(cfg, fs->is_on);
}

static inline bool is_switch_on(const uint8_t index)
{
	uint8_t i;
	uint8_t sum = 0;
	for (i = 0; i < CFG_NB_FS; ++i) 
	{
		if (g_cfg[i].sw.is_momentary && !(*g_cfg[i].sw.pin & _BV(g_cfg[i].sw.pos))) 
		{
			++sum;
			if (sum > 1)
			{
				return false;
			} 
		}
	}
	return (bool) (!(*g_cfg[index].sw.pin & _BV(g_cfg[index].sw.pos)) && sum < 2);
}

static void initial_delay(void)
{
	_delay_ms(50.);
}

static void init(void)
{
	init_io();
	init_uart();
	initial_delay();

	/* Read current switch values to set the initial states */
	uint8_t i;
	for (i = 0; i < CFG_NB_FS; ++i) 
	{
		if (!g_cfg[i].sw.is_momentary) 
		{
			g_fs[i].is_on = is_switch_on(i);
		}
	}

	/* Synchronize outputs with states */
	for (i = 0; i < CFG_NB_FS; ++i) 
	{
		sync_fs_outputs(&g_cfg[i], &g_fs[i]);
	}
	
	/* Default channel on at start */
	PORTC |= _BV(DEF_FS_ON);
}

static void check_and_update_fs(const uint8_t index)
{
	const struct cfg_fs * const cfg = &g_cfg[index];
	struct fs * const fsc = &g_fs[index];

	/* Initial read */
	bool sw_is_on = is_switch_on(index);

	if (cfg->sw.is_momentary) 
	{
		_delay_ms(DEB_TIME_DBL);
		sw_is_on = is_switch_on(index);
		if (sw_is_on != fsc->is_on) 
		{
			fsc->is_on = sw_is_on;	
			channel_change(cfg, fsc->is_on);
			fsc->is_on = !sw_is_on;	
		}
	} 
	else
	{
		if (sw_is_on)
		{
			_delay_ms(DEB_TIME_DBL);
			sw_is_on = is_switch_on(index);
			fsc->is_on = !fsc->is_on;
			channel_change(cfg, fsc->is_on);
			_delay_ms(700);
		}
	}
}


//================================================== MIDI

enum MIDIstatus {CTRL_CHANGE, CTRL_NUMBER, CTRL_VALUE};
volatile char MIDIbuffer[MIDI_RX_BUF_SIZE];
volatile uint8_t MIDIbufferHead; 
volatile uint8_t MIDIbufferTail;
volatile uint8_t MIDIbytes;


ISR(USART_RXC_vect)
{
	uint8_t newHead, Data;

	//Download next byte
	Data = UDR;

	//New head
	newHead = (MIDIbufferHead + 1) & MIDI_RX_BUF_MASK;


	if (newHead == MIDIbufferTail)
	{
		//Buffer overload
	}
	else
	{
		MIDIbufferHead = newHead;  //New head
		MIDIbuffer[newHead] = Data;  //Adding new byte to FIFO
		++MIDIbytes;
	}
}

uint8_t MIDIgetByte(void)
{
	volatile uint8_t Byte;
		MIDIbufferTail = (MIDIbufferTail + 1) & MIDI_RX_BUF_MASK;
		Byte =  MIDIbuffer[MIDIbufferTail];
		--MIDIbytes;

	return Byte;
}

bool MIDIisReady(void)
{
	if (MIDIbytes > 0)
	{
		return true;
	}
	return false;
}
	
void executeMIDIchanges(uint8_t CCvalue, uint8_t CCnumber)
{
	uint8_t i;
	for (i = 0; i < CFG_NB_FS; ++i) {
		const struct cfg_fs * const cfg = &g_cfg[i];
		if (cfg->midi.cc_number == CCnumber)
		{
			if (cfg->sw.is_momentary)
			{
				channel_change(cfg, true);
			}
			else
			{
				if (CCvalue == cfg->midi.on_cc_val)
				{
					channel_change(cfg, true);
				}
				else
				{
					channel_change(cfg, false);
				}
			}
		}
	}
}

void MIDIhandling(void)
{
	uint8_t receivedByte;
	static uint8_t CCnumber, CCvalue;
	static uint8_t MIDIstatus = CTRL_CHANGE;

	if(MIDIisReady())
	{
		receivedByte = MIDIgetByte();
		
		switch(MIDIstatus)
		{
			case CTRL_CHANGE:  //MIDI incoming byte
				if((receivedByte & 0xF0) == MIDI_CTRL_CHANGE 
				&& (receivedByte & 0x0F) == MIDI_CH - 1)
				MIDIstatus = CTRL_NUMBER;
				break;

			case CTRL_NUMBER:  //MIDI CC#
				if(!(receivedByte & 0x80))
				{
					CCnumber = receivedByte;
					MIDIstatus = CTRL_VALUE;
				}
				else MIDIstatus = CTRL_CHANGE;
				break;

			case CTRL_VALUE:  //MIDI value
				if(!(receivedByte & 0x80))
				{
					CCvalue = receivedByte;
					executeMIDIchanges(CCvalue, CCnumber);
				}
				MIDIstatus = CTRL_CHANGE;
				break;
		}
	}
}


//==================================== MAIN

static void run(void)
{
	for (;;) {
		uint8_t i;
		for (i = 0; i < CFG_NB_FS; ++i) {
			check_and_update_fs(i);
		}
		MIDIhandling();
	}
}

int main(void) 
{
	init();
	sei();
	run();
	return 0;
}



