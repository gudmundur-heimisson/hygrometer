from flask import Flask

app = Flask(__name__)

@app.route("/set-led/<float:value>")
def set_led(value):
  print(value);
  return str(value)

