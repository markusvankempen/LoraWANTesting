#
# FiFY connected wtih Band 4 (Bell Canada) etc)
# apn=super for the SuperSim from Twillo
#
# mvk@ca.ibm.com
# markus@vankempen.org
#
# 2023-apr-23
#

from network import LTE
import time
import socket

lte = LTE()
#some carriers have special requirements, check print(lte.send_at_cmd("AT+SQNCTM=?")) to see if your carrier is listed.
#when using verizon, use
#lte.init(carrier=verizon)
#when usint AT&T use,
#lte.init(carrier=at&t)
print(lte.send_at_cmd("AT+SQNCTM=?"))
#some carriers do not require an APN
#also, check the band settings with your carrier

lte = LTE()
print("----AT+CGMI----")
#lte.send_at_cmd('AT+CFUN=0')
#lte.send_at_cmd('AT!="clearscanconfig"')
#lte.send_at_cmd('AT!="addscanfreq band=28 dl-earfcn=9410"')
#lte.send_at_cmd('AT+CGDCONT=1, "IP", "telstra.internet"')
#lte.send_at_cmd('AT+CEMODE=0')
#lte.send_at_cmd('AT+CEREG=2')
#lte.send_at_cmd('AT+CFUN=1')
lte.send_at_cmd('AT+CGMI')
###

lte.attach(band=4, apn="super") #set band and APN
print("attaching..",end='')
while not lte.isattached():
    time.sleep(5)

    print('.',end='')
    print(lte.send_at_cmd('AT!="fsm"'))         # get the System FSM
print("attached!")

lte.connect()
print("connecting [##",end='')
while not lte.isconnected():
    time.sleep(0.25)
    print('#',end='')
    print(lte.send_at_cmd('AT!="showphy"'))
    print(lte.send_at_cmd('AT!="fsm"'))
print("] connected!")

print(socket.getaddrinfo('pycom.io', 80))
print('Disconnect')
lte.deinit()
sleeptime=30000
print('DeepSleep for (milli sec) :',sleeptime)
machine.deepsleep(sleeptime) #sleep for 1 minute
print('Normal Seelp for 60sec')
time.sleep(60)
