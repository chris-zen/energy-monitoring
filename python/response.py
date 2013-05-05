
from utils import ssplit

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

