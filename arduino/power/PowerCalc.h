#include <stdint.h>
#include <math.h>
#include "Arduino.h"

class PowerCalc {
	private:
		float v_rms;
		float i_rms;
		float p_real;
		float p_app;
		float pf;

		float VCAL, VR;
		float PHCAL;
		float ICAL, IR;

		uint32_t num_samples;

		float v_sum;
		float last_v_filt;
		float last_voltage;

		float i_sum;
		float last_i_filt;
		float last_intensity;

		float p_sum;

	public:
		PowerCalc(float VCAL, float PHCAL, float ICAL) {
			this->VCAL = VCAL;
			this->PHCAL = PHCAL;
			this->ICAL = ICAL;

			VR = IR = 0.0f;

			reset();
			reset_filters();
		};

		void reset() {
			v_rms = i_rms = 0.0f;
			p_real = p_app = 0.0f;
			pf = 0.0f;
			
			v_sum = i_sum = p_sum = 0.0f;

			num_samples = 0;
		};

		void reset_filters() {
			last_v_filt = 0.0f;
			last_i_filt = 0.0f;

			last_voltage = 0.0f;
			last_intensity = 0.0f;
		};

		void set_vcc(uint16_t vcc) {
			float R0 = vcc / (1000.0f * 1023.0f);

			VR = VCAL * R0;
			IR = ICAL * R0;
		};

		void compute_sample(uint16_t voltage, uint16_t intensity) {
			// Voltage High pass filter
			float v_filt = 0.996 * (last_v_filt + voltage - last_voltage);
			
			// Vsum = Sum(v_filt^2)
			v_sum += (v_filt * v_filt);

			// Intensity High pass filter
			float i_filt = 0.996 * (last_i_filt + intensity - last_intensity);
			
			// Isum = Sum(i_filt^2)
			i_sum += (i_filt * i_filt);

			float v_shift = last_v_filt + PHCAL * (v_filt - last_v_filt);
			
			// Instantaneous Power = V * I
			p_sum += (v_shift * i_filt);

			num_samples++;
			
			last_v_filt = v_filt;
			last_i_filt = i_filt;
			
			last_voltage = voltage;
			last_intensity = intensity;
		};

		void compute_total() {
			float inv_num_samples = 1.0f / num_samples;

			v_rms = VR * sqrt(v_sum * inv_num_samples);
			i_rms = IR * sqrt(i_sum * inv_num_samples);

			p_real = max(0, VR * IR * p_sum * inv_num_samples);
			p_app = v_rms * i_rms;
			
			pf = (p_app != 0.0f) ? p_real / p_app : 0.0f;
		};

		float get_v_rms() {
			return v_rms;
		};

		float get_i_rms() {
			return i_rms;
		};

		float get_real_power() {
			return p_real;
		};

		float get_apparent_power() {
			return p_app;
		}

		float get_power_factor() {
			return pf;
		}
		
		uint32_t get_num_samples() {
			return num_samples;
		}
		
		float get_vcal() {
			return VCAL;
		}
		
		void set_vcal(float vcal) {
			VCAL = vcal;
		}
		
		float get_phcal() {
			return PHCAL;
		}
		
		void set_phcal(float phcal) {
			PHCAL = phcal;
		}
		
		float get_ical() {
			return ICAL;
		}
		
		void set_ical(float ical) {
			ICAL = ical;
		}
};
