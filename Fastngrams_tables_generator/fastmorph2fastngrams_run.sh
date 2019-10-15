#!/bin/bash

CORPUS_VERSION="4"

/usr/bin/time /bin/bash ./fastmorph2fastngrams.sh /home/mansur/TATCORPUS/TOOLS/Fastmorph_tables_generator/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.fastmorph.main.txt.bz2
