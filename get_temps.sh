#!/bin/sh

temper1/temper1 -C temper1/temper1.conf | \
awk -F "\"*,\"*" '{ print $1","$2 >> "public_html/temp."$3".csv"}'

