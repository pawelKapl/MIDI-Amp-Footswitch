#include "config.h"

#define MIDI_ON_CC_VAL		0
#define MIDI_OFF_CC_VAL		127


/* channel configuration pins */
const struct init_ch ch_i = {
		.pin = &PIND,
		.ddr = &DDRD,
		.port = &PORTD,
		.pos = {
			PD4, PD5, PD6, PD7
		}
};

/* Individual footswitch configurations (as many as `CFG_NB_FS`) */
const struct cfg_fs g_cfg[] = {
	{
		.sw = {
			.is_momentary = true,
			.ddr = &DDRB,
			.pin = &PINB,
			.pos = PB3,
		},
		.midi = {
			.cc_number = 102,
			.on_cc_val = MIDI_ON_CC_VAL,
			.off_cc_val = MIDI_OFF_CC_VAL,
		},
		.led = {
			.is_enabled = true,
			.ddr = &DDRC,
			.port = &PORTC,
			.pos = PC3,
		},
	},
	{
		.sw = {
			.is_momentary = true,
			.ddr = &DDRB,
			.pin = &PINB,
			.pos = PB2,
		},
		.midi = {
			.cc_number = 103,
			.on_cc_val = MIDI_ON_CC_VAL,
			.off_cc_val = MIDI_OFF_CC_VAL,
		},
		.led = {
			.is_enabled = true,
			.ddr = &DDRC,
			.port = &PORTC,
			.pos = PC2,
		},
	},
	{
		.sw = {
			.is_momentary = true,
			.ddr = &DDRB,
			.pin = &PINB,
			.pos = PB1,
		},
		.midi = {
			.cc_number = 104,
			.on_cc_val = MIDI_ON_CC_VAL,
			.off_cc_val = MIDI_OFF_CC_VAL,
		},
		.led = {
			.is_enabled = true,
			.ddr = &DDRC,
			.port = &PORTC,
			.pos = PC1,
		},
	},
	{
		.sw = {
			.is_momentary = false,
			.ddr = &DDRB,
			.pin = &PINB,
			.pos = PB0,
		},
		.midi = {
			.cc_number = 105,
			.on_cc_val = MIDI_ON_CC_VAL,
			.off_cc_val = MIDI_OFF_CC_VAL,
		},
		.led = {
			.is_enabled = true,
			.ddr = &DDRC,
			.port = &PORTC,
			.pos = PC0,
		},
	},
};
