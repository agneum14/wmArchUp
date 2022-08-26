#!/bin/sh

if [ $# -eq 1 ]
then
    helper=$1
else
    helper='sudo pacman'
fi

$TERM -e sh -c "$helper -Syu; cd ~; $SHELL"
