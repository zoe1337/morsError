# morsError
low-footprint stateful runtime morse encoder designed for microcontrollers

## why
If you have an error LED on your device, why can't it tell you what is wrong?

## how
Include `morse.h` in your code.

You must edit two functions in `morse.c`: `errorledon()` and `errorledoff()`

In your main loop (or from a timer interrupt) you must call `updateMorsE()` at preferably regular intervals. A good period is somewhere between 200ms - 1s. If there is too much jitter, the code will be unreadable.

To set a code, call `setMorsError(char c)` where `c` is an ASCII character. Right now \[A-Za-z0-9\] is supported. Anything else will set the encoder into off state.

When you periodically call `updateMorsE()`, the function will return 1 at the end of each character transmitted. This is the time you might want to change the character to a different one! I included an example on how to send a text string in `main.c`

I wrote this for an MSP430 but it should be easy to run it on any other microcontroller. You only need to provide the `errorledon()` and `errorledoff()` functions.

## decoding
I found an android app which can more or less decode morse with the camera, it's called [Morse Code Agent](https://play.google.com/store/apps/details?id=com.erdatsai.morsecodeagent) - let me know if you find any open-source alternative.

## my favorite part
is the binary look-up table. the MSP compiler neatly packed the bytes, so the whole thing is very compact and still quite fast.

![Binary Lookup Table](https://raw.githubusercontent.com/zoe1337/morsError/master/docs/blut.png)

![Code Size reported by the compiler](https://raw.githubusercontent.com/zoe1337/morsError/master/docs/size.png)
