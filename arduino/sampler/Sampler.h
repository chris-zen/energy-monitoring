// Sampling -----------------------------------------------------------

#define SAMPLING_BUFFER_SIZE 1200
#define DEFAULT_SAMPLING_FREQ 4000
#define MAX_SAMPLING_CHANNELS 2
#define NUM_ANALOG_PORTS 6
#define DEFAULT_TRIGGER_LEVEL 495
#define DEFAULT_TRIGGER_TIMEOUT 1000

typedef uint16_t sample_t;

class Sampler {
  private:
  
  byte buf[SAMPLING_BUFFER_SIZE];

  public:

  uint16_t num_samples;
  uint16_t max_samples;
    
  uint16_t freq;
  
  byte channels[MAX_SAMPLING_CHANNELS];
  byte num_channels;
  
  boolean trigger_enabled;
  byte trigger_channel;
  uint16_t trigger_level;
  uint16_t trigger_timeout; // miliseconds
  
  Sampler() {
    num_samples = 0;
    max_samples = 0; // unlimited
    freq = DEFAULT_SAMPLING_FREQ;
    channels[0] = 1;
    channels[1] = 2;
    num_channels = 2;
    trigger_enabled = true;
    trigger_channel = 1;
    trigger_level = DEFAULT_TRIGGER_LEVEL;
    trigger_timeout = DEFAULT_TRIGGER_TIMEOUT;
  }
  
  void set_freq(uint16_t f) {
    if (f == 0)
      f = DEFAULT_SAMPLING_FREQ;
    uint32_t period = 1000000L / f;
    freq = 1000000L / period;
  }
  
  void set_period(uint32_t period) {
    if (period == 0)
      period = 1000000L / DEFAULT_SAMPLING_FREQ;
    freq = 1000000L / period;
  }
  
  uint32_t get_period() {
    return 1000000L / freq; // in microseconds
  }

  void set_max_samples(uint16_t ms) {
    max_samples = ms;
  }

  void set_channels(byte nchan, byte chans[]) {
    memcpy(channels, chans, sizeof(channels));
    num_channels = nchan;
  }
  
  void send_samples() {
    byte* pv = buf;
    
    uint16_t last_value[MAX_SAMPLING_CHANNELS];
    memset(last_value, 512, sizeof(last_value));
    
    for (uint16_t i = 0; i < num_samples; i++) {
      for (byte j = 0; j < num_channels; j++) {
        
        //response.print(" [%hhx]", *pv);
        
        int16_t diff = ((byte) *pv++) & 0xFF;
        
        //response.print("{%x}", diff & 0x80);
        
        uint16_t value;
        if ((diff & 0x80) == 0) {
          volatile int16_t v = diff << 9;
          value = last_value[j] + (v >> 9);
        }
        else {
          volatile int16_t v = diff << 9;
          
          //response.print("[%hhx]", *pv);
          
          value = (v >> 1) | (byte) *pv++;
        }
        last_value[j] = value;
        
        if (j == 0)
          response.print("%u", value);
        else
          response.print(" %u", value);
      }
      response.println();
    }
  }
  
  void sample(const char* args) {
    byte* ps = buf; // current sample pointer
    byte* pc = buf; // current channel pointer
    byte* buf_end = &buf[SAMPLING_BUFFER_SIZE];
    
    uint32_t sampling_period = get_period();
  
    uint16_t last_value[MAX_SAMPLING_CHANNELS];
    memset(last_value, 512, sizeof(last_value));
    
    uint16_t value[MAX_SAMPLING_CHANNELS];
    memset(value, 0, sizeof(value));

    uint16_t max_samp = max_samples;
    if (max_samp == 0)
      max_samp = SAMPLING_BUFFER_SIZE / num_channels;
      
    num_samples = 0;
    
    // Change de ADC prescaler to CLK/16
    bitClear(ADCSRA, ADPS0);
    bitClear(ADCSRA, ADPS1);
    bitSet(ADCSRA, ADPS2) ;
    
    // Triggering
    
    if (trigger_enabled) {
      uint16_t trigger_pin = channels[trigger_channel];
      uint16_t last_value;
      uint16_t value = 1024;
      boolean triggered = false;
      uint32_t start_time = millis();
      while (!triggered && ((millis() - start_time) < trigger_timeout)) {
        last_value = value;
        value = analogRead(trigger_pin);
        triggered = (last_value <= trigger_level && value >= trigger_level);
      }
      /*if (!triggered) {
        response.error("TRIGGER TIMEOUT");
        return;
      }*/
    }
    
    // Sampling
    
    uint32_t max_time = 0;
    uint32_t start_time = micros();
    uint32_t elapsed_time = start_time;
    
    while (pc < buf_end && num_samples < max_samp) {
      
      // Read sample for all the channels
      for (byte j = 0; j < num_channels; j++)
        value[j] = analogRead(channels[j]);
      
      // Compress and store information
      byte j = 0;
      while (j < num_channels && pc < buf_end) {
        uint16_t v = value[j];
        int16_t diff = v - last_value[j];
        //response.print(" [%u - %u = %i]", v, last_value[j], diff);
        last_value[j] = v;
        if (diff >= -(1 << (7 - 1)) && diff <= ((1 << (7 - 1)) - 1)) { // -2**(n-1) <= value <= 2**(n-1) - 1
          *pc++ = (byte) (diff & 0x7F);
          j++;
        }
        else {
          *pc++ = (byte) (((v >> 8) & 0x7F) | 0x80);
           if (pc < buf_end) {
             *pc++ = (byte) (v & 0xFF);
             j++;
           }
           else
             pc++;
        }
      }
      //response.println();
      if (j <= num_channels && pc <= buf_end) {
        ps = pc;
        num_samples++;
      }

      // Wait until the period time ends
      max_time += sampling_period;
      elapsed_time = micros() - start_time;
      while (elapsed_time < max_time)
        elapsed_time = micros() - start_time;
    }
    
    // Response
    
    uint16_t f = 1000000L * num_samples / elapsed_time;
    uint32_t period = 1000000L / f;
    
    response.ok("%u %hhu %u %lu", num_samples, num_channels, f, period);
    
    if (*args == 'R')
      send_samples();
  }
  
  /*void normal_dist(const char* args) {
    uint32_t sum[num_channels];
    uint32_t sum_sqr[num_channels];
    memset(sum, 0, sizeof(sum));
    memset(sum_sqr, 0, sizeof(sum_sqr));
    
    sample_t* bp = buf;
    for (uint16_t i = 0; i < num_samples; i++) {
      for (byte j = 0; j < num_channels; j++) {
        uint16_t value = *bp++;
        sum[j] += value;
        sum_sqr[j] += value * value;
      }
    }
    
    response.ok("%u %hhu", num_samples, num_channels);
    
    uint16_t N = num_samples;
    float inv_N = 1.0f / N;
    for (byte j = 0; j < num_channels; j++) {
      response.print("%hhu", j);
      char s[10];
      float mean = sum[j] * inv_N;
      dtostrf(mean, 1, 2, s);
      response.print(" %s", s);
      float sd = sqrt((N * sum_sqr[j]) - sum[j] * sum[j]) / N;
      dtostrf(sd, 1, 2, s);
      response.println(" %s", s);
    }
  }*/

};

Sampler sampler;
