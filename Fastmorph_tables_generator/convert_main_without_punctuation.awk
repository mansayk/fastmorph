#!/bin/awk -f

BEGIN {
	FS = "\t";
	sent = 0;
	sent_prev = 0;
}

{
	if($3 != sent_prev) {
		sent = (sent + 1);
	}
	sent_prev = $3;

	if($8 == "<sent>" || 
		$8 == "<cm>" || 
		$8 == "<apos>" || 
		$8 == "<quot>" || 
		$8 == "<rquot>" || 
		$8 == "<lquot>" || 
		$8 == "<rpar>" || 
		$8 == "<lpar>" || 
		$8 == "<asterisk>" || 
		$8 == "<vline>" || 
		$8 == "<invquest>" || 
		$8 == "<punct>") {
		sent = (int(sent) + 1);
		#print $1 "\t" $2 "\t" "-1" "\t" $4;
	} else {
		#print $1 "\t" $2 "\t" sent "\t" $4;
		print $1 "\t" $2 "\t" sent;
	}
}

END {
	print "Done!";
}

