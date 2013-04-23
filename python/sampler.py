import re
import numpy as np

from time import sleep

from serial import Serial

__ssplitter = re.compile(r'\s+')
def ssplit(s):
	return __ssplitter.split(s)

class Response(object):
	def __init__(self, line):
		line = line.rstrip()
		fields = ssplit(line)
		self.ok = (len(fields) > 0 and fields[0] == "OK")
		if self.ok:
			self.data = fields[1:]
			self.msg = ""
		else:
			self.data = []
			pos = line.find(" ")
			if pos >= 0 and pos + 1 < len(line):
				self.msg = line[pos + 1:]
			else:
				self.msg = ""
	
	def __repr__(self):
		if self.ok:
			sb = ["OK"]
			sb += self.data
		else:
			sb = ["ERROR"]
			sb += [self.msg]
		return " ".join(sb)

class Sampler(object):
	def __init__(self, dev="/dev/ttyACM0", baud=115200, debug=False):
		self._s = Serial(dev, baud)
		self._dev = dev
		self._baud = baud
		self._debug = debug
		sleep(1)
		self.flush()
		sleep(1)

	def write(self, data):
		if self._debug:
			print ":", data
		self._s.write(data)
	
	def readline(self):
		line = self._s.readline()
		if self._debug:
			print ">", line.rstrip()
		return line

	def read_response(self):
		r = Response(self.readline())
		return r
			
	def command(self, cmd):
		self.write(cmd + ";")
		return self.read_response()

	def print_conf(self):
		r = self.command("C")
		if r.ok:
			for i in xrange(int(r.data[0])):
				print self._s.readline().rstrip()

	def set_size(self, size):
		r = self.command("Csize={}".format(size))
		if r.ok:
			return int(r.data[0])
		else:
			return None
	
	def get_size(self):
		r = self.command("Csize")
		if r.ok:
			return int(r.data[0])
		else:
			return None
	
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

	def set_channels(self, channels):
		r = self.command("Cchannels={}".format(",".join([str(c) for c in channels])))
		if r.ok:
			return [int(c) for c in r.data[0].split(",")]
		else:
			return None
	
	def get_channels(self):
		r = self.command("Cchannels")
		if r.ok:
			return [int(c) for c in r.data[0].split(",")]
		else:
			return None

	def set_trigger_enabled(self, enabled):
		r = self.command("Ctrigger_enabled={}".format("1" if enabled else "0"))
		if r.ok:
			return r.data[0] == "1"
		else:
			return None
	
	def is_trigger_enabled(self):
		r = self.command("Ctrigger_enabled")
		if r.ok:
			return r.data[0] == "1"
		else:
			return None

	def set_trigger(self, channel, level):
		r = self.command("Ctrigger={},{}".format(channel, level))
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
	
	def set_trigger_timeout(self, timeout):
		r = self.command("Ctrigger_timeout={}".format(timeout))
		if r.ok:
			return int(r.data[0])
		else:
			return None
	
	def get_trigger_timeout(self):
		r = self.command("Ctrigger_timeout")
		if r.ok:
			return int(r.data[0])
		else:
			return None
			
	def sample(self):
		r = self.command("S")
		if r.ok:
			return tuple([int(v) for v in r.data])
		else:
			return None

	def read(self, sample=False, dtype=np.int, transform=None):
		if sample:
			r = self.command("SR")
		else:
			r = self.command("R")
		
		if r.ok:
			if sample:
				size, num_chan, freq, period = [int(v) for v in r.data]
			else:
				size, num_chan = [int(v) for v in r.data]
			
			data = np.empty((num_chan, size), dtype=dtype)
			for si in xrange(size):
				sample = [dtype(v) for v in ssplit(self._s.readline().rstrip())]
				if transform is not None:
					sample = [transform(x) for x in sample]
				data[:, si] = sample
			
			return data
		else:
			return None
	
	def get_vcc(self):
		r = self.command("V")
		if r.ok:
			return int(r.data[0])
		else:
			return None
			
	def open(self, dev=None, baud=None):
		if dev is None and baud is None:
			self._s.open()
		else:
			if dev is None:
				dev = self._dev
			if baud is None:
				baud = self._baud
			self._s = Serial(dev, baud)
	
	def flush(self):
		 while self._s.inWaiting():
		 	self._s.read()
	
	def close(self):
		self._s.close()

