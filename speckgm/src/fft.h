/******************************************************************************
** FFT
** Copyright (C) 2002 Antonenko V.N
**
** File: fft.h
** Revision:
**
** Implementation of several variations of the Fast Fourier Transform.
******************************************************************************/
#ifndef _FFT_H
#define _FFT_H

enum { RECTANGULAR, BARTLETT, HAMMING, HANNING, BLACKMAN, WELCH };

void dsp_realfft( float rex[], float imx[], unsigned size, int forward );
void dsp_rect2polar( float rex[], float imx[], unsigned size );
void dsp_window( float rex[], unsigned size, int window );
void dsp_window_apply( float dst[], const float src[], const float win[], const unsigned size );

#endif/*_FFT_H*/
