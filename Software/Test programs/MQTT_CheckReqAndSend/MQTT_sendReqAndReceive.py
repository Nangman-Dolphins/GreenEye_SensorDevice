import paho.mqtt.client as mqtt
import json
import time

# config
MQTT_BROKER_IP = "*"
MQTT_PORT = 1883
TOPIC_REQ = "test/req"   # topic to publish requests to
TOPIC_DATA = "test/data" # topic to subscribe to for data

# callback for when the client connects to the broker
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("[mqtt] connected to broker.")
        # subscribe to the data topic first
        client.subscribe(TOPIC_DATA)
        print(f"[mqtt] subscribed to {TOPIC_DATA}")
        
        # then, publish the request
        print(f"[mqtt] sending request to {TOPIC_REQ}...")
        client.publish(TOPIC_REQ, "get_data")
    else:
        print(f"[mqtt] connection failed with code: {rc}")

# callback for when a message is received
def on_message(client, userdata, msg):
    print("\n<-- response received -->")
    
    payload_str = msg.payload.decode('utf-8')
    print(f"  raw data: {payload_str}")

    # parse and print the data
    try:
        data = json.loads(payload_str)
        print("  json parsed:")
        print(f"    device id: {data.get('deviceId', 'n/a')}")
        print(f"    value: {data.get('value', 'n/a')}")
    except json.JSONDecodeError:
        print("  error: could not decode json.")
    
    # after processing the message, disconnect the client
    # this will cause loop_forever() to stop
    print("\n[info] task complete. disconnecting.")
    client.disconnect()

# main execution block
def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"connecting to {MQTT_BROKER_IP}...")
    try:
        client.connect(MQTT_BROKER_IP, MQTT_PORT, 60)
    except Exception as e:
        print(f"fatal: could not connect. {e}")
        return

    # loop_forever() blocks execution until the client is disconnected.
    # in this code, disconnect() is called inside on_message().
    client.loop_forever()
    print("[info] script finished.")

if __name__ == "__main__":
    main()
