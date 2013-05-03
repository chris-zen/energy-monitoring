#include <stdint.h>
#include <avr/pgmspace.h>

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734 
#ifdef PROGMEM 
#undef PROGMEM 
#define PROGMEM __attribute__((section(".progmem.data"))) 
#endif

#include "Stream.h"

#include "Command.h"
#include "Response.h"
#include "Sampler.h"
#include "PowerCalc.h"

Command command = Command(&Serial);

Response response = Response(&Serial);

Sampler sampler;

// power ------------------------------------------------------------

#define DEFAULT_MAX_CICLES 20

uint8_t max_cicles = DEFAULT_MAX_CICLES;

PowerCalc power = PowerCalc(185, -1.0, 61.5);

void calc_power(Sampler& sampler, PowerCalc& power, uint8_t max_cicles, uint32_t max_time) {

  sampler.fast_prescaler();
  
  power.reset();
  
  uint16_t vcc = sampler.read_vcc();
  power.set_vcc(vcc);
  
  //response.debug("vcc: %u", vcc);

  uint8_t cicles = 0;

  uint32_t elapsed_time = 0;
  
  boolean triggered = sampler.trigger();
  
  while (cicles < max_cicles && elapsed_time < max_time) {

    power.compute_sample(
      sampler.get_v(),
      sampler.get_i());

	elapsed_time = micros() - sampler.get_start_time();
	
    triggered = sampler.sample();

    if (triggered) {
      cicles++;
    }
  }
  
  power.compute_total();
  
  Serial.print("> ");
  uint16_t f = 1000000L * (uint64_t) power.get_num_samples() / elapsed_time;
  Serial.print(f);
  Serial.print(" ");
  Serial.print(power.get_num_samples());
  /*Serial.print(" ");
  Serial.print(elapsed_time);*/
  Serial.println();
}

// config -----------------------------------------------------------

#define CONF_FREQ              1
#define CONF_PERIOD            2
#define CONF_TRIGGER           3
#define CONF_CICLES            4
#define CONF_PHCAL             5

#define CONF_NUM_PARAMS        5

#define CONF_NAME_MAX_SIZE 16

char conf_name_buf[CONF_NAME_MAX_SIZE];

const char conf_name_freq[] PROGMEM = "freq";
const char conf_name_period[] PROGMEM = "period";
const char conf_name_trigger[] PROGMEM = "trigger";
const char conf_name_cicles[] PROGMEM = "cicles";
const char conf_name_phcal[] PROGMEM = "phcal";

PGM_P const conf_name[] PROGMEM = {
  conf_name_freq,
  conf_name_period,
  conf_name_trigger,
  conf_name_cicles,
  conf_name_phcal
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
    case CONF_FREQ:
      response.ok("%u", sampler.get_freq());
    break;
    case CONF_PERIOD:
      response.ok("%lu", sampler.get_period());
    break;
    case CONF_TRIGGER:
      response.ok("%u %lu", sampler.get_trigger_level(), sampler.get_trigger_timeout());
    break;
    case CONF_CICLES:
      response.ok("%hhu", max_cicles);
    break;
    case CONF_PHCAL:
      response.ok("%d", (int16_t)(power.get_phcal() * 1000.f));
    break;
  }
}

void config_set(byte param, char* val) {
  switch (param) {
    case CONF_FREQ:
      sampler.set_freq(atoi(val));
      response.ok("%u %lu", sampler.get_freq(), sampler.get_period());
    break;
    case CONF_PERIOD:
      sampler.set_period(atol(val));
      response.ok("%lu %u", sampler.get_period(), sampler.get_freq());
    break;
    case CONF_TRIGGER:
      {
        char* token = strtok(val, " ,");
        if (token != NULL && strlen(token) > 0) {
          byte level = atoi(token);
          if (level > 1023)
            level = sampler.get_trigger_level();
          uint32_t timeout = sampler.get_trigger_timeout();
          token = strtok(NULL, " ,");
          if (token != NULL && strlen(token) > 0)
            timeout = atoi(token);
          sampler.set_trigger(level, timeout);
        }
        response.ok("%u %lu", sampler.get_trigger_level(), sampler.get_trigger_timeout());
      }
    break;
    case CONF_CICLES:
      max_cicles = atol(val);
      if (max_cicles == 0)
        max_cicles = 1;
      else if (max_cicles > 200)
        max_cicles = 200;
      response.ok("%hhu", max_cicles);
    break;
    case CONF_PHCAL:
      power.set_phcal(atol(val) / 1000.0f);
      response.ok("%i", (int16_t)(power.get_phcal() * 1000.f));
    break;
  }
}

void config(const char* args) {
    
  if (strlen(args) == 0) {
    response.ok("%u", CONF_NUM_PARAMS);
    response.println("%s=%u", get_conf_name(CONF_FREQ), sampler.get_freq());
    response.println("%s=%li", get_conf_name(CONF_PERIOD), sampler.get_period());
    response.println("%s=%u,%lu", get_conf_name(CONF_TRIGGER), sampler.get_trigger_level(), sampler.get_trigger_timeout());
    response.println("%s=%hhu", get_conf_name(CONF_CICLES), max_cicles);
    response.println("%s=%i", get_conf_name(CONF_PHCAL), (int16_t)(power.get_phcal() * 1000.f));
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
      response.error("Unknown: %s", name);
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
  
  // warm up the power calculator
  calc_power(sampler, power, 20, 400000);
}

void loop() {
  
  const char* cmd = command.read();
  const char* ch = cmd;
  
  switch (*ch++) {
    case 'C': // Configuration
      config(ch);
    break;
    
    case 'V':
      response.ok("%lu", sampler.read_vcc());
    break;
    
    case 'F':
      response.ok("%i", free_mem());
    break;
    
    case 'P':
      calc_power(sampler, power, max_cicles, (max_cicles + 1) * 20000L);
      response.ok("%d %d %d %d %d",
        (uint16_t) (power.get_v_rms() * 10.0f),
        (uint16_t) (power.get_i_rms() * 10.0f),
        (uint16_t) power.get_real_power(),
        (uint16_t) power.get_apparent_power(),
        (uint16_t) (power.get_power_factor() * 1000.0f));
      Serial.print(power.get_v_rms());
      Serial.print(" ");
      Serial.print(power.get_i_rms());
      Serial.print(" ");
      Serial.print(power.get_real_power());
      Serial.print(" ");
      Serial.print(power.get_apparent_power());
      Serial.print(" ");
      Serial.print(power.get_power_factor());
      Serial.println();
    break;
    
    case 'A':
      response.ok("PowerMonitor 0.1");
    break;
    
    default:
      response.error("Unknown: %s", cmd);
  }
}

