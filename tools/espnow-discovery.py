#!/opt/homebrew/bin/python3

import sys, os, time
import logging

# HELLO <PROTOCOL_VERSION> "<USER_AGENT>"
# {
#   "eventType": "hello",
#   "protocolVersion": 1,
#   "message": "OK"
# }

# START
# {
#   "eventType": "start",
#   "message": "OK"
# }
# {
#   "eventType": "start",
#   "error": true,
#   "message": "Permission error"
# }

# STOP
# {
#   "eventType": "stop",
#   "message": "OK"
# }
# {
#   "eventType": "stop",
#   "error": true,
#   "message": "Resource busy"
# }

# QUIT
# {
#   "eventType": "quit",
#   "message": "OK"
# }

# LIST
# {
#   "eventType": "list",
#   "ports": [
#     {
#       "address":       <-- THE ADDRESS OF THE PORT
#       "label":         <-- HOW THE PORT IS DISPLAYED ON THE GUI
#       "protocol":      <-- THE PROTOCOL USED BY THE BOARD
#       "protocolLabel": <-- HOW THE PROTOCOL IS DISPLAYED ON THE GUI
#       "properties": {
#                        <-- A LIST OF PROPERTIES OF THE PORT
#       }
#     },
#     {
#       ...              <-- OTHER PORTS...
#     }
#   ]
# }
# {
#   "eventType": "list",
#   "error": true,
#   "message": "Resource busy"
# }

# START_SYNC
# {
#   "eventType": "start_sync",
#   "message": "OK"
# }
# {
#   "eventType": "start_sync",
#   "error": true,
#   "message": "Resource busy"
# }
# {
#   "eventType": "add",
#   "port": {
#     "address": "/dev/ttyACM0",
#     "label": "ttyACM0",
#     "properties": {
#       "pid": "0x804e",
#       "vid": "0x2341",
#       "serialNumber": "EBEABFD6514D32364E202020FF10181E",
#       "name": "ttyACM0"
#     },
#     "protocol": "serial",
#     "protocolLabel": "Serial Port (USB)"
#   }
# }
# {
#   "eventType": "remove",
#   "port": {
#     "address": "/dev/ttyACM0",
#     "protocol": "serial"
#   }
# }

# Invalid commands
# {
#   "eventType": "command_error",
#   "error": true,
#   "message": "Unknown command XXXX"
# }



logging.basicConfig(filename=os.path.dirname(os.path.realpath(__file__))+'/pluggable.log', filemode='a+', encoding='utf-8', level=logging.DEBUG)
log = logging.getLogger('espnow-discovery')

discovery_hello = False
discovery_started = False
discovery_sync = False

def send_msg(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()
    log.debug("TX: %s" % msg)

if __name__ == "__main__":
    try:
        while True:
            for line in sys.stdin:
                line = line.rstrip()
                log.debug("RX: %s" % line)
                if line.startswith("HELLO 1"):
                    discovery_hello = True
                    send_msg('{"eventType": "hello", "protocolVersion": 1, "message": "OK"}')
                elif line.startswith("LIST"):
                    send_msg('{"eventType": "list", "ports": [{"address": "aa:bb:cc:dd:ee:ff", "label": "ESP-NOW aa:bb:cc:dd:ee:ff", "protocol": "espnow", "protocolLabel": "ESP-NOW (WiFi)", "properties": {"channel": "1", "board": "esp32s2"}}]}')
                elif line.startswith("START_SYNC"):
                    discovery_sync = True
                    send_msg('{"eventType": "start_sync", "message": "OK"}')
                    # time.sleep(5)
                    send_msg('{"eventType": "add", "port": {"address": "aa:bb:cc:dd:ee:ff", "label": "ESP-NOW aa:bb:cc:dd:ee:ff", "protocol": "espnow", "protocolLabel": "ESP-NOW (WiFi)", "properties": {"channel": "1", "board": "esp32s2"}}}')
                elif line.startswith("START"):
                    discovery_started = True
                    send_msg('{"eventType": "start", "message": "OK"}')
                elif line.startswith("STOP"):
                    discovery_started = False
                    discovery_sync = False
                    send_msg('{"eventType": "stop", "message": "OK"}')
                elif line.startswith("QUIT"):
                    send_msg('{"eventType": "quit", "message": "OK"}')
                    sys.exit(0)
                else:
                    send_msg('{"eventType": "command_error", "error": true, "message": "Unknown command XXXX"}')

            time.sleep(0.1)
    except Exception as e:
        sys.exit(1)
sys.exit(0)
