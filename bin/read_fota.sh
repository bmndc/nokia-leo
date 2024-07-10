#!/system/bin/sh

/system/bin/qmi-framework-tests/qmi_ping_test 11 2497 0 | grep TA > /data/misc/radio/result

fota_info=$( cat /data/misc/radio/result)
rm -rf /data/misc/radio/result
fota_code=${fota_info:36:11}
ver_info=${fota_info:47:7}

setprop ro.fih.fota_code $fota_code
setprop ro.fih.ver_info $ver_info

