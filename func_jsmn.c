
/*
 * JSMN
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if(tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}


/*
 * JSMN
 */
int func_jsmn_json(char *strin, int len)
{
	int i;
	int r;
	int j;
	char *ptr;
	char buf[WORDS_BUFFER_SIZE];

	jsmn_parser p;
	jsmntok_t t[128]; // We expect no more than 128 tokens

	jsmn_init(&p);
	r = jsmn_parse(&p, strin, len, t, sizeof(t)/sizeof(t[0]));
	if(r < 0) {
		printf("\nFailed to parse JSON: %d", r);
		return 1;
	}

	/* Assume the top-level element is an object */
	if(r < 1 || t[0].type != JSMN_OBJECT) {
		printf("\nJSON object expected");
		return 1;
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		// Source for search
		if(jsoneq(strin, &t[i], "source") == 0) {
			printf("- source: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(source, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			source[t[i+1].end - t[i+1].start] = '\0';
			printf("%s\n", source);
			++i;
		// Words
		} else if(jsoneq(strin, &t[i], "word") == 0) {
			errno = 0;
			printf("- word[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue; // We expect groups to be an array of strings
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(word[j], strin + g->start, g->end - g->start);
				word[j][g->end - g->start] = '\0';
				printf("%s\n", word[j]);
			}
			i += t[i+1].size + 1;
		// Lemmas
		} else if(jsoneq(strin, &t[i], "lemma") == 0) {
			errno = 0;
			printf("- lemma[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(lemma[j], strin + g->start, g->end - g->start);
				lemma[j][g->end - g->start] = '\0';
				printf("%s\n", lemma[j]);
			}
			i += t[i+1].size + 1;
		// Tags
		} else if(jsoneq(strin, &t[i], "tags") == 0) {
			errno = 0;
			printf("- tags[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(tags[j], strin + g->start, g->end - g->start);
				tags[j][g->end - g->start] = '\0';
				printf("%s\n", tags[j]);
			}
			i += t[i+1].size + 1;
		// Wildmatch words
		} else if(jsoneq(strin, &t[i], "wildmatch") == 0) {
			errno = 0;
			printf("- wildmatch[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(wildmatch[j], strin + g->start, g->end - g->start);
				wildmatch[j][g->end - g->start] = '\0';
				printf("%s\n", wildmatch[j]);
			}
			i += t[i+1].size + 1;
		// Wildmatch lemmas
		} else if(jsoneq(strin, &t[i], "wildmatch_lemma") == 0) {
			errno = 0;
			printf("- wildmatch_lemma[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(wildmatch_lemma[j], strin + g->start, g->end - g->start);
				wildmatch_lemma[j][g->end - g->start] = '\0';
				printf("%s\n", wildmatch_lemma[j]);
			}
			i += t[i+1].size + 1;
		// Distances from
		} else if(jsoneq(strin, &t[i], "dist_from") == 0) {
			errno = 0;
			printf("- dist_from[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(buf, strin + g->start, g->end - g->start);
				buf[g->end - g->start] = '\0';
				errno = 0;
				dist_from[j] = strtoull(buf, &ptr, 10);
				// Check for various possible errors
				if(errno == ERANGE)
					perror("\nstrtol");
				if(ptr == buf)
					fprintf(stderr, "\nNo digits were found!");
				printf("%u\n", dist_from[j]);
			}
			i += t[i+1].size + 1;
		// Distances to
		} else if(jsoneq(strin, &t[i], "dist_to") == 0) {
			errno = 0;
			printf("- dist_to[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(buf, strin + g->start, g->end - g->start);
				buf[g->end - g->start] = '\0';
				errno = 0;
				dist_to[j] = strtoull(buf, &ptr, 10);
				// Check for various possible errors
				if(errno == ERANGE)
					perror("\nstrtol");
				if(ptr == buf)
					fprintf(stderr, "\nNo digits were found");
				printf("%u\n", dist_to[j]);
			}
			i += t[i+1].size + 1;
		// Case sensitive
		} else if(jsoneq(strin, &t[i], "case") == 0) {
			errno = 0;
			printf("- case[_]:\n");
			if(t[i+1].type != JSMN_ARRAY) {
				continue;
			}
			for (j = 0; j < t[i+1].size; j++) {
				jsmntok_t *g = &t[i+j+2];
				printf("  * %.*s / ", g->end - g->start, strin + g->start);
				strncpy(buf, strin + g->start, g->end - g->start);
				buf[g->end - g->start] = '\0';
				errno = 0;
				case_sensitive[j] = strtoull(buf, &ptr, 10);
				// Check for various possible errors
				if(errno == ERANGE)
					perror("\nstrtol");
				if(ptr == buf)
					fprintf(stderr, "\nNo digits were found");
				printf("%u\n", case_sensitive[j]);
			}
			i += t[i+1].size + 1;
		// Return (Number of sentences to return)
		} else if(jsoneq(strin, &t[i], "return") == 0) {
			printf("- return: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(buf, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			buf[t[i+1].end - t[i+1].start] = '\0';
			errno = 0;
			return_sentences = strtol(buf, &ptr, 10);
			// Check for ranges TODO: Merge variables return_sentences and return_ngrams into one???
			if(return_sentences < 1 || return_sentences > 1000) {
				return_sentences = FOUND_SENTENCES_LIMIT_DEFAULT;
			}
			if(return_ngrams < 1 || return_ngrams > 1000) {
				return_ngrams = FOUND_NGRAMS_LIMIT_DEFAULT;
			}
			// Check for various possible errors
			if(errno == ERANGE)
				perror("\nstrtol");
			if(ptr == buf)
				fprintf(stderr, "\nNo digits were found");
			printf("%d\n", return_sentences);
			++i;
		// Last position
		} else if(jsoneq(strin, &t[i], "last_pos") == 0) {
			printf("- last_pos: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(morph_last_pos, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			morph_last_pos[t[i+1].end - t[i+1].start] = '\0';
			printf("%s\n", morph_last_pos);
			++i;
		// Regex (Is regex mode enabled?)
		} else if(jsoneq(strin, &t[i], "regex") == 0) {
			printf("- regex: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(buf, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			buf[t[i+1].end - t[i+1].start] = '\0';
			errno = 0;
			regex = strtol(buf, &ptr, 10);
			// Check for various possible errors
			if(errno == ERANGE)
				perror("\nstrtol");
			if(ptr == buf)
				fprintf(stderr, "\nNo digits were found");
			printf("%d\n", regex);
			++i;
		// Extend context
		} else if(jsoneq(strin, &t[i], "extend") == 0) {
			printf("- extend: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(buf, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			buf[t[i+1].end - t[i+1].start] = '\0';
			errno = 0;
			extend = strtol(buf, &ptr, 10);
			// Check for ranges
			if(extend < -1 || extend >= SIZE_ARRAY_MAIN) {
				extend = -1;
			}
			// Check for various possible errors
			if(errno == ERANGE)
				perror("\nstrtol");
			if(ptr == buf)
				fprintf(stderr, "\nNo digits were found");
			printf("%d\n", extend);
			++i;
		// Extend context range
		} else if(jsoneq(strin, &t[i], "extend_range") == 0) {
			printf("- extend_range: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(buf, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			buf[t[i+1].end - t[i+1].start] = '\0';
			errno = 0;
			extend_range = strtol(buf, &ptr, 10);
			// Check for ranges
			if(/*extend_range < 0 ||*/ extend_range >= EXTEND_RANGE_MAX) {
				extend_range = EXTEND_RANGE_DEFAULT;
			}
			// Check for various possible errors
			if(errno == ERANGE)
				perror("\nstrtol");
			if(ptr == buf)
				fprintf(stderr, "\nNo digits were found");
			printf("%d\n", extend_range);
			++i;
		// Not recognized
		} else {
			printf("***ERROR: Unexpected key: %.*s\n", t[i].end-t[i].start, strin + t[i].start);
		}
	}
	return 0;
}
