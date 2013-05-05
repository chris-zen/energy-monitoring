import time

import numpy as np
from matplotlib import pyplot as plt
from matplotlib import animation
from signalv import rtrim, hp

class PowerScope(object):

	CHANNEL_COLOR = ["b", "r", "y", "g"] #TODO add more colors
	
	def __init__(self, sampler):
		self._sampler = sampler
		self._last_vf = 512.0
		self._last_if = 512.0
		self.VCAL = 185
		self.ICAL = 61.5
		
	def _init(self):
		self._label.set_text("")
		self._line_v.set_data([], [])
		self._line_i.set_data([], [])
		self._line_vf.set_data([], [])
		self._line_if.set_data([], [])
		return self._label, self._line_v, self._line_i, self._line_vf, self._line_if
	
	def _animate(self, frame):
		V, I, Vf, If, Vrms, Irms, Pr, Pa, PF = self.sample()
		
		self._label.set_text("Vrms={:.2f}, Irms={: 2.2f}, Pr={: 4.2f}, Pa={: 4.2f}, PF={:.3f}, VCAL={:.2f}, ICAL={:.2f}       ".format(Vrms, Irms, Pr, Pa, PF, self.VCAL, self.ICAL))
		
		self.calibrate(Vrms, Irms, Pr, Pa, PF)
		
		x = np.arange(0, V.size * self._period_ms, self._period_ms)
		self._line_v.set_data(x, V)
		self._line_i.set_data(x, I)
		self._line_vf.set_data(x, Vf)
		self._line_if.set_data(x, If)
		
		return self._label, self._line_v, self._line_i, self._line_vf, self._line_if

	def _update_ratios(self):
		self.VR = self.VCAL * self.step_vcc
		self.IR = self.ICAL * self.step_vcc
	
	def _update_vcc(self):
		self._vcc = self._sampler.get_vcc()
		self.step_vcc = self._vcc / (1000.0 * 1023.0)
		self._update_ratios()
		
	def sample(self):

		self._update_vcc()
		
		d = self._sampler.read(sample=True)
		
		V = d[1]
		I = d[0]

		start = 0
		end, cicles = rtrim(V)
		Vt = V[start:end]
		It = I[start:end]

		#Vf = Vt * 1.0
		#If = It * 1.0
		
		Vf = hp(Vt * 1.0, 0.996, last=self._last_vf)
		If = hp(It * 1.0, 0.996, last=self._last_if)
		
		self._last_vf = Vf[-1]
		self._last_if = If[-1]

		Vsq = Vf * Vf
		Vsum = Vsq.sum()

		Isq = If * If
		Isum = Isq.sum()

		Pi = Vf * If
		Pisum = Pi.sum()

		Vrms = self.VR * np.sqrt(Vsum / Vsq.size)
		Irms = self.IR * np.sqrt(Isum / Isq.size)

		Pr = self.VR * self.IR * Pisum / Pi.size
		Pa = Vrms * Irms
		
		Pr = max(0, Pr)
		
		if Pa != 0.0:
			PF = Pr / Pa
		else:
			PF = 0.0
		
		return Vt, It, Vf, If, Vrms, Irms, Pr, Pa, PF

	def calibrate(self, Vrms, Irms, Pr, Pa, PF):
		pass
		"""
		if Pr > 1015:
			self.ICAL = 0.9999 * self.ICAL
		elif Irms < 1015:
			self.ICAL = 1.0001 * self.ICAL
		"""
		"""
		if Vrms > 240.5:
			self.VCAL = 0.99999 * self.VCAL
		elif Vrms < 240.5:
			self.VCAL = 1.00001 * self.VCAL
		"""
			
	def run(self, interval=10, record_path=None, frames=None):
		
		self._size = 600
		self._freq = self._sampler.get_freq()
		self._period = self._sampler.get_period()
		self._vcc = self._sampler.get_vcc()
		
		self._period_ms = self._period / 1000.0

		self._fig = plt.figure(figsize=(11.75, 5.9))
		self._ax = plt.axes(xlim=(0, self._size * self._period_ms), ylim=(-512, 1023))
		self._ax.grid()
		self._fig.add_axes(self._ax)
		
		plt.title("Power Scope")
		plt.xlabel("time (ms)")
		plt.ylabel("value (ADC steps)")
		
		self._label = self._ax.text(0.01, 0.96, "",
									family="monospace", fontname="Courier", weight="bold",
									transform=self._ax.transAxes,
									bbox=dict(boxstyle="round", fc="0.9"))
									
		self._line_v, = self._ax.plot([], [], "r.-")
		self._line_i, = self._ax.plot([], [], "b.-")
		self._line_vf, = self._ax.plot([], [], "m-")
		self._line_if, = self._ax.plot([], [], "c-")
		
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
			self._sampler.flush()

def main():
	from sampler import Sampler

	s = Sampler("/dev/ttyACM0", debug=True)
	s.set_freq(4000)
	s.set_trigger(1, 490)
	s.print_conf()

	vcc = s.get_vcc()
	print "Vcc={}".format(vcc)

	scope = PowerScope(s)

	#scope.run(record_path="scope.avi", frames=40)
	scope.run()

	s.close()

if __name__ == "__main__":
	main()

