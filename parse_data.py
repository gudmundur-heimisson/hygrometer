import sys
import struct
import io
import re
from subprocess import Popen, PIPE

hex_pattern = re.compile(r"^.*?\'\: (.*?)\s*$")

def hex_list_to_bytes(hex_list):
  return bytes([int.from_bytes(bytes.fromhex(b), 'big') 
                  for b in hex_list])

def hex_list_to_float(hex_list):
  return struct.unpack('f', hex_list_to_bytes(hex_list))[0]

def hex_list_to_int(hex_list):
  return struct.unpack('i', hex_list_to_bytes(hex_list))[0]

def process_adv_hex(adv_hex_list):
  millis = hex_list_to_int(adv_hex_list[:4])
  temp = hex_list_to_float(adv_hex_list[4:8])
  humidity = hex_list_to_float(adv_hex_list[8:12])
  battery = hex_list_to_float(adv_hex_list[12:16])
  return millis, temp, humidity, battery

proc = Popen(['sudo',
              '/home/gummi/git/gattlib/build/examples/advertisement_data/advertisement_data'],
              stderr=PIPE,
              errors='utf-8')

while True:
  proc.stderr.flush()
  line = proc.stderr.readline()
  if 'Gummi' not in line:
    continue
  match = hex_pattern.match(line)
  adv_hex = match.groups()[0].split(' ')
  millis, temp, humidity, battery = process_adv_hex(adv_hex)
  print("{:d} ms".format(millis))
  print("{:.2f}°F".format(1.8 * temp + 32))
  print("{:.2f}%".format(humidity))
  print("{:.2f}%".format(battery))
  print()
