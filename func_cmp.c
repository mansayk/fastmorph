/*
 * Strings comparison functions are placed here
 */


/*
 * Comparison function for strings: char sort[TAGS_UNIQ_ARRAY_SIZE][TAGS_UNIQ_BUFFER_SIZE];
 */
static inline int func_cmp_strings(const void *a, const void *b)
{
	return strcmp(a, (const char*) b);
}


/*
 * Comparison function for pointers: char *ptr_words[WORDS_ARRAY_SIZE];
 */
static inline int func_cmp_strptrs(const void *a, const void *b)
{
	if(DEBUG)
		printf("\nfunc_cmp_bsearch: a(%p): %s, b(%p): %s", a, (char*) a, b, *(const char**) b);
	return strncmp((char*) a, *(const char**) b, WORDS_BUFFER_SIZE);
}


/*
 * Comparison function for integers
 */
static inline int func_cmp_ints(const void *a, const void *b)
{
	return(*(int*)a - *(int*)b);
}


/*
 * RegEx
 */
int func_regex(const char pattern[WORDS_BUFFER_SIZE], const int mask_offset, char *united_x[UNITED_ARRAY_SIZE], const int type, struct thread_data_united *thdata_united)
{
	// Setting search distances for current (by each) thread
	const unsigned long long united_begin = thdata_united->start;
	register const unsigned long long united_end = thdata_united->finish;
	regex_t start_state;
	char pattern_enclosed[WORDS_BUFFER_SIZE];

	if(DEBUG)
		printf("func_regex pattern: %s\n", pattern);

	// Add ^...$
	pattern_enclosed[0] = '\0';
	if(pattern[0] != '^')
		strncat(pattern_enclosed, "^", WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);
	strncat(pattern_enclosed, pattern, WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);
	if(pattern[strlen(pattern) - 1] != '$')
		strncat(pattern_enclosed, "$", WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);

	if(regcomp(&start_state, pattern_enclosed, REG_EXTENDED)) {
		fprintf(stderr, "RegEx: Bad pattern: '%s'\n", pattern); // TODO: return this to the user
		return 1;
	}
	for(unsigned long long i = united_begin; i < united_end; i++) {
		if(!regexec(&start_state, united_x[i], 0, NULL, 0)) {
			united_mask[i] += (unsigned long long)1 << (SEARCH_TYPES_OFFSET * mask_offset + type);
			if(DEBUG)
				printf("RegEx #%lld: %s (wordform: %s)\n", i, united_x[i], united_words[i]);
		}
	}
	regfree(&start_state);
	return 0;
}


/*
 * RegEx normalization \\ -> \ because JSON needs double backslash
 */
//int func_regex_normalization(const char match[WORDS_BUFFER_SIZE])
//{
/*
	char pattern[WORDS_BUFFER_SIZE];
	for(int x = 0; x < WORDS_BUFFER_SIZE; x++) {
		switch(match[x]) {
			case '\\':
				continue;
			default:
				pattern[x] = match[x];
				continue;
		}
	}
	printf("\nRegEx: %s -> %s", match, pattern);
*/
//	return 0;
//}


/*
 * RegEx sources
 */
int func_regex_sources(const char pattern[SOURCE_BUFFER_SIZE])
{
	regex_t start_state;
	const int type = 0; // Offset for full source. TODO: 1 - Author(s), 2 - Title, 3 - ...
	char pattern_enclosed[WORDS_BUFFER_SIZE];

	// Add ^...$
	pattern_enclosed[0] = '\0';
	if(pattern[0] != '^')
		strncat(pattern_enclosed, "^", WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);
	strncat(pattern_enclosed, pattern, WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);
	if(pattern[strlen(pattern) - 1] != '$')
		strncat(pattern_enclosed, "$", WORDS_BUFFER_SIZE - strlen(pattern_enclosed) - 1);

	if(regcomp(&start_state, pattern_enclosed, REG_EXTENDED|REG_ICASE)) { // Case insensitive
		fprintf(stderr, "RegEx: Bad pattern: '%s'\n", pattern); // TODO: return this to the user
		return 1;
	}
	for(unsigned long long i = 0; i < SOURCES_ARRAY_SIZE; i++) {
		//if(!regexec(&start_state, ptr_sources[i], 0, NULL, 0)) { // TODO: add filter by separate sources or genres?
		if(!regexec(&start_state, ptr_sources_author[i], 0, NULL, 0)) {
			source_mask[i] += (unsigned int)1 << (type);
			if(DEBUG)
				printf("\nfunc_regex_sources: %s -> %s", pattern, ptr_sources[i]);
		}
	}
	regfree(&start_state);
	return 0;
}


/*
 * Advanced pattern matching (*?) search
 */
int func_szWildMatch(const char match[WORDS_BUFFER_SIZE], const int mask_offset, char *united_x[UNITED_ARRAY_SIZE], const int type, struct thread_data_united *thdata_united)
{
	// Setting search distances for current (by each) thread
	//unsigned int thread_id = thdata_united->id;
	const unsigned long long united_begin = thdata_united->start;
	register const unsigned long long united_end = thdata_united->finish;

	char a;
	int star;
	register const char *str, *pat, *s, *p;

	if(DEBUG)
		printf("\nfunc_szWildMatch() match: %s; united_begin: %lld; united_end: %lld;\n", match, united_begin, united_end);

	for(unsigned long long i = united_begin; i < united_end; i++) {
		// Alessandro Felice Cantatore
		// http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
		// Warning! You should make *** -> * substitution in PHP (or here somewhere earlier) to work this algorithm correctly
		// or just uncomment 2 lines below.
		star = 0;
		pat = match;
		str = united_x[i];
loopStart:
		for(s = str, p = pat; *s; ++s, ++p) {
			switch(*p) {
				case '?':
					// for jumping correctly over UTF-8 character
					if(*s < 0) {
						a = *s;
						a <<= 1;
						while(a < 0) {
							a <<= 1;
							++s;
						}
					}
					break;
				case '*':
					star = 1;
					str = s, pat = p;
					//do { ++pat; } while(*pat == '*'); // PHP: *** -> *
					if(!*++pat) goto setMask;
					goto loopStart;
				default:
					//if(mapCaseTable[*s] != mapCaseTable[*p]) // Case sensitive
					if(*s != *p) goto starCheck;
					break;
			}
		}
		if(*p == '*') ++p;
		//while(*p == '*') ++p; // PHP: *** -> *
		if(!*p) goto setMask;
		continue;
starCheck:
		if(!star) continue;
		str++;
		goto loopStart;
setMask:
		united_mask[i] += (unsigned long long)1 << (SEARCH_TYPES_OFFSET * mask_offset + type);
		//if(DEBUG)
		//	printf("\nfunc_szWildMatch: %s -> %s", match, united_x[i]);
	}
	return 0;
}


/*
 * Advanced pattern matching (*?) search for sources
 */
int func_szWildMatchSource(const char match[SOURCE_BUFFER_SIZE])
{
	char a;
	int star;
	register const char *str, *pat, *s, *p;

	const int type = 0; // Offset for full source. TODO: 1 - Author(s), 2 - Title, 3 - ...

	//for(unsigned long long i = united_begin; i < united_end; i++) {
	for(unsigned long long i = 0; i < SOURCES_ARRAY_SIZE; i++) {
		// Alessandro Felice Cantatore
		// http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
		// Warning! You should make *** -> * substitution in PHP (or here somewhere earlier) to work this algorithm correctly
		// or just uncomment 2 lines below.
		star = 0;
		pat = match;
		str = ptr_sources[i];
loopStart:
		for(s = str, p = pat; *s; ++s, ++p) {
			switch(*p) {
				case '?':
					// for jumping correctly over UTF-8 character
					if(*s < 0) {
						a = *s;
						a <<= 1;
						while(a < 0) {
							a <<= 1;
							++s;
						}
					}
					break;
				case '*':
					star = 1;
					str = s, pat = p;
					//do { ++pat; } while(*pat == '*'); // PHP: *** -> *
					if(!*++pat) goto setMask;
					goto loopStart;
				default:
					//if(mapCaseTable[*s] != mapCaseTable[*p]) // Case sensitive
					if(*s != *p) goto starCheck;
					break;
			}
		}
		if(*p == '*') ++p;
		//while(*p == '*') ++p; // PHP: *** -> *
		if(!*p) goto setMask;
		continue;
starCheck:
		if(!star) continue;
		str++;
		goto loopStart;
setMask:
		source_mask[i] += (unsigned int)1 << (type);
		//if(DEBUG)
		//	printf("\nfunc_szWildMatchSource: %s -> %s", match, ptr_sources[i]);
	}
	return 0;
}


/*
 * Get id using binary search in the array of uniq elements
 */
int func_szExactMatch(char **ptr, char match[WORDS_BUFFER_SIZE], const int array_size, const int mask_offset, char *united_x[UNITED_ARRAY_SIZE], const int type)
{
	register char **found;
	found = bsearch(match, ptr, array_size, sizeof(char **), func_cmp_strptrs);
	if(found != NULL) {
		for(register int y = 0; y < UNITED_ARRAY_SIZE; y++ ) {
			if(united_x[y] == *found) {
				united_mask[y] += (unsigned long long)1 << (SEARCH_TYPES_OFFSET * mask_offset + type);
			}
		}
	}
	return 0;
}


/*
 * Fill morph_types mask
 */
int func_fill_search_mask()
{
	params = 0;
	morph_types = 0;

	// Calculate amount of params (words) to search
	for(int i = AMOUNT_TOKENS - 1; i >= 0; i--) {
		if(word[i][0] || lemma[i][0] || tags[i][0] || wildmatch[i][0] || wildmatch_lemma[i][0]) {
			params = i + 1;
			break;
		}
	}

	//if(DEBUG)
		printf("\nparams: %d", params);

	for(unsigned int x = 0; x < params; x++) {
		if(word[x][0])
			morph_types += ((unsigned long long)1 << (SEARCH_TYPES_OFFSET * x + WORD));
		if(lemma[x][0])
			morph_types += ((unsigned long long)1 << (SEARCH_TYPES_OFFSET * x + LEMMA));
		if(tags[x][0])
			morph_types += ((unsigned long long)1 << (SEARCH_TYPES_OFFSET * x + TAGS));
		if(wildmatch[x][0])
			morph_types += ((unsigned long long)1 << (SEARCH_TYPES_OFFSET * x + WILD));
		if(wildmatch_lemma[x][0])
			morph_types += ((unsigned long long)1 << (SEARCH_TYPES_OFFSET * x + WILD_LEMMA));
	}
	if(DEBUG)
		printf("\nmorph_types: %llu\n", morph_types);

	// Sources
	morph_types_source = 0; // Search in all sources
	if(strcmp(source, ""))
		morph_types_source += ((unsigned int)1 << (SOURCE)); // Search in particular sources

	if(DEBUG)
		printf("\nmorph_types_source: %d\n", morph_types_source);
	return 0;
}
