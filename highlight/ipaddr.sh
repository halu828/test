#!/bin/bash
ipaddr=$(LANG=C ifconfig eth0		|
		grep "inet addr"	|
		awk '{print $2}'	|
		sed 's/addr://'		)
echo $ipaddr
./proxy -1 $ipaddr 10080
exit 0
