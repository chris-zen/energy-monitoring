from time import sleep

from serial import Serial

from response import Response

from utils import ssplit

class SerialDevice(object):
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

