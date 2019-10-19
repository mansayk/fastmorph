#!/bin/bash

CORPUS_VERSION="4"
# If "" then the FULL version with punctuation will be used. Consumes much more RAM and gives dirty results!
# If ".wopunct" then the version without punctuation is used. Better use this!
WOPUNCT=".wopunct"

/usr/bin/time /bin/bash ./fastmorph2fastngrams.sh /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main${WOPUNCT}.txt.bz2
