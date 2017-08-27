#/bin/bash

declare -i begin=0	# Begin from
declare -i step=100000	# Increment by
declare -i max=2000000	# Amount of possible word forms
declare -i m=0

max=max-step
output="$1.ngrams.txt"

if [ "$1" == "" ] ; then
	echo "Usage: script <file.txt>"
	exit 1
elif [ ! -f "$1" ] ; then
	echo "File '$1' not found!"
	exit 1
fi

if [ -f "$output" ] ; then
	echo "Removing '$output'..."
	rm -f "$output"
fi

for i in `seq $begin $step $max`; do
	if [ $i -eq $max ] ; then
		m=i+step
	else
		m=i+step-1
	fi
	#awk '{print $2 "," $3}' $1 | ./parse -s $i -e $m > $1.ngrams.$i-$m.txt
	awk '{print $2 "," $3}' "$1" | ./parse -s $i -e $m | sed '/^ *$/d' >> "$output"
	#echo $i $m $max
done    

