#!/system/bin/sh
main_mic_loop_test()
{
	echo 'mic loop test'
	tinymix 'DEC1 MUX' 'ADC1'
	tinymix 'ADC1_INP1 Switch' '1'
	tinymix 'ADC1 Volume' '6'
	tinymix 'DEC1 Volume' '84'
	tinymix 'RDAC2 MUX' 'RX1'
	tinymix 'IIR1 INP1 MUX' 'DEC1'
	tinymix 'RX3 MIX1 INP1' 'IIR1'
	tinymix 'RX1 Digital Volume' '120'
	tinymix 'SPK' 'Switch'
	tinymix 'Loopback MCLK' 'ENABLE'
}

stop_main_mic_loop_test(){
	echo 'stop mic loop test'
	tinymix 'DEC1 MUX' 'ZERO'
	tinymix 'ADC1 Volume' '4'
	tinymix 'ADC1_INP1 Switch' '0'
	tinymix 'RDAC2 MUX' 'ZERO'
	tinymix 'IIR1 INP1 MUX' 'ZERO'
	tinymix 'RX3 MIX1 INP1' 'ZERO'
	tinymix 'SPK' 'ZERO'
	tinymix 'RX1 Digital Volume' '84'
	tinymix 'Loopback MCLK' 'DISABLE'
}

sub_mic_loop_test()
{
	echo 'sub-mic loop test'
	tinymix 'DEC1 MUX' 'ADC3'
	tinymix 'ADC2 MUX' 'INP3'
	tinymix 'RDAC2 MUX' 'RX1'
	tinymix 'IIR1 INP1 MUX' 'DEC1'
	tinymix 'RX3 MIX1 INP1' 'IIR1'
	tinymix 'RX1 Digital Volume' '120'
	tinymix 'SPK' 'Switch'
	tinymix 'Loopback MCLK' 'ENABLE'
}

stop_sub_mic_loop_test(){
	echo 'stop sub-mic loop test'
	tinymix 'DEC1 MUX' 'ZERO'
	tinymix 'ADC2 MUX' 'ZERO'
	tinymix 'RDAC2 MUX' 'ZERO'
	tinymix 'IIR1 INP1 MUX' 'ZERO'
	tinymix 'RX3 MIX1 INP1' 'ZERO'
	tinymix 'RX1 Digital Volume' '84'
	tinymix 'SPK' 'ZERO'
	tinymix 'Loopback MCLK' 'DISABLE'
}


headset_mic_loop_test()
{
	echo 'headset_mic_loop_test'
	tinymix 'ADC2 MUX' 'INP2'
	tinymix 'DEC1 MUX' 'ADC2'
	tinymix 'IIR1 INP1 MUX' 'DEC1'
	tinymix 'RX1 MIX1 INP1' 'IIR1'
	tinymix 'RX2 MIX1 INP1' 'IIR1'
	tinymix 'RDAC2 MUX' 'RX2'
	tinymix 'HPHL' 'Switch'
	tinymix 'HPHR' 'Switch'
	tinymix "ADC2 Volume" 40
	tinymix "DEC1 Volume" 72
	tinymix 'RX1 Digital Volume' '78'
	tinymix 'RX2 Digital Volume' '78'
	tinymix 'Loopback MCLK' 'ENABLE'
}

stop_headset_mic_loop_test(){
	echo 'stop_headset_mic_loop_test'
	tinymix 'ADC2 MUX' 'ZERO'
	tinymix 'DEC1 MUX' 'ZERO'
	tinymix 'IIR1 INP1 MUX' 'ZERO'
	tinymix 'RX1 MIX1 INP1' 'ZERO'
	tinymix 'RX2 MIX1 INP1' 'ZERO'
	tinymix 'RDAC2 MUX' 'ZERO'
	tinymix 'HPHL' 'ZERO'
	tinymix 'HPHR' 'ZERO'
	tinymix "ADC2 Volume" 4
	tinymix "DEC1 Volume" 84
	tinymix 'Loopback MCLK' 'DISABLE'
}

enable_headphone(){
	echo "manually enable headphone RX channel"
	tinymix 'SPK' 'ZERO'
	tinymix 'MI2S_RX Channels' 'Two'
	tinymix 'RX1 MIX1 INP1' 'RX2'
	tinymix 'RX2 MIX1 INP1' 'RX2'
	tinymix 'RX1 Digital Volume' '75'
	tinymix 'RX2 Digital Volume' '75'
	tinymix 'RDAC2 MUX' 'RX2'
	tinymix 'HPHL' 'Switch'
	tinymix 'HPHR' 'Switch'
}

disable_headphone(){
	echo "manually disable headphne RX channel"
        tinymix 'SPK' 'Switch'
        tinymix 'MI2S_RX Channels' 'One'
        tinymix 'RX1 MIX1 INP1' 'ZERO'
        tinymix 'RX2 MIX1 INP1' 'ZERO'
        tinymix 'RX1 Digital Volume' '84'
        tinymix 'RX2 Digital Volume' '84'
        tinymix 'RDAC2 MUX' 'ZERO'
        tinymix 'HPHL' 'ZERO'
        tinymix 'HPHR' 'ZERO'

}

if [ $1 == 'mic' ];then
	main_mic_loop_test
fi
if [ $1 == 'stop-mic' ];then
	stop_main_mic_loop_test
fi
if [ $1 == 'sub-mic' ];then
	sub_mic_loop_test
fi
if [ $1 == 'stop-sub-mic' ];then
	stop_sub_mic_loop_test
fi

if [ $1 == 'headset-mic' ];then
	headset_mic_loop_test
fi

if [ $1 == 'stop-headset-mic' ];then
	stop_headset_mic_loop_test
fi

if [ $1 == 'enable-headphone' ];then
        enable_headphone
fi

if [ $1 == 'disable-headphone' ];then
        disable_headphone
fi

if [ $1 == 'headset-left' ];then
        echo $$ > /data/audio_test.pid
	tinymix "SPK" "Switch"
	tinymix "MI2S_RX Channels"  Two
	tinymix "RX1 MIX1 INP1" RX1
	tinymix "RX2 MIX1 INP1" RX2
       tinymix 'RX1 Digital Volume' '70'
       tinymix 'RX2 Digital Volume' '70'
	tinymix "RDAC2 MUX" RX2
	tinymix "HPHL" Switch
	tinymix "HPHR" ZERO
	tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 1
	while [ 1 ];do
		tinyplay /vendor/112.wav -d 0
	done
fi

if [ $1 == 'headset-left-stop' ];then
        if [ -f /data/audio_test.pid ];then
	   pid=`cat /data/audio_test.pid`
	   kill -9 $pid
	   rm /data/audio_test.pid
	fi
	tinymix "MI2S_RX Channels" One 
	tinymix "RX1 MIX1 INP1" 0
	tinymix "RX2 MIX1 INP1" 0
	tinymix "RDAC2 MUX" 0
	tinymix "HPHL" 0
	tinymix "HPHR" 0
	tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 0
	tinymix "SPK" "Switch"
fi

if [ $1 == 'headset-right' ];then
        echo $$ > /data/audio_test.pid
	tinymix "SPK" "ZERO" 
	tinymix "MI2S_RX Channels"  Two
	tinymix "RX1 MIX1 INP1" RX1
	tinymix "RX2 MIX1 INP1" RX2
       tinymix 'RX1 Digital Volume' '70'
       tinymix 'RX2 Digital Volume' '70'
	tinymix "RDAC2 MUX" RX2
	tinymix "HPHL" 0
	tinymix "HPHR" Switch
	tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 1
	while [ 1 ];do
		tinyplay /vendor/112.wav -d 0
	done
fi

if [ $1 == 'headset-right-stop' ];then
        if [ -f /data/audio_test.pid ];then
	   pid=`cat /data/audio_test.pid`
	   kill -9 $pid
	   rm /data/audio_test.pid
	fi
	tinymix "MI2S_RX Channels" One 
	tinymix "RX1 MIX1 INP1" 0
	tinymix "RX2 MIX1 INP1" 0
	tinymix "RDAC2 MUX" 0
	tinymix "HPHL" 0
	tinymix "HPHR" 0
	tinymix "PRI_MI2S_RX Audio Mixer MultiMedia1" 0
	tinymix "SPK" "Switch"
fi
