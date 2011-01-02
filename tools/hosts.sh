#!/bin/bash
exec</etc/hosts
while read line
do
  if [[ `expr match "$line" .*goog.*` -gt 0 ]]
  then
	if [[ "${line:0:1}" = "#" ]] 
        then
	     echo "$line"
        else
             echo "#$line"
	fi
  else
	echo "${line}"
  fi
done
exit
