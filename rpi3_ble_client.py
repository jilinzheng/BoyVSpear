import asyncio
from bleak import BleakScanner, BleakClient
import sys

# --- Configuration - MUST match your ESP32 sketch ---
TARGET_DEVICE_NAME = "ESP32_Joystick_Server"
SERVICE_UUID = "90920b02-5bb3-4d6d-8322-d744ccf56a04"
CHARACTERISTIC_UUID_X = "c7db9d15-3b2c-4e76-b7c1-57e6178dfb6c"
CHARACTERISTIC_UUID_Y = "d43bfa36-280a-495c-9e10-99f5d1bdc43e"
CHARACTERISTIC_UUID_BTN = "0bc7ad76-3ffd-4da4-8e3e-09613eddf3c4"
# ----------------------------------------------------

# Global dictionary to store the latest values
joystick_data = {"X": None, "Y": None, "Button": None}


# --- Notification Handler ---
# This function gets called every time the ESP32 sends a notification
def notification_handler(characteristic_uuid: str, data: bytearray):
    """Handles incoming BLE notifications and updates the global state."""
    decoded_data = data.decode("utf-8")
    # Uncomment for debugging raw data
    print(f"Raw Notification: UUID={characteristic_uuid}, Data={decoded_data}")

    try:
        value = int(decoded_data)  # Convert received string to integer
        if characteristic_uuid == CHARACTERISTIC_UUID_X:
            joystick_data["X"] = value
            print(f"X: {value}")
        elif characteristic_uuid == CHARACTERISTIC_UUID_Y:
            joystick_data["Y"] = value
            print(f"Y: {value}")
        elif characteristic_uuid == CHARACTERISTIC_UUID_BTN:
            # Button sends '0' for pressed, '1' for not pressed
            button_state = "Pressed" if value == 0 else "Not Pressed"
            joystick_data["Button"] = value
            print(f"Button: {button_state} ({value})")
        else:
            print(
                f"Unknown Characteristic: {characteristic_uuid}, Data: {decoded_data}"
            )

    except ValueError:
        print(
            f"Error: Could not convert data '{decoded_data}' to integer for UUID {characteristic_uuid}"
        )
    except Exception as e:
        print(f"Error processing notification: {e}")


# --- Main Program Logic ---
async def main():
    print(f"Scanning for '{TARGET_DEVICE_NAME}'...")
    target_address = None

    # Scan for devices
    devices = await BleakScanner.discover()  # [3]
    for d in devices:
        # print(f"Found device: {d.name} ({d.address})") # Uncomment to see all found devices
        if d.name == TARGET_DEVICE_NAME:
            target_address = d.address
            print(f"Found target device: {d.name} ({target_address})")
            break

    if target_address is None:
        print(
            f"Could not find target device '{TARGET_DEVICE_NAME}'. Make sure it's powered on and advertising."
        )
        return

    print(f"Connecting to {target_address}...")
    # Connect to the device using an async context manager
    async with BleakClient(target_address) as client:  # [3]
        if client.is_connected:
            print("Successfully connected!")

            try:
                # Subscribe to notifications for each characteristic
                print("Subscribing to notifications...")
                await client.start_notify(CHARACTERISTIC_UUID_X, notification_handler)
                await client.start_notify(CHARACTERISTIC_UUID_Y, notification_handler)
                await client.start_notify(CHARACTERISTIC_UUID_BTN, notification_handler)
                print("Notifications enabled. Waiting for data...")

                # Keep the script running to receive notifications
                # You can add other logic here that uses the joystick_data dictionary
                while True:
                    # Check if still connected (optional, Bleak handles disconnects)
                    if not client.is_connected:
                        print("Device disconnected.")
                        break
                    # Print current state periodically (optional)
                    # print(f"Current State: X={joystick_data['X']}, Y={joystick_data['Y']}, Btn={joystick_data['Button']}")
                    await asyncio.sleep(1)  # Sleep to allow other tasks to run

            except Exception as e:
                print(f"An error occurred during communication: {e}")
            finally:
                # Unsubscribe from notifications (optional, good practice)
                # Note: Sometimes stopping notifications can cause issues if disconnect happens abruptly
                # try:
                #     if client.is_connected:
                #         print("Disabling notifications...")
                #         await client.stop_notify(CHARACTERISTIC_UUID_X)
                #         await client.stop_notify(CHARACTERISTIC_UUID_Y)
                #         await client.stop_notify(CHARACTERISTIC_UUID_BTN)
                # except Exception as e:
                #      print(f"Error stopping notifications: {e}")
                print("Disconnected.")
        else:
            print("Failed to connect.")


if __name__ == "__main__":
    try:
        asyncio.run(main())  # [3]
    except KeyboardInterrupt:
        print("\nScript stopped by user.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        sys.exit(1)
