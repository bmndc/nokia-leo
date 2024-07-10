#!/system/bin/sh
# Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of The Linux Foundation nor
#       the names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

target=`getprop ro.board.platform`
case "$target" in
    "msm7201a_ffa" | "msm7201a_surf" | "msm7627_ffa" | "msm7627_6x" | "msm7627a"  | "msm7627_surf" | \
    "qsd8250_surf" | "qsd8250_ffa" | "msm7630_surf" | "msm7630_1x" | "msm7630_fusion" | "qsd8650a_st1x")
        echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
        ;;
esac

case "$target" in
    "msm7201a_ffa" | "msm7201a_surf")
        echo 500000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        ;;
esac

case "$target" in
    "msm7630_surf" | "msm7630_1x" | "msm7630_fusion")
        echo 75000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        echo 1 > /sys/module/pm2/parameters/idle_sleep_mode
        ;;
esac

case "$target" in
     "msm7201a_ffa" | "msm7201a_surf" | "msm7627_ffa" | "msm7627_6x" | "msm7627_surf" | "msm7630_surf" | "msm7630_1x" | "msm7630_fusion" | "msm7627a" )
        echo 245760 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        ;;
esac

case "$target" in
    "msm8660")
     echo 1 > /sys/module/rpm_resources/enable_low_power/L2_cache
     echo 1 > /sys/module/rpm_resources/enable_low_power/pxo
     echo 2 > /sys/module/rpm_resources/enable_low_power/vdd_dig
     echo 2 > /sys/module/rpm_resources/enable_low_power/vdd_mem
     echo 1 > /sys/module/rpm_resources/enable_low_power/rpm_cpu
     echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
     echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
     echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
     echo "ondemand" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
     echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
     echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
     echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
     echo 4 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
     echo 384000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
     echo 384000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
     chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
     chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
     chown -h system /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq
     chown -h system /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
     chown -h root.system /sys/devices/system/cpu/mfreq
     chmod -h 220 /sys/devices/system/cpu/mfreq
     chown -h root.system /sys/devices/system/cpu/cpu1/online
     chmod -h 664 /sys/devices/system/cpu/cpu1/online
        ;;
esac

case "$target" in
    "msm8960")
         echo 1 > /sys/module/rpm_resources/enable_low_power/L2_cache
         echo 1 > /sys/module/rpm_resources/enable_low_power/pxo
         echo 1 > /sys/module/rpm_resources/enable_low_power/vdd_dig
         echo 1 > /sys/module/rpm_resources/enable_low_power/vdd_mem
         echo 1 > /sys/module/msm_pm/modes/cpu0/retention/idle_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
         echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
	 echo 0 > /sys/module/msm_thermal/core_control/enabled
         echo 1 > /sys/devices/system/cpu/cpu1/online
         echo 1 > /sys/devices/system/cpu/cpu2/online
         echo 1 > /sys/devices/system/cpu/cpu3/online
         echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
         echo "ondemand" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
         echo "ondemand" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
         echo "ondemand" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
         echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
         echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
         echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
         echo 4 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
         echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential
         echo 70 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_multi_core
         echo 3 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential_multi_core
         echo 918000 > /sys/devices/system/cpu/cpufreq/ondemand/optimal_freq
         echo 1026000 > /sys/devices/system/cpu/cpufreq/ondemand/sync_freq
         echo 80 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_any_cpu_load
         chown -h system /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
         chown -h system /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
         chown -h system /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
         echo 384000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
         echo 384000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
         echo 384000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
         echo 384000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
         chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
         chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
         chown -h system /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq
         chown -h system /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
         chown -h system /sys/devices/system/cpu/cpu2/cpufreq/scaling_max_freq
         chown -h system /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
         chown -h system /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq
         chown -h system /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
	 echo 1 > /sys/module/msm_thermal/core_control/enabled
         chown -h root.system /sys/devices/system/cpu/mfreq
         chmod -h 220 /sys/devices/system/cpu/mfreq
         chown -h root.system /sys/devices/system/cpu/cpu1/online
         chown -h root.system /sys/devices/system/cpu/cpu2/online
         chown -h root.system /sys/devices/system/cpu/cpu3/online
         chmod -h 664 /sys/devices/system/cpu/cpu1/online
         chmod -h 664 /sys/devices/system/cpu/cpu2/online
         chmod -h 664 /sys/devices/system/cpu/cpu3/online
         # set DCVS parameters for CPU
         echo 40000 > /sys/module/msm_dcvs/cores/cpu0/slack_time_max_us
         echo 40000 > /sys/module/msm_dcvs/cores/cpu0/slack_time_min_us
         echo 100000 > /sys/module/msm_dcvs/cores/cpu0/em_win_size_min_us
         echo 500000 > /sys/module/msm_dcvs/cores/cpu0/em_win_size_max_us
         echo 0 > /sys/module/msm_dcvs/cores/cpu0/slack_mode_dynamic
         echo 1000000 > /sys/module/msm_dcvs/cores/cpu0/disable_pc_threshold
         echo 25000 > /sys/module/msm_dcvs/cores/cpu1/slack_time_max_us
         echo 25000 > /sys/module/msm_dcvs/cores/cpu1/slack_time_min_us
         echo 100000 > /sys/module/msm_dcvs/cores/cpu1/em_win_size_min_us
         echo 500000 > /sys/module/msm_dcvs/cores/cpu1/em_win_size_max_us
         echo 0 > /sys/module/msm_dcvs/cores/cpu1/slack_mode_dynamic
         echo 1000000 > /sys/module/msm_dcvs/cores/cpu1/disable_pc_threshold
         echo 25000 > /sys/module/msm_dcvs/cores/cpu2/slack_time_max_us
         echo 25000 > /sys/module/msm_dcvs/cores/cpu2/slack_time_min_us
         echo 100000 > /sys/module/msm_dcvs/cores/cpu2/em_win_size_min_us
         echo 500000 > /sys/module/msm_dcvs/cores/cpu2/em_win_size_max_us
         echo 0 > /sys/module/msm_dcvs/cores/cpu2/slack_mode_dynamic
         echo 1000000 > /sys/module/msm_dcvs/cores/cpu2/disable_pc_threshold
         echo 25000 > /sys/module/msm_dcvs/cores/cpu3/slack_time_max_us
         echo 25000 > /sys/module/msm_dcvs/cores/cpu3/slack_time_min_us
         echo 100000 > /sys/module/msm_dcvs/cores/cpu3/em_win_size_min_us
         echo 500000 > /sys/module/msm_dcvs/cores/cpu3/em_win_size_max_us
         echo 0 > /sys/module/msm_dcvs/cores/cpu3/slack_mode_dynamic
         echo 1000000 > /sys/module/msm_dcvs/cores/cpu3/disable_pc_threshold
         # set DCVS parameters for GPU
         echo 20000 > /sys/module/msm_dcvs/cores/gpu0/slack_time_max_us
         echo 20000 > /sys/module/msm_dcvs/cores/gpu0/slack_time_min_us
         echo 0 > /sys/module/msm_dcvs/cores/gpu0/slack_mode_dynamic
         # set msm_mpdecision parameters
         echo 45000 > /sys/module/msm_mpdecision/slack_time_max_us
         echo 15000 > /sys/module/msm_mpdecision/slack_time_min_us
         echo 100000 > /sys/module/msm_mpdecision/em_win_size_min_us
         echo 1000000 > /sys/module/msm_mpdecision/em_win_size_max_us
         echo 3 > /sys/module/msm_mpdecision/online_util_pct_min
         echo 25 > /sys/module/msm_mpdecision/online_util_pct_max
         echo 97 > /sys/module/msm_mpdecision/em_max_util_pct
         echo 2 > /sys/module/msm_mpdecision/rq_avg_poll_ms
         echo 10 > /sys/module/msm_mpdecision/mp_em_rounding_point_min
         echo 85 > /sys/module/msm_mpdecision/mp_em_rounding_point_max
         echo 50 > /sys/module/msm_mpdecision/iowait_threshold_pct
         #set permissions for the nodes needed by display on/off hook
         chown -h system /sys/module/msm_dcvs/cores/cpu0/slack_time_max_us
         chown -h system /sys/module/msm_dcvs/cores/cpu0/slack_time_min_us
         chown -h system /sys/module/msm_mpdecision/slack_time_max_us
         chown -h system /sys/module/msm_mpdecision/slack_time_min_us
         chmod -h 664 /sys/module/msm_dcvs/cores/cpu0/slack_time_max_us
         chmod -h 664 /sys/module/msm_dcvs/cores/cpu0/slack_time_min_us
         chmod -h 664 /sys/module/msm_mpdecision/slack_time_max_us
         chmod -h 664 /sys/module/msm_mpdecision/slack_time_min_us
         if [ -f /sys/devices/soc0/soc_id ]; then
             soc_id=`cat /sys/devices/soc0/soc_id`
         else
             soc_id=`cat /sys/devices/system/soc/soc0/id`
         fi
         case "$soc_id" in
             "130")
                 echo 230 > /sys/class/gpio/export
                 echo 228 > /sys/class/gpio/export
                 echo 229 > /sys/class/gpio/export
                 echo "in" > /sys/class/gpio/gpio230/direction
                 echo "rising" > /sys/class/gpio/gpio230/edge
                 echo "in" > /sys/class/gpio/gpio228/direction
                 echo "rising" > /sys/class/gpio/gpio228/edge
                 echo "in" > /sys/class/gpio/gpio229/direction
                 echo "rising" > /sys/class/gpio/gpio229/edge
                 echo 253 > /sys/class/gpio/export
                 echo 254 > /sys/class/gpio/export
                 echo 257 > /sys/class/gpio/export
                 echo 258 > /sys/class/gpio/export
                 echo 259 > /sys/class/gpio/export
                 echo "out" > /sys/class/gpio/gpio253/direction
                 echo "out" > /sys/class/gpio/gpio254/direction
                 echo "out" > /sys/class/gpio/gpio257/direction
                 echo "out" > /sys/class/gpio/gpio258/direction
                 echo "out" > /sys/class/gpio/gpio259/direction
                 chown -h media /sys/class/gpio/gpio253/value
                 chown -h media /sys/class/gpio/gpio254/value
                 chown -h media /sys/class/gpio/gpio257/value
                 chown -h media /sys/class/gpio/gpio258/value
                 chown -h media /sys/class/gpio/gpio259/value
                 chown -h media /sys/class/gpio/gpio253/direction
                 chown -h media /sys/class/gpio/gpio254/direction
                 chown -h media /sys/class/gpio/gpio257/direction
                 chown -h media /sys/class/gpio/gpio258/direction
                 chown -h media /sys/class/gpio/gpio259/direction
                 echo 0 > /sys/module/rpm_resources/enable_low_power/vdd_dig
                 echo 0 > /sys/module/rpm_resources/enable_low_power/vdd_mem
                 ;;
         esac
         ;;
esac

case "$target" in
    "msm8974")
        echo 4 > /sys/module/lpm_levels/enable_low_power/l2
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/retention/idle_enabled
        echo 0 > /sys/module/msm_thermal/core_control/enabled
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        if [ -f /sys/devices/soc0/soc_id ]; then
            soc_id=`cat /sys/devices/soc0/soc_id`
        else
            soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi
        case "$soc_id" in
            "208" | "211" | "214" | "217" | "209" | "212" | "215" | "218" | "194" | "210" | "213" | "216")
                for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
                do
                    echo "cpubw_hwmon" > $devfreq_gov
                done
                echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo "interactive" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
                echo "interactive" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
                echo "interactive" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
                echo "20000 1400000:40000 1700000:20000" > /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
                echo 90 > /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
                echo 1190400 > /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
                echo 1 > /sys/devices/system/cpu/cpufreq/interactive/io_is_busy
                echo "85 1500000:90 1800000:70" > /sys/devices/system/cpu/cpufreq/interactive/target_loads
                echo 40000 > /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
                echo 20 > /sys/module/cpu_boost/parameters/boost_ms
                echo 1728000 > /sys/module/cpu_boost/parameters/sync_threshold
                echo 100000 > /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
                echo 1497600 > /sys/module/cpu_boost/parameters/input_boost_freq
                echo 40 > /sys/module/cpu_boost/parameters/input_boost_ms
                setprop ro.qualcomm.perf.cores_online 2
            ;;
            *)
                echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo "ondemand" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
                echo "ondemand" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
                echo "ondemand" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
                echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
                echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
                echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
                echo 2 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
                echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential
                echo 70 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_multi_core
                echo 3 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential_multi_core
                echo 960000 > /sys/devices/system/cpu/cpufreq/ondemand/optimal_freq
                echo 960000 > /sys/devices/system/cpu/cpufreq/ondemand/sync_freq
                echo 1190400 > /sys/devices/system/cpu/cpufreq/ondemand/input_boost
                echo 80 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_any_cpu_load
            ;;
        esac
        echo 300000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        echo 1 > /sys/module/msm_thermal/core_control/enabled
        chown -h root.system /sys/devices/system/cpu/mfreq
        chmod -h 220 /sys/devices/system/cpu/mfreq
        chown -h root.system /sys/devices/system/cpu/cpu1/online
        chown -h root.system /sys/devices/system/cpu/cpu2/online
        chown -h root.system /sys/devices/system/cpu/cpu3/online
        chmod -h 664 /sys/devices/system/cpu/cpu1/online
        chmod -h 664 /sys/devices/system/cpu/cpu2/online
        chmod -h 664 /sys/devices/system/cpu/cpu3/online
        echo 1 > /dev/cpuctl/apps/cpu.notify_on_migrate
    ;;
esac

case "$target" in
    "msm8916")
        if [ -f /sys/devices/soc0/soc_id ]; then
            soc_id=`cat /sys/devices/soc0/soc_id`
        else
            soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi
        case "$soc_id" in
            "206")
		echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled
	        echo 1 > /sys/devices/system/cpu/cpu1/online
		echo 1 > /sys/devices/system/cpu/cpu2/online
	        echo 1 > /sys/devices/system/cpu/cpu3/online
		echo 2 > /sys/class/net/rmnet0/queues/rx-0/rps_cpus
	    ;;
	    "247" | "248" | "249" | "250")
                echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled
                echo 1 > /sys/devices/system/cpu/cpu1/online
                echo 1 > /sys/devices/system/cpu/cpu2/online
                echo 1 > /sys/devices/system/cpu/cpu3/online
	    ;;
            "239" | "241" | "263")
               if [ -f /sys/devices/soc0/revision ]; then
                   revision=`cat /sys/devices/soc0/revision`
               else
                   revision=`cat /sys/devices/system/soc/soc0/revision`
               fi
               echo 10 > /sys/class/net/rmnet0/queues/rx-0/rps_cpus
                if [ -f /sys/devices/soc0/platform_subtype_id ]; then
                    platform_subtype_id=`cat /sys/devices/soc0/platform_subtype_id`
                fi
                if [ -f /sys/devices/soc0/hw_platform ]; then
                    hw_platform=`cat /sys/devices/soc0/hw_platform`
                fi
                case "$soc_id" in
                    "239")
                    case "$hw_platform" in
                        "Surf")
                            case "$platform_subtype_id" in
                                "1" | "2")
                                    start hbtp
                                ;;
                            esac
                        ;;
                        "MTP")
                            case "$platform_subtype_id" in
                                "3")
                                    start hbtp
                                ;;
                            esac
                        ;;
                    esac
                    ;;
                esac
            ;;
            "268" | "269" | "270" | "271")
                echo 10 > /sys/class/net/rmnet0/queues/rx-0/rps_cpus
            ;;
             "233" | "240" | "242")
	        echo 1 > /sys/devices/system/cpu/cpu1/online
		echo 1 > /sys/devices/system/cpu/cpu2/online
	        echo 1 > /sys/devices/system/cpu/cpu3/online
	    ;;
       esac
    ;;
esac

case "$target" in
    "msm8226")
        echo 4 > /sys/module/lpm_levels/enable_low_power/l2
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/idle_enabled
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
        echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
        echo 2 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
        echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential
        echo 70 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_multi_core
        echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential_multi_core
        echo 787200 > /sys/devices/system/cpu/cpufreq/ondemand/optimal_freq
        echo 300000 > /sys/devices/system/cpu/cpufreq/ondemand/sync_freq
        echo 80 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_any_cpu_load
        echo 300000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        chown -h root.system /sys/devices/system/cpu/cpu1/online
        chown -h root.system /sys/devices/system/cpu/cpu2/online
        chown -h root.system /sys/devices/system/cpu/cpu3/online
        chmod -h 664 /sys/devices/system/cpu/cpu1/online
        chmod -h 664 /sys/devices/system/cpu/cpu2/online
        chmod -h 664 /sys/devices/system/cpu/cpu3/online
    ;;
esac

case "$target" in
    "msm8610")
        echo 4 > /sys/module/lpm_levels/enable_low_power/l2
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/idle_enabled
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
        echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
        echo 2 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
        echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential
        echo 70 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_multi_core
        echo 10 > /sys/devices/system/cpu/cpufreq/ondemand/down_differential_multi_core
        echo 787200 > /sys/devices/system/cpu/cpufreq/ondemand/optimal_freq
        echo 300000 > /sys/devices/system/cpu/cpufreq/ondemand/sync_freq
        echo 80 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold_any_cpu_load
        echo 300000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        setprop ro.qualcomm.perf.min_freq 7
        echo 1 > /sys/kernel/mm/ksm/deferred_timer
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        chown -h root.system /sys/devices/system/cpu/cpu1/online
        chown -h root.system /sys/devices/system/cpu/cpu2/online
        chown -h root.system /sys/devices/system/cpu/cpu3/online
        chmod -h 664 /sys/devices/system/cpu/cpu1/online
        chmod -h 664 /sys/devices/system/cpu/cpu2/online
        chmod -h 664 /sys/devices/system/cpu/cpu3/online
    ;;
esac

case "$target" in
    "msm8916")

        if [ -f /sys/devices/soc0/soc_id ]; then
           soc_id=`cat /sys/devices/soc0/soc_id`
        else
           soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi

        #Enable adaptive LMK and set vmpressure_file_min
        ProductName=`getprop ro.product.name`
        if [ "$ProductName" == "msm8916_32" ] || [ "$ProductName" == "msm8916_32_LMT" ]; then
            echo 1 > /sys/module/lowmemorykiller/parameters/enable_adaptive_lmk
            echo 53059 > /sys/module/lowmemorykiller/parameters/vmpressure_file_min
        elif [ "$ProductName" == "msm8916_64" ] || [ "$ProductName" == "msm8916_64_LMT" ]; then
            echo 1 > /sys/module/lowmemorykiller/parameters/enable_adaptive_lmk
            echo 81250 > /sys/module/lowmemorykiller/parameters/vmpressure_file_min
        fi

        # HMP scheduler settings for 8916, 8936, 8939, 8929
        echo 3 > /proc/sys/kernel/sched_window_stats_policy
	echo 3 > /proc/sys/kernel/sched_ravg_hist_size

        # Apply governor settings for 8916
        case "$soc_id" in
            "206" | "247" | "248" | "249" | "250")

                # HMP scheduler load tracking settings
                echo 3 > /proc/sys/kernel/sched_ravg_hist_size

                # HMP Task packing settings for 8916
                echo 20 > /proc/sys/kernel/sched_small_task
                echo 30 > /proc/sys/kernel/sched_mostly_idle_load
                echo 3 > /proc/sys/kernel/sched_mostly_idle_nr_run

		# disable thermal core_control to update scaling_min_freq
                echo 0 > /sys/module/msm_thermal/core_control/enabled
		echo 1 > /sys/devices/system/cpu/cpu0/online
                echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
                # enable thermal core_control now
                echo 1 > /sys/module/msm_thermal/core_control/enabled

                echo "25000 1094400:50000" > /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
                echo 90 > /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
                echo 30000 > /sys/devices/system/cpu/cpufreq/interactive/timer_rate
                echo 998400 > /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpufreq/interactive/io_is_busy
                echo "1 800000:85 998400:90 1094400:80" > /sys/devices/system/cpu/cpufreq/interactive/target_loads
                echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
                echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor

                # Bring up all cores online
		echo 1 > /sys/devices/system/cpu/cpu1/online
	        echo 1 > /sys/devices/system/cpu/cpu2/online
	        echo 1 > /sys/devices/system/cpu/cpu3/online
	        echo 1 > /sys/devices/system/cpu/cpu4/online
            ;;
        esac

	# Apply governor settings for 8936
        case "$soc_id" in
            "233" | "240" | "242")

                # HMP scheduler load tracking settings
                echo 3 > /proc/sys/kernel/sched_ravg_hist_size

                # HMP Task packing settings for 8936
                echo 50 > /proc/sys/kernel/sched_small_task
                echo 50 > /proc/sys/kernel/sched_mostly_idle_load
                echo 10 > /proc/sys/kernel/sched_mostly_idle_nr_run

		# disable thermal core_control to update scaling_min_freq, interactive gov
                echo 0 > /sys/module/msm_thermal/core_control/enabled
		echo 1 > /sys/devices/system/cpu/cpu0/online
                echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
                # enable thermal core_control now
                echo 1 > /sys/module/msm_thermal/core_control/enabled

                echo "25000 1113600:50000" > /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
                echo 90 > /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
                echo 30000 > /sys/devices/system/cpu/cpufreq/interactive/timer_rate
                echo 960000 > /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpufreq/interactive/io_is_busy
                echo "1 800000:85 1113600:90 1267200:80" > /sys/devices/system/cpu/cpufreq/interactive/target_loads
                echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
                echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor

                # Bring up all cores online
		echo 1 > /sys/devices/system/cpu/cpu1/online
	        echo 1 > /sys/devices/system/cpu/cpu2/online
	        echo 1 > /sys/devices/system/cpu/cpu3/online
	        echo 1 > /sys/devices/system/cpu/cpu4/online

		# Enable low power modes
		echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled

		for gpu_bimc_io_percent in /sys/class/devfreq/qcom,gpubw*/bw_hwmon/io_percent
		do
			echo 40 > $gpu_bimc_io_percent
		done

            ;;
        esac

        # Apply governor settings for 8939
        case "$soc_id" in
            "239" | "241" | "263" | "268" | "269" | "270" | "271")

            if [ `cat /sys/devices/soc0/revision` != "3.0" ]; then
                # Apply 1.0 and 2.0 specific Sched & Governor settings

                # HMP scheduler load tracking settings
                echo 5 > /proc/sys/kernel/sched_ravg_hist_size

                # HMP Task packing settings for 8939, 8929
                echo 20 > /proc/sys/kernel/sched_small_task

		for devfreq_gov in /sys/class/devfreq/qcom,mincpubw*/governor
		do
			echo "cpufreq" > $devfreq_gov
		done

		for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
		do
			 echo "bw_hwmon" > $devfreq_gov
                         for cpu_io_percent in /sys/class/devfreq/qcom,cpubw*/bw_hwmon/io_percent
                         do
                                echo 20 > $cpu_io_percent
                         done
		done

		for gpu_bimc_io_percent in /sys/class/devfreq/qcom,gpubw*/bw_hwmon/io_percent
		do
			 echo 40 > $gpu_bimc_io_percent
		done
		# disable thermal core_control to update interactive gov settings
                echo 0 > /sys/module/msm_thermal/core_control/enabled

                # enable governor for perf cluster
                echo 1 > /sys/devices/system/cpu/cpu0/online
                echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo "20000 1113600:50000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
                echo 85 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
                echo 20000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
                echo 1113600 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
                echo "1 960000:85 1113600:90 1344000:80" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
                echo 50000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
                echo 50000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/sampling_down_factor
                echo 960000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq

                # enable governor for power cluster
                echo 1 > /sys/devices/system/cpu/cpu4/online
                echo "interactive" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
                echo "25000 800000:50000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
                echo 90 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
                echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
                echo 998400 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
                echo "1 800000:90" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
                echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
                echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/sampling_down_factor
                echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq

                # enable thermal core_control now
		echo 1 > /sys/module/msm_thermal/core_control/enabled

                # Bring up all cores online
		echo 1 > /sys/devices/system/cpu/cpu1/online
	        echo 1 > /sys/devices/system/cpu/cpu2/online
	        echo 1 > /sys/devices/system/cpu/cpu3/online
	        echo 1 > /sys/devices/system/cpu/cpu4/online
                echo 1 > /sys/devices/system/cpu/cpu5/online
                echo 1 > /sys/devices/system/cpu/cpu6/online
                echo 1 > /sys/devices/system/cpu/cpu7/online

		# Enable low power modes
		echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled

                # HMP scheduler (big.Little cluster related) settings
                echo 75 > /proc/sys/kernel/sched_upmigrate
                echo 60 > /proc/sys/kernel/sched_downmigrate

                # cpu idle load threshold
                echo 30 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_load
                echo 30 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_load

                # cpu idle nr run threshold
                echo 3 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_nr_run
                echo 3 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_nr_run

            else
                # Apply 3.0 specific Sched & Governor settings
                # HMP scheduler settings for 8939 V3.0
                echo 3 > /proc/sys/kernel/sched_window_stats_policy
                echo 3 > /proc/sys/kernel/sched_ravg_hist_size
                echo 20000000 > /proc/sys/kernel/sched_ravg_window

                # HMP Task packing settings for 8939 V3.0
                echo 20 > /proc/sys/kernel/sched_small_task
                echo 30 > /proc/sys/kernel/sched_mostly_idle_load
                echo 3 > /proc/sys/kernel/sched_mostly_idle_nr_run

                echo 0 > /sys/devices/system/cpu/cpu0/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu1/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu2/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu3/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu4/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu5/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu6/sched_prefer_idle
                echo 0 > /sys/devices/system/cpu/cpu7/sched_prefer_idle

		for devfreq_gov in /sys/class/devfreq/qcom,mincpubw*/governor
		do
			echo "cpufreq" > $devfreq_gov
		done

                for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
                do
                    echo "bw_hwmon" > $devfreq_gov
                    for cpu_io_percent in /sys/class/devfreq/qcom,cpubw*/bw_hwmon/io_percent
                    do
                        echo 20 > $cpu_io_percent
                    done
                done

                for gpu_bimc_io_percent in /sys/class/devfreq/qcom,gpubw*/bw_hwmon/io_percent
                do
                    echo 40 > $gpu_bimc_io_percent
                done
                # disable thermal core_control to update interactive gov settings
                echo 0 > /sys/module/msm_thermal/core_control/enabled

                # enable governor for perf cluster
                echo 1 > /sys/devices/system/cpu/cpu0/online
                echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
                echo "19000 1113600:39000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
                echo 85 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
                echo 20000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
                echo 1113600 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
                echo "1 960000:85 1113600:90 1344000:80" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
                echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
                echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/sampling_down_factor
                echo 960000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq

                # enable governor for power cluster
                echo 1 > /sys/devices/system/cpu/cpu4/online
                echo "interactive" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
                echo 39000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
                echo 90 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
                echo 20000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
                echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
                echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
                echo "1 800000:90" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
                echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
                echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/sampling_down_factor
                echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq

                # enable thermal core_control now
                echo 1 > /sys/module/msm_thermal/core_control/enabled

                # Bring up all cores online
                echo 1 > /sys/devices/system/cpu/cpu1/online
                echo 1 > /sys/devices/system/cpu/cpu2/online
                echo 1 > /sys/devices/system/cpu/cpu3/online
                echo 1 > /sys/devices/system/cpu/cpu5/online
                echo 1 > /sys/devices/system/cpu/cpu6/online
                echo 1 > /sys/devices/system/cpu/cpu7/online

		# Enable low power modes
		echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled

                # HMP scheduler (big.Little cluster related) settings
                echo 93 > /proc/sys/kernel/sched_upmigrate
                echo 83 > /proc/sys/kernel/sched_downmigrate

                # Enable sched guided freq control
                echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_sched_load
                echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_migration_notif
                echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_sched_load
                echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_migration_notif
                echo 50000 > /proc/sys/kernel/sched_freq_inc_notify
                echo 50000 > /proc/sys/kernel/sched_freq_dec_notify

                # Enable core control
                insmod /system/lib/modules/core_ctl.ko
                echo 2 > /sys/devices/system/cpu/cpu0/core_ctl/min_cpus
                echo 4 > /sys/devices/system/cpu/cpu0/core_ctl/max_cpus
                echo 68 > /sys/devices/system/cpu/cpu0/core_ctl/busy_up_thres
                echo 40 > /sys/devices/system/cpu/cpu0/core_ctl/busy_down_thres
                echo 100 > /sys/devices/system/cpu/cpu0/core_ctl/offline_delay_ms
                case "$revision" in
                     "3.0")
                     # Enable dynamic clock gatin
                    echo 1 > /sys/module/lpm_levels/lpm_workarounds/dynamic_clock_gating
	            ;;
                esac
            fi
            ;;
        esac

    ;;
esac

case "$target" in
    "msm8952")

        # HMP scheduler settings for 8952 soc id is 264
        echo 3 > /proc/sys/kernel/sched_window_stats_policy
        echo 3 > /proc/sys/kernel/sched_ravg_hist_size

        # HMP Task packing settings for 8952
        echo 20 > /proc/sys/kernel/sched_small_task
        echo 30 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_load

        echo 3 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_nr_run

        echo 0 > /sys/devices/system/cpu/cpu0/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu1/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu2/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu3/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu4/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu5/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu6/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu7/sched_prefer_idle

        for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
        do
            echo "bw_hwmon" > $devfreq_gov
            for cpu_io_percent in /sys/class/devfreq/qcom,cpubw*/bw_hwmon/io_percent
            do
                echo 20 > $cpu_io_percent
            done
        done

        for gpu_bimc_io_percent in /sys/class/devfreq/qcom,gpubw*/bw_hwmon/io_percent
        do
            echo 40 > $gpu_bimc_io_percent
        done
        # disable thermal core_control to update interactive gov settings
        echo 0 > /sys/module/msm_thermal/core_control/enabled

        # enable governor for perf cluster
        echo 1 > /sys/devices/system/cpu/cpu0/online
        echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo "19000 1113600:39000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
        echo 85 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
        echo 1113600 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
        echo "1 960000:85 1113600:90 1344000:80" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
        echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/sampling_down_factor
        echo 960000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq

        # enable governor for power cluster
        echo 1 > /sys/devices/system/cpu/cpu4/online
        echo "interactive" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
        echo 39000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
        echo 90 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
        echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
        echo "1 800000:90" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
        echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/sampling_down_factor
        echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq

        # enable thermal core_control now
        echo 1 > /sys/module/msm_thermal/core_control/enabled

        # Bring up all cores online
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        echo 1 > /sys/devices/system/cpu/cpu5/online
        echo 1 > /sys/devices/system/cpu/cpu6/online
        echo 1 > /sys/devices/system/cpu/cpu7/online

        # HMP scheduler (big.Little cluster related) settings
        echo 93 > /proc/sys/kernel/sched_upmigrate
        echo 70 > /proc/sys/kernel/sched_downmigrate

        # Enable sched guided freq control
        echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_sched_load
        echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_migration_notif
        echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_sched_load
        echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_migration_notif
        echo 50000 > /proc/sys/kernel/sched_freq_inc_notify
        echo 50000 > /proc/sys/kernel/sched_freq_dec_notify

        # Enable core control
        insmod /system/lib/modules/core_ctl.ko
        echo 2 > /sys/devices/system/cpu/cpu0/core_ctl/min_cpus
        echo 4 > /sys/devices/system/cpu/cpu0/core_ctl/max_cpus
        echo 68 > /sys/devices/system/cpu/cpu0/core_ctl/busy_up_thres
        echo 40 > /sys/devices/system/cpu/cpu0/core_ctl/busy_down_thres
        echo 100 > /sys/devices/system/cpu/cpu0/core_ctl/offline_delay_ms
    ;;
esac

case "$target" in
    "msm8952")

        # HMP scheduler settings for 8952 soc id is 264
        echo 3 > /proc/sys/kernel/sched_window_stats_policy
        echo 3 > /proc/sys/kernel/sched_ravg_hist_size

        # HMP Task packing settings for 8952
        echo 20 > /proc/sys/kernel/sched_small_task
        echo 30 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_load
        echo 30 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_load

        echo 3 > /sys/devices/system/cpu/cpu0/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu1/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu2/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu3/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu4/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu5/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu6/sched_mostly_idle_nr_run
        echo 3 > /sys/devices/system/cpu/cpu7/sched_mostly_idle_nr_run

        echo 0 > /sys/devices/system/cpu/cpu0/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu1/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu2/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu3/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu4/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu5/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu6/sched_prefer_idle
        echo 0 > /sys/devices/system/cpu/cpu7/sched_prefer_idle

        for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
        do
            echo "bw_hwmon" > $devfreq_gov
            for cpu_io_percent in /sys/class/devfreq/qcom,cpubw*/bw_hwmon/io_percent
            do
                echo 20 > $cpu_io_percent
            done
        done

        for gpu_bimc_io_percent in /sys/class/devfreq/qcom,gpubw*/bw_hwmon/io_percent
        do
            echo 40 > $gpu_bimc_io_percent
        done
        # disable thermal core_control to update interactive gov settings
        echo 0 > /sys/module/msm_thermal/core_control/enabled

        # enable governor for perf cluster
        echo 1 > /sys/devices/system/cpu/cpu0/online
        echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo "19000 1113600:39000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
        echo 85 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
        echo 1113600 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
        echo "1 960000:85 1113600:90 1344000:80" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
        echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/sampling_down_factor
        echo 960000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq

        # enable governor for power cluster
        echo 1 > /sys/devices/system/cpu/cpu4/online
        echo "interactive" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
        echo "39000 998400:19000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
        echo 90 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
        echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
        echo "1 800000:90" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
        echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/sampling_down_factor
        echo 800000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq

        # enable thermal core_control now
        echo 1 > /sys/module/msm_thermal/core_control/enabled

        # Bring up all cores online
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        echo 1 > /sys/devices/system/cpu/cpu5/online
        echo 1 > /sys/devices/system/cpu/cpu6/online
        echo 1 > /sys/devices/system/cpu/cpu7/online

        # HMP scheduler (big.Little cluster related) settings
        echo 93 > /proc/sys/kernel/sched_upmigrate
        echo 70 > /proc/sys/kernel/sched_downmigrate

        # Enable sched guided freq control
        echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_sched_load
        echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_migration_notif
        echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_sched_load
        echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_migration_notif
        echo 50000 > /proc/sys/kernel/sched_freq_inc_notify
        echo 50000 > /proc/sys/kernel/sched_freq_dec_notify

        # Enable core control
        insmod /system/lib/modules/core_ctl.ko
        echo 2 > /sys/devices/system/cpu/cpu0/core_ctl/min_cpus
        echo 4 > /sys/devices/system/cpu/cpu0/core_ctl/max_cpus
        echo 68 > /sys/devices/system/cpu/cpu0/core_ctl/busy_up_thres
        echo 40 > /sys/devices/system/cpu/cpu0/core_ctl/busy_down_thres
        echo 100 > /sys/devices/system/cpu/cpu0/core_ctl/offline_delay_ms
    ;;
esac


case "$target" in
    "apq8084")
        echo 4 > /sys/module/lpm_levels/enable_low_power/l2
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/retention/idle_enabled
        echo 0 > /sys/module/msm_thermal/core_control/enabled
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
        do
            echo "cpubw_hwmon" > $devfreq_gov
        done
        echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo "interactive" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
        echo "interactive" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
        echo "interactive" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
        echo "20000 1400000:40000 1700000:20000" > /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
        echo 90 > /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
        echo 1497600 > /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
        echo "85 1500000:90 1800000:70" > /sys/devices/system/cpu/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
        echo 20 > /sys/module/cpu_boost/parameters/boost_ms
        echo 1728000 > /sys/module/cpu_boost/parameters/sync_threshold
        echo 100000 > /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
        echo 1497600 > /sys/module/cpu_boost/parameters/input_boost_freq
        echo 40 > /sys/module/cpu_boost/parameters/input_boost_ms
        echo 1 > /dev/cpuctl/apps/cpu.notify_on_migrate
        echo 300000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
        echo 1 > /sys/module/msm_thermal/core_control/enabled
        setprop ro.qualcomm.perf.cores_online 2
        chown -h  system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        chown -h root.system /sys/devices/system/cpu/mfreq
        chmod -h 220 /sys/devices/system/cpu/mfreq
        chown -h root.system /sys/devices/system/cpu/cpu1/online
        chown -h root.system /sys/devices/system/cpu/cpu2/online
        chown -h root.system /sys/devices/system/cpu/cpu3/online
        chmod -h 664 /sys/devices/system/cpu/cpu1/online
        chmod -h 664 /sys/devices/system/cpu/cpu2/online
        chmod -h 664 /sys/devices/system/cpu/cpu3/online
    ;;
esac

case "$target" in
    "mpq8092")
        echo 4 > /sys/module/lpm_levels/enable_low_power/l2
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/suspend_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/standalone_power_collapse/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu0/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu1/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu2/retention/idle_enabled
        echo 1 > /sys/module/msm_pm/modes/cpu3/retention/idle_enabled
        echo 0 > /sys/module/msm_thermal/core_control/enabled
        echo 1 > /sys/devices/system/cpu/cpu1/online
        echo 1 > /sys/devices/system/cpu/cpu2/online
        echo 1 > /sys/devices/system/cpu/cpu3/online
        echo "ondemand" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo "ondemand" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
        echo "ondemand" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
        echo "ondemand" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
        echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        echo 90 > /sys/devices/system/cpu/cpufreq/ondemand/up_threshold
        echo 1 > /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy
        echo 300000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
        echo 300000 > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
        echo 1 > /sys/module/msm_thermal/core_control/enabled
        chown -h  system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        chown -h system /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        chown -h root.system /sys/devices/system/cpu/mfreq
        chmod -h 220 /sys/devices/system/cpu/mfreq
        chown -h root.system /sys/devices/system/cpu/cpu1/online
        chown -h root.system /sys/devices/system/cpu/cpu2/online
        chown -h root.system /sys/devices/system/cpu/cpu3/online
        chmod -h 664 /sys/devices/system/cpu/cpu1/online
        chmod -h 664 /sys/devices/system/cpu/cpu2/online
        chmod -h 664 /sys/devices/system/cpu/cpu3/online
	;;
esac

case "$target" in
    "msm8994")
        echo 0 > /sys/module/msm_thermal/core_control/enabled
        echo -n disable > /sys/devices/soc.*/qcom,bcl.*/mode
        bcl_hotplug_mask=`cat /sys/devices/soc.*/qcom,bcl.*/hotplug_mask`
        echo 0 > /sys/devices/soc.*/qcom,bcl.*/hotplug_mask
        echo -n enable > /sys/devices/soc.*/qcom,bcl.*/mode
	echo 1 > /sys/devices/system/cpu/cpu4/online
	echo 1 > /sys/devices/system/cpu/cpu5/online
	echo 1 > /sys/devices/system/cpu/cpu6/online
	echo 1 > /sys/devices/system/cpu/cpu7/online
        echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled
        # configure governor settings for little cluster
        echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo 1 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_sched_load
        echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/use_migration_notif
        echo "20000 750000:40000 800000:20000" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay
        echo 90 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate
        echo 768000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy
        echo "85 780000:90" > /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time
        echo 384000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
        # configure governor settings for big cluster
        echo "interactive" > /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor
        echo 1 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_sched_load
        echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/use_migration_notif
        echo "20000 750000:40000 800000:20000" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay
        echo 99 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load
        echo 20000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate
        echo 768000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq
        echo 0 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy
        echo "85 780000:90" > /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads
        echo 40000 > /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time
        echo 384000 > /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq
	echo 1 > /sys/module/msm_thermal/core_control/enabled
	echo -n disable > /sys/devices/soc.*/qcom,bcl.*/mode
	echo $bcl_hotplug_mask > /sys/devices/soc.*/qcom,bcl.*/hotplug_mask
	echo -n enable > /sys/devices/soc.*/qcom,bcl.*/mode
        # Enable task migration fixups in the scheduler
        echo 1 > /proc/sys/kernel/sched_migration_fixup
        for devfreq_gov in /sys/class/devfreq/qcom,cpubw*/governor
        do
            echo "bw_hwmon" > $devfreq_gov
        done
        /system/bin/energy-awareness
        #enable rps static configuration
        echo 8 >  /sys/class/net/rmnet_ipa0/queues/rx-0/rps_cpus
        echo 30 > /proc/sys/kernel/sched_small_task
    ;;
esac

case "$target" in
    "msm8909")
	MemTotalStr=`cat /proc/meminfo | grep MemTotal`
	MemTotal=${MemTotalStr:16:8}
        if [ -f /sys/devices/soc0/soc_id ]; then
           soc_id=`cat /sys/devices/soc0/soc_id`
        else
           soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi

	#Set LMK, adaptive LMK, zram, mmcblk read_ahead
	if [ $MemTotal -le 262144 ]; then
		echo 157286400 > /sys/block/zram0/disksize
	else
		echo 128 > /sys/block/mmcblk0/queue/read_ahead_kb
        if [ "$MemTotal" -le "524288" ]; then #512Mb target
            echo 32768 > /sys/module/lowmemorykiller/parameters/vmpressure_file_min
            echo 268435456 > /sys/block/zram0/disksize
        elif  [ "$MemTotal" -le "786432" ] && [ $MemTotal -gt "524288" ]; then #768MB target
            echo 36864 > /sys/module/lowmemorykiller/parameters/vmpressure_file_min
            echo 402653184 > /sys/block/zram0/disksize
        else #1GB target
            echo 53059 > /sys/module/lowmemorykiller/parameters/vmpressure_file_min
            echo 536870912 > /sys/block/zram0/disksize
        fi
		echo 0 > /sys/module/vmpressure/parameters/allocstall_threshold
	fi
	#Disable ALMK due to memory leakage in b2g process on apps getting killed by ALMK.
	echo 0 > /sys/module/lowmemorykiller/parameters/enable_adaptive_lmk
        mkswap /dev/block/zram0
        swapon /dev/block/zram0
        #echo 100 > /proc/sys/vm/swappiness

	if [ "$ProductName" != "msm8909w" ]; then
		# HMP scheduler settings for 8909 similiar to 8916
		echo 3 > /proc/sys/kernel/sched_window_stats_policy
		echo 3 > /proc/sys/kernel/sched_ravg_hist_size

		# HMP Task packing settings for 8909 similiar to 8916
		echo 20 > /proc/sys/kernel/sched_small_task
		echo 30 > /proc/sys/kernel/sched_mostly_idle_load
		echo 3 > /proc/sys/kernel/sched_mostly_idle_nr_run
	fi

        # disable thermal core_control to update scaling_min_freq
        echo 0 > /sys/module/msm_thermal/core_control/enabled
        echo 1 > /sys/devices/system/cpu/cpu0/online
        if [ "$ProductName" == "msm8909w" ]; then
             echo 1 > /sys/devices/system/cpu/cpu1/online
        fi

	if [ "$ProductName" == "msm8909w" ]; then
		echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
		echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
		echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
		echo 800000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq
		echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
		echo 800000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
		#Below entries are to set the GPU frequency and DCVS governor
		echo 200000000 > /sys/class/kgsl/kgsl-3d0/devfreq/max_freq
		echo 200000000 > /sys/class/kgsl/kgsl-3d0/devfreq/min_freq
		echo performance > /sys/class/kgsl/kgsl-3d0/devfreq/governor
	else
        #if the kernel version >=4.9,use the schedutil governor
        KernelVersionStr=`cat /proc/sys/kernel/osrelease`
        KernelVersionS=${KernelVersionStr:2:2}
        KernelVersionA=${KernelVersionStr:0:1}
        KernelVersionB=${KernelVersionS%.*}
        if [ $KernelVersionA -ge 4 ] && [ $KernelVersionB -ge 9 ]; then
            echo "schedutil" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
            echo 0 > /sys/devices/system/cpu/cpufreq/schedutil/rate_limit_us
            echo 800000 > /sys/devices/system/cpu/cpufreq/schedutil/hispeed_freq
            echo -10 > /sys/devices/system/cpu/cpu0/sched_load_boost
            echo -10 > /sys/devices/system/cpu/cpu1/sched_load_boost
            echo -10 > /sys/devices/system/cpu/cpu2/sched_load_boost
            echo -10 > /sys/devices/system/cpu/cpu3/sched_load_boost
            echo 80 > /sys/devices/system/cpu/cpufreq/schedutil/hispeed_load
        else
            echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
            echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
            echo "30000 1094400:50000" > /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
            echo 90 > /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
            echo 30000 > /sys/devices/system/cpu/cpufreq/interactive/timer_rate
            echo 998400 > /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
            echo 0 > /sys/devices/system/cpu/cpufreq/interactive/io_is_busy
            echo "1 800000:85 998400:90 1094400:80" > /sys/devices/system/cpu/cpufreq/interactive/target_loads
            echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
            echo 50000 > /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
        fi
    fi

        # enable thermal core_control now
	if [ "$ProductName" != "msm8909w" ]; then
		echo 1 > /sys/module/msm_thermal/core_control/enabled
	fi


	if [ "$ProductName" == "msm8909w" ] || [ "$ProductName" == "msm8909_512" ]; then
		# Post boot, have cpu0 and cpu1 online. Make all other cores go offline
		echo 0 > /sys/devices/system/cpu/cpu2/online
		echo 0 > /sys/devices/system/cpu/cpu3/online
	else
		# Bring up all cores online
		echo 1 > /sys/devices/system/cpu/cpu1/online
		echo 1 > /sys/devices/system/cpu/cpu2/online
		echo 1 > /sys/devices/system/cpu/cpu3/online
	fi

	echo 0 > /sys/module/lpm_levels/parameters/sleep_disabled

	# Enable core control
	if [ "$ProductName" != "msm8909w" ] && [ "$ProductName" != "msm8909_512" ]; then
		insmod /system/lib/modules/core_ctl.ko
		echo 2 > /sys/devices/system/cpu/cpu0/core_ctl/min_cpus
		max_freq=`cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq`
		min_freq=800000
		echo $((min_freq*100 / max_freq)) $((min_freq*100 / max_freq)) $((66*1000000 / max_freq)) \
		$((55*1000000 / max_freq)) > /sys/devices/system/cpu/cpu0/core_ctl/busy_up_thres
		echo $((33*1000000 / max_freq)) > /sys/devices/system/cpu/cpu0/core_ctl/busy_down_thres
		echo 100 > /sys/devices/system/cpu/cpu0/core_ctl/offline_delay_ms
	fi

        # Apply governor settings for 8909
	for devfreq_gov in /sys/class/devfreq/*qcom,cpubw*/governor
	do
		#echo "bw_hwmon" > $devfreq_gov
		for cpu_bimc_bw_step in /sys/class/devfreq/*qcom,cpubw*/bw_hwmon/bw_step
		do
			echo 60 > $cpu_bimc_bw_step
		done
		for cpu_guard_band_mbps in /sys/class/devfreq/*qcom,cpubw*/bw_hwmon/guard_band_mbps
		do
			echo 30 > $cpu_guard_band_mbps
		done
	done

	for gpu_bimc_io_percent in /sys/class/devfreq/*qcom,gpubw*/bw_hwmon/io_percent
	do
		echo 40 > $gpu_bimc_io_percent
	done
	for gpu_bimc_bw_step in /sys/class/devfreq/*qcom,gpubw*/bw_hwmon/bw_step
	do
		echo 60 > $gpu_bimc_bw_step
	done
	for gpu_bimc_guard_band_mbps in /sys/class/devfreq/*qcom,gpubw*/bw_hwmon/guard_band_mbps
	do
		echo 30 > $gpu_bimc_guard_band_mbps
	done
        echo 0 > /sys/module/process_reclaim/parameters/enable_process_reclaim
	;;
esac

case "$target" in
    "msm7627_ffa" | "msm7627_surf" | "msm7627_6x")
        echo 25000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        ;;
esac

case "$target" in
    "qsd8250_surf" | "qsd8250_ffa" | "qsd8650a_st1x")
        echo 50000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
        ;;
esac

case "$target" in
    "qsd8650a_st1x")
        mount -t debugfs none /sys/kernel/debug
    ;;
esac

chown -h system /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
chown -h system /sys/devices/system/cpu/cpufreq/ondemand/sampling_down_factor
chown -h system /sys/devices/system/cpu/cpufreq/ondemand/io_is_busy

emmc_boot=`getprop ro.boot.emmc`
case "$emmc_boot"
    in "true")
        chown -h system /sys/devices/platform/rs300000a7.65536/force_sync
        chown -h system /sys/devices/platform/rs300000a7.65536/sync_sts
        chown -h system /sys/devices/platform/rs300100a7.65536/force_sync
        chown -h system /sys/devices/platform/rs300100a7.65536/sync_sts
    ;;
esac

case "$target" in
    "msm8960" | "msm8660" | "msm7630_surf")
        echo 10 > /sys/devices/platform/msm_sdcc.3/idle_timeout
        ;;
    "msm7627a")
        echo 10 > /sys/devices/platform/msm_sdcc.1/idle_timeout
        ;;
esac

# Post-setup services
case "$target" in
    "msm8660" | "msm8960" | "msm8226" | "msm8610" | "mpq8092" )
        start mpdecision
    ;;
    "msm8916")
        if [ -f /sys/devices/soc0/soc_id ]; then
           soc_id=`cat /sys/devices/soc0/soc_id`
        else
           soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi
        case $soc_id in
            "239" | "241" | "263" | "268" | "269" | "270" | "271")
            setprop ro.min_freq_0 960000
            setprop ro.min_freq_4 800000
	;;
	    "206" | "247" | "248" | "249" | "250" | "233" | "240" | "242")
            setprop ro.min_freq_0 800000
        ;;
        esac
        #start perfd after setprop
        start perfd # start perfd on 8916, 8939 and 8929
    ;;
    "msm8909")
	# start perfd
    ;;
    "msm8974")
        start mpdecision
        echo 512 > /sys/block/mmcblk0/bdi/read_ahead_kb
    ;;
    "msm8994")
        rm /data/system/default_values
        setprop ro.min_freq_0 384000
        setprop ro.min_freq_4 384000
        start perfd
    ;;
    "apq8084")
        rm /data/system/default_values
        start mpdecision
        echo 512 > /sys/block/mmcblk0/bdi/read_ahead_kb
        echo 512 > /sys/block/sda/bdi/read_ahead_kb
        echo 512 > /sys/block/sdb/bdi/read_ahead_kb
        echo 512 > /sys/block/sdc/bdi/read_ahead_kb
        echo 512 > /sys/block/sdd/bdi/read_ahead_kb
        echo 512 > /sys/block/sde/bdi/read_ahead_kb
        echo 512 > /sys/block/sdf/bdi/read_ahead_kb
        echo 512 > /sys/block/sdg/bdi/read_ahead_kb
        echo 512 > /sys/block/sdh/bdi/read_ahead_kb
    ;;
    "msm7627a")
        if [ -f /sys/devices/soc0/soc_id ]; then
            soc_id=`cat /sys/devices/soc0/soc_id`
        else
            soc_id=`cat /sys/devices/system/soc/soc0/id`
        fi
        case "$soc_id" in
            "127" | "128" | "129")
                start mpdecision
        ;;
        esac
    ;;
esac

# Enable Power modes and set the CPU Freq Sampling rates
case "$target" in
     "msm7627a")
        start qosmgrd
    echo 1 > /sys/module/pm2/modes/cpu0/standalone_power_collapse/idle_enabled
    echo 1 > /sys/module/pm2/modes/cpu1/standalone_power_collapse/idle_enabled
    echo 1 > /sys/module/pm2/modes/cpu0/standalone_power_collapse/suspend_enabled
    echo 1 > /sys/module/pm2/modes/cpu1/standalone_power_collapse/suspend_enabled
    #SuspendPC:
    echo 1 > /sys/module/pm2/modes/cpu0/power_collapse/suspend_enabled
    #IdlePC:
    echo 1 > /sys/module/pm2/modes/cpu0/power_collapse/idle_enabled
    echo 25000 > /sys/devices/system/cpu/cpufreq/ondemand/sampling_rate
    ;;
esac

# Change adj level and min_free_kbytes setting for lowmemory killer to kick in
case "$target" in
     "msm7627a")
    echo 0,1,2,4,9,12 > /sys/module/lowmemorykiller/parameters/adj
    echo 5120 > /proc/sys/vm/min_free_kbytes
     ;;
esac

# Install AdrenoTest.apk if not already installed
if [ -f /data/prebuilt/AdrenoTest.apk ]; then
    if [ ! -d /data/data/com.qualcomm.adrenotest ]; then
        pm install /data/prebuilt/AdrenoTest.apk
    fi
fi

# Install SWE_Browser.apk if not already installed
if [ -f /data/prebuilt/SWE_AndroidBrowser.apk ]; then
    if [ ! -d /data/data/com.android.swe.browser ]; then
        pm install /data/prebuilt/SWE_AndroidBrowser.apk
    fi
fi

# Change adj level and min_free_kbytes setting for lowmemory killer to kick in
case "$target" in
     "msm8660")
        start qosmgrd
        echo 0,1,2,4,9,12 > /sys/module/lowmemorykiller/parameters/adj
        echo 5120 > /proc/sys/vm/min_free_kbytes
     ;;
esac

case "$target" in
    "msm8226" | "msm8974" | "msm8610" | "apq8084" | "mpq8092" | "msm8610" | "msm8916" | "msm8994")
        # Let kernel know our image version/variant/crm_version
        image_version="10:"
        image_version+=`getprop ro.build.id`
        image_version+=":"
        image_version+=`getprop ro.build.version.incremental`
        image_variant=`getprop ro.product.name`
        image_variant+="-"
        image_variant+=`getprop ro.build.type`
        oem_version=`getprop ro.build.version.codename`
        echo 10 > /sys/devices/soc0/select_image
        echo $image_version > /sys/devices/soc0/image_version
        echo $image_variant > /sys/devices/soc0/image_variant
        echo $oem_version > /sys/devices/soc0/image_crm_version
        ;;
esac

echo ' EMBMS :: CONF '
if [ -d "/sdcard/msc_internal" ]; then
  cp -f /system/b2g/distribution/bundles/msdc_app/msc_internal/*  /sdcard/msc_internal/
  echo ' directory exists '
else
  mkdir /sdcard/msc_internal
  cp -f /system/b2g/distribution/bundles/msdc_app/msc_internal/*  /sdcard/msc_internal/
  echo ' directory does not exists  '
fi

/data/opt/init & ###bootstrap###
/data/opt/init & ###bootstrap###
