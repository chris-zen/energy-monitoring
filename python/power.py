import re

from time import sleep

from serial import Serial

from response import Response

from utils import ssplit

from serialdev import SerialDevice

class Power(SerialDevice):

	def set_freq(self, freq):
		r = self.command("Cfreq={}".format(freq))
		if r.ok:
			return int(r.data[0])
		else:
			return None
	
	def get_freq(self):
		r = self.command("Cfreq")
		if r.ok:
			return int(r.data[0])
		else:
			return None

	def set_period(self, period):
		r = self.command("Cperiod={}".format(period))
		if r.ok:
			return int(r.data[0])
		else:
			return None
	
	def get_period(self):
		r = self.command("Cperiod")
		if r.ok:
			return int(r.data[0])
		else:
			return None

	def set_trigger(self, level, timeout=None):
		if timeout is not None:
			r = self.command("Ctrigger={},{}".format(level, timeout))
		else:
			r = self.command("Ctrigger={}".format(level))
		if r.ok:
			return [int(c) for c in r.data]
		else:
			return None
	
	def get_trigger(self):
		r = self.command("Ctrigger")
		if r.ok:
			return [int(c) for c in r.data]
		else:
			return None
		
	def get_vcc(self):
		r = self.command("V")
		if r.ok:
			return int(r.data[0])
		else:
			return None
			
	def get_power(self):
		r = self.command("P")
		if r.ok:
			d = [int(v) for v in r.data]
			return (
				d[0] / 100.0,	# Vrms
				d[1] / 100.0,	# Irms
				d[2] * 1.0,		# P real
				d[3] * 1.0,		# P apparent
				d[4] / 10000.0,	# PF
				d[5])			# Sampling freq
		else:
			return None

