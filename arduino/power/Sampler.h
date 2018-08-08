// Sampling -----------------------------------------------------------

#define DEFAULT_SAMPLING_FREQ 4000
#define NUM_ANALOG_PORTS 6
#define DEFAULT_TRIGGER_LEVEL 495
#define DEFAULT_TRIGGER_TIMEOUT 1000000

typedef uint16_t sample_t;

class Sampler {
  private:
  
  uint16_t freq;
  uint32_t period;
  
  uint8_t v_pin;
  uint8_t i_pin;

  uint16_t trigger_level;
  uint32_t trigger_timeout;

  uint32_t start_time;
  uint32_t total_time;
  uint32_t elapsed_time;

  uint16_t last_v_value;

  uint16_t v_value;
  uint16_t i_value;

  public:

  static const uint16_t TRIGGER_TIMEOUT = 0xFFFF;

  Sampler() {
    freq = DEFAULT_SAMPLING_FREQ;
    v_pin = 2;
    i_pin = 1;
    trigger_level = DEFAULT_TRIGGER_LEVEL;
    trigger_timeout = DEFAULT_TRIGGER_TIMEOUT;
    start_time = 0;
  }
  
  void set_freq(uint16_t f) {
    if (f == 0)
      f = DEFAULT_SAMPLING_FREQ;
    period = 1000000L / f;
    freq = 1000000L / period;
  }

  uint16_t get_freq() {
    return freq;
  }
  
  void set_period(uint32_t period) {
    if (period == 0)
      period = 1000000L / DEFAULT_SAMPLING_FREQ;
    this->period = period;
    freq = 1000000L / period;
  }
  
  uint32_t get_period() {
    //return 1000000L / freq; // in microseconds
    return period;
  }

  void set_pins(uint8_t vpin, uint8_t ipin) {
    v_pin = vpin;
    i_pin = ipin;
  }

  uint8_t get_v_pin() {
    return v_pin;
  }

  uint8_t get_i_pin() {
    return i_pin;
  }

  void set_trigger(uint16_t level, uint16_t timeout) {
    trigger_level = level;
    trigger_timeout = timeout;
  }

  uint16_t get_trigger_level() {
    return trigger_level;
  }

  uint32_t get_trigger_timeout() {
    return trigger_timeout;
  }

  uint16_t get_v() {
    return v_value;
  }

  uint16_t get_i() {
    return i_value;
  }
  
  uint32_t get_start_time() {
    return start_time;
  }
  
  uint32_t get_elapsed_time() {
    return elapsed_time;
  }

  void fast_prescaler() {
    // Change de ADC prescaler to CLK/16
    bitClear(ADCSRA, ADPS0);
    bitClear(ADCSRA, ADPS1);
    bitSet(ADCSRA, ADPS2) ;
  }

  uint16_t read_vcc() {
    #if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  
    #elif defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
    #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
    #endif

    delay(2);       // Wait for Vref to settle
    ADCSRA |= _BV(ADSC);      // Convert
    while (bit_is_set(ADCSRA, ADSC));
    
    uint32_t result = ADCL;
    result |= ADCH << 8;
    result = 1126400L / result;   //1100 mV * 1024 ADC steps http://openenergymonitor.org/emon/node/1186
    return result;
  }

  boolean trigger() {
    total_time = elapsed_time = 0;
    v_value = 1024;
    boolean triggered = false;
    start_time = micros();
    uint32_t timeout_start = start_time;
    while (!triggered && ((start_time - timeout_start) < trigger_timeout)) {
      last_v_value = v_value;
      start_time = micros();
      v_value = analogRead(v_pin);
      i_value = analogRead(i_pin);
      triggered = (last_v_value < trigger_level && v_value >= trigger_level);
    }

    return triggered;
  }

  boolean sample() {
    last_v_value = v_value;
    
    // Wait until the period time ends
    /*total_time += get_period();
    elapsed_time = micros() - start_time;
    while (elapsed_time < total_time)
      elapsed_time = micros() - start_time;*/

	//i_value = analogRead(i_pin);
    v_value = analogRead(v_pin);
    i_value = analogRead(i_pin);

    return last_v_value < trigger_level && v_value >= trigger_level;
  }

};
