#!/usr/bin/pypy3
# -*- coding: utf-8 -*-

##########################################################################
#
# Parsing Apertium's tagger output into database format for fastmorph v5!
# Copyright (c) 2014-present Mansur Saykhunov tatcorpus@gmail.com
#
# Usage: script.sh <tagged.bz2> <inv_so.bz2>
#
##########################################################################

# Modules
import sys
import re
import os	# os.fsync()
import bz2
from bisect import bisect_right
from collections import defaultdict
sys.path.append('/home/mansur/TATCORPUS/streamparser')
from streamparser import parse, parse_file, mainpos, reading_to_string
from array import array

# Variables
bz2ext = ".bz2"
input_filename = sys.argv[1]
input_filename_inv_so = sys.argv[2]

# Get basenames
input_basename = os.path.basename(input_filename)
input_basename_inv_so = os.path.basename(input_filename_inv_so)

# Get rid of .bz2 extension
input_basename = os.path.splitext(input_basename)[0]
input_basename_inv_so = os.path.splitext(input_basename_inv_so)[0]

output_filename = input_basename + '.fastmorph.main.txt'
output_filename_debug = input_basename + '.fastmorph.main.DEBUG.txt'
output_filename_tags = input_basename + '.fastmorph.tags.txt'
output_filename_tags_uniq = input_basename + '.fastmorph.tags-uniq' # Without .txt extension
output_filename_words = input_basename + '.fastmorph.words.txt'
output_filename_words_case = input_basename + '.fastmorph.words_case.txt'
output_filename_lemmas = input_basename + '.fastmorph.lemmas.txt'
output_filename_united = input_basename + '.fastmorph.united.txt'
output_filename_united_debug = input_basename + '.fastmorph.united.DEBUG.txt'

array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_united = ""
debug_word_case = ""
strmain = ""
result = ""
status = ""
blank = ""
blanktag = "<sym>"
word_case = ""
word = ""
lemma = ""
pos = ""
tags = ""
tags_uniq = ""
united = ""
length = 0
sentence = 0
source = 0
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_tags = -1
found_id_tags_uniq = -1
found_id_united = -1
found_id_sentences = -1
counter_sentences = 0

array_word_case_combinations = []
dict_word_case_combinations = defaultdict(int) # {'!': 0} # word, freq

array_word_combinations = []
dict_word_combinations = defaultdict(int) # {'!': 0} # word, freq # default symbol needs to be first after sort and in fastmorph we use id == 0 for the sentence boundaries ???????????????????????

array_lemma_combinations = []
dict_lemma_combinations = defaultdict(int) # {'!': 0} # lemma, freq

array_tag_combinations = []
dict_tag_combinations = defaultdict(int) # {'<sent>': 0} # tags, freq

array_tag_uniq_combinations = []
dict_tag_uniq_combinations = defaultdict(int) # {'<sent>': 0} # tag, freq

array_united_combinations = []
dict_united_combinations = defaultdict(int) # {(0, 0, 0, 0): 0} # united (word_case, word, lemma, tags), freq

array_inv_so = [] # id=sentence, value=source

# Open UTF-8 file:
# encoding="utf-8" - # Without BOM
# encoding="utf-8-sig" - # With BOM

#fr = open( input_filename, 'rt', encoding="utf-8" )
fr = bz2.open( input_filename, 'rt', encoding="utf-8-sig" )
fr_inv_so = bz2.open( input_filename_inv_so, 'rt', encoding="utf-8-sig" )

fw = bz2.open( output_filename + bz2ext, 'wt', encoding="utf-8" )
fw_debug = bz2.open( output_filename_debug + bz2ext, 'wt', encoding="utf-8" )
fw_words_case = bz2.open( output_filename_words_case + bz2ext, 'wt', encoding="utf-8" )
fw_words = bz2.open( output_filename_words + bz2ext, 'wt', encoding="utf-8" )
fw_lemmas = bz2.open( output_filename_lemmas + bz2ext, 'wt', encoding="utf-8" )
fw_tags = bz2.open( output_filename_tags + bz2ext, 'wt', encoding="utf-8" )
fw_tags_uniq = bz2.open( output_filename_tags_uniq + bz2ext, 'wt', encoding="utf-8" )
fw_united = bz2.open( output_filename_united + bz2ext, 'wt', encoding="utf-8" )
fw_united_debug = bz2.open( output_filename_united_debug + bz2ext, 'wt', encoding="utf-8" )

#
# Binary search algorithm
#
def binarySearch(l, e):
	index = bisect_right(l, e)
	if index == len(l) or l[index] != e:
		return {'id':index, 'bool':True}
	return {'id':index, 'bool':False}

def parse_apertium(word_case, word, lemma, pos, tags):
	#global first_sent

	# Find the main parameters
	word_case = lexicalUnit.wordform
	word = word_case.lower()
	lemma = ""
	pos = ""
	tags = ""
	for r in lexicalUnit.readings:
		lemma = r[0].baseform
		lemma = lemma.lower()
		#print(" R:" + str(r))
		if r[0].tags:
			pos = mainpos(r, ltr=True)
		for s in r:
			if tags == "":
				tags = '<'
				tags += '>,<'.join(s.tags)
				tags += '>'
			else:
				tags += ',+{}<'.format(s.baseform)
				tags += '>,<'.join(s.tags)
				tags += '>'

	# Don't leave these columns empty, assign "*"
	if pos == "":
		pos = "*"
	if tags == "" or tags == "<>":
		tags = "*"

	# comment   -- SketchEngine
	# uncomment -- NoSketchEngine
	# uncomment -- CWB/CQP
	word_case = word_case.replace(' ', '_')
	word = word.replace(' ', '_')
	lemma = lemma.replace(' ', '_')

	# TODO: fix ^///<sent>$
	if word == "" and lemma == "":
		#print('WARNING: Empty string found! Converting it to "/" symbol!')
		word = "/"
		lemma = "/"

	return 0, word_case, word, lemma, pos, tags

def collect_tokens(word_case, word, lemma, pos, tags):

	# words_case
	dict_word_case_combinations[word_case] += 1

	# words
	dict_word_combinations[word] += 1

	# lemmas
	dict_lemma_combinations[lemma] += 1

	# tags
	array_tags = tags.split( ',' )
	array_tags.sort()
	tags = ','.join( array_tags )
	dict_tag_combinations[tags] += 1

	# tags_uniq
	for s in array_tags:
		dict_tag_uniq_combinations[s] += 1
	
	return 0

def find_token_ids(word_case, word, lemma, pos, tags):
	
	# words_case
	result = binarySearch(array_word_case_combinations, [word_case, ])
	if not result['bool']:
		sys.exit("Couldn't find word in the array = " + str(word_case));
	found_id_words_case = result['id']

	# words
	result = binarySearch(array_word_combinations, [word, ])
	if not result['bool']:
		sys.exit("Couldn't find word in the array = " + str(word));
	found_id_words = result['id']

	# lemmas
	result = binarySearch(array_lemma_combinations, [lemma, ])
	if not result['bool']:
		sys.exit("Couldn't find lemma in the array = " + str(lemma));
	found_id_lemmas = result['id']

	# tags
	array_tags = tags.split( ',' )
	array_tags.sort()
	tags = ','.join( array_tags )
	result = binarySearch(array_tag_combinations, [tags, ])
	if not result['bool']:
		sys.exit("Couldn't find tags in the array = " + str(tags))
	found_id_tags = result['id']

	# tags_uniq
	#for s in array_tags:
		#result = binarySearch(array_tag_uniq_combinations, [s, ])
		#if not result['bool']:
			#sys.exit("Couldn't find tag_uniq in the array = " + str(s));
		#found_id_tags_uniq = result['id']

	return 0, found_id_words_case, found_id_words, found_id_lemmas, found_id_tags

######################################################################################################################

print( "\nProcessing main, words_case, words, lemmas, tags, tags_uniq section:" )

######################################################################################################################
#   1st cycle
######################################################################################################################

#
# Collecting words, lemmas, tags...
#
#for blank, lexicalUnit in parse_file(fr, with_text=True):
for line in fr:
	line = line.strip(" \r\n")
	#print("LINE:" + str(line))
	for blank, lexicalUnit in parse(line, with_text=True):
		# MySQL (MariaDB) does not differ Russian "ё" and "е" letters
		#line = re.sub( r'ё', r"е", line ) # ё => е
		#line = re.sub( r'Ё', r"Е", line ) # Ё => Е
		#line = re.sub( r'\\', r"", line ) # remove backslashes, otherwise mysql will not import those lines

		#print("BLANK:" + str(blank) + " lexicalUnit:" + str(lexicalUnit))
		#sys.stdout.flush()
		
		blank = blank.strip(' \t')
		if blank != "":
			status = collect_tokens(blank, blank, blank, blanktag, blanktag)
		
		status, word_case, word, lemma, pos, tags = parse_apertium(word_case, word, lemma, pos, tags)
		status = collect_tokens(word_case, word, lemma, pos, tags)

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Collecting words and lemmas from sentences... %d lines" % counter_sentences)
		sys.stdout.flush()
	counter_sentences += 1

sys.stdout.write("\r  Collecting words and lemmas from sentences... %d lines" % counter_sentences)
sys.stdout.flush()
print( "" )

# Convert dicts to lists
# words_case
for key, value in sorted(dict_word_case_combinations.items()):
	array_word_case_combinations.append([key, value])
#dict_word_case_combinations = sorted(dict_word_case_combinations.items())
#for key, value in dict_word_case_combinations:
#	print (key + " :: " + str(value))
dict_word_case_combinations.clear()
del dict_word_case_combinations

# words
for key, value in sorted(dict_word_combinations.items()):
	array_word_combinations.append([key, value])
dict_word_combinations.clear()
del dict_word_combinations

# lemmas
for key, value in sorted(dict_lemma_combinations.items()):
	array_lemma_combinations.append([key, value])
dict_lemma_combinations.clear()
del dict_lemma_combinations

# tags
for key, value in sorted(dict_tag_combinations.items()):
	array_tag_combinations.append([key, value])
dict_tag_combinations.clear()
del dict_tag_combinations

# tags_uniq
for key, value in sorted(dict_tag_uniq_combinations.items()):
	array_tag_uniq_combinations.append([key, value])
dict_tag_uniq_combinations.clear()
del dict_tag_uniq_combinations

######################################################################################################################
#   2nd cycle
######################################################################################################################

# set defult values
array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_tags_united = ""
word = ""
word_case = ""
lemma = ""
tags = ""
tags_uniq = ""
tags_united = ""
found_id_tags = -1
found_id_tags_uniq = -1
found_id_tags_united = -1
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_sentences = -1
counter_sentences = 0

#
# Reading tagged sentences file
#
fr.seek(0, 0)
for line in fr:
	line = line.strip(" \r\n")
	for blank, lexicalUnit in parse(line, with_text=True):
		
		blank = blank.strip(' \t')
		if blank != "":
			status, found_id_words_case, found_id_words, found_id_lemmas, found_id_tags = find_token_ids(blank, blank, blank, blanktag, blanktag)
			
			# united
			united = tuple((found_id_words_case, found_id_words, found_id_lemmas, found_id_tags))
			dict_united_combinations[united] += 1

		status, word_case, word, lemma, pos, tags = parse_apertium(word_case, word, lemma, pos, tags)
		status, found_id_words_case, found_id_words, found_id_lemmas, found_id_tags = find_token_ids(word_case, word, lemma, pos, tags)
		
		# united
		united = tuple((found_id_words_case, found_id_words, found_id_lemmas, found_id_tags))
		dict_united_combinations[united] += 1

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Building 'united' table from sentences... %d lines" % counter_sentences)
		sys.stdout.flush()
	counter_sentences += 1

sys.stdout.write("\r  Building 'united' table from sentences... %d lines" % (counter_sentences))
sys.stdout.flush()
print( "" )

# Convert dict to list
# united
for key, value in sorted(dict_united_combinations.items()):
	array_united_combinations.append([key, value])
dict_united_combinations.clear()
del dict_united_combinations

######################################################################################################################
#   3rd cycle
######################################################################################################################

#
# Reading inv_so file into array
#
array_inv_so_id = 0
for line in fr_inv_so:
	line = line.rstrip(" \r\n")
	ss = map(int, line.split("\t"))
	sentence, source = list(ss)
	if sentence != array_inv_so_id:
		sys.exit("Error loading 'array_inv_so' table in line #" + str(sentence) + " != " + str(array_inv_so_id))
	array_inv_so.append(source)
	array_inv_so_id += 1;
	if array_inv_so_id % 1000 == 0:
		sys.stdout.write("\r  Loading %d sent-source associations" % array_inv_so_id)
		sys.stdout.flush()
sys.stdout.write("\r  Loading %d sent-source associations " % (array_inv_so_id))
sys.stdout.flush()
fr_inv_so.close()
print( "" )

# Convert list to tuple
#array_inv_so = tuple(array_inv_so)

######################################################################################################################

# set defult values
array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_tags_united = ""
word = ""
word_case = ""
lemma = ""
tags = ""
tags_uniq = ""
tags_united = ""
found_id_tags = -1
found_id_tags_uniq = -1
found_id_tags_united = -1
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_sentences = -1
counter_sentences = 0
counter_id_main = 0
source_id = 0

#
# Reading tagged sentences file
#
fr.seek(0, 0)
for line in fr:
	line = line.strip(" \r\n")
	for blank, lexicalUnit in parse(line, with_text=True):
		
		blank = blank.strip(' \t')
		if blank != "":
			status, found_id_words_case, found_id_words, found_id_lemmas, found_id_tags = find_token_ids(blank, blank, blank, blanktag, blanktag)
				
			# united
			united = tuple((found_id_words_case, found_id_words, found_id_lemmas, found_id_tags))
			result = binarySearch(array_united_combinations, [united, ])
			if not result['bool']:
				sys.exit("Couldn't find 'united' in the array = " + str(united))
			found_id_united = result['id']

			# sources
			source_id = array_inv_so[counter_sentences]

			strmain = str(counter_id_main) + "\t" + str(found_id_united) + "\t" + str(counter_sentences) + "\t" + str(source_id)
			debug_word_case = str(array_word_case_combinations[ array_united_combinations[found_id_united][0][0] ][0])
			debug_word = str(array_word_combinations[ array_united_combinations[found_id_united][0][1] ][0])
			debug_lemma = str(array_lemma_combinations[ array_united_combinations[found_id_united][0][2] ][0])
			debug_tags = str(array_tag_combinations[ array_united_combinations[found_id_united][0][3] ][0])
			
			fw.write(strmain + "\n")
			fw_debug.write(strmain + "\t" + debug_word_case + "\t" + debug_word + "\t" + debug_lemma + "\t" + debug_tags + "\n" )
			
			counter_id_main += 1

		status, word_case, word, lemma, pos, tags = parse_apertium(word_case, word, lemma, pos, tags)
		status, found_id_words_case, found_id_words, found_id_lemmas, found_id_tags = find_token_ids(word_case, word, lemma, pos, tags)
			
		# united
		united = tuple((found_id_words_case, found_id_words, found_id_lemmas, found_id_tags))
		result = binarySearch(array_united_combinations, [united, ])
		if not result['bool']:
			sys.exit("Couldn't find 'united' in the array = " + str(united))
		found_id_united = result['id']

		# sources
		source_id = array_inv_so[counter_sentences]

		strmain = str(counter_id_main) + "\t" + str(found_id_united) + "\t" + str(counter_sentences) + "\t" + str(source_id)
		debug_word_case = str(array_word_case_combinations[ array_united_combinations[found_id_united][0][0] ][0])
		debug_word = str(array_word_combinations[ array_united_combinations[found_id_united][0][1] ][0])
		debug_lemma = str(array_lemma_combinations[ array_united_combinations[found_id_united][0][2] ][0])
		debug_tags = str(array_tag_combinations[ array_united_combinations[found_id_united][0][3] ][0])
		
		fw.write(strmain + "\n")
		fw_debug.write(strmain + "\t" + debug_word_case + "\t" + debug_word + "\t" + debug_lemma + "\t" + debug_tags + "\n")
		
		counter_id_main += 1

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Writing main file... %d lines" % counter_sentences)
		sys.stdout.flush()
	counter_sentences += 1

sys.stdout.write("\r  Writing main file... %d lines" % (counter_sentences))
sys.stdout.flush()
fw.flush()
os.fsync( fw )
fw.close()
fw_debug.flush()
os.fsync( fw_debug )
fw_debug.close()
print( "" )

# Write united file
size_list = len( array_united_combinations )
for x in range( 0, size_list ):
	united_freq = array_united_combinations[x][1]
	united_word_case = array_united_combinations[x][0][0]
	united_word = array_united_combinations[x][0][1]
	united_lemma = array_united_combinations[x][0][2]
	united_tags = array_united_combinations[x][0][3]
	
	strmain = str(x) + "\t" + str(united_freq) + "\t" + str(united_word_case) + "\t" + str(united_word) + "\t" + str(united_lemma) + "\t" + str(united_tags)
	debug_word_case = str(array_word_case_combinations[united_word_case][0])
	debug_word = str(array_word_combinations[united_word][0])
	debug_lemma = str(array_lemma_combinations[united_lemma][0])
	debug_tags = str(array_tag_combinations[united_tags][0])
	
	fw_united.write(strmain + '\n' )
	fw_united_debug.write(strmain + "\t" + debug_word_case + "\t" + debug_word + "\t" + debug_lemma + "\t" + debug_tags + '\n' )
	sys.stdout.write("\r  Writing united file... %d lines" % x)
	sys.stdout.flush()
fw_united.flush()
os.fsync( fw_united )
fw_united.close()
fw_united_debug.flush()
os.fsync( fw_united_debug )
fw_united_debug.close()
print( "" )

# Write words_case file
size_list = len( array_word_case_combinations )
for x in range( 0, size_list ):
	fw_words_case.write( str( x ) + "\t" + str( array_word_case_combinations[x][1] ) + "\t" + array_word_case_combinations[x][0] + '\n' )
	sys.stdout.write("\r  Writing words_case file... %d lines" % x)
	sys.stdout.flush()
fw_words_case.flush()
os.fsync( fw_words_case )
fw_words_case.close()
del array_word_case_combinations[:]
print( "" )

# Write words file
size_list = len( array_word_combinations )
for x in range( 0, size_list ):
	fw_words.write( str( x ) + "\t" + str( array_word_combinations[x][1] ) + "\t" + array_word_combinations[x][0] + '\n' )
	sys.stdout.write("\r  Writing words file... %d lines" % x)
	sys.stdout.flush()
fw_words.flush()
os.fsync( fw_words )
fw_words.close()
del array_word_combinations[:]
print( "" )

# Write lemmas file
size_list = len( array_lemma_combinations )
for x in range( 0, size_list ):
	fw_lemmas.write( str( x ) + "\t" + str( array_lemma_combinations[x][1] ) + "\t" + array_lemma_combinations[x][0] + '\n' )
	sys.stdout.write("\r  Writing lemmas file... %d lines" % x)
	sys.stdout.flush()
fw_lemmas.flush()
os.fsync( fw_lemmas )
fw_lemmas.close()
del array_lemma_combinations[:]
print( "" )

# Write tags file
size_list = len( array_tag_combinations )
for x in range( 0, size_list ):
	fw_tags.write( str(x) + "\t" + str( array_tag_combinations[x][1] ) + "\t" + array_tag_combinations[x][0] + '\n' )
	sys.stdout.write("\r  Writing tags file... %d lines" % x)
	sys.stdout.flush()
fw_tags.flush()
os.fsync( fw_tags )
fw_tags.close()
del array_tag_combinations[:]
print( "" )

# Write tags_uniq file
size_list = len( array_tag_uniq_combinations )
for x in range( 0, size_list ):
	fw_tags_uniq.write( str( x ) + "\t" + str( array_tag_uniq_combinations[x][1] ) + "\t" + array_tag_uniq_combinations[x][0] + '\n' )
	sys.stdout.write("\r  Writing tags_uniq file... %d lines" % x)
	sys.stdout.flush()
fw_tags_uniq.flush()
os.fsync( fw_tags_uniq )
fw_tags_uniq.close()
del array_tag_uniq_combinations[:]
print( "" )

#
# Sorting tags_uniq file
#
import os
print( "\nSorting tags_uniq file..." )
os.system( "bzcat " + output_filename_tags_uniq + bz2ext + " | sed '/\*/d' | sort -k2 -n -r | nl -v0 -nln | awk '{print $1 \"\\t\" $3 \"\\t\" $4}' | bzip2 > " + output_filename_tags_uniq + ".sorted.txt.bz2" )
os.system( "rm -f " + output_filename_tags_uniq )

print( "\nDone!" )
