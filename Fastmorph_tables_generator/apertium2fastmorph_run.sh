#!/bin/bash

CORPUS_VERSION="4"

/usr/bin/time /usr/bin/pypy3 ./apertium2fastmorph.py /home/mansur/TATCORPUS/DATA/tatcorpus${CORPUS_VERSION}/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.bz2 \
							/home/mansur/TATCORPUS/DATA/tatcorpus${CORPUS_VERSION}/tatcorpus${CORPUS_VERSION}.inv_so.txt.bz2

echo "Additionally generating the '.wopunct' version of main table for n-grams..."
/usr/bin/time /usr/bin/bzcat /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main.DEBUG.txt.bz2 | \
	/usr/bin/awk -f ./convert_main_without_punctuation.awk | \
	bzip2 > tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main.wopunct.txt.bz2
