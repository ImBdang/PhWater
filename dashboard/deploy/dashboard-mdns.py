#!/usr/bin/python3
"""Publish dashboard.local as an alias of the Pi's existing mDNS hostname.

Avahi's native hostname resolves to the address on the client's interface. Using
a CNAME keeps raspberrypi.local working and avoids advertising the AP address
to clients on Ethernet. systemd restarts this publisher with Avahi.
"""

import logging
import sys

import dbus
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

logging.basicConfig(level=logging.INFO, format="%(message)s")
DBusGMainLoop(set_as_default=True)
bus = dbus.SystemBus()
server = dbus.Interface(bus.get_object("org.freedesktop.Avahi", "/"), "org.freedesktop.Avahi.Server")
target = str(server.GetHostNameFqdn()).rstrip(".")
alias = "dashboard.local"

if target == alias:
    logging.info("The native Avahi hostname is already %s", alias)
else:
    rdata = b"".join(bytes([len(label.encode("utf-8"))]) + label.encode("utf-8") for label in target.split(".")) + b"\x00"
    group = dbus.Interface(bus.get_object("org.freedesktop.Avahi", server.EntryGroupNew()), "org.freedesktop.Avahi.EntryGroup")

    def group_changed(state, error):
        if int(state) in (3, 4):  # collision / failure
            logging.error("Cannot publish %s: state=%s, error=%s", alias, state, error)
            sys.exit(1)

    group.connect_to_signal("StateChanged", group_changed)
    # interface=-1, protocol=-1, flags=0, DNS class IN=1, CNAME=5.
    group.AddRecord(dbus.Int32(-1), dbus.Int32(-1), dbus.UInt32(0), alias,
                    dbus.UInt16(1), dbus.UInt16(5), dbus.UInt32(60), dbus.ByteArray(rdata))
    group.Commit()
    logging.info("Publishing %s -> %s", alias, target)


def avahi_owner_changed(name, old_owner, new_owner):
    if name == "org.freedesktop.Avahi" and old_owner and old_owner != new_owner:
        logging.error("Avahi restarted; republish through systemd")
        sys.exit(1)


bus.add_signal_receiver(avahi_owner_changed, signal_name="NameOwnerChanged", dbus_interface="org.freedesktop.DBus")
GLib.MainLoop().run()
