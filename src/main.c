#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "pico/stdlib.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "i2s.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "pico/bootrom.h"

#include "pitch_analysis.h"
#include "adc_stream.h"
#include "nco.h"

#define LED_PIN 0

#define ADC_GPIO_PIN 28
#define RESET_PIN 1
int reset_pin_check() {
    return ((sio_hw->gpio_in & (1<<RESET_PIN)) != 0);
}

// 20ms lets us see (theoretically) 50Hz
// 15kHz gives us enough granularity to differentiate higher pitches
#define YIN_WINDOW_WIDTH_MS 20 // how many ms of audio do we want to analyze? See Nyquist freq sampling.
#define ADC_SAMPLE_RATE_HZ 15000 // what's our sample rate? See Nyquist time sampling
#define AUDIO_BUFFER_SIZE          (YIN_WINDOW_WIDTH_MS * ADC_SAMPLE_RATE_HZ / 1000)


// ————————————————————————————————————————————————————————————————————————————————————————————————
//                      MULTICORE ENTRY
// ————————————————————————————————————————————————————————————————————————————————————————————————
#define MULTICORE_GOOD_FLAG 0xBEEF
void core1_entry() {

    // initialize...
    multicore_fifo_push_blocking(MULTICORE_GOOD_FLAG); // tell main that we're good

    FREQ_ANALYZER_T *fa_s = (FREQ_ANALYZER_T *)multicore_fifo_pop_blocking(); // get access to the freq analyzer struct

    int frequency, volume = 0;
    int i = 0;
    while(1) {
        multicore_fifo_pop_blocking();
        volume = get_peak_volume(fa_s);

        calculate_yin(fa_s);
        if(volume > 22) {
            frequency = dominant_freq(fa_s);
        }
        else {
            frequency = 0;
        }
        multicore_fifo_push_blocking((uint32_t)frequency); // tell main to continue
        i += 1;
    }


}



static __attribute__((aligned(8))) pio_i2s i2s; /****Not sure... **/

volatile int32_t outsignal[STEREO_BUFFER_SIZE];

NCO_T *osc;
static void process_audio(const int32_t* input, int32_t* output, size_t num_frames) {
    // Just copy the input to the output
    //uint32_t freq = adc_read() >> 7;
    for (size_t i = 0; i < num_frames * 2; i++) {
        output[i] = outsignal[i]; /** **CHANGED !!!! =input[i]*/
    }
//    uint16_t adc_val = adc_read();
//    nco_set_frequency( osc,
//                       0.000005f * (float)adc_val
//            );
    nco_get_samples_stereo(osc, outsignal, num_frames);
}

static void dma_i2s_in_handler(void) {
    /* We're double buffering using chained TCBs. By checking which buffer the
     * DMA is currently reading from, we can identify which buffer it has just
     * finished reading (the completion of which has triggered this interrupt).
     */
    if (*(int32_t**)dma_hw->ch[i2s.dma_ch_out_ctrl].read_addr == i2s.output_buffer) {
        // It is inputting to the second buffer so we can overwrite the first
        process_audio(i2s.input_buffer, i2s.output_buffer, AUDIO_BUFFER_FRAMES);
    } else {
        // It is currently inputting the first buffer, so we write to the second
        process_audio(&i2s.input_buffer[STEREO_BUFFER_SIZE], &i2s.output_buffer[STEREO_BUFFER_SIZE], AUDIO_BUFFER_FRAMES);
    }
    dma_hw->ints0 = 1u << i2s.dma_ch_out_data;  // clear the IRQ
}


// FROM I2S.C:      const i2s_config i2s_config_default = {48000, 256, 32, 10, 6, 7, 8, true};
/** **BECAUSE THE DAC IS L/R, WE NEED TO SEND DATA AT TWICE THE SPEED. THIS BRINGS US FROM 44.1kHz to 88.2kHz */
#define FS  100000 //for oscilliscope-based testing, you'll likely have to reduce this significantly...
/**** NOT SURE IF SCK SHOULD GO TO BCK OR SCK */
const i2s_config i2s_config_PCM1502a = {
    FS,     //fs
    256,    //sck_mult
    32,     //bit_depth
    18,     //sck_pin
    19,      //data out pin
    0,      //data in pin
    8,      //clock pin base
    true    //sck enable
};



int main() {
    set_sys_clock_khz(100000, true); //set sys clock to 132MHz
    stdio_init_all();

    printf("System Clock: %lu\n", clock_get_hz(clk_sys));
    
    

    // Init GPIO LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // I2C Initialisation. Using it at 100Khz.
    /**** ALTERED:  88.2kHz **/


//    // ADC
//    adc_init();
//    adc_gpio_init(27);
//    adc_select_input(27 - 26);

    // NCO
    osc = init_nco(0.01, 0, 0.04);
    nco_get_samples_stereo(osc, outsignal,AUDIO_BUFFER_FRAMES);

    // RESET PIN INIT
    gpio_init(RESET_PIN);
    gpio_pull_down(RESET_PIN);

    // AUDIO IN AND PITCH DETECTION
    adc_stream_t *adc_stream = init_adc_stream(ADC_GPIO_PIN, ADC_SAMPLE_RATE_HZ, AUDIO_BUFFER_SIZE);
    sleep_ms(1000);

    FREQ_ANALYZER_T *fa_s;
    fa_s = init_freq_analyzer(adc_stream->buffer, adc_stream->buffer_size, adc_stream->adc_sample_rate, 50);
    multicore_launch_core1(core1_entry);
    uint32_t mcfifo_val = multicore_fifo_pop_blocking();
    if(mcfifo_val != MULTICORE_GOOD_FLAG) printf("Failed to initialize core 1.\n");
    multicore_fifo_push_blocking((uint32_t)fa_s);
    sleep_ms(1000);

    // AUDIO OUTPUT BEGINS
    i2s_program_start_synched(pio0, &i2s_config_default, dma_i2s_in_handler, &i2s);


    int frequency = -1;
    while(1) {
        audio_capture_no_blocking(adc_stream);
        dma_channel_wait_for_finish_blocking(adc_stream->dma_channel); //wait for end of capture
        multicore_fifo_push_blocking(0); // tell other core to analyze data
        frequency = (int)multicore_fifo_pop_blocking(); // wait for end of analysis (any interrupts will be exucuted by this core, so that works out!
        if(frequency > 0) nco_set_frequency(osc, 2*(float)frequency / (float)FS);
    }

}

