import serial

ser = serial.Serial('/dev/ttyAMA1', 115200, 8, serial.PARITY_NONE, 1, 1)

while (True):
    if (ser.in_waiting > 0):
        print(ser.read(ser.in_waiting).decode('utf-8', errors='ignore'), end='')
