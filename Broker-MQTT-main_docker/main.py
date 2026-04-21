import paho.mqtt.client as mqtt

# MQTT broker configuration
BROKER_ADDRESS = "mosquitto"
BROKER_PORT = 1883
TOPIC_IN  = "topic/ping"  # Topic ESP to echo-server
TOPIC_OUT = "topic/pong"  # this echo-server to ESP

# Callback for when the client connects to the broker
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Successfully connected to the MQTT broker!")
        # Mi iscrivo SOLO al topic di ingresso
        client.subscribe(TOPIC_IN)
        print(f"Listening on topic: {TOPIC_IN}")
    else:
        print(f"Connection failed. Error code: {reason_code}")

# Callback for when a PUBLISH message is received from the server
def on_message(client, userdata, msg):
    # Decode the payload from bytes to string
    message = msg.payload.decode('utf-8')
    topic = msg.topic
    
    print(f"\nReceived from ESP32: '{message}' on topic '{topic}'")
    
    # ACTION: Rispondo immediatamente, ma sul TOPIC_OUT!
    print(f"I send immediately back the message '{message}' to topic '{TOPIC_OUT}'")
    client.publish(TOPIC_OUT, message)

# Main function
def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message

    print("Attempting to connect to the broker...")

    try:
        client.connect(BROKER_ADDRESS, BROKER_PORT, keepalive=60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nClosing the script...")
        client.disconnect()
    except Exception as e:
        print(f"Connection error: {e}")

# --- ENTRY POINT OF THE SCRIPT ---
if __name__ == "__main__":
    main()