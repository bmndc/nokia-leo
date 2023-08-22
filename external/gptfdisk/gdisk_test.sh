#!/bin/bash
# test gdisk and sgdisk by creating a dd file
# Copyright (C) 2011 Guillaume Delacour <gui@iroqwa.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#
# Requires: coreutils (mktemp, dd) and 64M of disk space in /tmp (temp dd disk)
#
# This script test gdisk commands through the following scenario:
# - Initialize a new GPT table
# - Create a single Linux partition
# - Change name of partition
# - Change type of partition
# - Backup to file the GPT table
# - Delete the single partition
# - Restore from backup file the GPT table
# - Wipe the GPT table

# TODO
# Try to generate a wrong GPT table to detect problems (test --verify)
# Create MBR partition table with fdisk and migrate it with gdisk

GDISK_BIN=./gdisk
SGDISK_BIN=./sgdisk

OPT_CLEAR="o"
OPT_NEW="n"
OPT_CHANGE_NAME="c"
OPT_CHANGE_TYPE="t"
OPT_BACKUP="b"
OPT_DELETE="d"
OPT_ZAP="z"

# temp disk for testing gdisk
TEMP_DISK=$(mktemp)
# 64 MiB
TEMP_DISK_SIZE=65536

# the test partition to create
TEST_PART_TYPE="8300"
TEST_PART_DEFAULT_NAME="Linux filesystem"

# newname for the partition
TEST_PART_NEWNAME=$(tr -dc "[:alpha:]" < /dev/urandom | head -c 8)
# and new type (swap for example)
TEST_PART_NEWTYPE="8200"

# GPT data backup to filename
GPT_BACKUP_FILENAME=$(mktemp)

# Pretty print string (Red if FAILED or green if SUCCESS)
# $1: string to pretty print
pretty_print() {
	if [ "$1" = "SUCCESS" ]
	then
		# green
		color="32"
	else
		# red
		color="31"
	fi

	printf "\033[0;${color}m**$1**\033[m $2\n"
}

# Verify that the partition exist and has the given type/name
# $1: Partition type to verify (ex.: 8300)
# $2: Partition name to verify (ex.: Linux filesystem)
# $3: Text to print
verify_part() {
	partition=$($GDISK_BIN -l $TEMP_DISK | tail -n 1)
	echo $partition | grep -q "$1[[:space:]]$2$"

	if [ $? -eq 0 ]
	then
		pretty_print "SUCCESS" "$3"
	else
		pretty_print "FAILED" "$3"
		exit 1
	fi
}


#####################################
# Get GUID of disk
#####################################
get_diskguid() {
	DISK_GUID=$($GDISK_BIN -l $TEMP_DISK | grep "^Disk identifier (GUID):" | awk '{print $4}')
	return $DISK_GUID
}


#####################################
# Create a new empty table
#####################################
create_table() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_CLEAR
Y
w
Y
EOF
		;;
		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_CLEAR}
		;;
	esac

	# verify that the table is empty
	# only the columns should appear in the table
	verify_part "Code" "Name" "Create new empty GPT table"
	echo ""
}



#####################################
# First create a new partition
#####################################
create_partition() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_NEW
1


$TEST_PART_TYPE
w
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_NEW} 1 -${OPT_CHANGE_NAME} 1:"${TEST_PART_DEFAULT_NAME}"
		;;
	esac

	verify_part "$TEST_PART_TYPE" "$TEST_PART_DEFAULT_NAME" "Create new partition"
	echo ""
}


#####################################
# Change name of partition
#####################################
change_partition_name() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_CHANGE_NAME
$TEST_PART_NEWNAME
w
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_CHANGE_NAME} 1:${TEST_PART_NEWNAME}
		;;
	esac

	verify_part "$TEST_PART_TYPE" "$TEST_PART_NEWNAME" "Change partition 1 name ($TEST_PART_DEFAULT_NAME -> $TEST_PART_NEWNAME)"
	echo ""
}


change_partition_type() {
#####################################
# Change type of partition
#####################################
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_CHANGE_TYPE
$TEST_PART_NEWTYPE
w
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_CHANGE_TYPE} 1:${TEST_PART_NEWTYPE}
		;;
	esac

	verify_part "$TEST_PART_NEWTYPE" "$TEST_PART_NEWNAME" "Change partition 1 type ($TEST_PART_TYPE -> $TEST_PART_NEWTYPE)"
	echo ""
}


#####################################
# Backup GPT data to file
#####################################
backup_table() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_BACKUP
$GPT_BACKUP_FILENAME
q
EOF
echo ""
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_BACKUP} ${GPT_BACKUP_FILENAME}
		;;
	esac

	# if exist and not empty; we will test it after
	if [ -s $GPT_BACKUP_FILENAME ]
	then
		pretty_print "SUCCESS" "GPT data backuped sucessfully"
	else
		pretty_print "FAILED" "Unable to create GPT backup file !"
		exit 1
	fi
}


#####################################
# Now, we can delete the partition
#####################################
delete_partition() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
$OPT_DELETE
w
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_DELETE} 1
		;;
	esac

	# verify that the table is empty (just one partition):
	# only the columns should appear in the table
	verify_part "Code" "Name" "Delete partition 1"
	echo ""
}


#####################################
# Restore GPT table
#####################################
restore_table() {
	$GDISK_BIN $TEMP_DISK << EOF
r
r
l
$GPT_BACKUP_FILENAME
w
Y
EOF

	verify_part "$TEST_PART_NEWTYPE" "$TEST_PART_NEWNAME" "Restore the GPT backup"
	echo ""
}


#####################################
# Change UID of disk
#####################################
change_disk_uid() {

	# get UID of disk before changing it
	GUID=get_diskguid


	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
x
g
R
w
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -U=R
		;;
	esac

	# get GUID after change
	NEW_DISK_GUID=get_diskguid

	# compare them
	if [ "$DISK_GUID" != "$NEW_DISK_GUID" ]
	then
		pretty_print "SUCCESS" "GUID of disk has been sucessfully changed"
	else
		pretty_print "FAILED" "GUID of disk is the same as the previous one"
	fi
}

#####################################
# Wipe GPT table
#####################################
wipe_table() {
	case $1 in
		gdisk)
			$GDISK_BIN $TEMP_DISK << EOF
x
$OPT_ZAP
Y
Y
EOF
		;;

		sgdisk)
			$SGDISK_BIN $TEMP_DISK -${OPT_ZAP}
	esac

	# verify that the table is empty (just one partition):
	# only the columns should appear in the table
	verify_part "Code" "Name" "Wipe GPT table"
	echo ""
}

#####################################
# Test stdin EOF
#####################################
eof_stdin() {
	$SGDISK_BIN $TEMP_DISK << EOF
^D
EOF
	pretty_print "SUCCESS" "EOF successfully exit gdisk"
}

###################################
# Main
###################################

# create a file to simulate a real device
dd if=/dev/zero of=$TEMP_DISK bs=1024 count=$TEMP_DISK_SIZE

if [ -s $TEMP_DISK ]
then
	pretty_print "SUCCESS" "Temp disk sucessfully created"
else
	pretty_print "FAILED" "Unable to create temp disk !"
	exit 1
fi

# test gdisk and sgdisk
for binary in gdisk sgdisk
do
	echo ""
	printf "\033[0;34m**Testing $binary binary**\033[m\n"
	echo ""
	create_table          "$binary"
	create_partition      "$binary"
	change_partition_name "$binary"
	change_partition_type "$binary"
	backup_table          "$binary"
	delete_partition      "$binary"
	restore_table         # only with gdisk
	change_disk_uid       "$binary"
	wipe_table            "$binary"
	eof_stdin             # only with gdisk
done

# remove temp files
rm -f $TEMP_DISK $GPT_BACKUP_FILENAME

exit 0
