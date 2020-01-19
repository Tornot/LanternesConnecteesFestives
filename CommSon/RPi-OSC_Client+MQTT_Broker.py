

# Import needed modules from osc4py3
from osc4py3.as_eventloop import *
from osc4py3 import oscmethod as osm
import paho.mqtt.publish as publish

sound_bass = 0
sound_snare = 0

def send_in_mqtt(topic, value):
    print("Test mqtt msg sending...")
    # publish.single(topic, payload=value, hostname="10.42.0.1", auth=None, protocol=mqtt.MQTTv311, transport = "udp")
    publish.single(topic, value, hostname="10.42.0.1")
    print("Message sent in MQTT")


def send_bass():
    # print("Test Bass send")
    send_in_mqtt("sound/bass", sound_bass)


def send_snare():
    send_in_mqtt("sound/snare", sound_snare)


def handlerfunction_bass(address, s):
    global sound_bass, sound_snare  # To write in the global variable
    # Will receive message address, and message data flattened in s, x, y
    print(address)
    sound_bass = round(s*1270)
    if sound_bass>127: sound_bass = 127
    print("bass : ", sound_bass)
    send_bass()
    pass


def handlerfunction_snare(address, s):
    global sound_snare  # To write in the global variable
    # Will receive message address, and message data flattened in s, x, y
    print(address)
    sound_snare = round(s*1270)
    if sound_snare>127: sound_snare = 127
    print("snare : ", sound_snare)
    send_snare()
    pass

# Start the system.
osc_startup()

# Make server channels to receive packets.
osc_udp_server("192.100.0.2", 9001, "aservername")

# Too, but request the message address pattern before in argscheme
osc_method("/sound/bass", handlerfunction_bass, argscheme=osm.OSCARG_ADDRESS + osm.OSCARG_DATAUNPACK)
osc_method("/sound/snare", handlerfunction_snare, argscheme=osm.OSCARG_ADDRESS + osm.OSCARG_DATAUNPACK)

# Periodically call osc4py3 processing method in your event loop.
finished = False
while not finished:

    osc_process()


# Properly close the system.
osc_terminate()

