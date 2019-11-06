/*
 * morse.c
 *
 *  Created on: Oct 28, 2019
 *      Author: zoe
 */

#include "morse.h"

#include <msp430.h> // for errorledon and errorledoff - change this according to your needs

/* you need to implement these */
void errorledon() {
    P1OUT |= 1;
    P6OUT |= 1<<6;
}

void errorledoff() {
    P1OUT &= ~1;
    P6OUT &= ~(1<<6);
}

/* constants */

/**
 *  DOT  DASH  DOT
 *   .    --    .
 *   ^ symbol
 *      ^ space between symbols
 *  ############## character, consisting of symbols and short spaces
 *                ######## longer space after character
 *  an even longer space is used between words
 *
 *  5 LSB stores DOTs and DASHes,
 *  DASH is 1
 *  DOT is 0
 *  3 MSBs store the number of symbols in the character
 */

#define DOT_LENGTH 1
#define DASH_LENGTH 3
#define SPACE_BETWEEN_SYMBOLS 1
#define SPACE_BETWEEN_CHARACTERS 3
#define SPACE_BETWEEN_WORDS 7

// binary encoding of the LUT
#define DASH_1 1
#define DASH_2 2
#define DASH_3 4
#define DASH_4 8
#define DASH_5 16
#define DOT_x 0
#define LENGTH_SHIFT 5
#define LENGTH_1 (1<<LENGTH_SHIFT)
#define LENGTH_2 (2<<LENGTH_SHIFT)
#define LENGTH_3 (3<<LENGTH_SHIFT)
#define LENGTH_4 (4<<LENGTH_SHIFT)
#define LENGTH_5 (5<<LENGTH_SHIFT)

typedef enum {
    WAITING = 0,
    SHOWING,
    FINISHED,
} morstate_t;

typedef enum {
    EMPTY = 2,
    DOT = 1,
    DASH = 3,
} morsecodes_t;

/* LUT */

unsigned char blut[36] = {
    DOT_x  + DASH_2 +   0    +   0    +   0    + LENGTH_2, // A: '. -' +
    DASH_1 + DOT_x  + DOT_x  + DOT_x  +   0    + LENGTH_4, // B: '- . . .' +
    DASH_1 + DOT_x  + DASH_3 + DOT_x  +   0    + LENGTH_4, // C: '- . - .' +
    DASH_1 + DOT_x  + DOT_x  +   0    +   0    + LENGTH_3, // D: '- . .' +
    DOT_x  +   0    +   0    +   0    +   0    + LENGTH_1, // E: '.' +
    DOT_x  + DOT_x  + DASH_3 + DOT_x  +   0    + LENGTH_4, // F: '. . - .' +
    DASH_1 + DASH_2 + DOT_x  +   0    +   0    + LENGTH_3, // G: '- - .' +
    DOT_x  + DOT_x  + DOT_x  + DOT_x  +   0    + LENGTH_4, // H: '. . . .' +
    DOT_x  + DOT_x  +   0    +   0    +   0    + LENGTH_2, // I: '. .' +
    DOT_x  + DASH_2 + DASH_3 + DASH_4 +   0    + LENGTH_4, // J: '. - - -' +
    DASH_1 + DOT_x  + DASH_3 +   0    +   0    + LENGTH_3, // K: '- . -' +
    DOT_x  + DASH_2 + DOT_x  + DOT_x  +   0    + LENGTH_4, // L: '. - . .' +
    DASH_1 + DASH_2 +   0    +   0    +   0    + LENGTH_2, // M: '- -' +
    DASH_1 + DOT_x  +   0    +   0    +   0    + LENGTH_2, // N: '- .' +
    DASH_1 + DASH_2 + DASH_3 +   0    +   0    + LENGTH_3, // O: '- - -' +
    DOT_x  + DASH_2 + DASH_3 + DOT_x  +   0    + LENGTH_4, // P: '. - - .' +
    DASH_1 + DASH_2 + DOT_x  + DASH_4 +   0    + LENGTH_4, // Q: '- - . -' +
    DOT_x  + DASH_2 + DOT_x  +   0    +   0    + LENGTH_3, // R: '. - .' +
    DOT_x  + DOT_x  + DOT_x  +   0    +   0    + LENGTH_3, // S: '. . .' +
    DASH_1 +   0    +   0    +   0    +   0    + LENGTH_1, // T: '-' +
    DOT_x  + DOT_x  + DASH_3 +   0    +   0    + LENGTH_3, // U: '. . -' +
    DOT_x  + DOT_x  + DOT_x  + DASH_4 +   0    + LENGTH_4, // V: '. . . -' +
    DOT_x  + DASH_2 + DASH_3 +   0    +   0    + LENGTH_3, // W: '. - -' +
    DASH_1 + DOT_x  + DOT_x  + DASH_4 +   0    + LENGTH_4, // X: '- . . -' +
    DASH_1 + DOT_x  + DASH_3 + DASH_4 +   0    + LENGTH_4, // Y: '- . - -' +
    DASH_1 + DASH_2 + DOT_x  + DOT_x  +   0    + LENGTH_4, // Z: '- - . .' +
    DASH_1 + DASH_2 + DASH_3 + DASH_4 + DASH_5 + LENGTH_5, // 0: '- - - - -' +
    DOT_x  + DASH_2 + DASH_3 + DASH_4 + DASH_5 + LENGTH_5, // 1: '. - - - -' +
    DOT_x  + DOT_x  + DASH_3 + DASH_4 + DASH_5 + LENGTH_5, // 2: '. . - - -' +
    DOT_x  + DOT_x  + DOT_x  + DASH_4 + DASH_5 + LENGTH_5, // 3: '. . . - -' +
    DOT_x  + DOT_x  + DOT_x  + DOT_x  + DASH_5 + LENGTH_5, // 4: '. . . . -' +
    DOT_x  + DOT_x  + DOT_x  + DOT_x  + DOT_x  + LENGTH_5, // 5: '. . . . .' +
    DASH_1 + DOT_x  + DOT_x  + DOT_x  + DOT_x  + LENGTH_5, // 6: '- . . . .' +
    DASH_1 + DASH_2 + DOT_x  + DOT_x  + DOT_x  + LENGTH_5, // 7: '- - . . .' +
    DASH_1 + DASH_2 + DASH_3 + DOT_x  + DOT_x  + LENGTH_5, // 8: '- - - . .' +
    DASH_1 + DASH_2 + DASH_3 + DASH_4 + DOT_x  + LENGTH_5, // 9: '- - - - .' +
};

char base = 37;
char index = 0;
char symbolsInCharacter = 0;
char countdown = 0;
morstate_t morstate = WAITING;

/* public functions */
/**
 * stops transmission of the morse character
 */
void clearMorsError() {
    index = 0;
    base = 37;
    errorledoff();
}

void setMorsError(char code) {
    base = 37;
    if (code == ' ') {
            base = 36;
    } else {
        if ((code >= 'a') && (code <= 'z')) {
            base = code - 'a';
        } else if ((code >= '0') && code <= '9') {
            base = code - '0' + 26;
        } else if ((code >= 'A') && (code <= 'Z')) {
            base = code - 'A';
        }

        symbolsInCharacter = (blut[base])>>LENGTH_SHIFT;
    }

    index = 0;
    morstate = WAITING;
    updateMorsE();
}

/**
 * this method must be called at preferably regular intervals
 * recommended is between 200 ms and 2 s
 * returns 1 if encoder is ready for next character
 * returns 0 otherwise
 * if not cleared, this will keep repeating the same character
 */
char updateMorsE() {
    char returnCode;
    if (base > 36) { // code point does not exist, idle
        returnCode = 1;
    } else {
        returnCode = 0;
        if (countdown) {
            // all set, we're waiting
            countdown--;
        } else { // countdown expired!
            if (SHOWING == morstate) {
                // a symbol was active, so turn off the LED for the space between symbols
                errorledoff();
                countdown = SPACE_BETWEEN_SYMBOLS;
                morstate = WAITING;
            } else { // load new symbol
                if (WAITING == morstate) {
                    morstate = SHOWING;

                    if (base == 36) {
                        // it's a space!
                        errorledoff();
                        countdown = SPACE_BETWEEN_WORDS;
                        morstate = FINISHED;
                    } else {
                        // LUT
                        unsigned char code = blut[base];
                        if (index > symbolsInCharacter-1) {
                            errorledoff();
                            countdown = SPACE_BETWEEN_CHARACTERS;
                            index = 0;
                            morstate = FINISHED;
                        } else {
                            errorledon();
                            if (code & (DASH_1<<index)) {
                                // it's a DASH!
                                countdown = DASH_LENGTH;
                            } else {
                                // it's a DOT we must deal with
                                countdown = DOT_LENGTH;
                            }
                        }
                        index++;
                    }
                } else {
                    // morstate is FINISHED
                    returnCode = 1; // end of symbol
                    morstate = WAITING;
                }
            }
        }
    }
    return returnCode;
}
