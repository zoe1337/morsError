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
 *      ^ space between current_character
 *  ############## character, consisting of current_character and short spaces
 *                ######## longer space after character
 *  an even longer space is used between words
 *
 *  5 LSB stores DOTs and DASHes,
 *  DASH is 1
 *  DOT is 0
 *  3 MSBs store the number of current_character in the character
 */

#define DOT_LENGTH 1
#define DASH_LENGTH 3
#define SPACE_BETWEEN_SYMBOLS 1
#define SPACE_BETWEEN_CHARACTERS 3 -1
#define SPACE_BETWEEN_WORDS 7 -1

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
    LED_OFF = 0,
    LED_ON,
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

char codepoint = 37;
char symbolIndex = 0;
char symbolsInCharacter = 0;
char countdown = 0;
morstate_t morstate = LED_OFF;
unsigned char mask_cache;
unsigned char current_character;

/* public functions */
/**
 * stops transmission of the morse character
 */
void clearMorsError() {
    symbolIndex = 0;
    codepoint = 37;
    errorledoff();
}

void setMorsError(char code) {
    codepoint = 37;
    if (code == ' ') {
            codepoint = 36;
    } else {
        if ((code >= 'a') && (code <= 'z')) {
            codepoint = code - 'a';
        } else if ((code >= '0') && code <= '9') {
            codepoint = code - '0' + 26;
        } else if ((code >= 'A') && (code <= 'Z')) {
            codepoint = code - 'A';
        }

        // it's not a space; set pointer to the correct LUT entry
        current_character = blut[codepoint];
        symbolsInCharacter = (current_character & (7<<LENGTH_SHIFT))>>LENGTH_SHIFT;
    }

    symbolIndex = 0;
    morstate = LED_OFF;
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
    if (codepoint > 36) {
        // code point does not exist, idle
        returnCode = 1;
    } else {
        returnCode = 0;
        if (countdown) {
            // all set, we just wait for the countdown
            countdown--;
        } else { // countdown expired!
            if (LED_ON == morstate) {
                // a symbol was active, so turn off the LED for the space between current_character
                errorledoff();
                countdown = SPACE_BETWEEN_SYMBOLS;
                morstate = LED_OFF;
            } else { // morstate is either LED_OFF or FINISHED

                if (LED_OFF == morstate) {
                    // advance to the next symbol

                    if (codepoint == 36) {
                        // it's a space! special case.
                        errorledoff();
                        countdown = SPACE_BETWEEN_WORDS;
                        morstate = FINISHED;
                    } else { // it's not a space

                        if (symbolIndex > symbolsInCharacter-1) {
                            // no current_character left, this means end of character
                            errorledoff();
                            countdown = SPACE_BETWEEN_CHARACTERS;
                            symbolIndex = 0;
                            morstate = FINISHED;
                            mask_cache = 1;
                        } else {
                            // advance to the next symbol in the same character
                            errorledon();

                            if (mask_cache & current_character) { // symbol decoder
                                // it's a DASH!
                                countdown = DASH_LENGTH;
                            } else {
                                // it's a DOT we must deal with
                                countdown = DOT_LENGTH;
                            }
                            morstate = LED_ON;

                            // shift the symbol cache to the right
                            mask_cache<<1;
                            symbolIndex++;
                        }
                    }
                } else {
                    // morstate is FINISHED
                    returnCode = 1; // signal the end of symbol
                    morstate = LED_OFF;
                    // reload character from LUT
                    mask_cache = 1;
                }
            }
        } // end of "countdown expired" path
    } // end of "valid character" code path
    return returnCode;
}
