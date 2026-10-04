#!/usr/bin/env bash
# Update the website on a Pi that has already been configured with nginx.
set -euo pipefail
dashboard_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ssh_target="${1:-haianh@192.168.1.16}"
release_id="$(date -u +%Y%m%dT%H%M%SZ)"
deploy_temp="$(mktemp -d)"
trap 'rm -rf -- "$deploy_temp"' EXIT

cd -- "$dashboard_root"
npm run build
tar -C dist -czf "$deploy_temp/dist.tar.gz" .
scp "$deploy_temp/dist.tar.gz" "$ssh_target:/tmp/ph-dashboard-$release_id.tar.gz"
ssh "$ssh_target" "sudo bash -s -- '$release_id'" <<'REMOTE'
set -euo pipefail
release_id="$1"
release_path="/var/www/ph-dashboard/releases/$release_id"
install -d -m 755 "$release_path"
tar -xzf "/tmp/ph-dashboard-$release_id.tar.gz" -C "$release_path"
chown -R root:root "$release_path"
chmod -R a+rX "$release_path"
ln -s "$release_path" /var/www/ph-dashboard/current.next
mv -Tf /var/www/ph-dashboard/current.next /var/www/ph-dashboard/current
nginx -t
systemctl enable --now nginx
systemctl reload nginx
rm -- "/tmp/ph-dashboard-$release_id.tar.gz"
REMOTE
echo "Deployed $release_id to $ssh_target"
