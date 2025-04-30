import asyncio
from bleak import BleakScanner, BleakClient

# --- Add this import ---
from bleak.backends.characteristic import BleakGATTCharacteristic

# -----------------------
import sys
import os
import stat
import errno
import time

# --- Configuration - MUST match your ESP32 sketch ---
# (Make sure these UUIDs match the ones in your ESP32 code exactly)
TARGET_DEVICE_NAME = "ESP32_Joystick_Server"
SERVICE_UUID = "90920b02-5bb3-4d6d-8322-d744ccf56a04"
CHARACTERISTIC_UUID_X = "c7db9d15-3b2c-4e76-b7c1-57e6178dfb6c"
CHARACTERISTIC_UUID_Y = "d43bfa36-280a-495c-9e10-99f5d1bdc43e"
CHARACTERISTIC_UUID_BTN = "0bc7ad76-3ffd-4da4-8e3e-09613eddf3c4"
# ----------------------------------------------------

# --- FIFO Configuration ---
FIFO_PATH = "/tmp/joystick_fifo"
# -------------------------

# Global dictionary to store the latest values
joystick_data = {
    "X": 4,
    "Y": 4,
    "Button": 1,
}  # Initialize with NEUTRAL/RELEASED defaults

fifo_out = None
fifo_ready = False


# --- CORRECTED Notification Handler ---
# Accepts BleakGATTCharacteristic object as first argument [5]
def notification_handler(characteristic: BleakGATTCharacteristic, data: bytearray):
    """Handles incoming BLE notifications, updates state, and writes to FIFO."""
    global fifo_out, fifo_ready, joystick_data

    # Get the UUID string from the characteristic object
    char_uuid = characteristic.uuid
    decoded_data = data.decode("utf-8")

    # print(f"Notify from {char_uuid}: {decoded_data}") # Optional debug

    data_changed = False
    try:
        value = int(decoded_data)  # Convert received string to integer

        # Compare the characteristic's UUID with your defined constants
        if char_uuid == CHARACTERISTIC_UUID_X:
            if joystick_data["X"] != value:
                joystick_data["X"] = value
                data_changed = True
        elif char_uuid == CHARACTERISTIC_UUID_Y:
            if joystick_data["Y"] != value:
                joystick_data["Y"] = value
                data_changed = True
        elif char_uuid == CHARACTERISTIC_UUID_BTN:
            if joystick_data["Button"] != value:
                joystick_data["Button"] = value
                data_changed = True
        else:
            # This should NOT be hit now if UUIDs match ESP32
            print(f"Unknown Characteristic UUID: {char_uuid}, Data: {decoded_data}")
            return  # Don't process if unknown

        # If data changed and FIFO is ready, write the current state
        if data_changed and fifo_ready and fifo_out:
            try:
                # Format: X Y Button\n (using current state)
                output_string = f"{joystick_data['X']} {joystick_data['Y']} {joystick_data['Button']}\n"
                fifo_out.write(output_string)
                fifo_out.flush()
            except BrokenPipeError:
                print("FIFO Error: Broken pipe. Reader might have closed.")
                fifo_ready = False
            except OSError as e:
                print(f"FIFO Write Error: {e}")
            except Exception as e:
                print(f"Error writing to FIFO: {e}")

    except ValueError:
        print(
            f"Error: Could not convert data '{decoded_data}' to integer for UUID {char_uuid}"
        )
    except Exception as e:
        print(f"Error processing notification: {e}")


# --- The rest of your main() function remains the same ---
# (Make sure SERVICE_UUID and CHARACTERISTIC_UUID_* constants above
# exactly match the #define values in your Arduino sketch)
async def main():
    # ... (rest of the main function is unchanged) ...
    global fifo_out, fifo_ready

    # --- Create FIFO ---
    try:
        if os.path.exists(FIFO_PATH):
            # Check if it's actually a FIFO file
            if not stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
                print(f"Error: {FIFO_PATH} exists but is not a FIFO. Please remove it.")
                return
            else:
                print(f"FIFO {FIFO_PATH} already exists.")
                # Ensure permissions are sufficient if it exists (optional)
                # os.chmod(FIFO_PATH, 0o666)
        else:
            os.mkfifo(FIFO_PATH, 0o666)
            print(f"Created FIFO at {FIFO_PATH}")
    except Exception as e:
        print(f"Error setting up FIFO: {e}")
        return

    # --- Scan and Connect ---
    print(f"Scanning for '{TARGET_DEVICE_NAME}'...")
    target_address = None
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name == TARGET_DEVICE_NAME:
            target_address = d.address
            print(f"Found target device: {d.name} ({target_address})")
            break

    if target_address is None:
        print(f"Could not find target device '{TARGET_DEVICE_NAME}'.")
        # Clean up FIFO if we created it and are exiting
        if not os.path.exists(
            FIFO_PATH
        ):  # Only remove if we potentially created it AND failed to find device
            try:
                os.remove(FIFO_PATH)
                print(f"Removed FIFO {FIFO_PATH}")
            except Exception as e:
                print(f"Error removing FIFO on exit: {e}")
        return

    # --- Open FIFO for Writing ---
    print(f"Opening FIFO {FIFO_PATH} for writing... Waiting for reader...")
    try:
        fifo_out = open(FIFO_PATH, "w")
        fifo_ready = True
        print("FIFO opened successfully. Reader is connected.")
    except Exception as e:
        print(f"Error opening FIFO for writing: {e}")
        # Clean up FIFO only if we definitely created it in this run
        if not os.path.exists(
            FIFO_PATH
        ):  # This check might be tricky if it failed mid-creation
            try:
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
                            await asyncio.sleep(5)

                    await asyncio.sleep(0.1)

            except Exception as e:
                print(f"An error occurred during BLE communication: {e}")
            finally:
                print("Disconnecting BLE...")
        else:
            print("Failed to connect to BLE device.")

    # --- Cleanup ---
    print("Closing FIFO...")
    if fifo_out:
        try:
            fifo_out.close()
        except Exception as e:
            print(f"Error closing FIFO: {e}")

    # Optionally remove FIFO on normal exit
    # try:
    #     if os.path.exists(FIFO_PATH) and stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
    #          os.remove(FIFO_PATH)
    #          print(f"Removed FIFO {FIFO_PATH}")
    # except OSError as e:
    #     print(f"Error removing FIFO: {e}")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nScript stopped by user.")
        if os.path.exists(FIFO_PATH) and stat.S_ISFIFO(os.stat(FIFO_PATH).st_mode):
            try:
                if fifo_out:
                    fifo_out.close()
                os.remove(FIFO_PATH)
                print(f"Removed FIFO {FIFO_PATH}")
            except Exception as e:
                print(f"Error removing FIFO during exit: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        sys.exit(1)
