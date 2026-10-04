"""Pi MQTT configuration. Credentials stay in the backend, never in browser assets."""
import os

MQTT_HOST = "mqtt.flespi.io"
MQTT_PORT = 8883
MQTT_TOKEN = "LWTxjtkahgMK2DnylDd97xvKdfL7aTsag5LEmfzlYum7XAuXVj4zIZd42kygFtDm"
MQTT_UP = "phwateresp32/#"
MQTT_DOWN = "phwaterraspi/"
DATABASE_PATH = os.environ.get("PH_DATABASE_PATH", "/var/lib/ph-monitor/history.sqlite3")
SERVICE_UUID = "3f7c2e91-6a4b-4d8f-9c25-71b0e6a4d853"
DEVICE_UUID = "3f7c2e92-6a4b-4d8f-9c25-71b0e6a4d853"
SSID_UUID = "3f7c2e93-6a4b-4d8f-9c25-71b0e6a4d853"
PASSWORD_UUID = "3f7c2e94-6a4b-4d8f-9c25-71b0e6a4d853"
