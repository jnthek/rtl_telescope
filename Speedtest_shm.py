from matplotlib import animation
from multiprocessing import shared_memory
import numpy as np
import matplotlib.pyplot as plt
import sys 
from multiprocessing.resource_tracker import unregister
import struct
import time

shm_a = shared_memory.SharedMemory(name="rtlbuff00000001")

serial_no = struct.unpack('<I', shm_a.buf[0:4])[0];
cfreq = struct.unpack('<I', shm_a.buf[4:8])[0]
srate = struct.unpack('<I', shm_a.buf[8:12])[0]
tv_sec = struct.unpack('<I', shm_a.buf[12:16])[0]
tv_nsec = struct.unpack('<I', shm_a.buf[16:20])[0]
bufsize = struct.unpack('<I', shm_a.buf[20:24])[0]
blockindex = struct.unpack('<I', shm_a.buf[24:28])[0]

print (srate)

print (tv_sec)
print (tv_nsec/1e9)
print (tv_sec + (tv_nsec/1e9))

prev_time = tv_sec + (tv_nsec/1e9)

exp_tdiff = float(bufsize/2)/srate
print (exp_tdiff)

NFFT = 65536

try:
    while (True):
        data = np.array(shm_a.buf[28::2]) + np.array(shm_a.buf[28+1::2])*1j
        meas_power = np.zeros(NFFT)
        for i in range(int(len(data)/NFFT)):
            meas_power = meas_power + 10*np.log10(np.abs(np.fft.fftshift(np.fft.fft(data[i*NFFT:(i+1)*NFFT])))**2 + 1)

        tv_sec = struct.unpack('<I', shm_a.buf[12:16])[0] #np.ndarray((1), '<I', shm_a.buf[12:16])[0]
        tv_nsec = struct.unpack('<I', shm_a.buf[16:20])[0]
        curr_time = tv_sec + (tv_nsec/1e9)

        if (curr_time > (prev_time + exp_tdiff)):
            print ("Can't keep up")
            print (curr_time  - (prev_time + exp_tdiff))
        prev_time = np.copy (curr_time)

except KeyboardInterrupt:
    shm_a.close()
    unregister(shm_a._name, 'shared_memory')
    sys.exit()

shm_a.close()
unregister(shm_a._name, 'shared_memory')
sys.exit()