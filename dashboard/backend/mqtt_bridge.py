import json
import logging
import ssl
import paho.mqtt.client as mqtt
from config import MQTT_HOST, MQTT_PORT, MQTT_TOKEN, MQTT_UP, MQTT_DOWN

log = logging.getLogger("ph.mqtt")

class MqttBridge:
    def __init__(self, database):
        self.database = database
        self.connected = False
        self.error = None
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="phwater-raspi4", protocol=mqtt.MQTTv311)
        self.client.username_pw_set(MQTT_TOKEN)
        self.client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
        self.client.reconnect_delay_set(1, 30)
        self.client.max_queued_messages_set(100)
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_subscribe = self.on_subscribe
        self.client.on_message = self.on_message

    def start(self):
        self.client.connect_async(MQTT_HOST, MQTT_PORT, keepalive=30)
        self.client.loop_start()

    def stop(self):
        self.client.disconnect()
        self.client.loop_stop()

    def on_connect(self, client, userdata, flags, reason, properties):
        self.connected = False
        if reason.is_failure:
            self.error = str(reason)
            log.error("MQTT rejected connection: %s", reason)
        else:
            self.error = None
            client.subscribe(MQTT_UP, qos=1)

    def on_subscribe(self, client, userdata, mid, reasons, properties):
        self.connected = bool(reasons) and all(not reason.is_failure for reason in reasons)
        self.error = None if self.connected else "subscription_rejected"
        log.info("MQTT subscription ready=%s topic=%s", self.connected, MQTT_UP)

    def on_disconnect(self, client, userdata, flags, reason, properties):
        self.connected = False
        self.error = str(reason) if reason.is_failure else None

    def on_message(self, client, userdata, message):
        try:
            if len(message.payload) > 4096:
                raise ValueError("payload too large")
            data = json.loads(message.payload)
            if self.database.ingest(message.topic, data, message.retain):
                log.info("RX type=%s device=%s command=%s", data.get("type"), data.get("device_id"), data.get("command_id", ""))
        except (ValueError, TypeError, UnicodeError) as error:
            log.warning("Ignored invalid MQTT message on %s: %s", message.topic, error)
        except Exception:
            log.exception("MQTT storage failed")

    def publish_command(self, device_id, payload):
        if not self.connected:
            raise RuntimeError("MQTT broker chưa kết nối")
        result = self.client.publish(MQTT_DOWN + device_id, json.dumps(payload, separators=(",", ":")), qos=1, retain=False)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            raise RuntimeError("Không gửi được lệnh MQTT")
