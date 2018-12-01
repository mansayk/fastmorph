#!/bin/bash

/usr/bin/time /usr/bin/pypy3 ./apertium2fastmorph.py /home/mansur/TATCORPUS/DATA/tatcorpus3/tatcorpus3.sentences.apertium.tagged.bz2 \
							/home/mansur/TATCORPUS/DATA/tatcorpus3/tatcorpus3.inv_so.txt.bz2
