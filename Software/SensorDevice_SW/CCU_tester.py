import paho.mqtt.client as mqtt
import json
import time

BROKER_ADDRESS = "*" 
ESP32_DEVICE_ID = "*" 
REQUEST_INTERVAL = 30 

class CCU:
    def __init__(self, broker_address, device_id):
        self.broker = broker_address
        self.device_id = device_id
        self.last_request_time = 0

        self.data_topic = f"GreenEye/data/{ESP32_DEVICE_ID}"
        self.req_topic = f"GreenEye/req/{ESP32_DEVICE_ID}"
        self.conf_topic = f"GreenEye/conf/{ESP32_DEVICE_ID}"

        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("=== MQTT Broker Connected Successfully! ===")
            client.subscribe(self.data_topic)
            print(f"Subscribed to topic: {self.data_topic}")
        else:
            print(f"Failed to connect, return code {rc}\n")

    def on_message(self, client, userdata, msg):
        # print data topic msg
        print("\n" + "="*30)
        print(f"| Message Received from SD")
        print(f"| Topic: {msg.topic}")
        try:
            payload_json = json.loads(msg.payload.decode())
            print("| Payload (JSON):")
            print(json.dumps(payload_json, indent=2, ensure_ascii=False))
        except json.JSONDecodeError:
            print(f"| Payload (Raw): {msg.payload.decode()}")
        print("="*30)

    def connect(self):
        print(f"Connecting to broker at {self.broker}...")
        self.client.connect(self.broker, 1883, 60)
        self.client.loop_start()

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        print("MQTT client disconnected.")

    def request_sensor_data(self):
        print(f"\n[Action] Sending data request to {self.req_topic}")
        self.client.publish(self.req_topic, '{"req":1}')
        self.last_request_time = time.time()

    def send_config(self):
        try:
            # get user input for power mode
            pwr_mode = input("Enter Power Mode (Z, L, M, H, U): ").upper()
            
            # --- [NEW] Get user input for night mode ---
            nht_mode_input = input("Enable Night Mode (1=Yes, 0=No): ")
            nht_mode = bool(int(nht_mode_input))

            # get user input for flash settings
            flash_en = int(input("Enable Flash (1=Yes, 0=No): "))
            flash_level = int(input("Enter Flash Level (0-255): "))

            # create the json payload
            config_payload = {
                "pwr_mode": pwr_mode,
                "nht_mode": nht_mode, # add night mode to the payload
                "flash_en": bool(flash_en),
                "flash_level": flash_level
            }
            
            # publish the configuration message
            self.client.publish(self.conf_topic, json.dumps(config_payload))
            print("[Action] Configuration sent successfully!")

        except ValueError:
            print("[Error] Invalid input. Please enter numbers where required.")
        except Exception as e:
            print(f"An error occurred: {e}")

    def manual_command_menu(self):
        # Ctrl+C -> go to menu
        while True:
            print("\n--- Manual Command Menu ---")
            print("1: Request Sensor Data")
            print("2: Send Configuration")
            print("q: Quit")
            choice = input("Enter your choice: ").lower()

            if choice == '1':
                self.request_sensor_data()
                break
            elif choice == '2':
                self.send_config()
                break
            elif choice == 'q':
                break
            else:
                print("Invalid choice, please try again.")
    
    def run(self):
        self.connect()
        print("\nCCU Program Started. Monitoring for data...")
        print(f"(Requesting data every {REQUEST_INTERVAL} seconds. Press Ctrl+C for manual commands)")
        
        try:
            # monitoring loop
            while True:
                if time.time() - self.last_request_time > REQUEST_INTERVAL:
                    self.request_sensor_data()
                time.sleep(1)
        except KeyboardInterrupt:
            # Ctrl+C -> go to menu
            self.manual_command_menu()
        finally:
            self.disconnect()

if __name__ == "__main__":
    if ESP32_DEVICE_ID == "XXXX":
        print("!!! PLEASE SET THE `ESP32_DEVICE_ID` VARIABLE IN THE SCRIPT !!!")
    else:
        ccu = CCU(BROKER_ADDRESS, ESP32_DEVICE_ID)
        ccu.run()
