#!/bin/sh

outdir="logs"

stdfile=$1-std.log
cmpfile=$1-cmp.log

mkdir -p $outdir
echo "testing ref..."
ref/taskMgr -f dofiles/$1 > $outdir/$stdfile 2>&1
echo "testing cmp..."
bin/taskMgr -f dofiles/$1 > $outdir/$cmpfile 2>&1
echo "diffing..."
vimdiff $outdir/$stdfile $outdir/$cmpfile
