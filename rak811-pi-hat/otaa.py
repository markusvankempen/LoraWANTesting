#!/usr/bin/env python3
"""RAK811 OTAA demo.

Minimalistic OTAA demo

"""
from random import randint
from sys import exit
from time import sleep

from rak811 import Mode, Rak811
from ttn_secrets import APP_EUI, APP_KEY
from cayennelpp import LppFrame
import binascii

lora = Rak811()

# Most of the setup should happen only once...
print('Setup')
lora.hard_reset()
lora.mode = Mode.LoRaWan
lora.band = 'US915'
lora.set_config(app_eui=APP_EUI,
                app_key=APP_KEY)

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
lora.dr = 0

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

print('Sending packets every minute - Interrupt to cancel loop')
print('You can send downlinks')
i=1
try:
    while True:
        print('Send packet')
        print('Link counter', lora.link_cnt)
        # Cayenne lpp random value as analog
        frame = LppFrame()
        frame.add_temperature(0, i)
        print(frame)
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

except:  # noqa: E722
    pass

print('Cleaning up')
lora.close()
exit(0)
