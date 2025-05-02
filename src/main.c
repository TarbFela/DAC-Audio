#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "i2s.h"

//#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

//#include "ssd1306.h"

#include "pico/stdlib.h"

#include "ece486_nco.h"

#define LED_PIN 0


static __attribute__((aligned(8))) pio_i2s i2s; /****Not sure... **/
NCO_T *osc1;

volatile int32_t outsignal[STEREO_BUFFER_SIZE];

static void process_audio(const int32_t* input, int32_t* output, size_t num_frames, NCO_T *osc) {
    float freq = (float)adc_read() / 4096;
    for (size_t i = 0; i < num_frames; i++) {
        uint32_t val = (uint32_t)(160000000.0f * sinf(freq * M_2_PI * i / num_frames));
        output[2*i] = val;
        output[2*i + 1] = val;
    }
    //float f = (float)adc_read() * 0.00001f;
    //nco_set_frequency(osc, f);
    //nco_get_samples_dual_mono_int(osc, output, num_frames);
    // left side
    /*
    for (size_t i = 0; i < num_frames; i++) {
        output[2*i] = nco_get_samples(osc, );
        // and right side...
    }
     */

    // right side
    /*
    for (size_t i = 0; i < num_frames; i++) {
        output[2*i + 1] = outsignal[i];
    }
     /**/
}

static void dma_i2s_in_handler(void) {
    /* Previously, this was set up to check which buffer the input dma was using
     * Now, we aren't dealing with any input dma, we just want to generate a signal
     * So, we will figure out where the output dma is reading from instead
     */
    // if reading from begging (&output_buffer[0]), write to middle i.e. buffer 2 (&output_buffer[STEREO_BUFF_SIZE])
    if (*(int32_t**)dma_hw->ch[i2s.dma_ch_out_ctrl].write_addr == i2s.output_buffer) {
        // It is inputting to the second buffer so we can overwrite the first
        process_audio(i2s.input_buffer, &i2s.output_buffer[STEREO_BUFFER_SIZE], AUDIO_BUFFER_FRAMES, osc1);
    } else {
        // It is currently inputting the first buffer, so we write to the second
        process_audio(&i2s.input_buffer[STEREO_BUFFER_SIZE], i2s.output_buffer, AUDIO_BUFFER_FRAMES, osc1);
    }
    dma_hw->ints0 = 1u << i2s.dma_ch_in_data;  // clear the IRQ
}


// FROM I2S.C:      const i2s_config i2s_config_default = {48000, 256, 32, 10, 6, 7, 8, true};
/** **BECAUSE THE DAC IS L/R, WE NEED TO SEND DATA AT TWICE THE SPEED. THIS BRINGS US FROM 44.1kHz to 88.2kHz */
#define FS  48000 //for oscilliscope-based testing, you'll likely have to reduce this significantly...
/**** NOT SURE IF SCK SHOULD GO TO BCK OR SCK */
const i2s_config i2s_config_PCM1502a = {
    FS,     //fs
    256,    //sck_mult
    32,     //bit_depth
    10,     //sck_pin
    6,      //data out pin
    0,      //data in pin
    8,      //clock pin base
    true    //sck enable
};



int main() {
    set_sys_clock_khz(100000, true); //set sys clock to 132MHz
    stdio_init_all();

    printf("System Clock: %lu\n", clock_get_hz(clk_sys));
    
    
    //populate the input buffer with something, idk???
    for(int i=0; i<STEREO_BUFFER_SIZE;i++) {
            outsignal[i] = 0;
       }

    // Init GPIO LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    adc_init();
    adc_gpio_init(27);
    adc_select_input(27 - 26);

    osc1 = init_nco(adc_read(),0);
    nco_set_amplitude(osc1, 100000000);
    
    
    // PROTOTYPE:       i2s_program_start_synched(pio0, &i2s_config_default, dma_i2s_in_handler, &i2s);
    // FROM I2S.C:      const i2s_config i2s_config_default = {48000, 256, 32, 10, 6, 7, 8, true};
    i2s_program_start_output(pio0, &i2s_config_default, dma_i2s_in_handler, &i2s);
    
    
    while(1) {
        
    }

}

