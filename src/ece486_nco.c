/*****************************************************************


 file: ece486_nco.c
 Description:  Subroutines to implement numerically controlled oscillators
        Multiple subroutine calls are used to create sample sequences
        for sinusoidal functions with programmable frequency and phase.
            y(n) = cos(2 M_PI f0 n + theta)
        

 Interface:
     init_nco() is called once (for each oscillator), and is used to create
                any required oscillator structures.  This routine handles all
                array initialization, allocates any required memory,
                and perform any other required initialization.
     nco_set_frequency() is used to change the frequency of the nco 
     nco_set_phase() is used to change the phase of the nco 
     nco_get_samples() return a block of nco output samples.
     destroy_nco() is called once at the end of the program, and is used
                  to de-allocate any memory associated with an nco.

  Function Prototypes and parameters:

        #include "nco.h"
        NCO_T *init_nco(float f0, float theta);
           Inputs:
                f0     Initial NCO discrete-time frequency for the output sample
                       sequence (in cycles/sample)
                theta  Initial Phase of the output sample sequence (radians)
           Returned:
                The function returns a pointer to a "NCO_T" data type
                (which is defined in "nco.h")
        
        void nco_get_samples(NCO_T *s, float *y, int n_samples);
           Inputs:
              s         pointer to initialized NCO_T
              y         Caller-supplied array to be replaced by NCO output samples
              n_samples Number of NCO output samples to generate.
           Returned:
              On return, y[i] (for i=0,1,...n_samples-1) contains the the next
              n_samples values of the NCO output sequence.  Subsequent calls to 
              nco_get_samples() continues to return NCO output samples with no 
              phase discontinuities from one call to the next. 
              Example:
                NCO_T *s;
                float y1[15],y2[5];
                s=init_nco(0.11, 0.0);       // set up NCO for .11 cycles/sample
                nco_get_samples(s, y1, 15);  // Generate samples 0,1, ..., 14 of y[n]
                nco_get_samples(s, y2, 5);   // Generate samples 15,16,17,18,19 of [n]
              
        void nco_set_frequency(NCO_T *s, float f_new);
          Inputs:
             s         pointer to NCO_T
             f_new     New NCO frequency (in cycles/sample).
          Returned:
             The NCO_T structure s is modified so that subsequent calls to nco_get_samples()
             will operate at frequency f_new (without loss of phase continuity).
        
        void nco_set_phase(NCO_T *s, float theta);
          Inputs:
             s         pointer to NCO_T
             theta     New NCO phase.
          Returned:
             The NCO_T structure s is modified so that subsequent calls to nco_get_samples()
             will operate with the phase shift given by theta (in radians)
        
        void destroy_nco(NCO_T *s);
           Inputs:
                s       pointer to NCO_T, as provided by init_nco()
           Returned: Any resources associated with the nco "s" are released.
 
 
*******************************************************************/ 

#include "ece486_nco.h"
#include "cos_vals.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

NCO_T *init_nco(float f0, float theta){
    /*******  ECE486 STUDENTS MODIFY THIS *******/
    NCO_T *s = (NCO_T *) malloc(sizeof(NCO_T));
    // let's convert our floats to uint32_t
    // why? so that we can mask with 512 w/o dealing with float-int-conversions
    // our cos table is 512 long. That's 9 bits. If we want the 9 MSB, that's 0xFF80 0000
    // so, a frequency of 1 would mean that we increment by 512 << (32 - 9) = 4,294,967,296 every n
    // we'll use the macros defined in the header file...

    uint32_t i_f0;

    if(f0 == 1) i_f0 = 0;
    else i_f0 = (uint32_t) (NCO_F_SCALAR * f0); // scale up then bring it down to an int

    uint32_t i_theta = (uint32_t) (NCO_F_SCALAR * theta); // scale up then bring it down to an int

    s->f0 = i_f0;
    s->theta = i_theta;
    s->A = 1;
    //s->accum = 0;
    return(s); 
}

void nco_get_samples(NCO_T *s, float *y, int n_samples){
    /*******  ECE486 STUDENTS MODIFY THIS *******/

    for(int i = 0; i<n_samples; i++) {
        y[i] = s->A * cos_vals[(s->theta >> NCO_BITSHIFT) & NCO_MASK]; //TODO: check for sign-extension. If there is no sign-extension you don't need the mask
        s->theta += s->f0; // since we're using MSB, there's no need to AND anything
    }
}

void nco_get_samples_dual_mono_int(NCO_T *s, uint32_t *y, int n_samples) {
    for(int i = 0; i<n_samples; i++) {
        float val = s->A * cos_vals[(s->theta >> NCO_BITSHIFT) & NCO_MASK];
        uint32_t valint = (uint32_t)val;
        y[2*i] = valint;
        y[2*i + 1] = valint;
        s->theta += s->f0; // since we're using MSB, there's no need to AND anything
    }
}

void nco_set_frequency(NCO_T *s, float f_new){
    /*******  ECE486 STUDENTS MODIFY THIS *******/
    uint32_t i_f0;
    if(f_new == 1) i_f0 = 0;
    else i_f0 = (uint32_t) (NCO_F_SCALAR * f_new); // scale up then bring it down to an int
    s->f0 = i_f0;
}

void nco_set_amplitude(NCO_T *s, float a_new) {
    s->A = a_new;
}

void nco_reset_phase(NCO_T *s, float theta) {
    uint32_t i_theta = (uint32_t) (NCO_F_SCALAR * theta); // scale up then bring it down to an int
    s->theta = i_theta;
}

void nco_set_phase(NCO_T *s, float theta){
    /*******  ECE486 STUDENTS MODIFY THIS *******/
    uint32_t i_theta = (uint32_t) (NCO_F_SCALAR * theta); // scale up then bring it down to an int
    s->theta += i_theta; //NOTE: we **increase** the theta, because the theta is also the accumulator!!
}

void destroy_nco(NCO_T *s){
    /*******  ECE486 STUDENTS MODIFY THIS *******/
    free(s);
}



