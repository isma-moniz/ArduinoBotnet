#!/bin/bash

set -e

CON_NAME="Mock_Lab"
IFACE="wlan0"

echo "[*] Removing existing '$CON_NAME' connection (if any)..."
sudo nmcli con delete "$CON_NAME" 2>/dev/null || true

echo "[*] Enabling IP forwarding..."
sudo sysctl -w net.ipv4.ip_forward=1

echo "[*] Creating access point..."
sudo nmcli con add type wifi ifname "$IFACE" mode ap con-name "$CON_NAME" ssid "$CON_NAME"

echo "[*] Forcing 2.4GHz band, channel 6..."
sudo nmcli con modify "$CON_NAME" 802-11-wireless.band bg
sudo nmcli con modify "$CON_NAME" 802-11-wireless.channel 6

echo "[*] Configuring WPA2 security..."
sudo nmcli con modify "$CON_NAME" 802-11-wireless-security.key-mgmt wpa-psk
sudo nmcli con modify "$CON_NAME" 802-11-wireless-security.psk "password123"

echo "[*] Setting static gateway and shared DHCP..."
sudo nmcli con modify "$CON_NAME" ipv4.addresses "10.42.0.1/24"
sudo nmcli con modify "$CON_NAME" ipv4.gateway "10.42.0.1"
sudo nmcli con modify "$CON_NAME" ipv4.method shared

echo "[*] Disabling autoconnect on hotspot..."
sudo nmcli con modify "$CON_NAME" connection.autoconnect no

echo "[*] Bringing up hotspot..."
sudo nmcli con up "$CON_NAME"

# Give dnsmasq a moment to start
sleep 2

echo ""
echo "[+] Hotspot '$CON_NAME' is active."
echo "[+] Gateway IP:"
ip addr show "$IFACE" | grep "inet " | awk '{print $2}'

echo "[+] dnsmasq DHCP status:"
sudo journalctl -u NetworkManager | grep -i "dhcp\|dnsmasq" | tail -5