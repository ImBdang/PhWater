#!/usr/bin/env bash
set -euo pipefail
dashboard_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ssh_target="${1:-haianh@192.168.1.16}"
deploy_temp="$(mktemp -d)"
trap 'rm -rf -- "$deploy_temp"' EXIT
tar -C "$dashboard_root" --exclude='__pycache__' -czf "$deploy_temp/backend.tar.gz" backend deploy/ph-dashboard-api.service deploy/nginx.conf
scp "$deploy_temp/backend.tar.gz" "$ssh_target:/tmp/ph-dashboard-backend.tar.gz"
ssh "$ssh_target" 'sudo bash -s' <<'REMOTE'
set -euo pipefail
install -d -m 755 /opt/ph-dashboard
tar -xzf /tmp/ph-dashboard-backend.tar.gz -C /opt/ph-dashboard
chmod 700 /opt/ph-dashboard/backend
chown -R haianh:haianh /opt/ph-dashboard/backend
install -d -m 700 -o haianh -g haianh /var/lib/ph-monitor
test -d /opt/ph-dashboard/venv || python3 -m venv /opt/ph-dashboard/venv
/opt/ph-dashboard/venv/bin/pip install --quiet /opt/ph-dashboard/backend
install -m 644 /opt/ph-dashboard/deploy/ph-dashboard-api.service /etc/systemd/system/ph-dashboard-api.service
install -m 644 /opt/ph-dashboard/deploy/nginx.conf /etc/nginx/sites-available/ph-dashboard
systemctl daemon-reload
nginx -t
systemctl enable ph-dashboard-api
systemctl restart ph-dashboard-api
systemctl reload nginx
rm /tmp/ph-dashboard-backend.tar.gz
REMOTE
