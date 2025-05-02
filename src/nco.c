/*****************************************************************

 file: ece486_nco.c
 Description: Subroutines to implement numerically controlled oscillators
 using a 512-sample cosine lookup table, with programmable
 frequency and phase. Supports multiple independent oscillators.

 Output:
    y[n] = A * cos(2π f0 n + θ)

*******************************************************************/

#include "nco.h"
#include "sin_vals.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

// Create a new oscillator with specified frequency and initial phase (in radians)
// amplitude: value between 0 and 1
NCO_T *init_nco(float f0, float theta_rad, float amplitude) {
    NCO_T *nco = (NCO_T *)malloc(sizeof(NCO_T));
    if (!nco) return NULL;

    // Convert frequency (cycles/sample) to 32-bit phase increment
    nco->phase_inc = (uint32_t)(f0 * NCO_F_SCALAR);

    // Convert initial phase (radians) to cycles → 32-bit phase
    float theta_cycles = theta_rad / (2.0f * M_PI);
    nco->phase_acc = (uint32_t)(theta_cycles * (float)(1ULL << 32));

    nco->inverse_amplitude = (uint32_t)(1/amplitude);

    // nco->A = 1.0f; // default amplitude
    return nco;
}

// Generate n_samples from the NCO
void nco_get_samples(NCO_T *nco, uint32_t *y, int n_samples) {
    for (int i = 0; i < n_samples; i++) {
        uint32_t index = (nco->phase_acc >> NCO_BITSHIFT) & NCO_MASK;
        y[i] = sin_vals[index] / nco->inverse_amplitude; //division might be the slowest way to do this...
        nco->phase_acc += nco->phase_inc;
    }
}

// Generate n_samples from NCO and put them into pairs
void nco_get_samples_stereo(NCO_T *nco, uint32_t *y, int n_samples) {
    for (int i = 0; i < n_samples; i++) {
        uint32_t index = (nco->phase_acc >> NCO_BITSHIFT) & NCO_MASK;
        uint32_t val = sin_vals[index] / nco->inverse_amplitude;
        y[2*i] = val; //division might be the slowest way to do this...
        y[2*i + 1] = val;
        nco->phase_acc += nco->phase_inc;
    }
}

// Set new frequency (in cycles/sample)
void nco_set_frequency(NCO_T *nco, float f_new) {
    nco->phase_inc = (uint32_t)(f_new * (float)(1ULL << 32));
}


// Set absolute phase (in radians)
void nco_set_phase(NCO_T *nco, float theta_rad) {
    float theta_cycles = theta_rad / (2.0f * M_PI);
    nco->phase_acc = (uint32_t)(theta_cycles * (float)(1ULL << 32));
}

// Set amplitude (somewhere between 0 and 1)
void nco_set_amplitude(NCO_T *nco, float amplitude) {
    nco->inverse_amplitude = (uint32_t)(1/amplitude);
}
// Free memory for NCO
void destroy_nco(NCO_T *nco) {
    free(nco);
}
