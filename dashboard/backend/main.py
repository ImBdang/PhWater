import asyncio
import csv
import io
import json
import logging
import math
import re
import time
import uuid
from contextlib import asynccontextmanager

from bleak import BleakClient, BleakScanner
from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import StreamingResponse
from pydantic import BaseModel, Field, StrictInt
from config import DATABASE_PATH, MQTT_HOST, MQTT_PORT, MQTT_UP, MQTT_DOWN, SERVICE_UUID, DEVICE_UUID, SSID_UUID, PASSWORD_UUID
from database import Database, DEVICE_ID_RE
from mqtt_bridge import MqttBridge

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(levelname)s %(message)s")
db = Database(DATABASE_PATH)
bridge = MqttBridge(db)
ble_lock = asyncio.Lock()
jobs = {}
tasks = set()

async def maintenance():
    while True:
        await asyncio.sleep(60)
        await asyncio.to_thread(db.prune)
        for job_id, job in list(jobs.items()):
            if job["status"] != "running" and time.time()-job["created_at"] > 600:
                del jobs[job_id]

@asynccontextmanager
async def lifespan(app):
    bridge.start()
    cleanup = asyncio.create_task(maintenance())
    yield
    cleanup.cancel()
    for task in list(tasks):
        task.cancel()
    await asyncio.gather(cleanup, *list(tasks), return_exceptions=True)
    bridge.stop()
    db.db.close()

app = FastAPI(title="pH Monitor Pi API", lifespan=lifespan)

def device_exists(device_id):
    if not DEVICE_ID_RE.fullmatch(device_id) or not any(d["id"] == device_id for d in db.devices()):
        raise HTTPException(404, "Không tìm thấy ESP32")

def interval(start, end):
    if not math.isfinite(start) or not math.isfinite(end) or start < 0 or end < start or end - start > 366 * 86400:
        raise HTTPException(422, "Khoảng thời gian không hợp lệ (tối đa 366 ngày)")

@app.get("/api/snapshot")
def snapshot():
    return {"devices": db.devices(), "settings": db.settings(),
            "mqtt": {"connected": bridge.connected, "error": bridge.error, "host": MQTT_HOST,
                     "port": MQTT_PORT, "subscribe": MQTT_UP, "publish": MQTT_DOWN+"<device_id>", "tls": True},
            "network": {"apName": "raspi-haianh", "apIp": "10.42.0.1", "hostname": "dashboard.local"},
            "serverTime": time.time(), "database": DATABASE_PATH}

class SettingsRequest(BaseModel):
    stationName: str = Field(min_length=1, max_length=80)
    phMin: str
    phMax: str
    retention: str

@app.put("/api/settings")
def save_settings(values: SettingsRequest):
    if not values.stationName.strip():
        raise HTTPException(422, "Tên trạm không được để trống")
    try:
        low, high, retention = float(values.phMin), float(values.phMax), int(values.retention)
        if not 0 <= low < high <= 14 or not 1 <= retention <= 365:
            raise ValueError()
    except ValueError:
        raise HTTPException(422, "Ngưỡng pH hoặc thời gian lưu không hợp lệ")
    db.save_settings({"stationName": values.stationName.strip(), "phMin": str(low), "phMax": str(high), "retention": str(retention)})
    db.prune()
    return db.settings()

class CommandRequest(BaseModel):
    cmd: str
    cycle_seconds: StrictInt | None = None

@app.post("/api/devices/{device_id}/commands", status_code=202)
def send_command(device_id: str, command: CommandRequest):
    device_exists(device_id)
    if command.cmd not in ("get_status", "set_cycle"):
        raise HTTPException(422, "cmd không được hỗ trợ")
    if command.cmd == "set_cycle" and (command.cycle_seconds is None or not 1 <= command.cycle_seconds <= 86400):
        raise HTTPException(422, "Chu kỳ phải là số nguyên từ 1 đến 86400 giây")
    if not bridge.connected:
        raise HTTPException(503, "Pi chưa kết nối MQTT broker")
    command_id = uuid.uuid4().hex
    payload = {"cmd": command.cmd, "command_id": command_id, "expires_at": int(time.time())+15}
    if command.cmd == "set_cycle":
        payload["cycle_seconds"] = command.cycle_seconds
    db.create_command(command_id, device_id, payload)
    try:
        bridge.publish_command(device_id, payload)
    except RuntimeError as error:
        db.fail_command(command_id, str(error))
        raise HTTPException(503, str(error))
    return db.command(command_id)

@app.get("/api/commands/{command_id}")
def command_status(command_id: str):
    command = db.command(command_id)
    if not command:
        raise HTTPException(404, "Không tìm thấy lệnh")
    return command

@app.get("/api/readings")
def history(device_id: str, start: float, end: float, limit: int = Query(20, ge=1, le=1000), offset: int = Query(0, ge=0)):
    device_exists(device_id)
    interval(start, end)
    return db.history(device_id, start, end, limit, offset)

@app.get("/api/chart")
def chart(device_id: str, start: float, end: float):
    device_exists(device_id)
    interval(start, end)
    return {"readings": db.chart(device_id, start, end)}

@app.get("/api/export.csv")
def export(device_id: str, start: float, end: float):
    device_exists(device_id)
    interval(start, end)
    def chunks():
        buffer = io.StringIO()
        writer = csv.writer(buffer)
        yield "\ufeff"
        writer.writerow(["timestamp_utc", "device_id", "ph", "voltage_v", "raw_adc", "adc_mv", "sensor_error"])
        yield buffer.getvalue()
        # Descending keyset pagination avoids duplicate/missing rows if new data arrives during export.
        last_time, last_id = end, 2**63-1
        while True:
            with db.lock:
                rows = db.db.execute("""SELECT * FROM readings WHERE device_id=? AND recorded_at>=? AND recorded_at<=?
                    AND (recorded_at<? OR (recorded_at=? AND id<?)) ORDER BY recorded_at DESC,id DESC LIMIT 500""",
                    (device_id, start, end, last_time, last_time, last_id)).fetchall()
            if not rows:
                break
            buffer.seek(0); buffer.truncate(0)
            for row in rows:
                stamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(row["recorded_at"]))
                writer.writerow([stamp, device_id, row["ph"], row["voltage"], row["raw_adc"], row["adc_mv"], row["sensor_error"]])
            yield buffer.getvalue()
            last_time, last_id = rows[-1]["recorded_at"], rows[-1]["id"]
    return StreamingResponse(chunks(), media_type="text/csv", headers={"Content-Disposition": f'attachment; filename="ph-monitor-{device_id}.csv"'})

def new_job(kind):
    if len(tasks) >= 4:
        raise HTTPException(409, "Pi đang xử lý BLE, thử lại sau")
    job_id = uuid.uuid4().hex
    job = {"id": job_id, "kind": kind, "status": "running", "step": 0, "devices": [], "error": None, "created_at": time.time()}
    jobs[job_id] = job
    return job

def start_task(coroutine):
    task = asyncio.create_task(coroutine)
    tasks.add(task)
    task.add_done_callback(tasks.discard)

async def scan(job):
    try:
        async with ble_lock:
            discovered = await BleakScanner.discover(timeout=6, return_adv=True)
        devices = []
        for device, adv in discovered.values():
            supported = SERVICE_UUID in [s.lower() for s in adv.service_uuids]
            devices.append({"id": device.address, "mac": device.address, "name": adv.local_name or device.name or "Thiết bị BLE",
                            "rssi": adv.rssi, "provisionable": supported})
        job.update(status="done", devices=sorted(devices, key=lambda d: (not d["provisionable"], -d["rssi"])))
    except Exception as error:
        logging.getLogger("ph.ble").warning("BLE scan failed: %s", type(error).__name__)
        job.update(status="failed", error="Không quét được BLE trên Pi: "+str(error))

@app.post("/api/ble/scan", status_code=202)
async def scan_ble():
    job = new_job("scan")
    start_task(scan(job))
    return job

class ProvisionRequest(BaseModel):
    mac: str
    name: str = Field(min_length=1, max_length=80)
    ssid: str
    password: str

async def provision(job, values):
    try:
        async with ble_lock:
            job["step"] = 0
            async with BleakClient(values.mac, timeout=15) as client:
                if not any(str(service.uuid).lower() == SERVICE_UUID for service in client.services):
                    raise ValueError("Thiết bị không có service provision của dự án")
                device_id = (await client.read_gatt_char(DEVICE_UUID)).decode().strip("\x00")
                if not DEVICE_ID_RE.fullmatch(device_id):
                    raise ValueError("Device ID BLE không hợp lệ")
                job.update(step=1, device_id=device_id)
                await client.write_gatt_char(SSID_UUID, values.ssid.encode(), response=True)
                # The firmware disconnects BLE as soon as both fields are written.
                started = time.time()
                await client.write_gatt_char(PASSWORD_UUID, values.password.encode(), response=True)
            db.register(device_id, values.name.strip(), values.mac)
        job["step"] = 2
        for _ in range(60):
            device = next((d for d in db.devices() if d["id"] == device_id), None)
            if device and device.get("lastSeenAt") and device["lastSeenAt"] >= started and device["status"] == "online":
                job.update(status="done", step=3)
                return
            await asyncio.sleep(1)
        raise TimeoutError("Đã gửi Wi-Fi qua BLE nhưng chưa nhận MQTT. Kiểm tra Wi-Fi, Internet và broker.")
    except Exception as error:
        logging.getLogger("ph.ble").warning("Provision failed: %s", type(error).__name__)
        job.update(status="failed", error=str(error))

@app.post("/api/ble/provision", status_code=202)
async def provision_ble(values: ProvisionRequest):
    if not re.fullmatch(r"(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}", values.mac):
        raise HTTPException(422, "Địa chỉ BLE không hợp lệ")
    if not 1 <= len(values.ssid.encode()) <= 32 or not 8 <= len(values.password.encode()) <= 63:
        raise HTTPException(422, "SSID tối đa 32 byte; mật khẩu từ 8 đến 63 byte")
    job = new_job("provision")
    start_task(provision(job, values))
    return job

@app.get("/api/ble/jobs/{job_id}")
def ble_job(job_id: str):
    if job_id not in jobs:
        raise HTTPException(404, "Không tìm thấy tác vụ BLE")
    return jobs[job_id]
