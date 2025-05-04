import serial
import time
import sys

# --- Configuration ---
# Find the correct serial port name for your ESP32 on your MacBook.
# You can usually find this in the Arduino IDE under Tools > Port,
# or by running 'ls /dev/tty.*' in your terminal before and after plugging in the ESP32.
# Common names include /dev/tty.usbserial-XXXX or /dev/tty.usbmodemXXXX
SERIAL_PORT = "/dev/tty.usbserial-10"  # <-- !!! CHANGE THIS TO YOUR ESP32's PORT !!!
BAUD_RATE = 115200  # Must match the baud rate in your Arduino code


# --- Script ---
def send_unix_time(port, baud):
    """Gets the current Unix time and sends it over the serial port."""
    try:
        # Open the serial port
        ser = serial.Serial(port, baud, timeout=1)
        print(f"Serial port {port} opened successfully.")
        time.sleep(2)  # Give the serial connection a moment to initialize

        # Get the current Unix time (seconds since epoch)
        current_unix_time = int(time.time())
        time_string = str(current_unix_time) + "\n"  # Add newline as a delimiter

        print(f"Sending Unix time: {current_unix_time}")

        # Send the time over the serial port
        ser.write(time_string.encode())  # Encode string to bytes

        print("Time sent.")

        # Close the serial port
        ser.close()
        print("Serial port closed.")

    except serial.SerialException as e:
        print(f"Error opening or communicating with serial port {port}: {e}")
        print("Please check the port name and ensure the ESP32 is connected.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


if __name__ == "__main__":
    # Check if the user changed the default port name
    if SERIAL_PORT == "/dev/cu.usbmodemXXXX":
        print("!!! IMPORTANT: Please change the SERIAL_PORT variable in the script !!!")
        print("    Find the correct port name for your ESP32 and update the script.")
        sys.exit(1)  # Exit if the default port is still used

    send_unix_time(SERIAL_PORT, BAUD_RATE)
