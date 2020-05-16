from flask import Flask
import board
import busio
from adafruit_ht16k33 import segments

i2c = busio.I2C(board.SCL, board.SDA)
seg = segments.Seg14x4(i2c)
app = Flask(__name__)

@app.route("/set-led/<float:value1>/<float:value2>")
def set_led(value1, value2):
  str_ = '{0:<2.0f}{1:>2.0f}'.format(value1, value2)
  seg.print(str_)
  return "Set LED to {0}".format(str_)
