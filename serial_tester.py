import serial
import threading
import time
import sys
import argparse

def str_hex_to_bytes(txt):
    txt = txt.replace(' ', '')
    return bytes.fromhex(txt)

def bytes_to_hex(arr):
    return ' '.join(['{:02X}'.format(b) for b in arr])


def listener():
    last_read = 0
    need_print = False
    buffer = b''

    while True:

        if ser.in_waiting:

            data = ser.read(ser.in_waiting)
            buffer += data
            need_print = True
            last_read = time.time()
            

        if need_print and time.time() - last_read > 0.1:
            if args.hex:
                print(f"{bytes_to_hex(buffer)}")
            else:
                #print(buffer)
                print(buffer.decode())

            buffer = b''
            need_print = False

        time.sleep(0.001)


parser = argparse.ArgumentParser(description='')
parser.add_argument('-p', '--port', metavar='', required=True, help='Device to connect. Supports S0 or /dev/ttyS0')
parser.add_argument('-b', '--baudrate', metavar='', type=int, default=9600, required=False, help='')
parser.add_argument('-x', '--hex', action='store_true', required=False, help='')
parser.add_argument('-i', '--pipe', action='store_true', required=False, help='')
parser.add_argument('-s', '--bytesize', metavar='', type=int, default=8, choices=[5,6,7,8], required=False, help='')
parser.add_argument('-r', '--parity', metavar='', default="N", choices=["N","E","O","M", "S"], required=False, help='')
parser.add_argument('-o', '--stopbits', metavar='', type=float, default=1, choices=[1,1.5,2], required=False, help='')
parser.add_argument('-t', '--timeout', metavar='', type=float, default=0.1, required=False, help='')

args = parser.parse_args()


if args.bytesize == 5:
    args.bytesize = serial.FIVEBITS
elif args.bytesize == 6:
    args.bytesize = serial.SIXBITS
elif args.bytesize == 7:
    args.bytesize = serial.SEVENBITS
elif args.bytesize == 8:
    args.bytesize = serial.EIGHTBITS

if args.parity == "N":
    args.parity = serial.PARITY_NONE
elif args.parity == "E":
    args.parity = serial.PARITY_EVEN
elif args.parity == "O":
    args.parity = serial.PARITY_ODD
elif args.parity == "M":
    args.parity = serial.PARITY_MARK
elif args.parity == "S":
    args.parity = serial.PARITY_SPACE

if args.stopbits == 1:
    args.stopbits = serial.STOPBITS_ONE
elif args.stopbits == 1.5:
    args.stopbits = serial.STOPBITS_ONE_POINT_FIVE
elif args.stopbits == 2:
    args.stopbits = serial.STOPBITS_TWO

if args.port[0:4] != "/dev":
    args.port = "/dev/tty" + args.port

ser = serial.Serial(port=args.port, baudrate=args.baudrate, timeout=args.timeout,
                bytesize=args.bytesize, parity=args.parity, stopbits=args.stopbits)

#print(args)
print(f"{args.port} {args.baudrate} {args.bytesize}{args.parity}{args.stopbits} open : {ser.is_open}")

t_receive = threading.Thread(target=listener)
t_receive.start()

while True:

    try:

        if args.pipe:
            b = sys.stdin.buffer.read()
        else:
            txt = input()

            if args.hex:
                b = str_hex_to_bytes(txt)
            else:
                b = txt.encode()

        ser.write(b)

    except KeyboardInterrupt as e:
        break

ser.close()