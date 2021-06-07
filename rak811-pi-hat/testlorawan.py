#!/usr/bin/env python3
"""RAK811 OTAA demo.

Minimalistic OTAA demo

it send the raspberry pi temp,cpu,freemory

"""
from random import randint
from sys import exit
from time import sleep

from rak811 import Mode, Rak811
from ttn_secrets import APP_EUI, APP_KEY
from cayennelpp import LppFrame
import binascii

import os
from datetime import datetime
now = datetime.now()

print(now.strftime("%d/%m/%Y %H:%M:%S"))


# Return CPU temperature as a character string                                      
def getCPUtemperature():
    res = os.popen('vcgencmd measure_temp').readline()
    return(res.replace("temp=","").replace("'C\n",""))

# Return RAM information (unit=kb) in a list                                        
# Index 0: total RAM                                                                
# Index 1: used RAM                                                                 
# Index 2: free RAM                                                                 
def getRAMinfo():
    p = os.popen('free')
    i = 0
    while 1:
        i = i + 1
        line = p.readline()
        if i==2:
            return(line.split()[1:4])

# Return % of CPU used by user as a character string                                
def getCPUuse():
    return(str(os.popen("top -n1 | awk '/Cpu\(s\):/ {print $2}'").readline().strip(\
)))

# Return information about disk space as a list (unit included)                     
# Index 0: total disk space                                                         
# Index 1: used disk space                                                          
# Index 2: remaining disk space                                                     
# Index 3: percentage of disk used                                                  
def getDiskSpace():
    p = os.popen("df -h /")
    i = 0
    while 1:
        i = i +1
        line = p.readline()
        if i==2:
            return(line.split()[1:5])

lora = Rak811()

# Most of the setup should happen only once...
print('LoraWAN Start Version 20210605 Send and Receive')
print('Setup')
lora.hard_reset()
lora.mode = Mode.LoRaWan
lora.band = 'US915'
lora.set_config(app_eui=APP_EUI,
                app_key=APP_KEY)


print("APP_EUI = ",str(APP_EUI))
print("APP_KEY = ",str(APP_KEY))
lora.set_config(ch_mask = '0,FF00')
lora.set_config(ch_mask = '1,0000')
lora.set_config(ch_mask = '2,0000')
lora.set_config(ch_mask = '3,0000')
lora.set_config(ch_mask = '4,0000')

print('Joining')
lora.join_otaa()
# Note that DR is different from SF and depends on the region
# See: https://docs.exploratory.engineering/lora/dr_sf/
# Set Data Rate to 5 which is SF7/125kHz for EU868
lora.dr = 3

# create empty frame
frame = LppFrame()
# add some sensor data
frame.add_temperature(0, -11.2)
frame.add_humidity(0, 30.5)
#frame.add_generic(0, 1)
# get byte buffer in CayenneLPP format
buffer = frame.bytes()
print(frame)
#print(bytes.fromhex('0102{:04x}'.format(randint(0, 0x7FFF))))
#print(frame.bytes())
print("hexlify")
print(bytes.fromhex(str(binascii.hexlify(frame.bytes()).decode())))
lora.send(bytes.fromhex(str(binascii.hexlify(frame.bytes()).decode() )))
#print(str(frame.bytes()))
#lora.send(bytes.fromhex(binascii.hexlify(frame.bytes()) ), port=11, confirm=True)
print('DR', lora.dr)

print('Signal', lora.signal)

print('Send string')
#lora.send('Hello')

print('Signal', lora.signal)

print('Link counter', lora.link_cnt)


#0067fff4066845
#lora.send(bytes.fromhex('0067fff4066845'))
#print(bytes.fromhex('0067fff4066845'))

#lora.send(binascii.hexlify(frame.bytes()) )
print('wait 10sec')
sleep(10)
print('Sending packets every minute - Interrupt to cancel loop')
print('You can send downlinks')
i=1
try:
    while True:
        print('Send packet')
        print(now.strftime("%d/%m/%Y %H:%M:%S"))
# CPU informatiom
        CPU_temp = getCPUtemperature()
        CPU_usage = getCPUuse()

        print("loop counter = ",i)
        print("pi temperature = ",CPU_temp)
        print("pi cpu use =",CPU_usage)
# RAM information
# Output is in kb, here I convert it in Mb for readability
        RAM_stats = getRAMinfo()
        RAM_total = round(int(RAM_stats[0]) / 1000)
        RAM_used = round(int(RAM_stats[1]) / 1000)
        RAM_free = round(int(RAM_stats[2]) / 1000)
        print("pi free memory = ",RAM_free)

        print('Link counter', lora.link_cnt)
        # Cayenne lpp random value as analog
# PIBoinnet
#    c = CayenneLPP()
#    c.addAnalogInput(1,lora.frame_counter)
#    c.addAnalogInput(2,myl)
#    c.addTemperature(1,temp)
#    c.addAnalogOutput(1,cpu)
#    c.addAnalogOutput(1,ram)


        frame = LppFrame()
        frame.add_temperature(1, float(CPU_temp))
        frame.add_analog_input(1,i)
        frame.add_analog_input(2,i)
        print("hier")
        frame.add_analog_output(1,float(CPU_usage))
        frame.add_analog_output(2,int(RAM_free))

        print("frame ",str(frame))
        print("frame counter",lora.link_cnt)
        print(bytes.fromhex(str(binascii.hexlify(frame.bytes()).decode())))
        lora.send(bytes.fromhex(str(binascii.hexlify(frame.bytes()).decode() )))
#        lora.send(bytes.fromhex(str(binascii.hexlify(frame.bytes()).decode() )))
#        lora.send(bytes.fromhex('0102{:04x}'.format(randint(0, 0x7FFF))))
#        lora.send(binascii.hexlify(frame.bytes()) )
#        lorar.send("Hello")
        while lora.nb_downlinks:
            print('Received', lora.get_downlink()['data'].hex())

        sleep(60) 
        i=i+1
        if (i>10):
                i=0
                print('Rejoining Joining')
                lora.join_otaa()
                sleep(10)
      	

	
except OSError as err:
    print("OS error: {0}".format(err))
#except ValueError:
#    print("Could not convert data to an integer.")
    	

#except:
#    print("Unexpected error:", sys.exc_info()[0])
    
   
except Exception as e:
    print("Unexpected error:",e)

    pass

print('Cleaning up')
lora.close()
exit(0)
