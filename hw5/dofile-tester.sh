#!/bin/sh

outdir="logs"

stdfile=$1-$2-std.log
cmpfile=$1-$2-cmp.log

mkdir -p $outdir
ref/adtTest.$2 -f tests/$1 > $outdir/$stdfile 2>&1
bin/adtTest.$2 -f tests/$1 > $outdir/$cmpfile 2>&1
vimdiff $outdir/$stdfile $outdir/$cmpfile

