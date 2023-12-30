from paho.mqtt import client as mqtt_client
import time
import paho.mqtt.client as paho
from paho import mqtt
import speech_recognition as sr

r = sr.Recognizer()

broker = 'mqtt-dashboard.com'
port = 1883
topic = "data_text"
# Generate a Client ID with the subscribe prefix.
client_id = 'clientId-F6YKSFU0O8'
# username = 'emqx'
# password = 'public'


def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    # client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")

    client.subscribe(topic)
    client.on_message = on_message





def run():
    client = connect_mqtt()

    set_time_list = ["mở", "đèn", "trong", "giây"]
    while True:
        text_send = ""
        with sr.Microphone() as source:
            print("Mời bạn nói: ")
            audio = r.listen(source)
        try:
            text = r.recognize_google(audio,language="vi-VI",)
            text = text.lower()
            print(text)
            client.loop_start()
            # a single publish, this can also be done in loops, etc.
            text_list = text.split()
            print(text_list)
            overlap = [element for element in set_time_list if element in text_list]
            print(overlap)


            if overlap == ["mở", "đèn", "trong", "giây"]:
                print("set time")
                time_delay = text_list[-2]
                text_send = "mo den trong "  + time_delay + " giay"

            if (text == "mở đèn"):
                text_send = "mo den trong 84600 giay"

            elif (text == "tắt đèn"):
                text_send = "tat den"

            elif (text == "chế độ tự động"):
                text_send = "che do tu dong"

            elif (text == "chế độ hẹn giờ"):
                text_send = "che do hen gio"
            
            print(text_send)
            if text_send != "":
                client.publish("esp/msg", payload=text_send, qos=1)
                client.loop_stop()
                # loop_forever for simplicity, here you need to stop the loop manually
                # you can also use loop_start and loop_stop
                # client.loop_forever()
                print("Bạn -->: {}".format(text))

        except:
            print("Xin lỗi! tôi không nhận được voice!")
        time.sleep(0.1)




if __name__ == '__main__':
    run()
