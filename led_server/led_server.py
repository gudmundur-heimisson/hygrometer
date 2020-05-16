from flask import Flask

app = Flask(__name__)

@app.route("/set-led/<float:value1>/<float:value2>")
def set_led(value1, value2):
  print(value1, value2);
  return str(value1) + " " + str(value2)
