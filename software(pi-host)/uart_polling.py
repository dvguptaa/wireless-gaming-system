import serial, time

ser = serial.Serial('/dev/ttyAMA1', 115200, 8, serial.PARITY_NONE, 1, 1)

while (True):
    ser.write("Testing\n".encode())
    print("test")
    time.sleep(1)
