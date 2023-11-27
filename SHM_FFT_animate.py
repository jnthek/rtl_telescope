from matplotlib import animation
from multiprocessing import shared_memory
import numpy as np
import matplotlib.pyplot as plt
import sys 
from multiprocessing.resource_tracker import unregister
import struct

shm_a = shared_memory.SharedMemory(name="rtlbuff00000001")

fig = plt.figure(figsize=(16,9))
ax = plt.axes(ylim=(-100, -50))
line, = ax.plot([], [], lw=1, color='red')
ax.grid()
ax.set_xlabel("Freq (MHz)")
ax.set_ylabel("Power (dBm)")
set_lims = True

for i in range(7):
    print (struct.unpack('<I', shm_a.buf[i*4:(i+1)*4])[0] ) #little endian is used

def animate(i, f_low, f_high, N_points):	
    global set_lims
    skipbytes = 28
    freq = np.linspace(f_low, f_high, N_points)
    data = np.array(shm_a.buf[skipbytes::2]) + np.array(shm_a.buf[skipbytes+1::2])*1j
    # meas_power, freq = plt.psd(data, Fc=97.7e6, Fs=2e6, NFFT=N_points)
    meas_power = np.zeros(N_points)
    for i in range(int(len(data)/N_points)):
        meas_power = meas_power + 10*np.log10(np.abs(np.fft.fftshift(np.fft.fft(data[i*N_points:(i+1)*N_points])))**2 + 1)
        # meas_power = meas_power + (np.abs(np.fft.fftshift(np.fft.fft(data[i*N_points:(i+1)*N_points])))**2)
    if set_lims:
        ax.set_xlim(min(freq/1e6), max(freq/1e6))
        ax.set_ylim(min(meas_power)-3, max(meas_power)+3)
        set_lims = False
    line.set_data(freq/1e6, meas_power)
    return line,

try:
    anim = animation.FuncAnimation(fig=fig, func=animate, fargs=[96.6e6, 98.6e6, 2048], frames=None, cache_frame_data=False)
    plt.show()
except KeyboardInterrupt:
    shm_a.close()
    unregister(shm_a._name, 'shared_memory')
    sys.exit()

shm_a.close()
unregister(shm_a._name, 'shared_memory')
sys.exit()