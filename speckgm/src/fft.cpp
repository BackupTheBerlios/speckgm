#include <stdlib.h>
#include <math.h>
#include "fft.h"

#define PI  3.1415926535897932384626433832795f
#define PI2 (PI*2.0f)
#define PI4 (PI*4.0f)

/*
    THE FAST FOURIER TRANSFORM
    Upon entry, size contains the number of points in the DFT, rex[] and
    imx[] contain the real and imaginary parts of the input. Upon return,
    rex[] & imx[] contain the DFT output. All signals run from 0 to size-1.
    INVERSE FAST FOURIER TRANSFORM
    Upon entry, size contains the number of points in the IDFT, rex[] & imx[]
    contain the real & imaginary parts of the complex frequency domain.
    Upon return, rex[] and imx[] contain the complex time domain.
    All signals run from 0 to size-1.
*/
void dsp_fft( float rex[], float imx[], unsigned size, int forward )
{
    unsigned nm1, nd2, i, j, k, l, m, le, le2, jm1, ip;
    float tr, ti, ur, ui, sr ,si;

    /* change the sign of imx[] */
    if( forward == -1 ) for (i = 0; i < size; i++) imx[i] = -imx[i];

    for( m = 0, i = size; i >>= 1; ++m ); /* log(size)/log(2) */

    nm1 = size - 1;
    nd2 = size >> 1;
    /* Bit reversal sorting */
    for(j = nd2, i = 1; i < nm1; i++)
    {
        if( i < j ) {
            tr = rex[j];
            ti = imx[j];
            rex[j] = rex[i];
            imx[j] = imx[i];
            rex[i] = tr;
            imx[i] = ti;
        }
        k = nd2;
        while( k <= j ) j -= k, k >>= 1;
        j += k;
    }
    
    for( l = 1; l <= m; ++l ) /* Loop for each stage */
    {
        le = 1 << l;
        le2 = le >> 1;
        ur = 1.0f;
        ui = 0.0f;
        sr = (float)cos(PI/le2); /* Calculate sine & cosine values */
        si = (float)-sin(PI/le2);
        for( j = 1; j <= le2; ++j ) /* Loop for each sub DFT */
        {
            jm1 = j-1;
            for( i = jm1; i <= nm1; i += le ) /* Loop for each butterfly */
            {
                ip = i+le2;
                tr = rex[ip]*ur - imx[ip]*ui; /* Butterfly calculation */
                ti = rex[ip]*ui + imx[ip]*ur;
                rex[ip] = rex[i]-tr;
                imx[ip] = imx[i]-ti;
                rex[i]  = rex[i]+tr;
                imx[i]  = imx[i]+ti;
            }
            tr = ur;
            ur = tr*sr - ui*si;
            ui = tr*si + ui*sr;
        }
    }

    if( forward == -1 ) {
        tr = 1.0f/(float)size;
        for (i = 0; i < size; i++) {
            /* divide the time domain by size and change the sign of imx[] */
            rex[i] *= tr;
            imx[i] *= -tr;
        }
    }
}

/*
    FFT FOR REAL SIGNALS
    Upon entry, size contains the number of points in the DFT, rex[] contains
    the real input signal, while values in imx[] are ignored. Upon return,
    rex[] & imx[] contain the DFT output. All signals run from 0 to size-1.
    INVERSE FFT FOR REAL SIGNALS
    Upon entry, size contains the number of points in the IDFT, rex[] and
    imx[] contain the real & imaginary parts of the frequency domain 
    running from index 0 to size/2. The remaining samples in rex[] and imx[] 
    are ignored. Upon return, rex[] contains the real time domain, imx[]
    contains zeros.
*/
void dsp_realfft( float rex[], float imx[], unsigned size, int forward )
{
    unsigned nm1, nd2, n4, i, im, ip2, ipm, j, l, le, le2, jm1, ip;
    float tr, ti, ur, ui, sr ,si;

    if( forward == -1 ) {
        /* Make frequency domain symmetrical */
        for (i = (size>>1)+1; i < size; i++) {
            rex[i] =  rex[size-i];
            imx[i] = -imx[size-i];
        }

        /* Add real and imaginary parts together */
        for (i = 0; i < size; i++) rex[i] += imx[i];
    }

    for( l = 0, i = size; i >>= 1; ++l ); /* log(size)/log(2) */

    nd2 = size >> 1;
    /* Separate even and odd points */
    for( i = 0; i < nd2; i++ ) {
        rex[i] = rex[2*i];
        imx[i] = rex[2*i+1];
    }

    dsp_fft(rex, imx, nd2, 1);

    nm1 = size - 1; /* even/odd frequency domain decomposition */
    n4 = size >> 2;
    for( i = 1; i < n4; i++ ) {
        im  = nd2-i;
        ip2 = i+nd2;
        ipm = im+nd2;
        rex[ip2] =  (imx[i] + imx[im])*0.5f;
        rex[ipm] =  rex[ip2];
        imx[ip2] = -(rex[i] - rex[im])*0.5f;
        imx[ipm] = -imx[ip2];
        rex[i]   =  (rex[i] + rex[im])*0.5f;
        rex[im]  =  rex[i];
        imx[i]   =  (imx[i] - imx[im])*0.5f;
        imx[im]  = -imx[i];
    }
    rex[(size*3)/4] = imx[size/4];
    rex[nd2] = imx[0];
    imx[(size*3)/4] = imx[nd2] = imx[size/4] = imx[0] = 0.0f;

    le = 1 << l;
    le2 = le >> 1;
    ur = 1.0f;
    ui = 0.0f;
    sr = (float)cos(PI/le2);
    si = (float)-sin(PI/le2);
    for( j = 1; j <= le2; ++j ) {
        jm1 = j-1;
        for( i = jm1; i <= nm1; i += le ) {
            ip = i+le2;
            tr = rex[ip]*ur - imx[ip]*ui;
            ti = rex[ip]*ui + imx[ip]*ur;
            rex[ip] = rex[i]-tr;
            imx[ip] = imx[i]-ti;
            rex[i]  = rex[i]+tr;
            imx[i]  = imx[i]+ti;
        }
        tr = ur;
        ur = tr*sr - ui*si;
        ui = tr*si + ui*sr;
    }

    if( forward == -1 ) {
        for (i = 0; i < size; i++) {
            rex[i] = (rex[i]+imx[i])*(1.0f/(float)size);
            imx[i] = 0.0f;
        }
    }
}

/* rectangular-to-polar conversion */
void dsp_rect2polar( float rex[], float imx[], unsigned size )
{
    register unsigned i;
    float mag, phase;

    /* rectangular-to-polar conversion */
    for (i = 0; i < size; i++) {
        mag = (float)sqrt(rex[i]*rex[i] + imx[i]*imx[i]);
        /* prevent divide by 0 
        phase = (rex[i] == 0.0f)? 0.0f: (float)atan(imx[i]/rex[i]);*/
        phase = (float)atan(imx[i]/rex[i]);

        if (rex[i] < 0.0f) phase += (imx[i] < 0.0f)? -PI: PI;
        rex[i] = mag;
        imx[i] = phase;
    }
}


void dsp_window( float coef[], unsigned size, int window )
{
	const unsigned size2 = (size-1)/2;
	unsigned       i;
	const float    k = 1.0f/size2;

    switch( window ) {
        case RECTANGULAR:
			for( i = 0; i < size; i++ ) {
                coef[i] = 1.0f;
			}
            break;
		case BARTLETT:
			for (i = 0; i < size; i++) {
				int n = i - size2;
				if (n < 0) n = -n;
				coef[i] = 1.0f - k*n;
			}
            break;
        case HAMMING:
			for( i = 0; i < size; i++ ) {
                coef[i] = 0.54f - 0.46f*cos(PI2*i*k);
			}
            break;
        case HANNING:
			for( i = 0; i < size; i++ ) {
                coef[i] = 0.5f - 0.5f*cos(PI2*i*k);
			}
            break;
        case BLACKMAN:
			for( i = 0; i < size; i++ ) {
                coef[i] = 0.42f - 0.5f*cos(PI2*i*k) + 0.08f*cos(PI4*i*k);
			}
            break;
        case WELCH:
			for( i = 0; i < size; i++ ) {
				int n = i - size2;
				if (n < 0) n = -n;
                coef[i] = 1.0f - sqrtf(k*n);
			}
            break;
    }
}

void dsp_window_apply0( float rex[], float coef[], unsigned size)
{
	unsigned       i;

	for (i = 0; i < size; i += 8) {
		rex[i  ] *= coef[i  ];
		rex[i+1] *= coef[i+1];
		rex[i+2] *= coef[i+2];
		rex[i+3] *= coef[i+3];
		rex[i+4] *= coef[i+4];
		rex[i+5] *= coef[i+5];
		rex[i+6] *= coef[i+6];
		rex[i+7] *= coef[i+7];
	}
}

void dsp_window_apply( float dst[], const float src[], const float coef[], const unsigned size)
{
	unsigned       i;

	for (i = 0; i < size; i += 8) {
		dst[i  ] = src[i  ] * coef[i  ];
		dst[i+1] = src[i+1] * coef[i+1];
		dst[i+2] = src[i+2] * coef[i+2];
		dst[i+3] = src[i+3] * coef[i+3];
		dst[i+4] = src[i+4] * coef[i+4];
		dst[i+5] = src[i+5] * coef[i+5];
		dst[i+6] = src[i+6] * coef[i+6];
		dst[i+7] = src[i+7] * coef[i+7];
	}
}
