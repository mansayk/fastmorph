/*
 * fastmorph.c - Fast corpus search engine.
 * Version v5.6.1 - 2018.03.25
 *
 * "fastmorph" is a high speed search engine for text corpora:
 *   - loads all preprocessed data from MySQL (MariaDB) into RAM;
 *   - creates UNIX domain socket file as a server and accepts queries in JSON format;
 *   - has multithreading support.
 *
 * Copyright (C) 2014-present Mansur Saykhunov <tatcorpus@gmail.com>
 * Acknowledgements:
 *   I am so grateful to Rustem Khusainov for his invaluable help during all this work!
 */

#define _GNU_SOURCE		/*   strndup		   */

#include <stdio.h>           	/*   printf, fgets	   */
#include <stdlib.h>          	/*   atol		   */
#include <string.h>         	/*   strcat, strlen	   */
#include <errno.h>          	/*   strerror(errno)	   */
#include <sys/stat.h>		/*   chmod		   */
#include <pthread.h>		/*   threading		   */
#include <semaphore.h>		/*   semaphores		   */
#include <sys/socket.h>		/*   socket		   */
#include <sys/un.h>		/*   socket		   */
#include <unistd.h>		/*   write		   */
#include <sys/time.h>		/*   gettimeofday	   */
#include <mysql/mysql.h>	/*   MySQL and MariaDB	   */
#include "jsmn-master/jsmn.h"	/*   JSON		   */
#include <limits.h>		/*   LONG_MIN, ULLONG_MAX  */
#include <locale.h>		/*   for regcomp, regexec  */
#include <regex.h>		/*   regcomp, regexec	   */

#include "fastmorph.h"
#include "credentials.h"	/*   DB login, password... */

/**************************************************************************************
 *
 *                          Global variables
 *
 **************************************************************************************/

MYSQL *myconnect;

// For socket file
int cl, rc;
char *socket_path = UNIX_DOMAIN_SOCKET;

// Query processing time
struct timeval tv1, tv2, tv3, tv4; // different counters
struct timezone tz;

// Semaphore
static sem_t count_sem;

// Counter from return_sentences to 0
unsigned int found_limit;

// main arrays
int *array_united;

// additional small arrays
char array_tags_uniq[TAGS_UNIQ_ARRAY_SIZE][WORDS_BUFFER_SIZE];		/*   Array of uniq tags   */
int size_array_tags_uniq;

char *list_words_case;							/*   Long string for words (case sensitive): "Кеше\0алманы\0табага\0"   */
int size_list_words_case;

char *list_words;							/*   Long string for words   */
int size_list_words;

char *list_lemmas;							/*   Long string for lemmas   */
int size_list_lemmas;

char *list_tags;							/*   Long string for tags   */
int size_list_tags;

char *list_sources;							/*   Long string for sources   */
int size_list_sources;

char *list_sources_author;						/*   Long string for sources authors   */
int size_list_sources_author;

char *list_sources_title;						/*   Long string for sources titles   */
int size_list_sources_title;

char *list_sources_date;						/*   Long string for sources dates   */
int size_list_sources_date;

char *list_sources_genre;						/*   Long string for sources genres   */
int size_list_sources_genre;

//char source_types[SOURCES_ARRAY_SIZE][SOURCE_TYPES_BUFFER_SIZE];	/*   source types: book, www...com   */

int sentence_source[AMOUNT_SENTENCES];					/*   Sentence-source associations. TODO: remove this (40Mb of RAM) and make through source in main array!   */

char *ptr_words_case[WORDS_CASE_ARRAY_SIZE];				/*   Array of pointers to list_words_case elements   */
char *ptr_words[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_lemmas[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_tags[TAGS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_sources[SOURCES_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_sources_author[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_title[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_date[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_genre[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */

char wildmatch[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: *еш* -> еш (кеше, ешрак, теш)   */
char wildmatch_lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: (*еш*) -> еш (кеше, ешрак, теш)   */
char word[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кешегә, ешрак, тешнең   */
char lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кеше, еш, теш   */
char tags[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: *<n>*<nom>*<pl>*   */

char source[SOURCE_BUFFER_SIZE];					/*   search string for sources: (*еш*) -> еш (кеше, ешрак, теш)   */

char *united_words_case[UNITED_ARRAY_SIZE];				/*   Array of not uniq pointers to list_words_case elements   */
char *united_words[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */
char *united_lemmas[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */
char *united_tags[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */

unsigned long long united_mask[UNITED_ARRAY_SIZE];			/*   Array of bits fields   */

unsigned int source_mask[SOURCES_ARRAY_SIZE];				/*   For search in subcorpora and particular texts (bitmask)   */

unsigned int dist_from[AMOUNT_TOKENS];					/*   search distances to the next word   */
unsigned int dist_to[AMOUNT_TOKENS];

struct thread_data {							/*   Structure to communicate with threads   */
	unsigned int id;
	unsigned long long start;
	unsigned long long finish;
	unsigned long long last_pos;
	unsigned int found_num;
	unsigned int first_sentence;
	unsigned int progress;
};
struct thread_data thread_data_array[SEARCH_THREADS];

struct thread_data_united {						/*   Structure to communicate with threads   */
	unsigned int id;
	unsigned long long start;
	unsigned long long finish;
};
struct thread_data_united thread_data_array_united[SEARCH_THREADS];

unsigned int size_array_found_sents_all_summ;				/*   For statistics: ...found all summary...   */

unsigned long long morph_types;						/*   Bits with search data   */
unsigned int morph_types_source;					/*   Bits with search data for sources   */
unsigned int case_sensitive[AMOUNT_TOKENS];				/*   Array for case sensitive (1) or not (0)   */
char morph_last_pos[WORDS_BUFFER_SIZE];					/*   The id of last found token to continue from   */
unsigned int return_sentences;						/*   Number of sentences to return   */
unsigned int params;							/*   Number of tokens (1-5) to search   */
unsigned int regex;							/*   Is regex mode enabled: 1 or 0   */
unsigned int extend_range;						/*   Range for extending context   */
int extend;								/*   Sentence id for extending context   */

// Mutexes and conditions for pthread_cond_wait()
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
int finished = 0;

// Mutexes and conditions for pthread_cond_wait() "united"
pthread_mutex_t mutex_united = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_united = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex2_united = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2_united = PTHREAD_COND_INITIALIZER;
int finished_united = 0;

/**************************************************************************************
 *
 *                                  Functions
 *
 **************************************************************************************/

/*
 * Get start time
 */
void time_start(struct timeval *tv)
{
	gettimeofday(tv, &tz);
}


/*
 * Get finish time and calculate the difference
 */
long time_stop(struct timeval *tv_begin)
{
	struct timeval tv, dtv;
	gettimeofday(&tv, &tz);
	dtv.tv_sec = tv.tv_sec - tv_begin->tv_sec;
	dtv.tv_usec = tv.tv_usec - tv_begin->tv_usec;
	if(dtv.tv_usec < 0) {
		dtv.tv_sec--;
		dtv.tv_usec += 1000000;
	}
	return (dtv.tv_sec * 1000) + (dtv.tv_usec / 1000);
}


/*
 * JSMN
 */
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
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
			// Check for ranges
			if(return_sentences < 1 || return_sentences > 1000) {
				return_sentences = FOUND_SENTENCES_LIMIT_DEFAULT;
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


/*
 * Connecting (or reconnecting) to MySQL (MariaDB)
 */
int func_connect_mysql()
{
	printf("MySQL: ");
	myconnect = mysql_init(NULL);

	if(!(mysql_real_connect(myconnect, "localhost", DBUSER, DBPASSWORD, DB, 3308, NULL, 0))) {
		printf("\n  ***ERROR: Cannot connect to MySQL: \n");
		fprintf(stderr, "%s\n", mysql_error(myconnect));
		mysql_close(myconnect);
		return -1;
	}
	if(!(mysql_set_character_set(myconnect, "utf8"))) {
		printf("\n  MySQL Server Status: %s", mysql_stat(myconnect));
		printf("\n  New client character set: %s", mysql_character_set_name(myconnect));
	}
	return 0;
}


/*
 * Reading data from MySQL (MariaDB) to arrays
 */
int func_read_mysql()
{
	MYSQL_RES *myresult;
	MYSQL_ROW row;
	char *ptr;
	char *ptr_author;
	char *ptr_title;
	char *ptr_date;
	char *ptr_genre;
	char *endptr;
	char mycommand[200];
	int i;
	int iauthor;
	int ititle;
	int idate;
	int igenre;
	int x;

	if(func_connect_mysql() == 0) {
		printf("\n  Beginning data import...");

		// load tags_uniq table
		sprintf(mycommand, "SELECT id, tag FROM morph6_tags_uniq_apertium LIMIT %d", TAGS_UNIQ_ARRAY_SIZE);
		if(mysql_query(myconnect, mycommand)) {
			printf("\n    ***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n    ***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				while((row = mysql_fetch_row(myresult))) {
					errno = 0;
					x = (int) strtol(row[0], &endptr, 10);
					if(errno == ERANGE)
						perror("\nstrtol");
					if(row[0] == endptr)
						fprintf(stderr, "\nNo digits were found");
					strncpy(array_tags_uniq[x], row[1], WORDS_BUFFER_SIZE - 1);
					array_tags_uniq[x][WORDS_BUFFER_SIZE - 1] = '\0';
					++i;
				}
				size_array_tags_uniq = mysql_num_rows(myresult);
				printf("\n    Tags-uniq: %d rows imported.", size_array_tags_uniq);
			}
			if(mysql_errno(myconnect)) {
				printf("\n    ***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load tags (combinations) table
		ptr = list_tags;
		if(mysql_query(myconnect, "SELECT id, combinations FROM morph6_tags_apertium")) {
			printf("\n    ***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n    ***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				while((row = mysql_fetch_row(myresult))) {
					ptr_tags[i] = ptr;
					ptr = strncpy(ptr, row[1], WORDS_BUFFER_SIZE - 1);
					ptr[WORDS_BUFFER_SIZE - 1] = '\0';
					ptr += strlen(row[1]) + 1;
					++i;
				}
				printf("\n    Tags: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n    ***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load words_case table
		printf("\n    Words_case: getting list...");
		fflush(stdout);
		ptr = list_words_case;
		if(mysql_query(myconnect, "SELECT word_case FROM morph6_words_case_apertium")) {
			printf("\n***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				printf("\33[2K\r    Words_case: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					ptr_words_case[i] = ptr;
					ptr = strncpy(ptr, row[0], WORDS_BUFFER_SIZE - 1);
					ptr[WORDS_BUFFER_SIZE - 1] = '\0';
					ptr += strlen(row[0]) + 1;
					++i;
				}
				printf("\33[2K\r    Words_case: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load words table
		printf("\n    Words: getting list...");
		fflush(stdout);
		ptr = list_words;
		if(mysql_query(myconnect, "SELECT word FROM morph6_words_apertium")) {
			printf("\n***ERROR: Cannot make query to MySQL!");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				printf("\33[2K\r    Words: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					ptr_words[i] = ptr;
					ptr = strncpy(ptr, row[0], WORDS_BUFFER_SIZE - 1);
					ptr[WORDS_BUFFER_SIZE - 1] = '\0';
					ptr += strlen(row[0]) + 1;
					++i;
				}
				printf("\33[2K\r    Words: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load lemmas
		printf("\n    Lemmas: getting list...");
		fflush(stdout);
		ptr = list_lemmas;
		if(mysql_query(myconnect, "SELECT lemma FROM morph6_lemmas_apertium")) {
			printf("\n***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				printf("\33[2K\r    Lemmas: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					ptr_lemmas[i] = ptr;
					ptr = strncpy(ptr, row[0], WORDS_BUFFER_SIZE - 1);
					ptr[WORDS_BUFFER_SIZE - 1] = '\0';
					ptr += strlen(row[0]) + 1;
					++i;
				}
				printf("\33[2K\r    Lemmas: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load united
		printf("\n    United: getting list...");
		fflush(stdout);
		if(mysql_query(myconnect, "SELECT word_case, word, lemma, tags FROM morph6_united_apertium")) {
			printf("\n***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				printf("\33[2K\r    United: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					x = (int) strtol(row[0], &endptr, 10);
					united_words_case[i] = ptr_words_case[x];
					x = (int) strtol(row[1], &endptr, 10);
					united_words[i] = ptr_words[x];
					x = (int) strtol(row[2], &endptr, 10);
					united_lemmas[i] = ptr_lemmas[x];
					x = (int) strtol(row[3], &endptr, 10);
					united_tags[i] = ptr_tags[x];
					++i;
				}
				printf("\33[2K\r    United: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load sources
		printf("\n    Sources: getting list...");
		fflush(stdout);
		ptr = list_sources;
		ptr_author = list_sources_author;
		ptr_title = list_sources_title;
		ptr_date = list_sources_date;
		ptr_genre = list_sources_genre;
		if(mysql_query(myconnect, "SELECT author, title, date, genre FROM sources ORDER BY id")) {
			printf("\n***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				iauthor = 0;
				ititle = 0;
				idate = 0;
				igenre = 0;
				printf("\33[2K\r    Sources: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					// full (author(s) + title)
					ptr_sources[i] = ptr;
					ptr = strncpy(ptr, row[1], SOURCE_BUFFER_SIZE - 1);
					ptr[SOURCE_BUFFER_SIZE - 1] = '\0';
					//printf("\nk: %s", ptr);
					ptr += strlen(row[1]) + 1;
					//strncpy(source_types[i], row[1], SOURCE_TYPES_BUFFER_SIZE - 1);
					//source_types[i][SOURCE_TYPES_BUFFER_SIZE - 1] = '\0';
					++i;
					// authors
					ptr_sources_author[iauthor] = ptr_author;
					ptr_author = strncpy(ptr_author, row[0], SOURCE_BUFFER_SIZE - 1);
					ptr_author[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_author += strlen(row[0]) + 1;
					++iauthor;
					// titles
					ptr_sources_title[ititle] = ptr_title;
					ptr_title = strncpy(ptr_title, row[1], SOURCE_BUFFER_SIZE - 1);
					ptr_title[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_title += strlen(row[1]) + 1;
					++ititle;
					// dates
					ptr_sources_date[idate] = ptr_date;
					ptr_date = strncpy(ptr_date, row[2], SOURCE_BUFFER_SIZE - 1);
					ptr_date[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_date += strlen(row[2]) + 1;
					++idate;
					// genres
					ptr_sources_genre[igenre] = ptr_genre;
					ptr_genre = strncpy(ptr_genre, row[3], SOURCE_BUFFER_SIZE - 1);
					ptr_genre[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_genre += strlen(row[3]) + 1;
					++igenre;
				}
				printf("\33[2K\r    Sources: %lld rows imported.", mysql_num_rows(myresult));
			}
			if(mysql_errno(myconnect)) {
				printf("\n***ERROR: MySQL - some error occurred!\n");
				exit(-1);
			}
			mysql_free_result(myresult);
		}
		fflush(stdout);

		// load main table and sentence-source associations
		printf("\n");
		i = 0;
		int p = 0;
		int m = 0;
		int sent_last = 0;
		int sent_curnt = 0;
		int source_last = 0;
		int source_curnt = 0;
		int n_mysql_load_limit = 0;

		for(int t = 0; t < SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE; t = t + MYSQL_LOAD_LIMIT) {
			if(SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - t < MYSQL_LOAD_LIMIT) {
				n_mysql_load_limit = SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - t;
			} else {
				n_mysql_load_limit = MYSQL_LOAD_LIMIT;
			}
			sprintf(mycommand, "SELECT united, sentence, source FROM morph6_main_apertium WHERE id >= %d LIMIT %d", t, n_mysql_load_limit);
			if(mysql_query(myconnect, mycommand)) {
				printf("\n    ***ERROR: Cannot make query to MySQL!\n");
				exit(-1);
			} else {
				if((myresult = mysql_store_result(myconnect)) == NULL) {
					printf("\n    ***ERROR: MySQL - problem in 'myresult'!\n");
					exit(-1);
				} else {
					while((row = mysql_fetch_row(myresult))) {
						errno = 0;
						sent_curnt = (int) strtol(row[1], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						errno = 0;
						source_curnt = (int) strtol(row[2], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						if(i == 0 || source_last != source_curnt) {
							array_united[i] = SOURCE_OFFSET - source_curnt;
							++i;
							source_last = source_curnt;
							sent_last = -1;
						}
						if(i != 0 && sent_last == sent_curnt) {
							errno = 0;
							array_united[i] = (int) strtol(row[0], &endptr, 10);
							if(errno == ERANGE)
								perror("\nstrtol");
							if(row[0] == endptr)
								fprintf(stderr, "\nNo digits were found");
						} else {
							errno = 0;
							array_united[i] = (int) -(strtol(row[0], &endptr, 10)); // unary "-"
							if(errno == ERANGE)
								perror("\nstrtol");
							if(row[0] == endptr)
								fprintf(stderr, "\nNo digits were found");

							sentence_source[m++] = (int) strtol(row[2], &endptr, 10); // source id
							sent_last = sent_curnt;
						}
						++i;
					}
					p += mysql_num_rows(myresult);
					printf("\33[2K\r    Main: %d / %d (of %d / %d) rows imported.", p, i, SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE, SIZE_ARRAY_MAIN);
					fflush(stdout);
				}
				if(mysql_errno(myconnect)) {
					printf("\n    ***ERROR: MySQL - some error occurred!\n");
					exit(-1);
				}
				mysql_free_result(myresult);
			}
		}
		printf("\n    Data import's done!");
	} else {
		exit(-1);
	}
	mysql_close (myconnect);
	return 0;
}


/*
 * Finding search distances for each thread
 */
int func_find_distances_for_threads()
{
	printf("\n\nSearch distances for each thread:");
	unsigned int x, y;
	unsigned int sentence = 0;
	unsigned long long min_part = SIZE_ARRAY_MAIN / SEARCH_THREADS;
	for(x = 0; x < SEARCH_THREADS; x++) {
		// distances
		thread_data_array[x].finish = x * min_part + min_part - 1;
		if(x == 0) {
			thread_data_array[x].start = 0;
		} else {
			thread_data_array[x].start = thread_data_array[x-1].finish + 1;
		}
		if(x == SEARCH_THREADS - 1) {
			thread_data_array[x].finish = SIZE_ARRAY_MAIN - 1;
		} else {
			while(array_united[thread_data_array[x].finish] > SOURCE_OFFSET) {
				++thread_data_array[x].finish;
			}
			--thread_data_array[x].finish;
		}
		// sentences
		thread_data_array[x].first_sentence = sentence;
		y = thread_data_array[x].start;
		while(y <= thread_data_array[x].finish) {
			if(array_united[y] > SOURCE_OFFSET && array_united[y] < 0) {
				++sentence;
			}
			++y;
		}
		printf("\n  Thread #%d part:%llu start:%llu finish:%llu first_sentence:%u first_element:%d", x, min_part, thread_data_array[x].start, thread_data_array[x].finish, thread_data_array[x].first_sentence, array_united[thread_data_array[x].start]);
	}
	return 0;
}


/*
 * Finding search distances for each thread united
 */
int func_find_distances_for_threads_united()
{
	printf("\n\nSearch distances for each thread united:");
	unsigned long long min_part = UNITED_ARRAY_SIZE / SEARCH_THREADS;

	for(int x = 0; x < SEARCH_THREADS; x++) {
		thread_data_array_united[x].finish = x * min_part + min_part - 1;
		if(x == 0) {
			thread_data_array_united[x].start = 0;
		} else {
			thread_data_array_united[x].start = (thread_data_array_united[x-1].finish + 1);
		}
		if(x == SEARCH_THREADS - 1) {
			thread_data_array_united[x].finish = UNITED_ARRAY_SIZE - 1;
		}
		printf("\n  Thread united #%d part:%llu start:%llu finish:%llu", x, min_part, thread_data_array_united[x].start, thread_data_array_united[x].finish);
	}
	return 0;
}


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
 * Build text sentences from array_main
 */
int func_build_sents(unsigned int end, unsigned int curnt_sent, unsigned long long sent_begin, unsigned long long z1, unsigned long long z2, unsigned long long z3, unsigned long long z4, unsigned long long z5, unsigned long long z6)
{
	int x, y;
	unsigned long long z[] = {z1, z2, z3, z4, z5, z6};
	int lspace = 1;
	int rspace = 1;
	int quote_open = 0;
	//char bufout[SOCKET_BUFFER_SIZE];
	char bufout[SOCKET_BUFFER_SIZE];
	char temp[SOCKET_BUFFER_SIZE];

	strncpy(bufout, "{\"id\":", SOCKET_BUFFER_SIZE - 1);
	bufout[SOCKET_BUFFER_SIZE - 1] = '\0';
	snprintf(temp, 100, "%d", curnt_sent);
	strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	/*
	strncat(bufout, ",\"source\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	temp[SOCKET_BUFFER_SIZE - 1] = '\0';
	x = strlen(bufout);
	y = 0;
	while(temp[y]) {
		if(temp[y] == '"') {
			bufout[x] = '\\';
			bufout[++x] = '"';
		} else {
			bufout[x] = temp[y];
		}
		++y;
		++x;
	}
	bufout[x] = '"';
	bufout[++x] = ',';
	bufout[++x] = '\0';
	*/

	strncat(bufout, ",\"author\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources_author[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	temp[SOCKET_BUFFER_SIZE - 1] = '\0';
	x = strlen(bufout);
	y = 0;
	while(temp[y]) {
		if(temp[y] == '"') {
			bufout[x] = '\\';
			bufout[++x] = '"';
		} else {
			bufout[x] = temp[y];
		}
		++y;
		++x;
	}
	bufout[x] = '"';
	//bufout[++x] = ',';
	bufout[++x] = '\0';

	strncat(bufout, ",\"title\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources_title[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	temp[SOCKET_BUFFER_SIZE - 1] = '\0';
	x = strlen(bufout);
	y = 0;
	while(temp[y]) {
		if(temp[y] == '"') {
			bufout[x] = '\\';
			bufout[++x] = '"';
		} else {
			bufout[x] = temp[y];
		}
		++y;
		++x;
	}
	bufout[x] = '"';
	//bufout[++x] = ',';
	bufout[++x] = '\0';

	strncat(bufout, ",\"date\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources_date[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	temp[SOCKET_BUFFER_SIZE - 1] = '\0';
	x = strlen(bufout);
	y = 0;
	while(temp[y]) {
		if(temp[y] == '"') {
			bufout[x] = '\\';
			bufout[++x] = '"';
		} else {
			bufout[x] = temp[y];
		}
		++y;
		++x;
	}
	bufout[x] = '"';
	//bufout[++x] = ',';
	bufout[++x] = '\0';

	strncat(bufout, ",\"genre\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources_genre[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	temp[SOCKET_BUFFER_SIZE - 1] = '\0';
	x = strlen(bufout);
	y = 0;
	while(temp[y]) {
		if(temp[y] == '"') {
			bufout[x] = '\\';
			bufout[++x] = '"';
		} else {
			bufout[x] = temp[y];
		}
		++y;
		++x;
	}
	bufout[x] = '"';
	//bufout[++x] = ',';
	bufout[++x] = '\0';

	/*
	strncat(bufout, "\"source_type\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncat(bufout, source_types[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	*/

	strncat(bufout, ",\"sentence\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	y = 0;
	do {
		// Punctuation and whitespaces
		rspace = 1;
		switch(united_words_case[abs(array_united[sent_begin])][0]) {
			case '(':
			case '[':
			case '{':
				rspace = 0;
				break;
			case '.':
			case '!':
			case '?':
			case ',':
			case ':':
			case ';':
			case ')':
			case ']':
			case '}':
			case '%':
				lspace = 0;
				break;
			case '`':
				rspace = 0;
				lspace = 0;
				break;
			case '"':
				if(quote_open)
					lspace = 0;
				else
					rspace = 0;
				quote_open = !quote_open;
				break;
		}

		if(DEBUG)
			printf("\nfunc_build_sents: z1=%lld\tsent_begin=%lld\tsentence=%d\tposition=%lld\tunited=%d\tword=%s", z1, sent_begin, curnt_sent, sent_begin, array_united[sent_begin], united_words_case[abs(array_united[sent_begin])]);

		// 2-byte quotes ( « » ) !!! NOT TESTED !!!
		if(!strncmp(united_words_case[abs(array_united[sent_begin])], "«", 2))
			rspace = 0;
		else if(!strncmp(united_words_case[abs(array_united[sent_begin])], "»", 2))
			lspace = 0;

		if(lspace)
			strncat(bufout, " ", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

		// html tag open
		if(z[y] == sent_begin) {
			// found: lemma + tags
			if(united_lemmas[abs(array_united[sent_begin])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, FOUND_HTML_OPEN, y, "\\", united_lemmas[abs(array_united[z[y]])], united_tags[abs(array_united[z[y]])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, FOUND_HTML_OPEN, y, "", united_lemmas[abs(array_united[z[y]])], united_tags[abs(array_united[z[y]])]);
			y++;
		} else {
			// regular: lemma + tags
			if(united_lemmas[abs(array_united[sent_begin])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_united[sent_begin])], united_tags[abs(array_united[sent_begin])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_united[sent_begin])], united_tags[abs(array_united[sent_begin])]);
		}
		strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

		// escape if word is '"'
		if(united_words_case[abs(array_united[sent_begin])][0] == '"')
			strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

		// word itself
		strncat(bufout, united_words_case[abs(array_united[sent_begin])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

		// html tag close
		strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

		if(!rspace)
			lspace = rspace;
		else
			lspace = 1;
	} while(array_united[++sent_begin] >= 0 && sent_begin < end);

	strncat(bufout, "\"},", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	if(DEBUG)
		printf("\nDEBUG> bufout: %s", bufout);

	// Sending JSON string over socket file
	if(write(cl, bufout, strlen(bufout)) != (unsigned int) strlen(bufout)) {
		if(rc > 0) {
			fprintf(stderr,"Partial write");
		} else {
			perror("Write error");
			exit(-1);
		}
	}
	return 0;
}


/*
 * Run search cycle
 */
void * func_run_cycle(struct thread_data *thdata)
{
	// Setting search distances for current (by each) thread
	//unsigned int thread_id = thdata->id;
	const unsigned long long main_begin = thdata->start;
	register const unsigned long long main_end = thdata->finish;
	const unsigned int first_sentence = thdata->first_sentence;

	// search types for every token
	register unsigned long long search_types;

	// search types for every token
	register unsigned int search_types_source;

	// distances
	unsigned int dist1_start;
	unsigned int dist1_end;

	unsigned int dist2_start;
	unsigned int dist2_end;

	unsigned int dist3_start;
	unsigned int dist3_end;

	unsigned int dist4_start;
	unsigned int dist4_end;

	unsigned int dist5_start;
	unsigned int dist5_end;

	// positions
	register unsigned long long z1;
	unsigned long long z2;
	unsigned long long z3;
	unsigned long long z4;
	unsigned long long z5;
	unsigned long long z6;

	unsigned long long x2;
	unsigned long long x3;
	unsigned long long x4;
	unsigned long long x5;
	unsigned long long x6;

	register int valid_source;
	register int positive;
	unsigned int progress;
	unsigned int found_all;
	unsigned long long sent_begin = 0;
	register unsigned int curnt_sent;
	register unsigned long long last_pos;

	while(1) {
		// set threads to waiting state
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);

		// search types for every token
		search_types = morph_types;

		// search types for every source
		search_types_source = morph_types_source;

		// distances
		dist1_start = dist_from[0];
		dist1_end = dist_to[0];

		dist2_start = dist_from[1];
		dist2_end = dist_to[1];

		dist3_start = dist_from[2];
		dist3_end = dist_to[2];

		dist4_start = dist_from[3];
		dist4_end = dist_to[3];

		dist5_start = dist_from[4];
		dist5_end = dist_to[4];

		progress = 0;
		found_all = 0;
		sent_begin = 0;
		valid_source = 0;
		z1 = main_begin;
		last_pos = thdata->last_pos;
		curnt_sent = first_sentence;

		if(DEBUG)
			printf("\nz1=%lld, positive=%d, last_pos=%lld, curnt_sent=%d, sent_begin=%lld, found_limit=%d", z1, positive, last_pos, curnt_sent, sent_begin, found_limit);

		// param1
		while(z1 < main_end) {
			positive = array_united[z1];
			// Check if a new text and/or sentence beginning
			if(positive < 0) {
				// Calculate sentence id
				if(z1) 
					++curnt_sent;
				// source_mask to check if this text(s) is selected (subcorpora)
				if(positive <= SOURCE_OFFSET) {
					if(source_mask[SOURCE_OFFSET - positive] == search_types_source)
						valid_source = 1;
					else
						valid_source = 0;
					positive = -array_united[++z1];
				} else {
					positive = -positive;
				}
				sent_begin = z1;
			}
			if(valid_source) {
				if((united_mask[positive] & 0xFF) == (search_types & 0xFF)) {
					// param2
					if(params > 1) {
						z2 = z1 + dist1_start;
						x2 = z1 + dist1_end;
						while(z2 < main_end && z2 <= x2 && array_united[z2] >= 0)  {
							if((united_mask[array_united[z2]] & 0xFF00) == (search_types & 0xFF00)) {
								// param3
								if(params > 2) {
									z3 = z2 + dist2_start;
									x3 = z2 + dist2_end;
									while(z3 < main_end && z3 <= x3 && array_united[z3] >= 0) {
										if((united_mask[array_united[z3]] & 0xFF0000) == (search_types & 0xFF0000)) {
											// param4
											if(params > 3) {
												z4 = z3 + dist3_start;
												x4 = z3 + dist3_end;
												while(z4 < main_end && z4 <= x4 && array_united[z4] >= 0) {
													if((united_mask[array_united[z4]] & 0xFF000000) == (search_types & 0xFF000000)) {
														// param5
														if(params > 4) {
															z5 = z4 + dist4_start;
															x5 = z4 + dist4_end;
															while(z5 < main_end && z5 <= x5 && array_united[z5] >= 0) {
																if((united_mask[array_united[z5]] & 0xFF00000000) == (search_types & 0xFF00000000)) {
																	// param6
																	if(params > 5) {
																		z6 = z5 + dist5_start;
																		x6 = z5 + dist5_end;
																		while(z6 < main_end && z6 <= x6 && array_united[z6] >= 0) {
																			if((united_mask[array_united[z6]] & 0xFF0000000000) == (search_types & 0xFF0000000000)) {
																				if((z1 > last_pos || !last_pos) && found_limit) {
																					sem_wait(&count_sem);
																					if(found_limit) {
																						--found_limit;
																						sem_post(&count_sem);
																						func_build_sents(main_end, curnt_sent, sent_begin, z1, z2, z3, z4, z5, z6);
																						last_pos = z1;
																					} else {
																						sem_post(&count_sem);
																					}
																				}
																				++found_all;
																				if(z1 == last_pos) {
																					progress = found_all;
																				}
																			}
																			++z6;
																		}
																	} else {
																		if((z1 > last_pos || !last_pos) && found_limit) {
																			sem_wait(&count_sem);
																			if(found_limit) {
																				--found_limit;
																				sem_post(&count_sem);
																				func_build_sents(main_end, curnt_sent, sent_begin, z1, z2, z3, z4, z5, 0);
																				last_pos = z1;
																			} else {
																				sem_post(&count_sem);
																			}
																		}
																		++found_all;
																		if(z1 == last_pos) {
																			progress = found_all;
																		}
																	}
																}
																++z5;
															}
														} else {
															if((z1 > last_pos || !last_pos) && found_limit) {
																sem_wait(&count_sem);
																if(found_limit) {
																	--found_limit;
																	sem_post(&count_sem);
																	func_build_sents(main_end, curnt_sent, sent_begin, z1, z2, z3, z4, 0, 0);
																	last_pos = z1;
																} else {
																	sem_post(&count_sem);
																}
															}
															++found_all;
															if(z1 == last_pos) {
																progress = found_all;
															}
														}
													}
													++z4;
												}
											} else {
												if((z1 > last_pos || !last_pos) && found_limit) {
													sem_wait(&count_sem);
													if(found_limit) {
														--found_limit;
														sem_post(&count_sem);
														func_build_sents(main_end, curnt_sent, sent_begin, z1, z2, z3, 0, 0, 0);
														last_pos = z1;
													} else {
														sem_post(&count_sem);
													}
												}
												++found_all;
												if(z1 == last_pos) {
													progress = found_all;
												}
											}
										}
										++z3;
									}
								} else {
									if((z1 > last_pos || !last_pos) && found_limit) {
										sem_wait(&count_sem);
										if(found_limit) {
											--found_limit;
											sem_post(&count_sem);
											func_build_sents(main_end, curnt_sent, sent_begin, z1, z2, 0, 0, 0, 0);
											last_pos = z1;
										} else {
											sem_post(&count_sem);
										}
									}
									++found_all;
									if(z1 == last_pos) {
										progress = found_all;
									}
								}
							}
							++z2;
						}
					} else {
						if((z1 > last_pos || !last_pos) && found_limit) {
							sem_wait(&count_sem);
							if(found_limit) {
								--found_limit;
								sem_post(&count_sem);
								func_build_sents(main_end, curnt_sent, sent_begin, z1, 0, 0, 0, 0, 0);
								last_pos = z1;
							} else {
								sem_post(&count_sem);
							}
						}
						++found_all;
						if(z1 == last_pos) {
							progress = found_all;
						}
					}
				}
			}
			++z1;
		}
		thdata->last_pos = last_pos;
		thdata->found_num = found_all;
		thdata->progress = progress;

		// Decrementing counter and sending signal to func_run_socket()
		pthread_mutex_lock(&mutex2);
		--finished;
		//pthread_cond_broadcast(&cond2);
		pthread_cond_signal(&cond2);
		pthread_mutex_unlock(&mutex2);

		//pthread_exit(NULL);
	} // while(1)
}


/*
 * Expand context
 */
int func_extend_context(unsigned int extend_sent)
{
	register unsigned long long z1 = 0;
	register unsigned int curnt_sent = 0;
	register unsigned int extend_min;
	register unsigned int extend_max;
	register const unsigned long long main_end = SIZE_ARRAY_MAIN;

	// Make sure we are not out of range 
	if(extend_sent < extend_range)
		extend_min = 0;
	else
		extend_min = extend_sent - extend_range;
	
	if(extend_sent + extend_range > AMOUNT_SENTENCES)
		extend_max = AMOUNT_SENTENCES;
	else
		extend_max = extend_sent + extend_range;

	while(z1 < main_end) {
		if(array_united[z1] < 0) {
			if(array_united[z1] <= SOURCE_OFFSET) {
				++z1;
			}
			if(curnt_sent >= extend_min) {
				if(curnt_sent <= extend_max)
					func_build_sents(SIZE_ARRAY_MAIN, curnt_sent, z1, -1, -1, -1, -1, -1, -1);
				else
					break;
			}
			++curnt_sent;
		}
		++z1;
	}
	return 0;
}


/*
 * Sort tags in the string
 */
int func_sort_tags(char *tags)
{
	printf("\nTags:  %s", tags);
	int x = 0;
	char *delimiter = ","; // Delimiter for tags in the string
	char *token;
	char sort[TAGS_UNIQ_ARRAY_SIZE][TAGS_UNIQ_BUFFER_SIZE];

	token = strtok(tags, delimiter);
	while(token != NULL && x < TAGS_UNIQ_ARRAY_SIZE) {
		strncpy(sort[x], token, TAGS_UNIQ_BUFFER_SIZE - 1);
		sort[x][TAGS_UNIQ_BUFFER_SIZE - 1] = '\0';
		token = strtok(NULL, delimiter);
		x++;
	}
	qsort(sort, x, sizeof(char) * TAGS_UNIQ_BUFFER_SIZE, func_cmp_strings);
	tags[0] = '*';
	tags[1] = '\0';
	for(int y = 0; y < x; y++) {
		strncat(tags, sort[y], SOCKET_BUFFER_SIZE - strlen(tags) - 1);
		strncat(tags, "*", SOCKET_BUFFER_SIZE - strlen(tags) - 1);
	}
	printf("  =>  %s", tags);
	return 0;
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

	//if(regcomp(&start_state, pattern, REG_EXTENDED|REG_ICASE)) { // TODO: Case insensitive
	if(regcomp(&start_state, pattern, REG_EXTENDED)) {
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
int func_regex_normalization(const char match[WORDS_BUFFER_SIZE])
{
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
	return 0;
}


/*
 * RegEx sources
 */
int func_regex_sources(const char pattern[SOURCE_BUFFER_SIZE])
{
	regex_t start_state;
	const int type = 0; // Offset for full source. TODO: 1 - Author(s), 2 - Title, 3 - ...

	if(regcomp(&start_state, pattern, REG_EXTENDED|REG_ICASE)) { // Case insensitive
		fprintf(stderr, "RegEx: Bad pattern: '%s'\n", pattern); // TODO: return this to the user
		return 1;
	}
	for(unsigned long long i = 0; i < SOURCES_ARRAY_SIZE; i++) {
		if(!regexec(&start_state, ptr_sources[i], 0, NULL, 0)) {
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
//int func_szWildMatchSource(const char match[SOURCE_BUFFER_SIZE], const int mask_offset, char *united_x[UNITED_ARRAY_SIZE], const int type, struct thread_data_united *thdata_united)
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
		//united_mask[i] += (unsigned long long)1 << (SEARCH_TYPES_OFFSET * mask_offset + type);
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
 * Run search united
 */
void * func_run_united(struct thread_data_united *thdata_united)
{
	while(1) {
		// set threads to waiting state
		pthread_mutex_lock(&mutex_united);
		pthread_cond_wait(&cond_united, &mutex_united);
		pthread_mutex_unlock(&mutex_united);

		// Find ids and set masks for all search words, lemmas, tags and patterns
		for(unsigned int x = 0; x < params; x++) {
			if(tags[x][0]) {
				//func_sort_tags(tags[x]);
				func_szWildMatch(tags[x], x, united_tags, TAGS, thdata_united);
			}
			if(wildmatch[x][0]) {
				if(case_sensitive[x]) {
					if(regex)
						func_regex(wildmatch[x], x, united_words_case, WILD, thdata_united);
					else
						func_szWildMatch(wildmatch[x], x, united_words_case, WILD, thdata_united);
				} else {
					if(regex)
						func_regex(wildmatch[x], x, united_words, WILD, thdata_united);
					else
						func_szWildMatch(wildmatch[x], x, united_words, WILD, thdata_united);
				}
			}
			if(wildmatch_lemma[x][0]) {
				if(regex)
					func_regex(wildmatch_lemma[x], x, united_lemmas, WILD_LEMMA, thdata_united);
				else
					func_szWildMatch(wildmatch_lemma[x], x, united_lemmas, WILD_LEMMA, thdata_united);
			}
		}

		// Decrementing counter and sending signal to func_run_socket()
		pthread_mutex_lock(&mutex2_united);
		--finished_united;
		//pthread_cond_broadcast(&cond2_united);
		pthread_cond_signal(&cond2_united);
		pthread_mutex_unlock(&mutex2_united);
	} // while(1)
}


/*
 * Parsing last found positions for every thread
 */
int func_parse_last_pos()
{
	if(DEBUG)
		printf("\nLast positions for threads:");
	int x = 0;
	int y = 0;
	int z = 0;
	char *endptr;
	char temp[WORDS_BUFFER_SIZE];
	for(y = 0; y < SEARCH_THREADS; y++)
		thread_data_array[y].last_pos = 0;
	y = 0;
	do {
		if(morph_last_pos[y] != 'x' && morph_last_pos[y] != '\0') {
			temp[x++] = morph_last_pos[y];
		} else {
			temp[x] = '\0';
			errno = 0;
			thread_data_array[z].last_pos = strtoull(temp, &endptr, 10);
			if(errno == ERANGE) {
				perror("\nstrtol");
			}
			if(endptr == temp) {
				fprintf(stderr, "\nNo digits were found!");
			}
			if(DEBUG)
				printf("\n  DEBUG> %d) %s => %llu", z, temp, thread_data_array[z].last_pos);
			x = 0;
			z++;
		}
	} while(y < WORDS_BUFFER_SIZE && z < SEARCH_THREADS && morph_last_pos[y++]);
	if(DEBUG)
		for(x = 0; x < SEARCH_THREADS; x++)
			printf("\n  %d: %llu", x, thread_data_array[x].last_pos);
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
	while(params < AMOUNT_TOKENS && (word[params][0] || lemma[params][0] || tags[params][0] || wildmatch[params][0] || wildmatch_lemma[params][0])) {
		++params;
	}

	if(DEBUG)
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
	if(source[0] == '\0')
		morph_types_source = 0; // Search in all sources
	else
		morph_types_source += ((unsigned int)1 << (SOURCE)); // Search in particular sources

	if(DEBUG)
		printf("\nmorph_types_source: %d\n", morph_types_source);
	return 0;
}


/*
 * Validate distances: range, order
 */
int func_validate_distances()
{
	if(DEBUG)
		printf("\n\nDEBUG> Validated distances:");
	int temp;
	for(unsigned int x = 0; x < params - 1; x++) {
		if(dist_from[x] < 1)
			dist_from[x] = 1;
		if(dist_from[x] > 1000)
			dist_from[x] = 1000;

		if(dist_to[x] < 1)
			dist_to[x] = 1;
		if(dist_to[x] > 1000)
			dist_to[x] = 1000;

		if(dist_from[x] > dist_to[x]) {
			temp = dist_from[x];
			dist_from[x] = dist_to[x];
			dist_to[x] = temp;
		}

		if(DEBUG)
			printf("\n  #%d: from %d to %d", x + 1, dist_from[x], dist_to[x]);
	}
	return 0;
}


/*
 * Create UNIX domain socket
 * 	netstat -ap unix |grep fastmorph.socket
 * 	ncat -U /tmp/fastmorph.socket
 */
void * func_run_socket(/*int argc, char *argv[]*/)
{
	printf("\n  Begin of socket listening:\n");
	unsigned int t;
	int fd; // int fd, cl, rc;
	struct sockaddr_un addr;
	char bufin[SOCKET_BUFFER_SIZE];
	char bufout[SOCKET_BUFFER_SIZE];
	char temp[SOCKET_BUFFER_SIZE];
	int rc_thread[SEARCH_THREADS];
	pthread_t threads[SEARCH_THREADS]; // thread identifier
	int rc_thread_united[SEARCH_THREADS];
	pthread_t threads_united[SEARCH_THREADS]; // thread identifier
	unsigned int progress; // The position of last returned sentence in 'found_all'

	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("Socket error");
		exit(-1);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
	unlink(socket_path);

	if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Bind error");
		exit(-1);
	}
	// Giving permissions to all processes to access the socket file
	chmod(socket_path, 0666);

	if(listen(fd, 5) == -1) {
		perror("Listen error");
		exit(-1);
	}
	// Creating threads for the big cycle
	printf("\n\nCreating threads...");
	for(t = 0; t < SEARCH_THREADS; t++) {
		printf("\n  Thread: creating thread %d", t);
		thread_data_array[t].id = t;

		// Create worker thread
		rc_thread[t] = pthread_create(&threads[t], NULL, (void *) &func_run_cycle, (void *) &thread_data_array[t]);
		if(rc_thread[t]) {
			printf("\n  ERROR: return code from pthread_create() is %d", rc_thread[t]);
			exit(-1);
		}
	}
	// Creating threads for the united cycle
	printf("\n\nCreating threads united...");
	for(t = 0; t < SEARCH_THREADS; t++) {
		printf("\n  Thread: creating thread united %d", t);
		thread_data_array_united[t].id = t;

		// Create worker thread
		rc_thread_united[t] = pthread_create(&threads_united[t], NULL, (void *) &func_run_united, (void *) &thread_data_array_united[t]);
		if(rc_thread_united[t]) {
			printf("\n  ERROR: return code from pthread_create() united is %d", rc_thread_united[t]);
			exit(-1);
		}
	}
	while(1) {
		if((cl = accept(fd, NULL, NULL)) == -1) {
			perror("Accept error");
			continue;
		}
		while((rc = read(cl, bufin, sizeof(bufin))) > 0) {
			// Initial search time value
			time_start(&tv1);

			printf("\n-----------------------------------------------------------------");
			printf("\nRead %u bytes: %.*s\n", rc, rc, bufin);

			// Parsing incoming JSON string and setting global variables
			func_jsmn_json(bufin, rc);

			if(extend >= 0) {
				// Sending JSON string over socket file
				strncpy(bufout, "{\"example\":[", SOCKET_BUFFER_SIZE - 1);
				bufout[SOCKET_BUFFER_SIZE - 1] = '\0';
				if(write(cl, bufout, strlen(bufout)) != (unsigned int) strlen(bufout)) {
					if(rc > 0) {
						fprintf(stderr,"Partial write");
					} else {
						perror("Write error");
						exit(-1);
					}
				}
				bufout[0] = '\0';

				func_extend_context(extend);

				// closing marker
				strncpy(bufout, "{\"id\":-1}", SOCKET_BUFFER_SIZE - 1);
				bufout[SOCKET_BUFFER_SIZE - 1] = '\0';

				strncat(bufout, "]", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

				// Clear for the next request
				extend = -1;
			} else {
				// Preparing arrays
				memset(united_mask, 0, sizeof(united_mask));
				memset(source_mask, 0, sizeof(source_mask));

				// Setting global variables
				func_fill_search_mask();
				func_parse_last_pos();
				func_validate_distances();

				// Set the initial value of counters
				found_limit = return_sentences;

				// Find ids and set masks for all search words, lemmas, tags and patterns
				time_start(&tv2);
				for(t = 0; t < params; t++) {
					if(word[t][0]) {
						if(case_sensitive[t])
							func_szExactMatch(ptr_words_case, word[t], WORDS_CASE_ARRAY_SIZE, t, united_words_case, WORD);
						else
							func_szExactMatch(ptr_words, word[t], WORDS_ARRAY_SIZE, t, united_words, WORD);
					}
					if(lemma[t][0])
						func_szExactMatch(ptr_lemmas, lemma[t], LEMMAS_ARRAY_SIZE, t, united_lemmas, LEMMA);
					if(tags[t][0])
						func_sort_tags(tags[t]);
				}
				printf("\n\nszExactMatch() time: %ld milliseconds.", time_stop(&tv2));

				// Find source(s)
				time_start(&tv2);
				if(regex) {
					func_regex_sources(source);
					printf("\n\nsz_regex_sources() time: %ld milliseconds.", time_stop(&tv2));
				} else {
					func_szWildMatchSource(source);
					printf("\n\nszWildMatchSource() time: %ld milliseconds.", time_stop(&tv2));
				}

				time_start(&tv3);
				// broadcast to workers to work
				pthread_mutex_lock(&mutex_united);
				finished_united = SEARCH_THREADS;
				pthread_cond_broadcast(&cond_united);
				pthread_mutex_unlock(&mutex_united);
				// wait for workers to finish
				pthread_mutex_lock(&mutex2_united);
				while(finished_united)
					pthread_cond_wait(&cond2_united, &mutex2_united);
				pthread_mutex_unlock(&mutex2_united);
				printf("\nThreads func_run_united time: %ld milliseconds.", time_stop(&tv3));

				// Sending JSON string over socket file
				strncpy(bufout, "{\"example\":[", SOCKET_BUFFER_SIZE - 1);
				bufout[SOCKET_BUFFER_SIZE - 1] = '\0';
				if(write(cl, bufout, strlen(bufout)) != (unsigned int) strlen(bufout)) {
					if(rc > 0) {
						fprintf(stderr,"Partial write");
					} else {
						perror("Write error");
						exit(-1);
					}
				}

				time_start(&tv4);
				// broadcast to workers to work
				pthread_mutex_lock(&mutex);
				finished = SEARCH_THREADS;
				pthread_cond_broadcast(&cond);
				pthread_mutex_unlock(&mutex);
				// wait for workers to finish
				pthread_mutex_lock(&mutex2);
				while(finished)
					pthread_cond_wait(&cond2, &mutex2);
				pthread_mutex_unlock(&mutex2);
				printf("\nThreads func_run_cycle time: %ld milliseconds.", time_stop(&tv4));

				// Summ amount of found occurences and left ones from all threads
				progress = 0;
				size_array_found_sents_all_summ = 0;
				for(t = 0; t < SEARCH_THREADS; t++) {
					progress += thread_data_array[t].progress;
					size_array_found_sents_all_summ += thread_data_array[t].found_num;
				}

				// closing marker
				strncpy(bufout, "{\"id\":-1}", SOCKET_BUFFER_SIZE - 1);
				bufout[SOCKET_BUFFER_SIZE - 1] = '\0';

				// last_pos
				strncat(bufout, "],\"last_pos\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
				for(t = 0; t < SEARCH_THREADS; t++) {
					if(t != SEARCH_THREADS - 1)
						snprintf(temp, 100, "%llux", thread_data_array[t].last_pos);
					else
						snprintf(temp, 100, "%llu", thread_data_array[t].last_pos);
					strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
				}
				strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

				// found_all
				strncat(bufout, ",\"found_all\":", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
				snprintf(temp, 100, "%d", size_array_found_sents_all_summ);
				strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

				// progress
				strncat(bufout, ",\"progress\":", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
				snprintf(temp, 100, "%d", progress);
				strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			}

			// close JSON string
			strncat(bufout, "}\r\n", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// Sending JSON string over socket file
			if(write(cl, bufout, strlen(bufout)) != (unsigned int) strlen(bufout)) {
				if(rc > 0) {
					fprintf(stderr,"Partial write");
				} else {
					perror("Write error");
					exit(-1);
				}
			}
			printf("\nQuery processing time: %ld milliseconds.\n", time_stop(&tv1));
		}
		if(rc == -1) {
			perror("Read error");
			exit(-1);
		} else if(rc == 0) {
			printf("EOF\n");
			close(cl);
		}
	}
	// wait for our thread to finish before continuing
	for(t = 0; t < SEARCH_THREADS; t++){
		rc_thread[t] = pthread_join(threads[t], NULL);
		if(rc_thread[t]) {
			printf("\n  ERROR; return code from pthread_join() is %d", rc_thread[t]);
			exit(-1);
		}
		printf("\n  Thread: completed join with thread #%d having a status of '%s'", t, strerror(rc_thread[t]));
	}
	// wait for our thread united to finish before continuing
	for(t = 0; t < SEARCH_THREADS; t++){
		rc_thread_united[t] = pthread_join(threads_united[t], NULL);
		if(rc_thread_united[t]) {
			printf("\n  ERROR; return code from pthread_join() united is %d", rc_thread_united[t]);
			exit(-1);
		}
		printf("\n  Thread: completed join with thread united #%d having a status of '%s'", t, strerror(rc_thread_united[t]));
	}
	close(fd);
	unlink(socket_path);
	pthread_exit(NULL);
}


/*
 * Show "Enter 'exit' to quit" message
 */
int prompt()
{
	printf("\n\nEnter 'version' or 'exit': ");
	char *buffer = NULL;
	do {
		if(scanf("%ms", &buffer)) {
			if(strstr(buffer, "version") != NULL) {
				printf("%s\n", VERSION);
			}
		}
	} while(strstr(buffer, "exit") == NULL && strstr(buffer, "quit") == NULL);
	return 0;
}


/*
 * Main
 */
int main(/* int argc, char *argv[] */)
{
	// set locale for regex functions
	setlocale(LC_ALL, "ru_RU.UTF-8");

	// initial values
	extend = -1;

	// main big array
	array_united = malloc(sizeof(*array_united) * SIZE_ARRAY_MAIN);

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

	// sources full
	size_list_sources = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources = malloc(sizeof(*list_sources) * size_list_sources);
	memset(list_sources, 1, sizeof(*list_sources) * size_list_sources);

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

	// genres
	size_list_sources_genre = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	list_sources_genre = malloc(sizeof(*list_sources_genre) * size_list_sources_genre);
	memset(list_sources_genre, 1, sizeof(*list_sources_genre) * size_list_sources_genre);

	func_read_mysql();
	func_find_distances_for_threads();
	func_find_distances_for_threads_united();

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

	// resizing list_sources
	//printf("\n\nList_sources:");
	//printf("\n  Old size: %d bytes", size_list_sources);
	//do { --size_list_sources; } while(list_sources[size_list_sources]);
	//++size_list_sources;
	//printf("\n  New size: %d bytes", size_list_sources);
	//list_sources = realloc(list_sources, size_list_sources);

	// resizing authors
	printf("\n\nList_sources:");
	printf("\n  Old size: %d bytes", size_list_sources_author);
	do { --size_list_sources_author; } while(list_sources_author[size_list_sources_author]);
	++size_list_sources_author;
	printf("\n  New size: %d bytes", size_list_sources_author);
	list_sources_author = realloc(list_sources_author, size_list_sources_author);

	// resizing titles
	printf("\n\nList_sources:");
	printf("\n  Old size: %d bytes", size_list_sources_title);
	do { --size_list_sources_title; } while(list_sources_title[size_list_sources_title]);
	++size_list_sources_title;
	printf("\n  New size: %d bytes", size_list_sources_title);
	list_sources_title = realloc(list_sources_title, size_list_sources_title);

	// resizing dates
	printf("\n\nList_sources:");
	printf("\n  Old size: %d bytes", size_list_sources_date);
	do { --size_list_sources_date; } while(list_sources_date[size_list_sources_date]);
	++size_list_sources_date;
	printf("\n  New size: %d bytes", size_list_sources_date);
	list_sources_date = realloc(list_sources_date, size_list_sources_date);

	// resizing genres
	printf("\n\nList_sources:");
	printf("\n  Old size: %d bytes", size_list_sources_genre);
	do { --size_list_sources_genre; } while(list_sources_genre[size_list_sources_genre]);
	++size_list_sources_genre;
	printf("\n  New size: %d bytes", size_list_sources_genre);
	list_sources_genre = realloc(list_sources_genre, size_list_sources_genre);

	sem_init(&count_sem, 0, 1);

	// Sentence-source compliance test for Written Corpus... Correct: 260 == 0, 261 == 1 
	//for(int m = 259; m <= 262; m++)
	//	printf(">>>> %d <> %d", m, sentence_source[m]);

	// run func_run_socket() in a thread
	printf("\n\nCreating socket listening thread...");
	pthread_t thread; // thread identifier
	pthread_attr_t attr; // thread attributes
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // run in detached mode
	int rc_thread = pthread_create(&thread, &attr, (void *) &func_run_socket, NULL);
	if(rc_thread){
		printf("\n  ERROR: return code from pthread_create() is %d", rc_thread);
		exit(-1);
	}

	// Show 'exit' message in a couple of seconds
	sleep(2);
	prompt();

	// free
	free(array_united);
	free(list_words_case);
	free(list_words);
	free(list_lemmas);
	free(list_tags);
	return 0;
}

