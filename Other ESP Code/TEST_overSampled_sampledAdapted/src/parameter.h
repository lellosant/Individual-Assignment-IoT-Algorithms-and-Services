

//note DMA minimum sample rate is 20 kHz, i do some test and with 10Khz the relevated frequency is Half and a 5Khz is a quarter
#define MAX_SAMPLE_RATE 1800000 // Hz 1.8MHz
#define SAMPLES 1024 // for FFT (must be a power of 2)
#define MIN_SAMPLE_RATE 500 // Hz



#define BUFFER_DMA_SIZE 512
#define I2S_NUM     I2S_NUM_0
#define NR_BUFFER_DMA 16
#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHANNEL ADC1_CHANNEL_6


#define SNR_MULTIPLIER 3

#define WINDOW_TIME_SAMPLING_SECONDS 5


