#/bin/bash

echo "Don't forget to set up additional swap if your RAM is not enough:"
echo " dd if=/dev/zero of=/mnt/DATA700/swap bs=1G count=6"
echo " mkswap /mnt/DATA700/swap"
echo " swapon /mnt/DATA700/swap"
read -p "Press enter to continue"

declare -i begin=30	# Begin from
declare -i step=100000	# Increment by
declare -i max=6000000	# Amount of possible united elements
declare -i i=1		# Should not be 0
declare -i e=0

mode="tree" # tree, row

max=$((max - step))
basename=$(basename -s ".txt.bz2" $1)
output="${basename}.ngrams.txt"
output_sorted="${basename}.ngrams.sorted.txt"

if [[ "$1" == "" ]] ; then
	echo "Usage: script <file.bz2>"
	exit 1
elif [[ ! -f "$1" ]] ; then
	echo "File '$1' not found!"
	exit 1
fi

if [[ -f "$output" ]] ; then
	echo "Removing '$output'..."
	rm -f "$output"
fi

# Adaptation for my low resource maschine
for s in `seq 0 2 29`; do
	if [[ $i -eq 29 ]] ; then
		e=$((s + 2))
	else
		e=$((s + 2 - 1))
	fi
	echo "NEXT: $i, begin: $begin, step: $step, max: $max, s: $s, e: $e"
	if [[ "$mode" == "tree" ]] ; then
		if [[ -f "$output" ]] ; then
			i=$(tail -n 1 "$output" | sed -r 's/^ *#Next free id:([1234567890]+)$/\1/')
		fi
		bzcat "$1" | awk '{print $2 "," $3}' | ./parse -q -t -i $i -s $s -e $e | sed '/^ *$/d' >> "$output"
	else
		bzcat "$1" | awk '{print $2 "," $3}' | ./parse -q -s $s -e $e | sed '/^ *$/d' >> "$output"
	fi
	#exit 1
done

for s in `seq $begin $step $max`; do
	if [[ $i -eq $max ]] ; then
		e=$((s + step))
	else
		e=$((s + step - 1))
	fi
	echo "NEXT: $i, begin: $begin, step: $step, max: $max, s: $s, e: $e"
	if [[ "$mode" == "tree" ]] ; then
		if [[ -f "$output" ]] ; then
			i=$(tail -n 1 "$output" | sed -r 's/^ *#Next free id:([1234567890]+)$/\1/')
		fi
		bzcat "$1" | awk '{print $2 "," $3}' | ./parse -q -t -i $i -s $s -e $e | sed '/^ *$/d' >> "$output"
	else
		bzcat "$1" | awk '{print $2 "," $3}' | ./parse -q -s $s -e $e | sed '/^ *$/d' >> "$output"
	fi
	#exit 1
done

./sortngram < "$output" -o "$output_sorted"

lbzip2 -k "$output"
lbzip2 -k "$output_sorted"

echo "Done!"
