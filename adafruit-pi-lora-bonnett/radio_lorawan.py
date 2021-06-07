"""
Example for using the RFM9x Radio with Raspberry Pi and LoRaWAN
"Learn Guide: https://learn.adafruit.com/lora-and-lorawan-for-raspberry-pi

Mod: mvk@ca.ibm.com

this program gets  the temp,cpu,mem infos and sends them via LoraWAN

"""
import threading
import time
import subprocess
import busio
from digitalio import DigitalInOut, Direction, Pull
import board
from cayennelpp import CayenneLPP
import os
from datetime import datetime
now = datetime.now()

print(now.strftime("%d/%m/%Y %H:%M:%S"))

# Import thte SSD1306 module.
import adafruit_ssd1306
# Import Adafruit TinyLoRa
from adafruit_tinylora.adafruit_tinylora import TTN, TinyLoRa

# Button A
btnA = DigitalInOut(board.D5)
btnA.direction = Direction.INPUT
btnA.pull = Pull.UP

# Button B
btnB = DigitalInOut(board.D6)
btnB.direction = Direction.INPUT
btnB.pull = Pull.UP

# Button C
btnC = DigitalInOut(board.D12)
btnC.direction = Direction.INPUT
btnC.pull = Pull.UP

# Create the I2C interface.
i2c = busio.I2C(board.SCL, board.SDA)

# 128x32 OLED Display
reset_pin = DigitalInOut(board.D4)
display = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c, reset=reset_pin)
# Clear the display.
display.fill(0)
display.show()
width = display.width
height = display.height

# TinyLoRa Configuration
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
cs = DigitalInOut(board.CE1)
irq = DigitalInOut(board.D22)
rst = DigitalInOut(board.D25)

# TTN Device Address, 4 Bytes, MSB
devaddr = bytearray([ 0x26, 0x01, 0x35, 0x2A ])
# TTN Network Key, 16 Bytes, MSB
nwkey = bytearray([ 0x1C, 0x44, 0x12, 0x61, 0xD3, 0xB7, 0xF6, 0xE1, 0x67, 0x4D, 0x32, 0xAF, 0x9C, 0x19, 0x7B, 0x72 ])
# TTN Application Key, 16 Bytess, MSB
app = bytearray([ 0xFF, 0xFB, 0x76, 0x66, 0x10, 0x36, 0x7E, 0x3A, 0x9B, 0x30, 0x72, 0xB6, 0x85, 0xF9, 0x30, 0x46 ])


# Initialize ThingsNetwork configuration
ttn_config = TTN(devaddr, nwkey, app, country='US')
# Initialize lora object
lora = TinyLoRa(spi, cs, irq, rst, ttn_config)
# 2b array to store sensor data
data_pkt = bytearray(2)
# time to delay periodic packet sends (in seconds)
data_pkt_delay = 300.0
myl=0


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

def send_pi_data_periodic():
    threading.Timer(data_pkt_delay, send_pi_data_periodic).start()
    print("Sending periodic data...")
    # read the raspberry pi cpu load
    cmd = "top -bn1 | grep load | awk '{printf \"%.1f\", $(NF-2)}'"
    CPU = subprocess.check_output(cmd, shell = True )
    CPU = float(CPU)
#    print(CPU)
    send_analogdata(CPU)
    print('CPU:', CPU)

def send_analogdata(data):
    print("Send Data =",data)
    lora.frame_counter=lora.frame_counter+1
    c = CayenneLPP()
    c.addAnalogInput(1,lora.frame_counter)
    c.addAnalogInput(2,myl)
    c.addAnalogOutput(1,data)
    
    #c.addTemperature(1, 23.5) # Add temperature read to channel 1  
#    c.addAnalogInput(1,lora.frame_counter)
#c.addTemperature(2, 22.7) # Add another temperature read to channel 2
    #c.addRelativeHumidity(3, 88.5) # Add relative humidity read to channel 3
    frame = c.getBuffer() # Get bytes

    #Send data packet
    lora.send_data(frame, len(frame), lora.frame_counter)

    print(lora.frame_counter) 
    display.fill(0)
    display.text('Sent LPP Data to Lora!', 0,0, 1)
    display.text('Sending '+str(data)+" / "+str(lora.frame_counter),0, 15,1)
    print('Data sent!')
    display.show()
    time.sleep(10)
    display.fill(0)
	
#send pi,temp,cpu and counters
def send_all_pi_data(temp,cpu,ram,myl):
    print("Send all Pi Data =",str(temp))
    lora.frame_counter=lora.frame_counter+1

    c = CayenneLPP()
    c.addAnalogInput(1,lora.frame_counter)
    c.addAnalogInput(2,myl)
    c.addTemperature(1,temp)
    c.addAnalogOutput(1,cpu)
    c.addAnalogOutput(1,ram)

    frame = c.getBuffer() # Get bytes

    #Send data packet
    lora.send_data(frame, len(frame), lora.frame_counter)

    print("lora frame counter",lora.frame_counter) 
    display.fill(0)
    display.text('Sent LPP Data to Lora!', 0,0, 1)
    display.text('temp,cpu,ram t='+str(temp),0, 10,1)
    display.text('lora frame = '+str(lora.frame_counter),0, 20,1)
    print('Data All Pi sent!')
    display.show()
    time.sleep(10)
    


def send_pi_data(data):

    print("pi data")
    print(data)
    # Encode float as int
    data = int(data * 100)
    lora.frame_counter +=1
    data=lora.frame_counter
    print(data)
    # Encode payload as bytes
    data_pkt[0] = (data >> 8) & 0xff
    data_pkt[1] = data & 0xff
    # Send data packet
    lora.send_data(data_pkt, len(data_pkt), lora.frame_counter)
#    lora.frame_counter += 1
    print(lora.frame_counter) 
    display.fill(0)
    display.text('Sent Data to TTN!', 15, 15, 0)
    display.text('Sending'+str(data), 15, 30, 0)
    print('Data sent!')
    display.show()
    #time.sleep(60)")
    print(data)
    # Encode float as int
    data = int(data * 100)
    lora.frame_counter +=1
    data=lora.frame_counter
    print(data)
    # Encode payload as bytes
    data_pkt[0] = (data >> 8) & 0xff
    data_pkt[1] = data & 0xff
    # Send data packet
    lora.send_data(data_pkt, len(data_pkt), lora.frame_counter)
#    lora.frame_counter += 1
    print(lora.frame_counter) 
    display.fill(0)
    display.text('Sent Data to TTN!', 0, 0, 1)
    display.text('Sending'+str(data), 0, 15, 1)
    print('Data sent!')
    display.show()
    #time.sleep(60)


display.fill(0)
VERSION="20210604"
print("Start RasPI LoraWAN")
print(VERSION);
display.text('Start RasPi LoRaWAN', 0, 0, 1)
display.text('Version:'+VERSION, 0, 15, 1)
display.show()
time.sleep(2)

# CPU informatiom
CPU_temp = getCPUtemperature()
CPU_usage = getCPUuse()

print("loop counter = ",myl)
print("pi temperature = ",CPU_temp)
print("pi cpu use =",CPU_usage)
# RAM information
# Output is in kb, here I convert it in Mb for readability
RAM_stats = getRAMinfo()
RAM_total = round(int(RAM_stats[0]) / 1000,1)
RAM_used = round(int(RAM_stats[1]) / 1000,1)
RAM_free = round(int(RAM_stats[2]) / 1000,1)
#send_pi_data_periodic()
print("send 1 message")
send_all_pi_data(float(CPU_temp),float(CPU_usage),float(RAM_free),myl)

while True:
    now = datetime.now()
    print("------------------------------")
    print(now.strftime("%d/%m/%Y %H:%M:%S"))
    packet = None
    myl=myl+1
    # draw a box to clear the image

# CPU informatiom
    CPU_temp = getCPUtemperature()
    CPU_usage = getCPUuse()

    print("loop counter = ",myl)
    print("pi temperature = ",CPU_temp)
    print("pi cpu use =",CPU_usage)
# RAM information
# Output is in kb, here I convert it in Mb for readability
    RAM_stats = getRAMinfo()
    RAM_total = round(int(RAM_stats[0]) / 1000,1)
    RAM_used = round(int(RAM_stats[1]) / 1000,1)
    RAM_free = round(int(RAM_stats[2]) / 1000,1)
    print("pi free memory = ",RAM_free)
    display.fill(0)
    display.text(''+now.strftime("%d/%m/%Y %H:%M:%S"), 0, 0, 1)
    display.text('Loop/Lora msg='+str(myl),0,10,1)
    display.text('T/CPU/MEM='+str(CPU_temp)+"/"+str(CPU_usage)+"/"+str(RAM_free),0,20,1)
    display.show()
    time.sleep(10)
    #print("lora")
    # read the raspberry pi cpu load
    #cmd = "top -bn1 | grep load | awk '{printf \"%.1f\", $(NF-2)}'"
    #CPU = subprocess.check_output(cmd, shell = True )
    #CPU = float(CPU)
    #print(CPU)
    
    if not btnA.value:
        # Send Packet
        print("btna")
        send_pi_data(CPU)
    if not btnB.value:
        # Display CPU Load
        display.fill(0)
        display.text('CPU Load %', 45, 0, 1)
        display.text(str(CPU), 60, 15, 1)
        display.show()
        time.sleep(0.1)
    if not btnC.value:
        display.fill(0)
        display.text('* Periodic Mode *', 15, 0, 1)
        display.show()
        time.sleep(0.5)
        send_pi_data_periodic()
    if myl > 1000:
        myl=0
        print("Restarting...")
        display.fill(0)
        display.text('* Restarting *', 15, 0, 1)
        display.show()
        time.sleep(10)
        #os.system("python3 radio_lorawan.py")
    #send_pi_data_periodic()
    #display.text('T/CPU/MEM= '+str(CPU_temp)+"/"+str(CPU_usage)+"/"+str(RAM_free),0,20,1)
    if myl % 2:
        print("Send message every 30*10 sec")
        send_all_pi_data(float(CPU_temp),float(CPU_usage),float(RAM_free),myl)
   
    #time.sleep(.100)
    #send_pi_data(1)

    time.sleep(30)
