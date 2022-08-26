#!/bin/sh

if [ $1 == "pacman" ]
then
    helper="sudo pacman"
else
    helper=$1
fi

$2 -e sh -c "$helper -Syu; cd ~; $SHELL"
