#!/bin/sh

if [ -z ${report_file+x} ]; then
  report_file="aamp.csv"
fi


for approach in {0..6} ; do
  aamp -r ${report_file} --approach $approach
done

