
/*
 * 
 */
void func_realloc() {
	
	// resizing list_words_case
	printf("\n\nList_words_case:");
	printf("\n  Old size: %d bytes", size_list_words_case);
	fflush(stdout);
	do { --size_list_words_case; } while(list_words_case[size_list_words_case]); // we needed memset 1 for this
	++size_list_words_case;
	printf("\n  New size: %d bytes", size_list_words_case);
	list_words_case = realloc(list_words_case, size_list_words_case);

	// resizing list_words
	printf("\n\nList_words:");
	printf("\n  Old size: %d bytes", size_list_words);
	do { --size_list_words; } while(list_words[size_list_words]);
	++size_list_words;
	printf("\n  New size: %d bytes", size_list_words);
	list_words = realloc(list_words, size_list_words);

	// resizing list_lemmas
	printf("\n\nList_lemmas:");
	printf("\n  Old size: %d bytes", size_list_lemmas);
	do { --size_list_lemmas; } while(list_lemmas[size_list_lemmas]);
	++size_list_lemmas;
	printf("\n  New size: %d bytes", size_list_lemmas);
	list_lemmas = realloc(list_lemmas, size_list_lemmas);

	// resizing list_tags
	printf("\n\nList_tags:");
	printf("\n  Old size: %d bytes", size_list_tags);
	do { --size_list_tags; } while(list_tags[size_list_tags]);
	++size_list_tags;
	printf("\n  New size: %d bytes", size_list_tags);
	list_tags = realloc(list_tags, size_list_tags);
	
	/*
	// resizing list_tags_uniq
	printf("\n\nList_tags_uniq:");
	for(int i = 0; i < TAGS_UNIQ_ARRAY_SIZE; i++) {
		printf("\n  Old size: %d bytes", size_list_tags_uniq[i]);
		do { --size_list_tags_uniq[i]; } while(list_tags_uniq[i][size_list_tags_uniq[i]]);
		++size_list_tags_uniq[i];
		printf("\n  New size: %d bytes", size_list_tags_uniq[i]);
		list_tags_uniq[i] = realloc(list_tags_uniq[i], size_list_tags_uniq[i]);
	}
	*/
	// resizing list_tags_uniq
	printf("\n\nList_tags_uniq:");
	printf("\n  Old size: %d bytes", size_list_tags_uniq);
	do { --size_list_tags_uniq; } while(list_tags_uniq[size_list_tags_uniq]);
	++size_list_tags_uniq;
	printf("\n  New size: %d bytes", size_list_tags_uniq);
	list_tags_uniq = realloc(list_tags_uniq, size_list_tags_uniq);

	

	
	// resizing list_sources
	//printf("\n\nList_sources:");
	//printf("\n  Old size: %d bytes", size_list_sources);
	//do { --size_list_sources; } while(list_sources[size_list_sources]);
	//++size_list_sources;
	//printf("\n  New size: %d bytes", size_list_sources);
	//list_sources = realloc(list_sources, size_list_sources);

	// resizing nice
	printf("\n\nList_sources (nice):");
	printf("\n  Old size: %d bytes", size_list_sources_nice);
	do { --size_list_sources_nice; } while(list_sources_nice[size_list_sources_nice]);
	++size_list_sources_nice;
	printf("\n  New size: %d bytes", size_list_sources_nice);
	list_sources_nice = realloc(list_sources_nice, size_list_sources_nice);

	// resizing authors
	printf("\n\nList_sources (authors):");
	printf("\n  Old size: %d bytes", size_list_sources_author);
	do { --size_list_sources_author; } while(list_sources_author[size_list_sources_author]);
	++size_list_sources_author;
	printf("\n  New size: %d bytes", size_list_sources_author);
	list_sources_author = realloc(list_sources_author, size_list_sources_author);

	// resizing titles
	printf("\n\nList_sources (titles):");
	printf("\n  Old size: %d bytes", size_list_sources_title);
	do { --size_list_sources_title; } while(list_sources_title[size_list_sources_title]);
	++size_list_sources_title;
	printf("\n  New size: %d bytes", size_list_sources_title);
	list_sources_title = realloc(list_sources_title, size_list_sources_title);

	// resizing dates
	printf("\n\nList_sources (dates):");
	printf("\n  Old size: %d bytes", size_list_sources_date);
	do { --size_list_sources_date; } while(list_sources_date[size_list_sources_date]);
	++size_list_sources_date;
	printf("\n  New size: %d bytes", size_list_sources_date);
	list_sources_date = realloc(list_sources_date, size_list_sources_date);

	// resizing type
	printf("\n\nList_sources (types):");
	printf("\n  Old size: %d bytes", size_list_sources_type);
	do { --size_list_sources_type; } while(list_sources_type[size_list_sources_type]);
	++size_list_sources_type;
	printf("\n  New size: %d bytes", size_list_sources_type);
	list_sources_type = realloc(list_sources_type, size_list_sources_type);

	// resizing genres
	printf("\n\nList_sources (genres):");
	printf("\n  Old size: %d bytes", size_list_sources_genre);
	do { --size_list_sources_genre; } while(list_sources_genre[size_list_sources_genre]);
	++size_list_sources_genre;
	printf("\n  New size: %d bytes", size_list_sources_genre);
	list_sources_genre = realloc(list_sources_genre, size_list_sources_genre);

	// resizing source
	printf("\n\nList_sources (sources):");
	printf("\n  Old size: %d bytes", size_list_sources_source);
	do { --size_list_sources_source; } while(list_sources_source[size_list_sources_source]);
	++size_list_sources_source;
	printf("\n  New size: %d bytes", size_list_sources_source);
	list_sources_source = realloc(list_sources_source, size_list_sources_source);

	// resizing url
	printf("\n\nList_sources (url):");
	printf("\n  Old size: %d bytes", size_list_sources_url);
	do { --size_list_sources_url; } while(list_sources_url[size_list_sources_url]);
	++size_list_sources_url;
	printf("\n  New size: %d bytes", size_list_sources_url);
	list_sources_url = realloc(list_sources_url, size_list_sources_url);

	// resizing meta
	printf("\n\nList_sources (meta):");
	printf("\n  Old size: %d bytes", size_list_sources_meta);
	do { --size_list_sources_meta; } while(list_sources_meta[size_list_sources_meta]);
	++size_list_sources_meta;
	printf("\n  New size: %d bytes", size_list_sources_meta);
	list_sources_meta = realloc(list_sources_meta, size_list_sources_meta);

	// resizing full
	printf("\n\nList_sources (full):");
	printf("\n  Old size: %d bytes", size_list_sources_full);
	do { --size_list_sources_full; } while(list_sources_full[size_list_sources_full]);
	++size_list_sources_full;
	printf("\n  New size: %d bytes", size_list_sources_full);
	list_sources_full = realloc(list_sources_full, size_list_sources_full);

		
	
}