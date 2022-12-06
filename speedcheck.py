#!/usr/bin/python3

import usb.core
import usb.util
import time
import threading
import concurrent.futures
import random

TOTALSIZE = 2*1024*1024
INSIZE = 64
OUTSIZE = 32

# find our device
dev = usb.core.find(idVendor=0xcafe, idProduct=0x4010)

# was it found?
if dev is None:
    raise ValueError('Device not found')

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0,0)]

epout = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_OUT)

epin  = usb.util.find_descriptor(
    intf,
    # match the first IN endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_IN)


assert epout is not None
assert epin  is not None

start = time.perf_counter()

def read_thread():
    donein = 0
    while donein < (TOTALSIZE * 2)/INSIZE:
        donein = donein + len(epin.read(64))
    stopin = time.perf_counter()
    return stopin

def write_thread():
    doneout = 0
    while doneout < TOTALSIZE/OUTSIZE:
        doneout = doneout + epout.write(b'\xff' * random.randint(32,64))
        time.sleep(0.0001)
    stopout = time.perf_counter()
    return stopout

loop = 0
with concurrent.futures.ThreadPoolExecutor() as executor:
    while True:
        loop += 1
        start = time.perf_counter()
        readt = executor.submit(read_thread,)
        writet = executor.submit(write_thread,)
        intime = readt.result()
        outtime = writet.result()
        print("Loop %d" % loop)
        print("Device -> Host: %.2f kBytes/s" % ((TOTALSIZE * 2)/INSIZE/(intime-start)/1024))
        print("Host -> Device: %.2f kBytes/s" % (TOTALSIZE/OUTSIZE/(outtime-start)/1024))
