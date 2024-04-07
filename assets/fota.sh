#!/bin/sh

usage () {
  echo "error: $1"
  echo "Usage: $0 COMMAND CUREF VERSION [TO_VERSION FWID] [ENV]"
  echo " - TO_VERSION and FWID are required when COMMAND is 'download'."
  echo " - ENV can be a URL or one of: prod, test, dev"
  exit 1
}

id=0123456780123456
[ $# -ge 1 ] || usage "Missing parameter: COMMAND"
cmd=$1
shift
[ $# -ge 1 ] || usage "Missing parameter: CUREF"
cu=$1
shift
[ $# -ge 1 ] || usage "Missing parameter: VERSION"
fv=$1
shift
env=prod

get_env () {
  if [ -n "$1" ]; then
    env=$1
  fi
  case "$env" in
    prod)
      env="http://mst.fota.kaiostech.com"
      ;;
    test)
      env="http://mst.test.kaiostech.com"
      ;;
    dev)
      env="http://mst.fota.dev.kaiostech.com"
      ;;
    http://*|https://*)
      ;;
    *)
      echo "Invalid environment $env"
      exit 1
      ;;
  esac
}

f_check () {
  local data tv fwid
  data=$(curl -s "$env/check.php?id=$id&cltp=1&fv=$fv&mode=2&type=FIRMWARE&curef=$cu")
  tv=$(echo "$data" | grep -o '<TV>[^<]*</TV>' | sed 's/<[^>]*>//g')
  fwid=$(echo "$data" | grep -o '<FW_ID>[^<]*</FW_ID>' | sed 's/<[^>]*>//g')
  echo "$cu: $fv: ${tv:+"$tv ($fwid)"}"
}
f_download () {
  local data slv url
  data=$(echo "id=$id&cltp=1&fv=$fv&tv=$tv&mode=2&type=FIRMWARE&curef=$cu&salt=1&vk=1&fw_id=$fwid" | curl -d@- -X POST -s "$env/download_request.php")
  url=$(echo "$data" | grep -o '<DOWNLOAD_URL>[^<]*</DOWNLOAD_URL>' | sed 's/<[^>]*>//g')
  slv=$(echo "$data" | grep -o '<SLAVE>[^<]*</SLAVE>' | sed 's/<[^>]*>//g')
  echo "http://$slv$url"
}

case "$cmd" in
  check)
    get_env "$1"
    f_check
    ;;
  download)
    tv=$1
    [ $# -ge 1 ] || usage "Missing parameter: TO_VERSION"
    shift
    [ $# -ge 1 ] || usage "Missing parameter: FWID"
    fwid=$1
    shift
    get_env "$1"
    f_download
    ;;
  *)
    echo "Unknown command: $1"
    exit 1
    ;;
esac
