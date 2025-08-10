import paho.mqtt.client as mqtt
import json
import time
import random

# --- Config ---
MQTT_BROKER_IP = "*"
MQTT_PORT = 1883
MQTT_TOPIC = "GreenEye/test/data" 

# --- Client Setup ---
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

def main():
    # --- Connect to Broker ---
    try:
        print(f"Connecting to MQTT Broker @ {MQTT_BROKER_IP}...")
        client.connect(MQTT_BROKER_IP, MQTT_PORT, 60)
        client.loop_start() 
        print("OK.")
    except Exception as e:
        print(f"FATAL: Could not connect to broker. {e}")
        return

    # --- Main Loop ---
    try:
        while True:
            # Prepare data as a Python dictionary
            payload = {
                "config_id": f"cfg_{int(time.time())}",
                "settings": {
                    "resolution": "VGA",
                    "jpeg_quality": random.randint(10, 20),
                    "enable_led": random.choice([True, False]),
                },
                "action": "apply_now"
            }

            # Serialize dictionary to a JSON string
            json_payload = json.dumps(payload)

            # Publish the payload
            result = client.publish(MQTT_TOPIC, json_payload)
            if result.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"TX: {json_payload}")
            else:
                print(f"TX failed, code: {result.rc}")

            time.sleep(5)

    except KeyboardInterrupt:
        print("\nShutdown.")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
