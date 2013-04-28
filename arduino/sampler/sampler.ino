#include <stdint.h>
#include <avr/pgmspace.h>
#include "Stream.h"

#include "Command.h"
#include "Response.h"
#include "Sampler.h"

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

const char conf_name_size[] PROGMEM = "size";
const char conf_name_freq[] PROGMEM = "freq";
const char conf_name_period[] PROGMEM = "period";
const char conf_name_channels[] PROGMEM = "channels";
const char conf_name_trigger_enabled[] PROGMEM = "trigger_enabled";
const char conf_name_trigger_params[] PROGMEM = "trigger";
const char conf_name_trigger_timeout[] PROGMEM = "trigger_timeout";

PGM_P const conf_name[] PROGMEM = {
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
      response.error("Unknown conf: %s", name);
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
      response.ok("emonSampler 0.1");
    break;
    
    default:
      response.error("Unknown: %s", cmd);
  }
}

