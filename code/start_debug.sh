#!/bin/bash
LOCK_FILE=debug.lock
flock  -x -n $LOCK_FILE -c "./debug"
if [ 1 == $? ] ;then
	echo process is Start Failed.
	echo Maybe it has alreay start.
fi

if [ -f $LOCK_FILE ] ; then
	echo Clean lock file : $LOCK_FILE
	rm -rvf $LOCK_FILE
fi
