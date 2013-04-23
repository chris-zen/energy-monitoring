#include <stdint.h>
#include <avr/pgmspace.h>
#include "Stream.h"

// command -----------------------------------------------------------

#define CMD_END1 ';'
#define CMD_END2 '\n'

#define MAX_CMD_SIZE 24

class Command {
  private:
    Stream* _stream;
    char _cmd[MAX_CMD_SIZE];
    char _cmd_len;
    
    boolean is_end(char ch) {
      return ch == CMD_END1 || ch == CMD_END2;
    }
  
  public:
    Command(Stream* stream) {
      _stream = stream;
      _cmd[0] = 0;
      _cmd_len = 0;
    }
    
    const char* read() {
      boolean done = false;
      _cmd_len = 0;
      while (!done) {
        if (_stream->available() > 0) {
          // Check for command overflow
          if (_cmd_len >= MAX_CMD_SIZE) {
            // Discard all the command
            while (!is_end(_stream->read()));
            _cmd_len = 0;
            _cmd[0] = 0;
            while (_stream->available() == 0);
          }
          
          char ch = _stream->read();
          if (is_end(ch)) {
            ch = 0;
            done = true;
          }
          _cmd[_cmd_len++] = ch;
        }
      }
      return _cmd;
    }
};

Command command = Command(&Serial);

// response -------------------------------------------------------------

class Response {
  private:
    Stream* _stream;
    size_t _ssize;
    
    void _print(char* type, const char* fmt, va_list args) {
      _stream->print(type);
      _stream->print(" ");
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->println(s);
    }
    
  public:
    Response(Stream* stream, size_t ssize = 32) {
      _stream = stream;
      _ssize = ssize;
    }
    
    void ok(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      _print("OK", fmt, args);
      va_end(args);
    }
    
    void ok() {
      ok("");
    }
    
    void error(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      _print("ERROR", fmt, args);
      va_end(args);
    }
    
    void error() {
      error("");
    }
    
    void print(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->print(s);
      va_end(args);
    }
    
    void println(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->println(s);
      va_end(args);
    }
    
    void println() {
      println("");
    }
    
    void debug(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      char s[_ssize];
      vsnprintf(s, _ssize, fmt, args);
      _stream->print("# ");
      _stream->println(s);
      va_end(args);
    }
};

Response response = Response(&Serial);

// Vcc ----------------------------------------------------------------

uint32_t read_vcc() {
  
  #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  
  #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
  #endif

  delay(2);				// Wait for Vref to settle
  ADCSRA |= _BV(ADSC);			// Convert
  while (bit_is_set(ADCSRA, ADSC));
  
  uint32_t result = ADCL;
  result |= ADCH << 8;
  result = 1126400L / result;		//1100 mV * 1024 ADC steps http://openenergymonitor.org/emon/node/1186
  return result;
}

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
    memset(last_value, 0, sizeof(last_value));
    
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
    memset(last_value, 0, sizeof(last_value));
    
    uint16_t value[MAX_SAMPLING_CHANNELS];
    memset(value, 0, sizeof(value));

    uint16_t max_samp = max_samples;
    if (max_samp == 0)
      max_samp = SAMPLING_BUFFER_SIZE / num_channels;
      
    num_samples = 0;
    
    uint32_t max_time = 0;
    uint32_t elapsed_time;
      
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
    
    uint32_t start_time = micros();
    
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

// config -----------------------------------------------------------

#define CONF_SIZE              1
#define CONF_FREQ              2
#define CONF_PERIOD            3
#define CONF_CHANNELS          4
#define CONF_TRIGGER_ENABLED   5
#define CONF_TRIGGER_PARAMS    6
#define CONF_TRIGGER_TIMEOUT   7

#define CONF_NUM_PARAMS        7

#define CONF_NAME_MAX_SIZE 16

char conf_name_buf[CONF_NAME_MAX_SIZE];

char conf_name_size[] PROGMEM = "size";
char conf_name_freq[] PROGMEM = "freq";
char conf_name_period[] PROGMEM = "period";
char conf_name_channels[] PROGMEM = "channels";
char conf_name_trigger_enabled[] PROGMEM = "trigger_enabled";
char conf_name_trigger_params[] PROGMEM = "trigger";
char conf_name_trigger_timeout[] PROGMEM = "trigger_timeout";

PGM_P conf_name[] PROGMEM = {
  conf_name_size,
  conf_name_freq,
  conf_name_period,
  conf_name_channels,
  conf_name_trigger_enabled,
  conf_name_trigger_params,
  conf_name_trigger_timeout
};

const char* get_conf_name(byte index) {
  strncpy_P(conf_name_buf, (PGM_P)pgm_read_word(&(conf_name[index - 1])), sizeof(conf_name_buf));
  return conf_name_buf;
}

byte get_conf_name_index(const char* name) {
  byte i = 0;
  byte param = atoi(name);
  while (param == 0 && i < CONF_NUM_PARAMS) {
    if (strncmp_P(name, (PGM_P)pgm_read_word(&(conf_name[i])), CONF_NAME_MAX_SIZE) == 0)
      param = i + 1;
    i++;
  }
  return param;
}

void substrn(char* dst, size_t n, const char* src, const char* src_end) {
  char* dst_end = dst + n;
  char* dp = dst;
  for (const char* sp = src; sp != src_end && dp != dst_end; sp++, dp++)
    *dp = *sp;
  *dp = 0;
}

void byte_array_string(char* s, size_t n, byte v[], int len, const char* sep=",") {
  char* sp = s;
  char* slast = s + n;
  if (len > 0) {
    sp += snprintf(sp, slast - sp, "%hhu", v[0]);
    for (int i = 1; i < len && sp < slast; i++) {
      sp += snprintf(sp, slast - sp, "%s%hhu", sep, v[i]);
    }
  }
  sp = min(sp, slast - 1);
  *sp = 0;
}

void config_get(byte param) {
  switch (param) {
    case CONF_SIZE:
      response.ok("%u", sampler.max_samples);
    break;
    case CONF_FREQ:
      response.ok("%u", sampler.freq);
    break;
    case CONF_PERIOD:
      response.ok("%lu", sampler.get_period());
    break;
    case CONF_CHANNELS:
      {
        char s[32] = {0};
        byte_array_string(s, sizeof(s), sampler.channels, sampler.num_channels);
        response.ok("%s", s);
      }
    break;
    case CONF_TRIGGER_ENABLED:
      response.ok("%hhu", sampler.trigger_enabled ? 1 : 0);
    break;
    case CONF_TRIGGER_PARAMS:
      response.ok("%hhu %u", sampler.trigger_channel, sampler.trigger_level);
    break;
    case CONF_TRIGGER_TIMEOUT:
      response.ok("%u", sampler.trigger_timeout);
    break;
  }
}

void config_set(byte param, char* val) {
  switch (param) {
    case CONF_SIZE:
      sampler.set_max_samples(atoi(val));
      response.ok("%u", sampler.max_samples);
    break;
    case CONF_FREQ:
      sampler.set_freq(atoi(val));
      response.ok("%u %lu", sampler.freq, sampler.get_period());
    break;
    case CONF_PERIOD:
      sampler.set_period(atol(val));
      response.ok("%lu %u", sampler.get_period(), sampler.freq);
    break;
    case CONF_CHANNELS:
      {
        byte channels[MAX_SAMPLING_CHANNELS] = {0};
        byte chi = 0;
        
        char* token = strtok(val, " ,");
        while (token != NULL) {
          if (strlen(token) > 0) {
            byte chn = atoi(token);
            if (chi < MAX_SAMPLING_CHANNELS && chn < NUM_ANALOG_PORTS)
              channels[chi++] = chn;
          }
          token = strtok(NULL, " ,");
        }
        
        if (chi > 0) {
          sampler.set_channels(chi, channels);
          char s[24];
          byte_array_string(s, sizeof(s), sampler.channels, sampler.num_channels);
          response.ok("%s", s);
        }
        else {
          response.error();
        }
      }
    break;
    case CONF_TRIGGER_ENABLED:
      sampler.trigger_enabled = atoi(val);
      response.ok("%hhu", sampler.trigger_enabled ? 1 : 0);
    break;
    case CONF_TRIGGER_PARAMS:
      {
        boolean ok = false;
        char* token = strtok(val, " ,");
        if (token != NULL && strlen(token) > 0) {
          byte channel = atoi(token);
          if (channel > sampler.num_channels)
            channel = sampler.num_channels - 1;
          token = strtok(NULL, " ,");
          if (token != NULL && strlen(token) > 0) {
            uint16_t level = atoi(token);
            if (level > 1023)
              level = 1023;
            sampler.trigger_channel = channel;
            sampler.trigger_level = level;
            ok = true;
          }
        }
        if (ok)
          response.ok("%hhu %u", sampler.trigger_channel, sampler.trigger_level);
        else
          response.error("<chn> <lvl>");
      }
    break;
    case CONF_TRIGGER_TIMEOUT:
      sampler.trigger_timeout = atoi(val);
      response.ok("%u", sampler.trigger_timeout);
    break;
  }
}

void config(const char* args) {
    
  if (strlen(args) == 0) {
    response.ok("%u", CONF_NUM_PARAMS);
    response.println("%s=%u", get_conf_name(CONF_SIZE), sampler.max_samples);
    response.println("%s=%u", get_conf_name(CONF_FREQ), sampler.freq);
    response.println("%s=%li", get_conf_name(CONF_PERIOD), sampler.get_period());
    char s[32] = {0};
    byte_array_string(s, sizeof(s), sampler.channels, sampler.num_channels);
    response.println("%s=%s", get_conf_name(CONF_CHANNELS), s);
    response.println("%s=%u", get_conf_name(CONF_TRIGGER_ENABLED), sampler.trigger_enabled ? 1 : 0);
    snprintf(s, sizeof(s), "%hhu,%u", sampler.trigger_channel, sampler.trigger_level);
    response.println("%s=%s", get_conf_name(CONF_TRIGGER_PARAMS), s);
    response.println("%s=%u", get_conf_name(CONF_TRIGGER_TIMEOUT), sampler.trigger_timeout);
  }
  else {
    const char* name = args;
    char* sep = strchr(args, '=');
    if (sep != NULL) {
      substrn(conf_name_buf, CONF_NAME_MAX_SIZE, args, sep); // memcpy(conf_name_buf, args, sep - args);
      name = conf_name_buf;
    }
    
    byte param = get_conf_name_index(name);
    
    if (param == 0) {
      response.error("Unknown config parameter: %s", name);
      return;
    }
      
    if (sep == NULL)     // get
      config_get(param);
    else                 // set
      config_set(param, ++sep);
  }
}

// -----------------------------------------------------------

int free_mem() {  
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);  
}

void setup() {
  Serial.begin(115200);

  while (Serial.available() > 0)
    Serial.read();
    
  sampler.set_freq(DEFAULT_SAMPLING_FREQ);
  
  //Serial.print('>'); Serial.println(free_mem());
}

void loop() {
  
  const char* cmd = command.read();
  const char* ch = cmd;
  
  switch (*ch++) {
    case 'C': // Configuration
      config(ch);
    break;
    
    case 'S': // Sample
      sampler.sample(ch);
    break;

    case 'R': // Read samples
      sampler.send_samples();
    break;
    
    case 'N': // Number of samples
      response.ok("%u", sampler.num_samples);
    break;
    
    case 'V':
      response.ok("%lu", read_vcc());
    break;
    
    case 'F':
      response.ok("%i", free_mem());
    break;
    
    case 'A':
      response.ok("%s", "emonSampler 0.1");
    break;
    
    default:
      response.error("Unknown: %s", cmd);
  }
}

