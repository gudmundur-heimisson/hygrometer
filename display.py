import re
from subprocess import Popen, PIPE
import board
import busio
from adafruit_ht16k33 import segments

i2c = busio.I2C(board.SCL, board.SDA)
seg = segments.Seg14x4(i2c)
seg.print(str_)

output_pattern = re.compile(r"millis: (?P<millis>\d+) "
                            r"temp: (?P<temp>[\d\.]+) "
                            r"humidity: (?P<humidity>[\d\.]+) "
                            r"battery: (?P<battery>[\d\.])")

proc = Popen(['/home/gummi/git/hygrometer/scanner/scanner'],
              stderr=PIPE,
              errors='utf-8')

with proc:
  while True:
    line = proc.stderr.readline()
    m = output_pattern.match(line)
    millis, temp, humidity, battery = m.groups()
    millis = int(millis)
    temp = float(temp)
    humidity = float(humidity)
    battery = float(battery)
    print(millis, temp, humidity, battery, sep="\n")
    seg.print(f"{temp:0.0f}{humidity:0.0f}")
