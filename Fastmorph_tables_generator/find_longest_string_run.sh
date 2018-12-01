#!/bin/bash

echo -n "WORDS_CASE: "
/usr/bin/bzcat /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus3.sentences.apertium.tagged.fastmorph.words_case.txt.bz2 | \
	/usr/bin/awk -f ./find_longest_string.awk

echo -n "WORDS: "
/usr/bin/bzcat /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus3.sentences.apertium.tagged.fastmorph.words.txt.bz2 | \
	/usr/bin/awk -f ./find_longest_string.awk

echo -n "LEMMAS: "
/usr/bin/bzcat /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus3.sentences.apertium.tagged.fastmorph.lemmas.txt.bz2 | \
	/usr/bin/awk -f ./find_longest_string.awk
