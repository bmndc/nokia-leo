#!/bin/bash
# Taken and modified from https://unix.stackexchange.com/a/415155
function select_option {
    ESC=$( printf "\e")
    cursor_blink_on()  { printf "\e[?25h"; }
    cursor_to()        { printf "\e[$1;${2:-1}H"; }
    get_cursor_row()   { IFS=';' read -sdR -p $'\E[6n' ROW COL; echo ${ROW#*[}; }
    key_input()        { read -s -n3 key 2>/dev/null >&2
                         if [[ $key = $ESC[A ]]; then echo up;    fi
                         if [[ $key = $ESC[B ]]; then echo down;  fi
                         if [[ $key = ""     ]]; then echo enter; fi; }
    # initially print empty new lines (scroll down if at bottom of screen)
    for opt; do printf "\n"; done
    # determine current screen position for overwriting the options
    local lastrow=`get_cursor_row`
    local startrow=$(($lastrow - $#))
    # ensure cursor and input echoing back on upon a ctrl+c during read -s
    trap "cursor_blink_on; stty echo; printf '\n'; exit" 2
    printf "\e[?25l";

    local selected=0
    while true; do 
        # print options by overwriting the last lines
        local idx=0
        for opt; do
            cursor_to $(($startrow + $idx))
            if [ $idx -eq $selected ]; then
                printf "> \e[34m$opt";
            else
                printf "  \e[0m$opt";
            fi
            ((idx++))
        done
        case `key_input` in 
            enter)  break;;
            up)     ((selected--));
                    if [ $selected -lt 0 ]; then selected=$(($# - 1)); fi;;
            down)   ((selected++));
                    if [ $selected -ge $# ]; then selected=0; fi;;
        esac
    done
    # cursor position back to normal
    cursor_to $lastrow
    printf "\n\e[?25h";
    return $selected
}
function select_opt {
    select_option "$@" 1>&2
    local result=$?
    echo $result
    return $result
    echo -en "\e[0m"
}
# Prompt user confirmation before proceeding, kill script if user didn't confirm
echo ":: Begin boot partition patching process"
echo "Make sure you've backed up all your data before proceeding! See https://wiki.bananahackers.net/backup for details."
echo -e "\e[0;31mWARNING: This process will void your warranty and disable system functions. See https://github.com/minhduc-bui1/nokia-leo for information. I will not be responsible for any damages or loss from now on. Proceed?"
case `select_opt "Exit program" "I understand"` in
    0)
        echo "\e[0mThe process was aborted. Run this script again when you're ready."
        exit -1;;
    1)
        echo -en "\e[0m";;
esac
# Interactive menu for choosing AUR helpers to install dependencies
echo ":: Setting up environment for EDL tools"
echo "Which AUR helpers do you want to install 'edl-git' and 'abootimg' with?"
case `select_opt "yay" "pamac" "Build manually" "Exit program"` in
    0)
        echo ":: Building and installing 'edl-git' and 'abootimg' from AUR (yay)"
        yay -S edl-git abootimg;;
    1)
        echo ":: Building and installing 'edl-git' and 'abootimg' from AUR (pamac)"
        pamac build edl-git abootimg;;
    2)
        echo ":: Installing necessary development tools"
        sudo pacman -S base-devel
        git clone https://aur.archlinux.org/edl-git.git && cd edl-git && makepkg -si
        echo ":: Building and installing 'edl-git' directly from AUR"
        sudo pacman -U edl-git-*.pkg.tar.zst
        echo ":: Building and installing 'abootimg' directly from AUR"
        cd .. && git clone https://aur.archlinux.org/abootimg.git && cd abootimg && makepkg -si
        sudo pacman -U abootimg-*.pkg.tar.zst;;
    3)
        echo "The process was aborted. Run this script again when you're ready."
        exit -1;;
esac
echo ":: Installing Android Debug Bridge and downloading additional files"
sudo pacman -S android-tools
curl -L -o adbd https://gitlab.com/suborg/8k-boot-patcher/-/raw/master/adbd
curl -L -o slua https://gitlab.com/suborg/8k-boot-patcher/-/raw/master/slua
curl -L -o sluac https://gitlab.com/suborg/8k-boot-patcher/-/raw/master/sluac
echo ":: Attempting to reboot your phone into EDL mode"
read -rsn1 -p "Connect your phone to the computer and press any key when you're ready."
adb reboot edl
echo ":: Pulling the boot partition"
edl r boot boot.img
echo ":: Patching the boot partition"
# assume we have image dir mounted to /image and image/boot.img present
cd /image
cp boot.img boot-orig.img
echo 'Boot image found, patching...'
abootimg -x boot.img
mkdir initrd
cd initrd
cat ../initrd.img | gunzip | cpio -vid
# initrd root patch process start
cp ../adbd ./sbin/
cp ../slua ./sbin/
cp ../sluac ./sbin/
sed -i 's/ro\.secure.*/ro.secure=0/' ./default.prop
sed -i 's/ro\.debuggable.*/ro.debuggable=1/' ./default.prop
sed -i 's/.*perf_harden.*/security.perf_harden=0/' ./default.prop
sed -i '/.*reload_policy.*/d' ./init.rc
echo 'setenforce 0' >> ./init.qcom.early_boot.sh
echo 'echo -n 1 > /data/enforce' >> ./init.qcom.early_boot.sh
echo 'mount -o bind /data/enforce /sys/fs/selinux/enforce' >> ./init.qcom.early_boot.sh
# initrd root patch process end
find . | cpio --create --format='newc' | gzip > ../myinitrd.img
cd ..
# bootimg.cfg patch process start
sed -i 's/^cmdline.*/& androidboot.selinux=permissive enforcing=0/' bootimg.cfg
# bootimg.cfg patch process end
abootimg -u boot.img -f bootimg.cfg -r myinitrd.img
rm -rf *initrd* bootimg.cfg zImage
echo 'Boot image patched!'
# ...Nothing can stop an idea whose time has come
echo ":: Writing the modified boot partition and resetting"
edl w boot boot.img
edl reset
echo ":: Done!"
exit 0