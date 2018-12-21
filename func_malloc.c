



/*
 * 
 */
void func_malloc() {
	
	// wordforms case sensitive
	size_list_words_case = WORDS_CASE_ARRAY_SIZE * WORDS_BUFFER_SIZE;
	list_words_case = malloc(sizeof(*list_words_case) * size_list_words_case);
	memset(list_words_case, 1, sizeof(*list_words_case) * size_list_words_case); // for detecting the very end of the string correctly

	// wordforms
	size_list_words = WORDS_ARRAY_SIZE * WORDS_BUFFER_SIZE;
	list_words = malloc(sizeof(*list_words) * size_list_words);
	memset(list_words, 1, sizeof(*list_words) * size_list_words);

	// lemmas
	size_list_lemmas = WORDS_ARRAY_SIZE * WORDS_BUFFER_SIZE;
	list_lemmas = malloc(sizeof(*list_lemmas) * size_list_lemmas);
	memset(list_lemmas, 1, sizeof(*list_lemmas) * size_list_lemmas);

	// tags
	size_list_tags = TAGS_ARRAY_SIZE * WORDS_BUFFER_SIZE;
	list_tags = malloc(sizeof(*list_tags) * size_list_tags);
	memset(list_tags, 1, sizeof(*list_tags) * size_list_tags);

	/*
	// tags_uniq
	for(int i = 0; i < TAGS_UNIQ_ARRAY_SIZE; i++) {
		size_list_tags_uniq[i] = WORDS_BUFFER_SIZE;
		list_tags_uniq[i] = malloc(sizeof(*list_tags_uniq) * size_list_tags_uniq[i]);
		memset(list_tags_uniq[i], 1, sizeof(*list_tags_uniq) * size_list_tags_uniq[i]);
	}
	*/
	// tags_uniq
	size_list_tags_uniq = TAGS_UNIQ_ARRAY_SIZE * WORDS_BUFFER_SIZE;
	list_tags_uniq = malloc(sizeof(*list_tags_uniq) * size_list_tags_uniq);
	memset(list_tags_uniq, 1, sizeof(*list_tags_uniq) * size_list_tags_uniq);

	
	
	
	
	
	// sources full
	size_list_sources = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources = malloc(sizeof(*list_sources) * size_list_sources);
	memset(list_sources, 1, sizeof(*list_sources) * size_list_sources);

	// nice
	size_list_sources_nice = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_nice = malloc(sizeof(*list_sources_nice) * size_list_sources_nice);
	memset(list_sources_nice, 1, sizeof(*list_sources_nice) * size_list_sources_nice);

	// authors
	size_list_sources_author = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_author = malloc(sizeof(*list_sources_author) * size_list_sources_author);
	memset(list_sources_author, 1, sizeof(*list_sources_author) * size_list_sources_author);

	// titles
	size_list_sources_title = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_title = malloc(sizeof(*list_sources_title) * size_list_sources_title);
	memset(list_sources_title, 1, sizeof(*list_sources_title) * size_list_sources_title);

	// dates
	size_list_sources_date = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_date = malloc(sizeof(*list_sources_date) * size_list_sources_date);
	memset(list_sources_date, 1, sizeof(*list_sources_date) * size_list_sources_date);

	// type
	size_list_sources_type = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_type = malloc(sizeof(*list_sources_type) * size_list_sources_type);
	memset(list_sources_type, 1, sizeof(*list_sources_type) * size_list_sources_type);

	// genres
	size_list_sources_genre = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_genre = malloc(sizeof(*list_sources_genre) * size_list_sources_genre);
	memset(list_sources_genre, 1, sizeof(*list_sources_genre) * size_list_sources_genre);

	// source
	size_list_sources_source = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_source = malloc(sizeof(*list_sources_source) * size_list_sources_source);
	memset(list_sources_source, 1, sizeof(*list_sources_source) * size_list_sources_source);

	// url
	size_list_sources_url = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_url = malloc(sizeof(*list_sources_url) * size_list_sources_url);
	memset(list_sources_url, 1, sizeof(*list_sources_url) * size_list_sources_url);

	// meta
	size_list_sources_meta = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_meta = malloc(sizeof(*list_sources_meta) * size_list_sources_meta);
	memset(list_sources_meta, 1, sizeof(*list_sources_meta) * size_list_sources_meta);

	// full
	size_list_sources_full = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_full = malloc(sizeof(*list_sources_full) * size_list_sources_full);
	memset(list_sources_full, 1, sizeof(*list_sources_full) * size_list_sources_full);
}
	