import paho.mqtt.client as mqtt
import datetime

# --- Configuration ---
# Enter the IP address
MQTT_BROKER = "*" 
MQTT_PORT = 1883

# topic
MQTT_TOPIC = "esp32/cam/image"

# --- Callback Functions ---
# callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT Broker.")
        # Subscribe to the topic to receive messages.
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Failed to connect, return code {rc}")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    # Create a filename using the current timestamp (e.g., received_image_20250809_203500.jpg)
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    file_name = f"received_image_{timestamp}.jpg"
    
    # The message payload is the image data (byte array).
    # Open the file in 'wb' (write binary) mode and save the data.
    try:
        with open(file_name, "wb") as f:
            f.write(msg.payload)
        print(f" Image received and saved as '{file_name}'")
    except Exception as e:
        print(f" Error saving file: {e}")


# --- Main Execution ---
def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)

    # Assign callback functions.
    client.on_connect = on_connect
    client.on_message = on_message

    print("Attempting to connect to the broker...")
    try:
        # Connect to the broker.
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    except Exception as e:
        print(f"An error occurred while connecting to the broker: {e}")
        print("--> Please check if the IP address is correct and the Mosquitto broker is running.")
        return

    # Keep receive msg
    client.loop_forever()

if __name__ == "__main__":
    main()
