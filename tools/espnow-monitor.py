#!/opt/homebrew/bin/python3

import sys, os, time
import logging

# HELLO <PROTOCOL_VERSION> "<USER_AGENT>"
# {
#   "eventType": "hello",
#   "protocolVersion": 1,
#   "message": "OK"
# }

# QUIT
# {
#   "eventType": "quit",
#   "message": "OK"
# }

# Invalid commands
# {
#   "eventType": "command_error",
#   "error": true,
#   "message": "Unknown command XXXX"
# }

# DESCRIBE
# {
#   "eventType": "describe",
#   "message": "ok",
#   "port_description": {
#     "protocol": "serial",
#     "configuration_parameters": {
#       "baudrate": {
#         "label": "Baudrate",
#         "type": "enum",
#         "values": [
#           "300", "600", "750", "1200", "2400", "4800", "9600",
#           "19200", "38400", "57600", "115200", "230400", "460800",
#           "500000", "921600", "1000000", "2000000"
#         ],
#         "selected": "9600"
#       },
#       "parity": {
#         "label": "Parity",
#         "type": "enum",
#         "values": [ "N", "E", "O", "M", "S" ],
#         "selected": "N"
#       },
#       "bits": {
#         "label": "Data bits",
#         "type": "enum",
#         "values": [ "5", "6", "7", "8", "9" ],
#         "selected": "8"
#       },
#       "stop_bits": {
#         "label": "Stop bits",
#         "type": "enum",
#         "values": [ "1", "1.5", "2" ],
#         "selected": "1"
#       }
#     }
#   }
# }

# CONFIGURE <PARAMETER_NAME> <VALUE>
# {
#   "eventType": "configure",
#   "message": "ok"
# }
# {
#   "eventType": "configure",
#   "error": true,
#   "message": "invalid value for parameter baudrate: 123456"
# }

# OPEN <CLIENT_TCPIP_ADDRESS> <BOARD_PORT>
# {
#   "eventType": "open",
#   "message": "ok"
# }
# {
#   "eventType": "open",
#   "error": true,
#   "message": "unknown port /dev/ttyACM23"
# }
# {
#   "eventType": "port_closed",
#   "message": "serial port disappeared!"
# }

# CLOSE
# {
#   "eventType": "close",
#   "message": "ok"
# }
# {
#   "eventType": "close",
#   "error": true,
#   "message": "port already closed"
# }




logging.basicConfig(filename=os.path.dirname(os.path.realpath(__file__))+'/pluggable.log', filemode='a+', encoding='utf-8', level=logging.DEBUG)
log = logging.getLogger('espnow-monitor')

monitor_hello = False
monitor_open = False

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
                    monitor_hello = True
                    send_msg('{"eventType": "hello", "protocolVersion": 1, "message": "OK"}')
                elif line.startswith("DESCRIBE"):
                    send_msg('{"eventType": "describe", "message": "ok", "port_description": {"protocol": "espnow", "configuration_parameters": {"baudrate": {"label": "Baudrate", "type": "enum", "values": ["9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"], "selected": "921600"}}}}')
                elif line.startswith("CONFIGURE "):
                    send_msg('{"eventType": "configure", "message": "OK"}')
                elif line.startswith("OPEN"):
                    monitor_open = True
                    send_msg('{"eventType": "open", "message": "OK"}')
                elif line.startswith("CLOSE"):
                    monitor_open = False
                    send_msg('{"eventType": "close", "message": "OK"}')
                elif line.startswith("QUIT"):
                    send_msg('{"eventType": "quit", "message": "OK"}')
                    sys.exit(0)
                else:
                    send_msg('{"eventType": "command_error", "error": true, "message": "Unknown command XXXX"}')

            time.sleep(0.1)
    except Exception as e:
        sys.exit(1)
sys.exit(0)
