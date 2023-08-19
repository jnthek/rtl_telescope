import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation
from pylab import *
import serial
from time import sleep

def get_rbuff_spec():
    bufmap = np.memmap("/dev/shm/rtlbuff00000001", dtype=np.int8, mode='r')
    NFFT = 4096*2
    bufspec = np.zeros(int(NFFT/2), dtype=np.float64)
    for i in range(1):
        buf1 = bufmap[i*NFFT:(i+1)*NFFT]
        buf_complex = buf1[0::2] + buf1[1::2]*1j
        bufspec = bufspec+np.abs(np.fft.fftshift(np.fft.fft(buf_complex)))
    return bufspec

fig = plt.figure(figsize=(16,9))
ax = plt.axes(ylim=(-100, -50))
line, = ax.plot([], [], lw=1, color='red')
ax.grid()
set_lims = True

def animate(i):	
	global set_lims
	meas_power = get_rbuff_spec()
	if set_lims:
		ax.set_xlim(0, 4096)
		ax.set_ylim(min(meas_power)-10, max(meas_power)+10)
		set_lims = False
	line.set_data(np.arange(0,4096), meas_power)
	return line,
	
anim = animation.FuncAnimation(fig=fig, func=animate, frames=None, cache_frame_data=False)
plt.show()