import numpy as np

def normalize(x, center=512, max_val=1023):
	w = np.abs(x - center).max() * 2
	xn = ((x - center) * max_val / w) + center
	return xn

def ltrim(x, level=512):
	last_x = x[0]
	i = 1
	while i < len(x) and not (last_x <= level and x[i] >= level):
		last_x = x[i]
		i += 1
	start = i
	return start

def rtrim(x, start=0, level=512):
	last_x = x[start]
	end = start
	cicles = 0
	for i in xrange(start + 1, x.size):
		if last_x <= level and x[i] > level:
			end = i
			cicles += 1
		last_x = x[i]
	return end, cicles
	
def trim(x):
	"""
	Get start and end positions so x[start:pos+1] have complete signal cicles
	"""
	
	start = ltrim(x)
	
	end, cicles = rtrim(x, start)
	
	return start, end, cicles

def lp(x, alpha, last=None):
	"""
	Simple low pass filter
	"""
	
	y = np.empty_like(x)
	if len(x) > 0:
		y[0] = x[0] if last is None else last
		for i in xrange(1, x.size):
			y[i] = y[i-1] + alpha * (x[i] - y[i-1])
	return y

def hp(x, alpha, last=None):
	"""
	Simple high pass filter
	"""
	
	y = np.empty_like(x)
	if len(x) > 0:
		y[0] = x[0] if last is None else last
		for i in xrange(1, x.size):
			y[i] = alpha * (y[i-1] + x[i] - x[i-1])
	return y
