#! /bin/bash

username=$(whoami)

mypid=`ps -u ${username} | grep "keymngserver" | awk {'print $1'}`

if [ -z $mypid ]; then 
	echo "the keymnserver is not start"
	exit 1
fi
kill -SIGUSR1 $mypid
echo "the keymngserver is killed."
