/*
 * Copyright (c) 2013, Majenko Technologies
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

#include <ULK.h>

_ULK ULK;

void _ULK::begin(uint8_t nchips, uint8_t l) {
    _latch = l;
    pinMode(_latch, OUTPUT);
    digitalWrite(_latch, LOW);
    _chips = nchips;
    _totbits = _chips * 16;
    _leds = (uint8_t *)malloc(_totbits);
    setAll(0);


#if defined(__PIC32MX1__) || defined(__PIC32MX2__) 
    mapPps(6, PPS_OUT_SDO2);
    SPI2CONbits.MODE32 = 0;
    SPI2CONbits.MODE16 = 1;
    SPI2CONbits.MSTEN = 1;
    SPI2BRG = 1;
    SPI2CONbits.CKE = 0;
    SPI2CONbits.CKP = 1;
    SPI2CONbits.SRXISEL = 0b00;
    IFS1bits.SPI2RXIF = 0;
    IPC9bits.SPI2IP = 0b110;
    IPC9bits.SPI2IS = 0b00;
    IEC1bits.SPI2RXIE = 1;  
    SPI2CONbits.ON = 1;
    SPI2BUF = 0;
#else 
    SPI2CONbits.MODE32 = 0;
    SPI2CONbits.MODE16 = 1;
    SPI2CONbits.MSTEN = 1;
    SPI2BRG = 10;
    SPI2CONbits.CKE = 0;
    SPI2CONbits.CKP = 1;
    IFS1bits.SPI2RXIF = 0;
    IPC7bits.SPI2IP = 0b110;
    IPC7bits.SPI2IS = 0b00;
    IEC1bits.SPI2RXIE = 1;  
    SPI2CONbits.ON = 1;
    SPI2BUF = 0;
#endif

    _bit = _totbits;
    _chip = 0;
    _pos = 0;
}

void _ULK::setAll(uint8_t val) {
    for (uint8_t i = 0; i < _totbits; i++) {
        _leds[i] = val;
    }
}

void _ULK::analogWrite(uint8_t pin, uint8_t val) {
    if (pin >= _totbits) {
        return;
    }
    _leds[pin] = val;
}

void _ULK::update() {
    if (_chip == 0) {
        digitalWrite(_latch, HIGH);
        digitalWrite(_latch, LOW);
    }

    unsigned short bval = 0;
    for (int i = 0; i < 16; i++) {
        _bit--;
        bval >>= 1;
        if (_pos < _leds[_bit]) {
            bval |= 0x8000;
        }
    }
    (void)SPI2BUF;
    SPI2BUF = bval;
    _chip++;
    if (_chip >= _chips) {
        _chip = 0;
        _bit = _totbits;
        _pos++;
    }
}

uint8_t _ULK::get(uint8_t pin) {
    if (pin >= _totbits) {
        return 0;
    }
    return _leds[pin];
}

uint32_t _ULK::getBits() {
    return _totbits;
}

extern "C" {
    void __ISR(_SPI_2_VECTOR, ipl6) updateLEDs() {
        ULK.update();
        IFS1bits.SPI2RXIF = 0;
        return; 
    }
}
