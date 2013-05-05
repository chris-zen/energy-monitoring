import time

import numpy as np
from matplotlib import pyplot as plt
from matplotlib import animation

from signalv import lp

class PowerAnalyzer(object):
	
	def __init__(self, power, size=100):
		self._power = power
		self._size = size
		self._vrms = np.zeros(size)
		self._vrms_s = np.zeros(size)
		self._irms = np.zeros(size)
		self._p_real = np.zeros(size)
		self._p_app = np.zeros(size)
		
	def _init(self):
		self._line_vrms.set_data([], [])
		self._line_irms.set_data([], [])
		self._line_p_real.set_data([], [])
		self._line_p_app.set_data([], [])
		return self._line_vrms, self._line_irms, self._line_p_real, self._line_p_app
	
	def _animate(self, frame):
		v_rms, i_rms, p_real, p_app, pf, freq = self._power.get_power()
		
		self._vrms[0:-1] = self._vrms[1:]
		self._vrms[-1] = v_rms
		
		self._vrms_s[0:-1] = self._vrms_s[1:]
		self._vrms_s[-1] = np.mean(self._vrms)
		
		self._irms[0:-1] = self._irms[1:]
		self._irms[-1] = i_rms
		
		self._p_real[0:-1] = self._p_real[1:]
		self._p_real[-1] = p_real
		
		self._p_app[0:-1] = self._p_app[1:]
		self._p_app[-1] = p_app
		
		x = range(1, self._size + 1)
		self._line_vrms.set_data(x, self._vrms)
		self._line_irms.set_data(x, self._irms)
		self._line_p_real.set_data(x, self._p_real)
		self._line_p_app.set_data(x, self._p_app)
		
		return self._line_vrms, self._line_irms, self._line_p_real, self._line_p_app

	def run(self, interval=10, record_path=None, frames=None):
		
		self._fig = plt.figure(figsize=(11.75, 5.9))
		self._fig.suptitle("Power Analyzer")
		
		self._ax_power = self._fig.add_subplot(3, 1, 3, ylim=(0, 7200))
		self._ax_power.set_xlim(1, self._size)
		
		self._ax_vrms = self._fig.add_subplot(3, 1, 1, ylim=(230, 250), sharex=self._ax_power)
		self._ax_power.set_xlim(1, self._size)
		
		self._ax_irms = self._fig.add_subplot(3, 1, 2, ylim=(0, 30), sharex=self._ax_power)
		self._ax_power.set_xlim(1, self._size)

		self._ax_vrms.grid()
		self._ax_irms.grid()
		self._ax_power.grid()
		
		self._ax_vrms.set_ylabel("Vrms (V)")
		self._ax_irms.set_ylabel("Irms (A)")
		self._ax_power.set_ylabel("Power (W)")
		self._ax_power.set_xlabel("time (ms)")
		
		self._line_vrms, = self._ax_vrms.plot([], [], "r.-")
		self._line_irms, = self._ax_irms.plot([], [], "b.-")
		self._line_p_real, = self._ax_power.plot([], [], "m.-")
		self._line_p_app, = self._ax_power.plot([], [], "c.-")
		
		anim = animation.FuncAnimation(self._fig,
										self._animate,
										init_func=self._init,
										interval=interval,
										frames=frames,
										blit=True)

		if record_path is not None:
			print "Starting to record into {}".format(record_path)
			anim.save(record_path, codec="avi")
		
		try:
			plt.show()
		except AttributeError:
			return
		finally:
			self._power.flush()

def main():
	from power import Power

	p = Power("/dev/ttyACM0", debug=False)
	p.print_conf()

	vcc = p.get_vcc()
	print "Vcc={}".format(vcc)

	a = PowerAnalyzer(p)

	#a.run(record_path="analyzer.avi", frames=40)
	a.run()

	p.close()

if __name__ == "__main__":
	main()

