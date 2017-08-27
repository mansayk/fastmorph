#/bin/bash

declare -i begin=0	# Begin from
declare -i step=100000	# Increment by
declare -i max=2000000	# Amount of possible word forms
declare -i x=1		# Should not be 0
declare -i m=0

mode="tree" # tree, row

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

	echo "NEXT: $x"
	if [ "$mode" == "tree" ] ; then
		if [ -f "$output" ] ; then
			x=$(tail -n 1 "$output" | sed -r 's/^ *#Next free id:([1234567890]+)$/\1/')
		fi
		awk '{print $2 "," $3}' "$1" | ./parse -q -t -i $x -s $i -e $m | sed '/^ *$/d' >> "$output"
	else
		awk '{print $2 "," $3}' "$1" | ./parse -q -s $i -e $m | sed '/^ *$/d' >> "$output"
	fi

	#echo $i $m $max
done    

