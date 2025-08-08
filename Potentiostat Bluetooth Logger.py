import asyncio
from bleak import BleakClient

# Replace these with your BLE device's address and the characteristic UUIDs
BLE_DEVICE_ADDRESS = "C6:14:EB:C8:08:02"  # The MAC address of your BLE device
WRITE_CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef0"  # UUID of the characteristic to send data to
NOTIFY_CHARACTERISTIC_UUID = "12345678-1234-5678-1234-56789abcdef2"  # UUID of the characteristic to subscribe to

# File where data will be logged
LOG_FILE = "ble_data_log.txt"

# Callback to handle the data when a notification is received
def notification_handler(sender, data):
    # Convert the byte data to a human-readable format (for example, int or string)
    data_value = int.from_bytes(data, byteorder="little", signed=True)  # Example: interpreting data as a little-endian integer
    print(f"Received notification from {sender}: {data_value}")
    
    # Log the received data into a file
    with open(LOG_FILE, "a") as log_file:
        log_file.write(f"Data from {sender}: {data_value}\n")

async def main():
    async with BleakClient(BLE_DEVICE_ADDRESS) as client:
        if client.is_connected:
            print(f"Connected to {BLE_DEVICE_ADDRESS}")

            # Step 1: Write a single byte (0x01) to the specified characteristic
            await client.write_gatt_char(WRITE_CHARACTERISTIC_UUID, bytearray([0x01]))
            print(f"Sent 0x01 to characteristic {WRITE_CHARACTERISTIC_UUID}")

            # Step 2: Start receiving notifications from the notification characteristic
            await client.start_notify(NOTIFY_CHARACTERISTIC_UUID, notification_handler)
            print(f"Subscribed to notifications from characteristic {NOTIFY_CHARACTERISTIC_UUID}")

            # Keep the connection alive and continue receiving notifications
            try:
                while True:
                    await asyncio.sleep(1)
            except KeyboardInterrupt:
                print("Disconnecting...")

            # Stop notifications before disconnecting
            await client.stop_notify(NOTIFY_CHARACTERISTIC_UUID)

if __name__ == "__main__":
    # Run the async main function
    asyncio.run(main())