# ===
# mqtt client script to receive and print sensor/image data
# decodes base64 image data and saves it as a jpg file
# ===

import paho.mqtt.client as mqtt
import json
import base64
import os
from datetime import datetime

# ===
# configuration
# ===
BROKER_ADDRESS = "" 
PORT = 1883
TOPIC = "GreenEye/data/+" # subscribe to all devices under GreenEye/data
IMAGE_DIR = "received_images" # directory to save received images


# ===
# callback function for when the client connects to the broker
# ===
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("=== Connected to MQTT Broker! ===")
        # subscribe to the topic
        client.subscribe(TOPIC)
        print(f"=== Subscribed to topic: {TOPIC} ===")
    else:
        print(f"=== Failed to connect, return code {rc}\n ===")


# ===
# callback function for when a message is received
# ===
def on_message(client, userdata, msg):
    # decode the message payload from bytes to string
    payload_str = msg.payload.decode("utf-8")

    print("\n--- New Message Received ---")
    print(f"Topic: {msg.topic}")
    print(f"Payload Snippet: {payload_str[:200]}...\n") # Print first 200 characters

    try:
        # attempt to parse the string as json
        data = json.loads(payload_str)

        # check if the 'plant_img' key exists, indicating it's an image payload
        if "plant_img" in data and data["plant_img"]:
            print(f"Image data received from topic '{msg.topic}'...")
            
            try:
                # get the base64 encoded string from the json
                base64_data = data["plant_img"]
                # decode it into image bytes
                image_bytes = base64.b64decode(base64_data)

                # generate a unique filename using device_id and timestamp
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                device_id = msg.topic.split('/')[-1]
                filename = f"{device_id}_{timestamp}.jpg"
                filepath = os.path.join(IMAGE_DIR, filename)

                # write the bytes to a new jpg file
                with open(filepath, "wb") as f:
                    f.write(image_bytes)
                
                print(f"=== Successfully saved image to {filepath} ===")

            except (base64.binascii.Error, TypeError) as e:
                print(f"!!! Could not decode Base64 string: {e} !!!")

        else:
            # it's json, but not an image, so it's regular sensor data
            print(f"Received sensor data from topic '{msg.topic}':")
            print(payload_str)

    except json.JSONDecodeError:
        # if it's not json, it's probably an unexpected raw message
        print(f"Received non-JSON message from topic '{msg.topic}':")
        print(payload_str)
        
    print("---------------------------------")


# ===
# main script execution
# ===
if __name__ == "__main__":
    # create the directory for images if it doesn't exist
    if not os.path.exists(IMAGE_DIR):
        print(f"Creating directory for images: {IMAGE_DIR}")
        os.makedirs(IMAGE_DIR)

    # create a new mqtt client instance
    client = mqtt.Client()

    # assign the callback functions
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # connect to the broker
        print(f"=== Connecting to broker at {BROKER_ADDRESS} ===")
        client.connect(BROKER_ADDRESS, PORT, 60)

        # start the network loop to process messages
        client.loop_forever()

    except ConnectionRefusedError:
        print("=== Connection refused. Please check the broker address and make sure it is running. ===")
    except OSError as e:
        print(f"=== Connection failed due to a network error: {e} ===")
    except KeyboardInterrupt:
        print("\n=== Subscriber stopped by user. ===")
        client.disconnect()
        print("=== Disconnected from MQTT Broker. ===")
