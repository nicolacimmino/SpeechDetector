With this project I am exploring the possibility of recognizing speech with a low end microcontroller such as an ATmega-328. I am not expecting to recognize full sentences as this wouldn't be realistic with the hardware at hand.

I am hoping anyway to be able to at least recognize one of few words after the device has been trained with them. After all there is a very cheap chip, used in many toys alreay in the 90's, that is able to recognize few preset words, so there has to be a way to at least discern some words from others.

The hardware is a very simple board I used in many other my DSP projects, just an Arduino Nano with a microphone and a pre-amp.

![Proto](documentation/proto.png)

Alghorithm
=============

As a first approach I have been starting from the observation that sounds like wovels have a lower complexity than other sounds and, in particular, fricative consonants. Fricatives are emitted passing air through an occlusion in the oral cavity and are very rich in harmonic content, while wovels are nearly pure tones. This idea is not completely new, similar work has been done in uSpeech (https://github.com/arjo129/uSpeech).

The current code takes some audio samples and calculates the complexity of the signal defined as:

![equation](http://latex.codecogs.com/gif.latex?c%3D%20%5Csum_%7Bt%3D1%7D%5E%7Bn%7D%20%5Cleft%20%7C%20s(t)-s(t-1)%5Cright%20%7C)
