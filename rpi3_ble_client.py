import asyncio
from bleak import BleakScanner, BleakClient
import sys
import os
import stat
import errno
import time  # For potential small delays if needed

# --- Configuration - MUST match your ESP32 sketch ---
TARGET_DEVICE_NAME = "ESP32_Joystick_Server"
SERVICE_UUID = "90920b02-5bb3-4d6d-8322-d744ccf56a04"
CHARACTERISTIC_UUID_X = "c7db9d15-3b2c-4e76-b7c1-57e6178dfb6c"
CHARACTERISTIC_UUID_Y = "d43bfa36-280a-495c-9e10-99f5d1bdc43e"
CHARACTERISTIC_UUID_BTN = "0bc7ad76-3ffd-4da4-8e3e-09613eddf3c4"
# ----------------------------------------------------

# --- FIFO Configuration ---
# Path for the named pipe (FIFO)
# Using /tmp is common for temporary IPC files
FIFO_PATH = "/tmp/joystick_fifo"
# -------------------------

# Global dictionary to store the latest values
# Initialize with default values to ensure we always write 3 values
joystick_data = {
    "X": 0,
    "Y": 0,
    "Button": 1,  # Assuming 1 is the default 'not pressed' state
}

# FIFO file descriptor - will be opened later
fifo_out = None

# Flag to indicate if FIFO is ready for writing
fifo_ready = False


# --- Notification Handler ---
def notification_handler(characteristic_uuid: str, data: bytearray):
    """Handles incoming BLE notifications, updates state, and writes to FIFO."""
    global fifo_out, fifo_ready, joystick_data
    decoded_data = data.decode("utf-8")

    data_changed = False
    try:
        value = int(decoded_data)  # Convert received string to integer
        if characteristic_uuid == CHARACTERISTIC_UUID_X:
            if joystick_data["X"] != value:
                joystick_data["X"] = value
                data_changed = True
                # print(f"X: {value}") # Optional: print to console
        elif characteristic_uuid == CHARACTERISTIC_UUID_Y:
            if joystick_data["Y"] != value:
                joystick_data["Y"] = value
                data_changed = True
                # print(f"Y: {value}") # Optional: print to console
        elif characteristic_uuid == CHARACTERISTIC_UUID_BTN:
            if joystick_data["Button"] != value:
                joystick_data["Button"] = value
                data_changed = True
                # button_state = "Pressed" if value == 0 else "Not Pressed" # Optional
                # print(f"Button: {button_state} ({value})") # Optional: print to console
        else:
            print(
                f"Unknown Characteristic: {characteristic_uuid}, Data: {decoded_data}"
            )

        # If data changed and FIFO is ready, write the current state
        if data_changed and fifo_ready and fifo_out:
            try:
                # Format: X Y Button\n
                output_string = f"{joystick_data['X']} {joystick_data['Y']} {joystick_data['Button']}\n"
                fifo_out.write(output_string)
                fifo_out.flush()  # Ensure data is written immediately [4]
                # print(f"Wrote to FIFO: {output_string.strip()}") # Debug print
            except BrokenPipeError:
                print("FIFO Error: Broken pipe. Reader might have closed.")
                # Handle broken pipe - maybe try reopening later or stop?
                # For now, just print and continue trying
                fifo_ready = False  # Mark as not ready until potentially reopened
            except OSError as e:
                print(f"FIFO Write Error: {e}")
                # Consider marking fifo_ready = False
            except Exception as e:
                print(f"Error writing to FIFO: {e}")

    except ValueError:
        print(
            f"Error: Could not convert data '{decoded_data}' to integer for UUID {characteristic_uuid}"
        )
    except Exception as e:
        print(f"Error processing notification: {e}")


# --- Main Program Logic ---
async def main():
    global fifo_out, fifo_ready

    # --- Create FIFO ---
    try:
        # Create the FIFO file with read/write permissions for owner/group/others
        os.mkfifo(FIFO_PATH, 0o666)  # [3] [4] [6]
        print(f"Created FIFO at {FIFO_PATH}")
    except OSError as oe:
        if oe.errno == errno.EEXIST:
            # Check if it's actually a FIFO file
            if not stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
                print(f"Error: {FIFO_PATH} exists but is not a FIFO. Please remove it.")
                return
            else:
                print(f"FIFO {FIFO_PATH} already exists.")
        else:
            print(f"Error creating FIFO: {oe}")
            return
    except Exception as e:
        print(f"Unexpected error setting up FIFO: {e}")
        return

    # --- Scan and Connect ---
    print(f"Scanning for '{TARGET_DEVICE_NAME}'...")
    target_address = None
    # (Scan logic remains the same as previous script)
    devices = (
        await BleakScanner.discover()
    )  # [3] - Although search result is C++, Bleak uses similar discover concept
    for d in devices:
        if d.name == TARGET_DEVICE_NAME:
            target_address = d.address
            print(f"Found target device: {d.name} ({target_address})")
            break

    if target_address is None:
        print(f"Could not find target device '{TARGET_DEVICE_NAME}'.")
        # Clean up FIFO if we created it and are exiting
        try:
            if os.path.exists(FIFO_PATH) and stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
                os.remove(FIFO_PATH)
                print(f"Removed FIFO {FIFO_PATH}")
        except Exception as e:
            print(f"Error removing FIFO on exit: {e}")
        return

    # --- Open FIFO for Writing ---
    # Note: Opening O_WRONLY will block until a reader opens the FIFO [6]
    # It's often better to start the reader *before* this script, or handle this opening carefully.
    print(f"Opening FIFO {FIFO_PATH} for writing... Waiting for reader...")
    try:
        # Use os.open for lower-level control, might be slightly more robust with FIFOs
        # Use O_WRONLY and O_NONBLOCK initially to see if a reader is present
        # fd = os.open(FIFO_PATH, os.O_WRONLY | os.O_NONBLOCK)
        # If fd >= 0, a reader is present. Clear O_NONBLOCK for blocking writes.
        # fcntl.fcntl(fd, fcntl.F_SETFL, os.O_WRONLY)
        # fifo_out = os.fdopen(fd, 'w')

        # Simpler approach: Use standard open, which will block until reader connects
        fifo_out = open(
            FIFO_PATH, "w"
        )  # [3] [4] - Similar concept to C++ open(path, O_WRONLY)
        fifo_ready = True
        print("FIFO opened successfully. Reader is connected.")

    except Exception as e:
        print(f"Error opening FIFO for writing: {e}")
        # Clean up FIFO if we created it and are exiting
        try:
            if os.path.exists(FIFO_PATH) and stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
                os.remove(FIFO_PATH)
                print(f"Removed FIFO {FIFO_PATH}")
        except Exception as e_rem:
            print(f"Error removing FIFO on exit after open failed: {e_rem}")
        return

    # --- Connect to BLE Device and Run ---
    print(f"Connecting to {target_address}...")
    async with BleakClient(target_address) as client:
        if client.is_connected:
            print("Successfully connected to BLE device!")

            try:
                print("Subscribing to notifications...")
                await client.start_notify(CHARACTERISTIC_UUID_X, notification_handler)
                await client.start_notify(CHARACTERISTIC_UUID_Y, notification_handler)
                await client.start_notify(CHARACTERISTIC_UUID_BTN, notification_handler)
                print("Notifications enabled. Forwarding data to FIFO...")

                while True:
                    if not client.is_connected:
                        print("BLE device disconnected.")
                        break
                    # If the pipe broke, try reopening (basic attempt)
                    if not fifo_ready:
                        print("Attempting to reopen FIFO...")
                        try:
                            if fifo_out:
                                fifo_out.close()
                            fifo_out = open(FIFO_PATH, "w")
                            fifo_ready = True
                            print("FIFO reopened.")
                        except Exception as e_reopen:
                            print(f"Failed to reopen FIFO: {e_reopen}. Waiting...")
                            await asyncio.sleep(5)  # Wait before retrying

                    await asyncio.sleep(
                        0.1
                    )  # Small sleep, notifications are event-driven

            except Exception as e:
                print(f"An error occurred during BLE communication: {e}")
            finally:
                print("Disconnecting BLE...")
                # Stop notifications (optional)
                # await client.stop_notify(...)
        else:
            print("Failed to connect to BLE device.")

    # --- Cleanup ---
    print("Closing FIFO...")
    if fifo_out:
        try:
            fifo_out.close()
        except Exception as e:
            print(f"Error closing FIFO: {e}")

    # Optionally remove the FIFO file on exit
    # Keep it if you want the C++ reader to persist reading last values or wait
    # try:
    #     os.remove(FIFO_PATH)
    #     print(f"Removed FIFO {FIFO_PATH}")
    # except OSError as e:
    #     print(f"Error removing FIFO: {e}")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScript stopped by user.")
        # Ensure FIFO is removed on Ctrl+C if it exists
        if os.path.exists(FIFO_PATH) and stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
            try:
                if fifo_out:  # Close if open
                    fifo_out.close()
                os.remove(FIFO_PATH)
                print(f"Removed FIFO {FIFO_PATH}")
            except Exception as e:
                print(f"Error removing FIFO during exit: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        sys.exit(1)
