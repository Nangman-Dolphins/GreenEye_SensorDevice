import paho.mqtt.client as mqtt
import json
import time

# config
MQTT_BROKER_IP = "*"
MQTT_PORT = 1883
TOPIC_REQ = "test/req"   # topic to publish requests to
TOPIC_DATA = "test/data" # topic to subscribe to for data

# callback function for when the client connects to the broker
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("[mqtt] connected to broker successfully.")
        # subscribe to the data topic upon successful connection
        client.subscribe(TOPIC_DATA)
        print(f"[mqtt] subscribed to topic: {TOPIC_DATA}")
    else:
        print(f"[mqtt] connection failed with code: {rc}")

# callback function for when a message is received
def on_message(client, userdata, msg):
    # this function is automatically called by the background thread
    print("\n<-- message received! -->") # add newlines to not interrupt user input
    
    payload_str = msg.payload.decode('utf-8')
    print(f"  topic: {msg.topic} | payload: {payload_str}")

    try:
        data = json.loads(payload_str)
        print("  json parsed successfully:")
        print(f"    deviceId: {data.get('deviceId', 'n/a')}")
        print(f"    value: {data.get('value', 'n/a')}")
    except json.JSONDecodeError:
        print("  error: could not decode json.")
    
    print("\nenter 'req' to send another request or 'exit' to quit:") # re-prompt the user
    
# main execution block
def main():
    # create a new mqtt client
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    
    # assign the callback functions
    client.on_connect = on_connect
    client.on_message = on_message

    # connect to the broker
    try:
        print(f"connecting to mqtt broker @ {MQTT_BROKER_IP}...")
        client.connect(MQTT_BROKER_IP, MQTT_PORT, 60)
    except Exception as e:
        print(f"fatal: could not connect. {e}")
        return

    # use loop_start() to run the network loop in a background thread.
    # this frees up the main thread to handle user input.
    client.loop_start()
    print("[info] listener started in background. type 'req' to send a request.")

    try:
        while True:
            # wait for user input in the main thread
            command = input("enter 'req' to send request or 'exit' to quit: ")
            
            if command.lower() == 'req':
                # publish the request message
                print(f"[mqtt] sending 'req' to topic {TOPIC_REQ}...")
                client.publish(TOPIC_REQ, "get_data_please")
            elif command.lower() == 'exit':
                print("[info] exit command received.")
                break
            else:
                print(f"unknown command: '{command}'")
    
    except KeyboardInterrupt:
        print("\n[info] ctrl+c detected.")

    finally:
        # clean up
        print("[info] shutting down...")
        client.loop_stop() # stop the background thread
        client.disconnect()
        print("[mqtt] disconnected.")

if __name__ == "__main__":
    main()
