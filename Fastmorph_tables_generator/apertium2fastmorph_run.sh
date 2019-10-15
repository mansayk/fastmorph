#!/bin/bash

CORPUS_VERSION="4"

/usr/bin/time /usr/bin/pypy3 ./apertium2fastmorph.py /home/mansur/TATCORPUS/DATA/tatcorpus${CORPUS_VERSION}/tatcorpus${CORPUS_VERSION}.sentences.apertium.tagged.bz2 \
							/home/mansur/TATCORPUS/DATA/tatcorpus${CORPUS_VERSION}/tatcorpus${CORPUS_VERSION}.inv_so.txt.bz2
