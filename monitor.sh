device=${1:-/dev/ttyACM0}
baud=${2:-115200}

python3 -m serial.tools.miniterm --filter=direct $device $baud