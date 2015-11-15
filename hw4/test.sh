#!/bin/sh

outdir="logs"

mkdir -p $outdir
ref/memTest.debug -f tests/$1 > $outdir/$1-std.log 2>&1
bin/memTest.debug -f tests/$1 > $outdir/$1-cmp.log 2>&1
vimdiff $outdir/$1-std.log $outdir/$1-cmp.log


