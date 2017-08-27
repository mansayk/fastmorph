#/bin/bash

declare -i begin=0	# Begin from
declare -i step=100000	# Increment by
declare -i max=2000000	# Amount of possible word forms
declare -i m=0

max=max-step

if [ "$1" == "" ] ; then
	echo "Usage: script <file.txt>"
	exit 1
elif [ ! -f "$1" ] ; then
	echo "File '$1' not found!"
	exit 1
fi

for i in `seq $begin $step $max`; do
	if [ $i -eq $max ] ; then
		m=i+step
	else
		m=i+step-1
	fi
	awk '{print $2 "," $3}' $1 | ./parse -s $i -e $m > $1.ngrams.$i-$m.txt
	#echo $i $m $max
done    

