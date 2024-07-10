#!/system/bin/sh

if [ ! -d /data/local/service/updater ]; then
  mkdir -p /data/local/service/updater
  cp -f /system/kaios/updater-daemon /data/local/service/updater/
  cp -f /system/kaios/updater.toml /data/local/service/updater/
  chmod 755 /data/local/service/updater/updater-daemon
fi
if [ -d /system/kaios/test-fixtures ]; then
  cp -rf /system/kaios/test-fixtures  /data/local/service/updater/
fi
if [ -f /system/kaios/updatercmd ]; then
  cp -f /system/kaios/updatercmd /data/local/service/updater/
  chmod 755 /data/local/service/updater/updatercmd
fi

exec /data/local/service/updater/updater-daemon /data/local/service/updater/updater.toml
