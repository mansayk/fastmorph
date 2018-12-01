#!/bin/awk -f

function ltrim(s) { sub(/^[ \t\r\n]+/, "", s); return s }
function rtrim(s) { sub(/[ \t\r\n]+$/, "", s); return s }
function trim(s) { return rtrim(ltrim(s)); }

BEGIN {
    FS="\t";
    current_length=0;
    longest_string=0;
    longest_string_text;
}

{
	current_length = length( $3 );
	if( longest_string < current_length ) {
		longest_string = current_length;
		longest_string_text = $3;
	}
}

END {
    print "Longest string: " longest_string "\t" longest_string_text;
}
