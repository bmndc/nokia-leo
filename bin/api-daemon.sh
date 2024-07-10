#!/system/bin/sh
SERVICE_DIR=/data/local/service
API_DAEMON_DIR=${SERVICE_DIR}/api-daemon
API_REMOTE_DIR=${API_DAEMON_DIR}/remote

if [ ! -d ${API_DAEMON_DIR} ]; then
  mkdir -p ${API_DAEMON_DIR}
  cp -rf /system/kaios/http_root ${API_DAEMON_DIR}
  cp -f /system/kaios/api-daemon ${API_DAEMON_DIR}
  cp -f /system/kaios/config.toml ${API_DAEMON_DIR}
fi

if [ ! -d ${API_REMOTE_DIR} ]; then
  mkdir -p ${API_REMOTE_DIR}
  chmod 755 ${SERVICE_DIR}
  chmod 755 ${API_DAEMON_DIR}
  chmod 755 ${API_REMOTE_DIR}
fi

exec ${API_DAEMON_DIR}/api-daemon ${API_DAEMON_DIR}/config.toml
