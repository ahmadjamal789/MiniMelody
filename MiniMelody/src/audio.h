#pragma once

void audioBegin();   // start I2S and build the tables (call from setup1)
void audioLoop();    // renders and outputs a block of samples (call from loop1)
