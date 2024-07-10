#!/system/bin/sh
# Copyright (c) 2014, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#


panelname=`getprop ro.boot.panelname`
echo $panelname
case $panelname in
hx8394d_hvga_video)
  abs_x_axis=320
  abs_y_axis=480
  ;;
hx8394d_wvga_video)
  abs_x_axis=480
  abs_y_axis=800
  ;;
hx8394d_qhd_video)
  abs_x_axis=540
  abs_y_axis=960
  ;;
*)
  echo $0: Unknown panelname: $panelname
  log -t $0 Unknown panelname: $panelname
  exit
  ;;
esac

log -t $0 $panelname: $abs_x_axis x $abs_y_axis
# Loop through the sysfs nodes and determine
# the correct sysfs to write to
for count in 0 1 2 3 4 5 6 7 8
do
dir="/sys/bus/i2c/devices/5-0020/input/input"$count
  if [ -d "$dir" ]
  then
	echo $abs_x_axis > $dir/set_abs_x_axis
	echo $abs_y_axis > $dir/set_abs_y_axis
  fi
done

