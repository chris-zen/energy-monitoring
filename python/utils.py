import re

__ssplitter = re.compile(r'\s+')

def ssplit(s):
	return __ssplitter.split(s)
