#!/usr/bin/pypy3
# -*- coding: utf-8 -*-

#!/usr/bin/pypy3
#!/usr/bin/python3

##########################################################################
#
# Parsing Apertium's tagger output into database format for fastmorph v4!
# Copyright (c) 2014-present Mansur Saykhunov tatcorpus@gmail.com
#
##########################################################################

# Modules
import sys
import re
import os	# os.fsync()
import bisect

# Variables
input_filename = sys.argv[1]
input_filename_inv_so = 'inv_so'
output_filename = sys.argv[1]+'.main.output.txt'
output_filename_tags = sys.argv[1]+'.tags.output.txt'
output_filename_tags_uniq = sys.argv[1]+'.tags-uniq.output.txt'
output_filename_words = sys.argv[1]+'.words.output.txt'
output_filename_words_case = sys.argv[1]+'.words_case.output.txt'
output_filename_lemmas = sys.argv[1]+'.lemmas.output.txt'
output_filename_united = sys.argv[1]+'.united.output.txt'
array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_united = ""
current_word = ""
current_word_case = ""
current_lemma = ""
current_tags = ""
current_tags_uniq = ""
current_united = ""
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_tags = -1
found_id_tags_uniq = -1
found_id_united = -1
found_id_sentences = -1
counter_id_main = 0
counter_id_words = 0
counter_id_words_case = 0
counter_id_lemmas = 0
counter_id_tags = 0
counter_id_tags_uniq = 0
counter_id_united = 0
counter_wordpos = 1
counter_tags = 0
counter_tags_uniq = 0
counter_united = 0
counter_sentences = 1
word_len = 0

array_tag_combinations = [[' '], [0]] # tags, freq
array_tag_combinations_id = 0

array_tag_uniq_combinations = [[' '], [0]] # tag, freq
array_tag_uniq_combinations_id = 0

array_word_combinations = [[' '], [0], [0]] # word, freq, id # whitespace symbol needs to be first after sort and in fastmorph we use id == 0 for the sentence boundaries
array_word_combinations_id = 0

array_word_case_combinations = [[' '], [0], [0]] # word, freq, id # whitespace symbol needs to be first after sort and in fastmorph we use id == 0 for the sentence boundaries
array_word_case_combinations_id = 0

array_lemma_combinations = [[' '], [0], [0]] # * - lemma, freq, id
array_lemma_combinations_id = 0

array_united_combinations = [[' '], [0]] # united, freq
array_united_combinations_id = 0

array_inv_so = [[1], [1]] # sentence <=> source 
array_inv_so_id = 0

fr = open( input_filename , 'rt' )
fr_inv_so = open( input_filename_inv_so , 'rt' )
fw = open( output_filename, 'wt' )
fw2 = open( output_filename_tags, 'wt' )
fw3 = open( output_filename_words, 'wt' )
fw4 = open( output_filename_lemmas, 'wt' )
fw5 = open( output_filename_tags_uniq, 'wt' )
fw6 = open( output_filename_words_case, 'wt' )
fw7 = open( output_filename_united, 'wt' )

print( "" )
print( "!!! tat-tagger sometimes merges lines; in this case the script dublicates that line or use: !!!" )
print( "!!! sed -r 's/([^.?!])$/\\1./' INPUT_FILE.TXT | apertium -n -d . tat-tagger | cg-proc dev/mansur.bin !!!" )


#
# binary search algorithm
#
def binarySearch(alist, item):
	first = 0
	last = len(alist)-1
	found = False
	while first<=last and not found:
		midpoint = (first + last)//2
		if alist[midpoint] == item:
			found = True
		else:
			if item < alist[midpoint]:
				last = midpoint-1
			else:
				first = midpoint+1
	if found:
		return midpoint
	else:
		return -1


print( "\nProcessing main, words, lemmas, tags, tags_uniq section:" )
#
# Reading inv_so file into array
#
#array_inv_so[0].pop( 0 ) # deleting fake empty string 
#array_inv_so[1].pop( 0 ) # deleting fake empty string 
for line in fr_inv_so:
	line = line.rstrip( '\n' ) # remove newline symbol
	sent = re.sub( r'^([1234567890]+)\t[1234567890]+$', r"\1", line)
	source = re.sub( r'^[1234567890]+\t([1234567890]+)$', r"\1", line)
	array_inv_so[0].append(int(sent))
	array_inv_so[1].append(int(source))
	array_inv_so_id += 1;
	if array_inv_so_id % 1000 == 0:
		sys.stdout.write("\r  Loading %d sent-source associations" % array_inv_so_id)
		sys.stdout.flush()
sys.stdout.write("\r  Loading %d sent-source associations " % (array_inv_so_id) )
sys.stdout.flush()
print( "" )

######################################################################################################################
#   1st cycle
######################################################################################################################

#
# Collecting words and lemmas...
#
insert_id = 0
for line in fr:
	line = re.sub( r'—–―–―—–‒‑‐﹣－֊-', r"-", line ) # replace different hyphens, but you better do normalization before tagging

	# MySQL (MariaDB) does not differ Russian "ё" and "е" letters
	#line = re.sub( r'ё', r"е", line ) # ё => е
	#line = re.sub( r'Ё', r"Е", line ) # Ё => Е

	line = re.sub( r'\\', r"", line ) # remove backslashes, otherwise mysql will not import those lines
	line = re.sub( r'^[^^]+\^', r"^", line ) # remove some trash (not recognized by Apertium) in the line beginning
	line = re.sub( r'\$[^$]+$', r"$", line ) # remove some trash (not recognized by Apertium) in the line ending
	line = line.rstrip( '\n' ) # remove newline symbol
	line = re.sub( r'\$[^^]*\^', r"$$^^", line ) # remove some trash between $ ^
	line = re.sub( r'(^\s+)|(\s$)', r"", line ) # trim (remove whitespaces) the string
	line = re.sub( r'(>)\s*([<+])', r"\1,\2", line ) # for split
	
	array_words = line.split( '$^' )
	for i in array_words:
		current_word = re.sub( r'^.*\^(.+)\/.*$', r"\1", i )
		current_word_case = current_word
		current_lemma = re.sub( r'^.*\/([^<+]+)[<+$].*$', r"\1", i )
		current_tags = re.sub( r'^.*\/\*.*$', r"*", i )
		if current_tags != "*":
			current_tags = re.sub( r'^[^<+]*([<+].+)\$.*$', r"\1", i )
		else:
			current_lemma = re.sub( r'^\*(.*)$', r"\1", current_lemma )

		# words_case
		found_id_words_case = -1
		word_len = len( current_word_case )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words_case = binarySearch(array_word_case_combinations[0], current_word_case)
		except ValueError:
			found_id_words_case = -1

		if found_id_words_case == -1:
			insert_id = bisect.bisect(array_word_case_combinations[0], current_word_case)
			array_word_case_combinations[0].insert(insert_id, current_word_case )
			array_word_case_combinations[1].insert(insert_id, 1 )
			#counter_id_words_case += 1
			#array_word_case_combinations_id += 1
			#found_id_words_case = array_word_case_combinations_id
			found_id_words_case = insert_id
		else:
			array_word_case_combinations[1][found_id_words_case] += 1

		# words
		found_id_words = -1
		current_word = current_word.lower()
		word_len = len( current_word )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words = binarySearch(array_word_combinations[0], current_word)
		except ValueError:
			found_id_words = -1

		if found_id_words == -1:
			insert_id = bisect.bisect(array_word_combinations[0], current_word)
			array_word_combinations[0].insert(insert_id, current_word )
			array_word_combinations[1].insert(insert_id, 1 )
			#counter_id_words += 1
			#array_word_combinations_id += 1
			#found_id_words = array_word_combinations_id
			found_id_words = insert_id
		else:
			array_word_combinations[1][found_id_words] += 1

		# lemmas
		found_id_lemmas = -1
		current_lemma = current_lemma.lower()

		try:
			found_id_lemmas = binarySearch(array_lemma_combinations[0], current_lemma)
		except ValueError:
			found_id_lemmas = -1

		if found_id_lemmas == -1:
			insert_id = bisect.bisect(array_lemma_combinations[0], current_lemma)
			array_lemma_combinations[0].insert(insert_id, current_lemma )
			array_lemma_combinations[1].insert(insert_id, 1 )
			#counter_id_lemmas += 1
			#array_lemma_combinations_id += 1
			#found_id_lemmas = array_lemma_combinations_id
			found_id_lemmas = insert_id
		else:
			array_lemma_combinations[1][found_id_lemmas] += 1

		# tags
		array_tags = current_tags.split( ',' )
                # If you gonna uncomment this, you should also uncomment the same code in the next (Reading...) section
		#for element in array_tags:
		#	if element[0] == "+":
		#		element = re.sub( r'\+.+(<.+>)', r"\1", element )  # +сана<emph> => +сана<emph>,<emph>
		#		array_tags.append( element )
		array_tags.sort()
		current_tags = ','.join( array_tags )
		
		found_id_tags = -1

		try:
			found_id_tags = binarySearch(array_tag_combinations[0], current_tags)
		except ValueError:
			found_id_tags = -1

		if found_id_tags == -1:
			insert_id = bisect.bisect(array_tag_combinations[0], current_tags)
			array_tag_combinations[0].insert(insert_id, current_tags )
			array_tag_combinations[1].insert(insert_id, 1 )
			#counter_id_tags += 1
			#array_tag_combinations_id += 1
			#found_id_tags = array_tag_combinations_id
			found_id_tags = insert_id
		else:
			array_tag_combinations[1][found_id_tags] += 1

		# tags_uniq
		array_tags_uniq = array_tags
		size_array_tags_uniq = len( array_tags_uniq )

		for x in range( 0, size_array_tags_uniq ):
			found_id_tags_uniq = -1

			try:
				found_id_tags_uniq = binarySearch(array_tag_uniq_combinations[0], array_tags_uniq[x])
			except ValueError:
				found_id_tags_uniq = -1

			if found_id_tags_uniq == -1:
				insert_id = bisect.bisect(array_tag_uniq_combinations[0], array_tags_uniq[x])
				array_tag_uniq_combinations[0].insert(insert_id, array_tags_uniq[x] )
				array_tag_uniq_combinations[1].insert(insert_id, 1 )
				#counter_id_tags_uniq += 1
				#array_tag_uniq_combinations_id += 1
				#found_id_tags_uniq = array_tag_uniq_combinations_id
			else:
				array_tag_uniq_combinations[1][found_id_tags_uniq] += 1

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Collecting words and lemmas from sentences... %d lines" % counter_sentences)
		sys.stdout.flush()

	counter_sentences += 1

sys.stdout.write("\r  Collecting words and lemmas from sentences... %d lines" % counter_sentences)
sys.stdout.flush()
fr.close()
print( "" )

######################################################################################################################
#   2nd cycle
######################################################################################################################

fr = open( input_filename , 'rt' )

# set defult values
array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_tags_united = ""
current_word = ""
current_word_case = ""
current_lemma = ""
current_tags = ""
current_tags_uniq = ""
current_tags_united = ""
found_id_tags = -1
found_id_tags_uniq = -1
found_id_tags_united = -1
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_sentences = -1
counter_sentences = 1
counter_wordpos = 1
counter_id_main = 0
#counter_id_words = 0
#counter_id_lemmas = 0
counter_id_tags = 0
counter_id_tags_uniq = 0
counter_id_tags_united = 0
counter_tags = 0
counter_tags_uniq = 0
counter_tags_united = 0
word_len = 0
line_prev = ""

#
# Reading tagged sentences file
#
insert_id = 0
for line in fr:
	line = re.sub( r'—–―–―—–‒‑‐﹣－֊-', r"-", line ) # replace different hyphens, but you better do normalization before tagging

	# MySQL (MariaDB) does not differ "ё" and "е" letters
	#line = re.sub( r'ё', r"е", line ) # ё => е
	#line = re.sub( r'Ё', r"Е", line ) # Ё => Е

	line = re.sub( r'\\', r"", line ) # remove backslashes, otherwise mysql will not import those lines
	line = re.sub( r'^[^^]+\^', r"^", line ) # remove some trash (not recognized by Apertium) in the line beginning
	line = re.sub( r'\$[^$]+$', r"$", line ) # remove some trash (not recognized by Apertium) in the line ending
	line = line.rstrip( '\n' ) # remove newline symbol
	line = re.sub( r'\$[^^]*\^', r"$$^^", line ) # remove some trash between $ ^
	line = re.sub( r'(^\s+)|(\s$)', r"", line ) # trim (remove whitespaces) the string
	line = re.sub( r'(>)\s*([<+])', r"\1,\2", line ) # for split

	# Dublicating empty strings (after Apertium's tagger merged two line)
	if line == "":
		line = line_prev
		print("    Empty string found on line #" + str(counter_sentences) + ". Dublicating previous sentence: " + line);
		#sys.exit("Empty string found! It means that 2 sentences might be merged! " +
		#		"Please add . to the end of such sentences to avoid it, " +
		#		"or permit merging strings by editing this code!" +
		#		"Or dublicate previous sentences on empty strings.")	# quit with error
		#continue 	# ignore empty strings
	line_prev = line

	array_words = line.split( '$^' )

	# sources
	found_id_sentences = -1
	try:
		found_id_sentences = binarySearch(array_inv_so[0], counter_sentences)
	except ValueError:
		found_id_sentences = -1
		sys.exit("Couldn't find in inv_so a sentence #" + str(counter_sentences))

	if found_id_sentences != -1:
		source_id = array_inv_so[1][found_id_sentences]

	counter_wordpos = 1
	for i in array_words:
		current_word = re.sub( r'^.*\^(.+)\/.*$', r"\1", i )
		current_word_case = current_word
		current_lemma = re.sub( r'^.*\/([^<+]+)[<+$].*$', r"\1", i )
		current_tags = re.sub( r'^.*\/\*.*$', r"*", i )
		if current_tags != "*":
			current_tags = re.sub( r'^[^<+]*([<+].+)\$.*$', r"\1", i )
		else:
			current_lemma = re.sub( r'^\*(.*)$', r"\1", current_lemma )

		# tags
		array_tags = current_tags.split( ',' )
                # If you gonna uncomment this, you should also uncomment the same code in the previous (Collecting...) section
		#for element in array_tags:
		#	if element[0] == "+":
		#		element = re.sub( r'\+.+(<.+>)', r"\1", element )  # +сана<emph> => +сана<emph>,<emph>
		#		array_tags.append( element )
		array_tags.sort()
		current_tags = ','.join( array_tags )
		
		found_id_tags = -1
		try:
			found_id_tags = binarySearch(array_tag_combinations[0], current_tags)
		except ValueError:
			sys.exit("Couldn't find tags in the array = " + str(current_tags))
			found_id_tags = -1

		# tags_uniq
		array_tags_uniq = array_tags
		size_array_tags_uniq = len( array_tags_uniq )
		for x in range( 0, size_array_tags_uniq ):
			found_id_tags_uniq = -1
			try:
				found_id_tags_uniq = binarySearch(array_tag_uniq_combinations[0], array_tags_uniq[x])
			except ValueError:
				sys.exit("Couldn't find tag_uniq in the array = " + str(array_tags_uniq[x]));
				found_id_tags_uniq = -1

		# words_case
		found_id_words_case = -1
		word_len = len( current_word_case )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words_case = binarySearch(array_word_case_combinations[0], current_word_case)
		except ValueError:
			sys.exit("Couldn't find word in the array = " + str(current_word_case));
			found_id_words_case = -1

		# words
		found_id_words = -1
		current_word = current_word.lower()
		word_len = len( current_word )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words = binarySearch(array_word_combinations[0], current_word)
		except ValueError:
			sys.exit("Couldn't find word in the array = " + str(current_word));
			found_id_words = -1

		# lemmas
		found_id_lemmas = -1
		current_lemma = current_lemma.lower()
		try:
			found_id_lemmas = binarySearch(array_lemma_combinations[0], current_lemma)
		except ValueError:
			sys.exit("Couldn't find lemma in the array = " + str(current_lemma))
			found_id_lemmas = -1





		# united
		found_id_united = -1
		#current_united = current_united.lower()
		current_united = str( found_id_words_case ) + "x" + str( found_id_words ) + "x" + str( found_id_lemmas ) + "x" + str( found_id_tags )
		#word_len = len( current_united )
		# Verify if fits into short in fastmorph (DEPRECATED)
		#if word_len > 255:
		#	word_len = 255

		try:
			found_id_united = binarySearch(array_united_combinations[0], current_united)
		except ValueError:
			found_id_united = -1

		if found_id_united == -1:
			insert_id = bisect.bisect(array_united_combinations[0], current_united)
			array_united_combinations[0].insert(insert_id, current_united )
			array_united_combinations[1].insert(insert_id, 1 )
			#counter_id_words += 1
			#array_word_combinations_id += 1
			#found_id_words = array_word_combinations_id
			found_id_united = insert_id
		else:
			array_united_combinations[1][found_id_united] += 1













		#fw.write( str( counter_id_main ) + "\t" + str( found_id_words_case ) + "\t" + str( found_id_words ) + "\t" + str( found_id_lemmas ) + "\t" + str( found_id_tags ) + "\t" + str( counter_sentences ) + "\t" + str( source_id ) + "\n" )
		counter_wordpos += 1
		counter_id_main += 1

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Building 'united' table from sentences... %d lines" % counter_sentences)
		sys.stdout.flush()

	counter_sentences += 1

sys.stdout.write("\r  Building 'united' table from sentences... %d lines" % (counter_sentences) )
sys.stdout.flush()
#fw.flush()
#os.fsync( fw )
#fw.close()
fr.close()
print( "" )














######################################################################################################################
#   3rd cycle
######################################################################################################################

fr = open( input_filename , 'rt' )

# set defult values
array_words = ""
array_words_case = ""
array_lemmas = ""
array_tags = ""
array_tags_uniq = ""
array_tags_united = ""
current_word = ""
current_word_case = ""
current_lemma = ""
current_tags = ""
current_tags_uniq = ""
current_tags_united = ""
found_id_tags = -1
found_id_tags_uniq = -1
found_id_tags_united = -1
found_id_words = -1
found_id_words_case = -1
found_id_lemmas = -1
found_id_sentences = -1
counter_sentences = 1
counter_wordpos = 1
counter_id_main = 0
#counter_id_words = 0
#counter_id_lemmas = 0
counter_id_tags = 0
counter_id_tags_uniq = 0
counter_id_tags_united = 0
counter_tags = 0
counter_tags_uniq = 0
counter_tags_united = 0
word_len = 0
line_prev = ""

#
# Reading tagged sentences file
#
insert_id = 0
for line in fr:
	line = re.sub( r'—–―–―—–‒‑‐﹣－֊-', r"-", line ) # replace different hyphens, but you better do normalization before tagging

	# MySQL (MariaDB) does not differ "ё" and "е" letters
	#line = re.sub( r'ё', r"е", line ) # ё => е
	#line = re.sub( r'Ё', r"Е", line ) # Ё => Е

	line = re.sub( r'\\', r"", line ) # remove backslashes, otherwise mysql will not import those lines
	line = re.sub( r'^[^^]+\^', r"^", line ) # remove some trash (not recognized by Apertium) in the line beginning
	line = re.sub( r'\$[^$]+$', r"$", line ) # remove some trash (not recognized by Apertium) in the line ending
	line = line.rstrip( '\n' ) # remove newline symbol
	line = re.sub( r'\$[^^]*\^', r"$$^^", line ) # remove some trash between $ ^
	line = re.sub( r'(^\s+)|(\s$)', r"", line ) # trim (remove whitespaces) the string
	line = re.sub( r'(>)\s*([<+])', r"\1,\2", line ) # for split

	# Dublicating empty strings (after Apertium's tagger merged two line)
	if line == "":
		line = line_prev
		print("    Empty string found on line #" + str(counter_sentences) + ". Dublicating previous sentence: " + line);
		#sys.exit("Empty string found! It means that 2 sentences might be merged! " +
		#		"Please add . to the end of such sentences to avoid it, " +
		#		"or permit merging strings by editing this code!" +
		#		"Or dublicate previous sentences on empty strings.")	# quit with error
		#continue 	# ignore empty strings
	line_prev = line

	array_words = line.split( '$^' )

	# sources
	found_id_sentences = -1
	try:
		found_id_sentences = binarySearch(array_inv_so[0], counter_sentences)
	except ValueError:
		found_id_sentences = -1
		sys.exit("Couldn't find in inv_so a sentence #" + str(counter_sentences))

	if found_id_sentences != -1:
		source_id = array_inv_so[1][found_id_sentences]

	counter_wordpos = 1
	for i in array_words:
		current_word = re.sub( r'^.*\^(.+)\/.*$', r"\1", i )
		current_word_case = current_word
		current_lemma = re.sub( r'^.*\/([^<+]+)[<+$].*$', r"\1", i )
		current_tags = re.sub( r'^.*\/\*.*$', r"*", i )
		if current_tags != "*":
			current_tags = re.sub( r'^[^<+]*([<+].+)\$.*$', r"\1", i )
		else:
			current_lemma = re.sub( r'^\*(.*)$', r"\1", current_lemma )

		# tags
		array_tags = current_tags.split( ',' )
                # If you gonna uncomment this, you should also uncomment the same code in the previous (Collecting...) section
		#for element in array_tags:
		#	if element[0] == "+":
		#		element = re.sub( r'\+.+(<.+>)', r"\1", element )  # +сана<emph> => +сана<emph>,<emph>
		#		array_tags.append( element )
		array_tags.sort()
		current_tags = ','.join( array_tags )
		
		found_id_tags = -1
		try:
			found_id_tags = binarySearch(array_tag_combinations[0], current_tags)
		except ValueError:
			sys.exit("Couldn't find tags in the array = " + str(current_tags))
			found_id_tags = -1

		# tags_uniq
		array_tags_uniq = array_tags
		size_array_tags_uniq = len( array_tags_uniq )
		for x in range( 0, size_array_tags_uniq ):
			found_id_tags_uniq = -1
			try:
				found_id_tags_uniq = binarySearch(array_tag_uniq_combinations[0], array_tags_uniq[x])
			except ValueError:
				sys.exit("Couldn't find tag_uniq in the array = " + str(array_tags_uniq[x]));
				found_id_tags_uniq = -1

		# words_case
		found_id_words_case = -1
		word_len = len( current_word_case )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words_case = binarySearch(array_word_case_combinations[0], current_word_case)
		except ValueError:
			sys.exit("Couldn't find word in the array = " + str(current_word_case));
			found_id_words_case = -1

		# words
		found_id_words = -1
		current_word = current_word.lower()
		word_len = len( current_word )
		# Verify if fits into short in fastmorph (DEPRECATED)
		if word_len > 255:
			word_len = 255

		try:
			found_id_words = binarySearch(array_word_combinations[0], current_word)
		except ValueError:
			sys.exit("Couldn't find word in the array = " + str(current_word));
			found_id_words = -1

		# lemmas
		found_id_lemmas = -1
		current_lemma = current_lemma.lower()
		try:
			found_id_lemmas = binarySearch(array_lemma_combinations[0], current_lemma)
		except ValueError:
			sys.exit("Couldn't find lemma in the array = " + str(current_lemma))
			found_id_lemmas = -1





		# united
		found_id_united = -1
		#current_united = current_united.lower()
		current_united = str( found_id_words_case ) + "x" + str( found_id_words ) + "x" + str( found_id_lemmas ) + "x" + str( found_id_tags )
		#word_len = len( current_united )
		# Verify if fits into short in fastmorph (DEPRECATED)
		#if word_len > 255:
		#	word_len = 255

		try:
			found_id_united = binarySearch(array_united_combinations[0], current_united)
		except ValueError:
			sys.exit("Couldn't find 'united' in the array = " + str(current_united))
			found_id_united = -1







		fw.write( str( counter_id_main ) + "\t" + str( found_id_united ) + "\t" + str( counter_sentences ) + "\t" + str( source_id ) + "\n" )







		#fw.write( str( counter_id_main ) + "\t" + str( found_id_words_case ) + "\t" + str( found_id_words ) + "\t" + str( found_id_lemmas ) + "\t" + str( found_id_tags ) + "\t" + str( counter_sentences ) + "\t" + str( source_id ) + "\n" )
		counter_wordpos += 1
		counter_id_main += 1

	if counter_sentences % 1000 == 0:
		sys.stdout.write("\r  Writing main file... %d lines" % counter_sentences)
		sys.stdout.flush()

	counter_sentences += 1

sys.stdout.write("\r  Writing main file... %d lines" % (counter_sentences) )
sys.stdout.flush()
fw.flush()
os.fsync( fw )
fw.close()
print( "" )






















# Write tags file
#array_tag_combinations[0].pop( 0 ) # deleting fake empty string 
#array_tag_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_tag_combinations[0] )
for x in range( 0, size_list ):
	fw2.write( str( x ) + "\t" + str( array_tag_combinations[1][x] ) + "\t" + array_tag_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing tags file... %d lines" % x)
	sys.stdout.flush()
fw2.flush()
os.fsync( fw2 )
fw2.close()
print( "" )

# Write tags_uniq file
#array_tag_uniq_combinations[0].pop( 0 ) # deleting fake empty string 
#array_tag_uniq_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_tag_uniq_combinations[0] )
for x in range( 0, size_list ):
	fw5.write( str( x ) + "\t" + str( array_tag_uniq_combinations[1][x] ) + "\t" + array_tag_uniq_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing tags_uniq file... %d lines" % x)
	sys.stdout.flush()
fw5.flush()
os.fsync( fw5 )
fw5.close()
print( "" )

# Write words_case file
#array_word_case_combinations[0].pop( 0 ) # deleting fake empty string 
#array_word_case_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_word_case_combinations[0] )
for x in range( 0, size_list ):
	fw6.write( str( x ) + "\t" + str( array_word_case_combinations[1][x] ) + "\t" + array_word_case_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing words_case file... %d lines" % x)
	sys.stdout.flush()
fw6.flush()
os.fsync( fw6 )
fw6.close()
print( "" )

# Write words file
#array_word_combinations[0].pop( 0 ) # deleting fake empty string 
#array_word_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_word_combinations[0] )
for x in range( 0, size_list ):
	fw3.write( str( x ) + "\t" + str( array_word_combinations[1][x] ) + "\t" + array_word_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing words file... %d lines" % x)
	sys.stdout.flush()
fw3.flush()
os.fsync( fw3 )
fw3.close()
print( "" )

# Write lemmas file
#array_lemma_combinations[0].pop( 0 ) # deleting fake empty string 
#array_lemma_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_lemma_combinations[0] )
for x in range( 0, size_list ):
	fw4.write( str( x ) + "\t" + str( array_lemma_combinations[1][x] ) + "\t" + array_lemma_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing lemmas file... %d lines" % x)
	sys.stdout.flush()
fw4.flush()
os.fsync( fw4 )
fw4.close()
print( "" )










# Write united file
#array_united_combinations[0].pop( 0 ) # deleting fake empty string 
#array_united_combinations[1].pop( 0 ) # deleting fake empty string 
size_list = len( array_united_combinations[0] )
#for x in range( 0, size_list ):
for x in range( 1, size_list ):
	
	
	
	current_united_word_case =	re.sub( r'^([1234567890]+)x([1234567890]+)x([1234567890]+)x([1234567890]+)$', r"\1", array_united_combinations[0][x] )
	current_united_word =		re.sub( r'^([1234567890]+)x([1234567890]+)x([1234567890]+)x([1234567890]+)$', r"\2", array_united_combinations[0][x] )
	current_united_lemma =		re.sub( r'^([1234567890]+)x([1234567890]+)x([1234567890]+)x([1234567890]+)$', r"\3", array_united_combinations[0][x] )
	current_united_tags =		re.sub( r'^([1234567890]+)x([1234567890]+)x([1234567890]+)x([1234567890]+)$', r"\4", array_united_combinations[0][x] )
	
	
	
	fw7.write( str( x ) + "\t" + str( array_united_combinations[1][x] ) + "\t"            + current_united_word_case + "\t" + current_united_word + "\t" + current_united_lemma + "\t" + current_united_tags              + '\n' )
	
	
	#fw7.write( str( x ) + "\t" + str( array_united_combinations[1][x] ) + "\t" + array_united_combinations[0][x] + '\n' )
	sys.stdout.write("\r  Writing united file... %d lines" % x)
	sys.stdout.flush()
fw7.flush()
os.fsync( fw7 )
fw7.close()
print( "" )













#
# Sorting tags_uniq file
#
import os
print( "\n\nSorting tags_uniq file...\n" )
print( "cat " + output_filename_tags_uniq + " | sed '/\*/d' | sort -k2 -n -r | nl -v0 -nln | awk '{print $1 \"\\t\" $3 \"\\t\" $4}' > " + output_filename_tags_uniq + ".sorted.txt" )
os.system( "cat " + output_filename_tags_uniq + " | sed '/\*/d' | sort -k2 -n -r | nl -v0 -nln | awk '{print $1 \"\\t\" $3 \"\\t\" $4}' > " + output_filename_tags_uniq + ".sorted.txt" )
os.system( "rm -f " + output_filename_tags_uniq )

print( "\nDone!" )

