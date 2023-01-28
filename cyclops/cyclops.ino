/*
 * Cyclops Camera interface
 */
#include <stdint.h>
#include <stdarg.h>
#include <Arduino.h>

#define CS 5
#define RW 7
#define DO 1
#define DI 0

// address pins are in order on PORTB0-7 and PORTD0 + PORTD1

void setup(void)
{
	Serial.begin(115200);

	pinMode(RW, OUTPUT);
	digitalWrite(RW, 1);

	pinMode(CS, OUTPUT);
	digitalWrite(CS, 1);

	pinMode(DO, OUTPUT);
	digitalWrite(DO, 0);

	pinMode(DI, INPUT);
	digitalWrite(DI, 0);

	// address pins are written as groups
	DDRB = 0xFF;
	DDRD |= 0x03;
}

void sram_addr(uint16_t addr)
{
	PORTB = addr & 0xFF;
	PORTD = (addr >> 8) & 3;
}

void sram_write(uint16_t addr, unsigned bit)
{
	sram_addr(addr);
	digitalWrite(DO, bit);

	// select the chip and have it write to the selected address
	digitalWrite(CS, 0);
	delayMicroseconds(1);
	digitalWrite(RW, 0);

	delayMicroseconds(10);

	// deselect the chip
	digitalWrite(RW, 1);
	delayMicroseconds(1);
	digitalWrite(CS, 1);
}

unsigned sram_read(uint16_t addr)
{
	sram_addr(addr);

	// select the chip in read mode
	digitalWrite(RW, 1);
	digitalWrite(CS, 0);

	delayMicroseconds(10);
	unsigned bit = digitalRead(DI);

	// deselect the chip
	digitalWrite(CS, 1);

	return bit;
}

uint16_t lfsr_update(uint16_t lfsr)
{
	unsigned lsb = lfsr & 1u;
	lfsr >>= 1;
	if (lsb)
		lfsr ^= 0xB400u;

	return lfsr;
}

void memtest(int reps, uint16_t seed)
{
	for(int i = 1 ; i <= reps ; i++)
	{
		unsigned fail = 0;

		uint16_t lfsr = i + seed;
		for(int a = 0 ; a < 1023 ; a++)
		{
			lfsr = lfsr_update(lfsr);

			uint16_t addr = a; //(lfsr >> 1) & 0x3FF;
			unsigned bit = lfsr & 1;

			sram_write(addr, bit);
		}

		lfsr = i + seed;
		for(int a = 0 ; a < 1023 ; a++)
		{
			lfsr = lfsr_update(lfsr);

			uint16_t addr = a; //(lfsr >> 1) & 0x3FF;
			unsigned expected_bit = lfsr & 1;

			unsigned bit = sram_read(addr);
			if (bit != expected_bit)
			{
				Serial.print(addr | 0x1000, 16);
				Serial.print("=");
				Serial.print(bit);
				Serial.println(expected_bit);
				fail++;
			}
		}

		Serial.print(i);
		Serial.print("=");
		Serial.println(fail);
	}
}

void loop(void)
{
	Serial.println("writing tests");

	//memtest(100, micros());

	// write all ones patterns to the chip and read it back
	// until it fails
	for(uint16_t i = 0 ; i < 1024 ; i++)
		sram_write(i, i & 1);

	for(int reps = 0 ; reps < 32000 ; reps++)
	{
	if((reps & 0xFF) == 0) {
		Serial.print("----");
		Serial.println(reps);
	}
	for(uint16_t i = 0 ; i < 1024 ; i++)
	{
		const unsigned bit = sram_read(i);
		if (bit == (i & 1))
			continue;
		Serial.print(i);
		Serial.print("=");
		Serial.println(bit);
	}
	}

	delay(1000);
}
