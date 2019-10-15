#!/bin/bash

CORPUS_VERSION="4"

/usr/bin/time /usr/bin/bzcat /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main.DEBUG.txt.bz2 | \
	/usr/bin/awk -f ./convert_main_without_punctuation.awk | \
	bzip2 > tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main.wopunct.txt.bz2
