#!/bin/bash
# Usage SCRIPTS/SERIAL_SAVER.sh <port> <output_file>
# Example: SCRIPTS/SERIAL_SAVER.sh 2 output.txt

PORT=$1
OUTPUT_FILE=$2
# Check if the port is valid
if [[ ! $PORT =~ ^[0-9]+$ ]]; then
    echo "Error: Port must be a number."
    exit 1
fi
cp OUTPUT/SERIAL/SERIAL${PORT}.data $OUTPUT_FILE
echo "Serial data from port ${PORT} saved to ${OUTPUT_FILE}."
echo "Clear the serial data file for port ${PORT}? (y/n)"
read -r answer
if [[ $answer == "y" ]]; then
    > OUTPUT/SERIAL/SERIAL${PORT}.data
    echo "Serial data file for port ${PORT} cleared."
else
    echo "Serial data file for port ${PORT} not cleared."
fi