/*
 * Copyright (c) 2014, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <LEDMux.h>
#include <PWMAudio.h>
#include <DebouncedInput.h>

#include "sounds.h"

PWMAudio sound(AUDIO, 11025, AMP);
LEDMux display(8, 2, 2000);

DebouncedInput prTrigger(IO0, 10, true);
DebouncedInput magazine(IO1, 10, true);
DebouncedInput glTrigger(IO2, 10, true);
DebouncedInput glRack(IO3, 10, true);

unsigned int prAmmo = 99;
unsigned int glAmmo = 5;
unsigned int glBreech = 0;

const unsigned int AMMO_TICK_SPEED = 100;
const unsigned int PR_FIRE_SPEED = 1000 / 20;
const unsigned int PR_SWEEP_SPEED = 1;
const unsigned int FLASH_FADE_SPEED = 2;

int prSweep = 0;
int prSweepStep;
int prSweepCount = 0;
int prFlash = 0;
int glFlash = 0;

void firePulseRifle();

void setup() {
//	Serial.begin(9600);
//	while (!Serial);
//	delay(1000);
	sound.begin();

	prTrigger.begin();
	magazine.begin();
	glTrigger.begin();
	glRack.begin();

	prTrigger.attachInterrupt(fireItNow, FALLING);

	display.setCathodes(DC0, DC1);
	display.setAnodes(DA0, DA1, DA2, DA3, DA4, DA5, DA6, DA7);
	display.setBrightness(6);
	display.begin();
	display.print("\n");
	display.print(prAmmo);
}




void fireItNow(int val) {
	if (val == LOW) {
		firePulseRifle();
	}
}

void loop() {
	unsigned long now = millis();
	static unsigned long ammoLoadTick = millis();
	static unsigned long displayTimeout = 0;
	static unsigned long mfFade = millis();	char t[4];

	if (now - mfFade >= FLASH_FADE_SPEED) {
		mfFade = now;

		if (glFlash > 0) {
			glFlash--;
		}

		if (prFlash > 0) {
			prFlash -= 4;
		}

		if (prFlash < 0) {
			prFlash = 0;
		}

		analogWrite(LED0, prFlash);
		analogWrite(LED1, glFlash);
	}

	if (displayTimeout > 0) {
		if (now - displayTimeout >= 1000) {
			displayTimeout = 0;
			sprintf(t, "\n%02d", prAmmo);
			display.print(t);
		}
	}

	if (prAmmo > 95) {
		if (now - ammoLoadTick >= AMMO_TICK_SPEED) {
			ammoLoadTick = now;
			prAmmo --;

			if (prAmmo < 100) {
				display.print("\n");
				display.print(prAmmo);
				sound.queueSample(load_click, load_click_len, 0, 0);

			} else {
				display.print("\n--");
			}
		}
	}

	if (magazine.changed()) {
		if (magazine.read() == LOW) {
			sound.queueSingleSample(magazine_insert, magazine_insert_len, 0, 0);
			prAmmo = 104;
			glAmmo = 5;

		} else {
			sound.queueSingleSample(magazine_remove, magazine_remove_len, 0, 0);
			prAmmo = 0;
			glAmmo = 0;
			display.print("\n--");
		}
	}

	if (glRack.changed()) {

		if (glRack.read() == LOW) {
			if (glBreech == 1) {
				sound.queueSingleSample(gl_rack_pull, gl_rack_pull_len, 0, 0);

			} else {
				sound.queueSingleSample(gl_rack_pull_empty, gl_rack_pull_empty_len, 0, 0);
			}

			glBreech = 0;
			display.setDecimalPoint(0, LOW);
			sprintf(t, "\n%02d", glAmmo);
			display.print(t);
			displayTimeout = now;

		} else {
			if (glAmmo > 0) {
				glAmmo--;
				glBreech = 1;
				display.setDecimalPoint(0, HIGH);
				sound.queueSingleSample(gl_rack_push, gl_rack_push_len, 0, 0);

			} else {
				sound.queueSingleSample(gl_rack_push_empty, gl_rack_push_empty_len, 0, 0);
			}

			sprintf(t, "\n%02d", glAmmo);
			display.print(t);
			displayTimeout = now;
		}
	}

	if (glTrigger.changed()) {
		if (glTrigger.read() == LOW) {
			if (glBreech == 1) {
				glBreech = 0;
				display.setDecimalPoint(0, LOW);
				glFlash = 255;
				sound.queueSample(gl_fire, gl_fire_len, 0, 0);
				sound.queueSample(gl_boom, gl_boom_len, 2000 + (rand() % 10000), (rand() % 100) - 50);

			} else {
				sound.queueSample(pr_click, pr_click_len, 0, 0);
			}
		}
	}
}

void firePulseRifle() {
	char t[4];

	if (prAmmo > 0 && prAmmo <= 95) {
		prAmmo --;
		sound.queueSample(pr_fire, pr_fire_len, 0, prSweep * 4);

		if (prSweepCount > 3 && prSweepCount < 10) {
			prSweep += prSweepStep;
		}

		prSweepCount++;
		sprintf(t, "\n%02d", prAmmo);
		display.print(t);
		prFlash = 255;

	} else {
		sound.queueSample(pr_click, pr_click_len, 0, 0);
	}
}
