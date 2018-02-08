/*
 * fastngrams.c - Fast search engine for n-grams.
 * Version v5.5.0 - 2018.02.08
 *
 * "fastngrams" is a high speed search engine for n-grams:
 *   - loads all preprocessed data from MySQL (MariaDB) into RAM;
 *   - creates UNIX domain socket file as a server and accepts queries in JSON format;
 *   - has multithreading support.
 *
 * Copyright (C) 2017-present Mansur Saykhunov <tatcorpus@gmail.com>
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

#include "fastngrams.h"
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

// Counter from return_ngrams to 0
unsigned int found_limit;

// main arrays
int *array_united;
/*
int *array_ngrams1;
int *array_ngrams2;
int *array_ngrams3;
int *array_ngrams4;
int *array_ngrams5;
int *array_ngrams6;
*/
int array_ngrams1[NGRAMS_COLUMNS-1][NGRAMS1_ARRAY_SIZE];
int array_ngrams2[NGRAMS_COLUMNS][NGRAMS2_ARRAY_SIZE];
int array_ngrams3[NGRAMS_COLUMNS][NGRAMS3_ARRAY_SIZE];
int array_ngrams4[NGRAMS_COLUMNS][NGRAMS4_ARRAY_SIZE];
int array_ngrams5[NGRAMS_COLUMNS][NGRAMS5_ARRAY_SIZE];
int array_ngrams6[NGRAMS_COLUMNS][NGRAMS6_ARRAY_SIZE];

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

//char *list_sources;							/*   Long string for sources   */
//int size_list_sources;

char *list_sources_author;						/*   Long string for sources authors   */
int size_list_sources_author;

char *list_sources_title;						/*   Long string for sources titles   */
int size_list_sources_title;

char *list_sources_date;						/*   Long string for sources dates   */
int size_list_sources_date;

char *list_sources_genre;						/*   Long string for sources genres   */
int size_list_sources_genre;

//char source_types[SOURCES_ARRAY_SIZE][SOURCE_TYPES_BUFFER_SIZE];	/*   source types: book, www...com   */

int source_mask[SOURCES_ARRAY_SIZE][3];					/*   TODO: For subcorpora. Texts: bitmask, begin, end   */

//int sentence_source[AMOUNT_SENTENCES];				/*   Sentence-source associations   */

char *ptr_words_case[WORDS_CASE_ARRAY_SIZE];				/*   Array of pointers to list_words_case elements   */
char *ptr_words[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_lemmas[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_tags[TAGS_ARRAY_SIZE];					/*   Array of pointers...   */
//char *ptr_sources[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_author[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_title[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_date[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_genre[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */

char wildmatch[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: *еш* -> еш (кеше, ешрак, теш)   */
char wildmatch_lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: (*еш*) -> еш (кеше, ешрак, теш)   */
char word[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кешегә, ешрак, тешнең   */
char lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кеше, еш, теш   */
char tags[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: *<n>*<nom>*<pl>*   */

char *united_words_case[UNITED_ARRAY_SIZE];				/*   Array of not uniq pointers to list_words_case elements   */
char *united_words[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */
char *united_lemmas[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */
char *united_tags[UNITED_ARRAY_SIZE];					/*   Array of not uniq pointers   */

unsigned long long united_mask[UNITED_ARRAY_SIZE];			/*   Array of bits fields   */

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
unsigned int case_sensitive[AMOUNT_TOKENS];				/*   Array for case sensitive (1) or not (0)   */
char morph_last_pos[WORDS_BUFFER_SIZE];					/*   The id of last found token to continue from   */
unsigned int return_ngrams;						/*   Number of sentences to return   */

//unsigned int search_mode;						/*   n   */

unsigned int params;							/*   Number of tokens (1-6) to search   */

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
		// Words
		if(jsoneq(strin, &t[i], "word") == 0) {
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
		// Return (Number of ngrams to return)
		} else if(jsoneq(strin, &t[i], "return") == 0) {
			printf("- return: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(buf, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			buf[t[i+1].end - t[i+1].start] = '\0';
			errno = 0;
			return_ngrams = strtol(buf, &ptr, 10);
			// Check for ranges
			if(return_ngrams < 1 || return_ngrams > 1000) {
				return_ngrams = FOUND_NGRAMS_LIMIT_DEFAULT;
			}
			// Check for various possible errors
			if(errno == ERANGE)
				perror("\nstrtol");
			if(ptr == buf)
				fprintf(stderr, "\nNo digits were found");
			printf("%d\n", return_ngrams);
			++i;
		// Last position
		} else if(jsoneq(strin, &t[i], "last_pos") == 0) {
			printf("- last_pos: %.*s / ", t[i+1].end - t[i+1].start, strin + t[i+1].start);
			strncpy(morph_last_pos, strin + t[i+1].start, t[i+1].end - t[i+1].start);
			morph_last_pos[t[i+1].end - t[i+1].start] = '\0';
			printf("%s\n", morph_last_pos);
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
	//char *ptr_author;
	//char *ptr_title;
	//char *ptr_date;
	//char *ptr_genre;
	char *endptr;
	char mycommand[200];
	int i;
	//int iauthor;
	//int ititle;
	//int idate;
	//int igenre;
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

		// load ngrams
		printf("\n");
		i = 0;
		int level = 0;
		int node = 0;
		int parent = 0;
		int united = 0;
		int freq = 0;
		int n_mysql_load_limit = 0;
		//n_mysql_load_limit = 0;

		for(int t = 0; t < NGRAMS_ARRAY_SIZE; t = t + MYSQL_LOAD_LIMIT) {
			if(NGRAMS_ARRAY_SIZE - t < MYSQL_LOAD_LIMIT) {
				n_mysql_load_limit = NGRAMS_ARRAY_SIZE - t;
			} else {
				n_mysql_load_limit = MYSQL_LOAD_LIMIT;
			}
			sprintf(mycommand, "SELECT level, node, parentnode, united, freq FROM morph6_ngrams WHERE id >= %d LIMIT %d", t, n_mysql_load_limit);
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
						level = (int) strtol(row[0], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						errno = 0;
						node = (int) strtol(row[1], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						errno = 0;
						parent = (int) strtol(row[2], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						errno = 0;
						united = (int) strtol(row[3], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						errno = 0;
						freq = (int) strtol(row[4], &endptr, 10);
						if(errno == ERANGE)
							perror("\nstrtol");
						if(row[0] == endptr)
							fprintf(stderr, "\nNo digits were found");

						if(level == 1) {
							array_ngrams1[0][node] = united;
							array_ngrams1[1][node] = freq;
						} else if(level == 2) {
							array_ngrams2[0][node] = united;
							array_ngrams2[1][node] = freq;
							array_ngrams2[2][node] = parent;
						} else if(level == 3) {
							array_ngrams3[0][node] = united;
							array_ngrams3[1][node] = freq;
							array_ngrams3[2][node] = parent;
						} else if(level == 4) {
							array_ngrams4[0][node] = united;
							array_ngrams4[1][node] = freq;
							array_ngrams4[2][node] = parent;
						} else if(level == 5) {
							array_ngrams5[0][node] = united;
							array_ngrams5[1][node] = freq;
							array_ngrams5[2][node] = parent;
						} else if(level == 6) {
							array_ngrams6[0][node] = united;
							array_ngrams6[1][node] = freq;
							array_ngrams6[2][node] = parent;
						}
						++i;
					}
					printf("\33[2K\r    Ngrams: %d (of %d) rows imported.", i, NGRAMS_ARRAY_SIZE);
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
//int func_build_sents(unsigned int end, unsigned int curnt_sent, unsigned long long sent_begin, unsigned long long z1, unsigned long long z2, unsigned long long z3, unsigned long long z4, unsigned long long z5, unsigned long long z6)
int func_build_ngrams(unsigned int z)
{
	unsigned int t;
	int freq;
	char bufout[SOCKET_BUFFER_SIZE];
	char temp[SOCKET_BUFFER_SIZE];

	// id
	strncpy(bufout, "{\"id\":", SOCKET_BUFFER_SIZE - 1);
	bufout[SOCKET_BUFFER_SIZE - 1] = '\0';
	snprintf(temp, 100, "%d", z);
	strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	// freq
	switch(params) {
		case 1:
			freq = array_ngrams1[1][z];
			break;
		case 2:
			freq = array_ngrams2[1][z];
			break;
		case 3:
			freq = array_ngrams3[1][z];
			break;
		case 4:
			freq = array_ngrams4[1][z];
			break;
		case 5:
			freq = array_ngrams5[1][z];
			break;
		case 6:
			freq = array_ngrams6[1][z];
			break;
	}
	strncat(bufout, ",\"freq\":", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	snprintf(temp, 100, "%d", freq);
	strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	// ngrams
	strncat(bufout, ",\"ngram\":[", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	t = z;


	// TODO: Add tips tags and lemmas


	switch(params) {
		case 6:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams6[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams6[0][t])], united_tags[abs(array_ngrams6[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams6[0][t])], united_tags[abs(array_ngrams6[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams6[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams6[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			t = array_ngrams6[2][t];
			__attribute__((fallthrough)); // To avoid compiler warning 'this statement may fall through...'
		case 5:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams5[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams5[0][t])], united_tags[abs(array_ngrams5[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams5[0][t])], united_tags[abs(array_ngrams5[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams5[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams5[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			t = array_ngrams5[2][t];
			__attribute__((fallthrough));
		case 4:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams4[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams4[0][t])], united_tags[abs(array_ngrams4[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams4[0][t])], united_tags[abs(array_ngrams4[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams4[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams4[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			t = array_ngrams4[2][t];
			__attribute__((fallthrough));
		case 3:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams3[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams3[0][t])], united_tags[abs(array_ngrams3[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams3[0][t])], united_tags[abs(array_ngrams3[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams3[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams3[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			t = array_ngrams3[2][t];
			__attribute__((fallthrough));
		case 2:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams2[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams2[0][t])], united_tags[abs(array_ngrams2[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams2[0][t])], united_tags[abs(array_ngrams2[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams2[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams2[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			t = array_ngrams2[2][t];
			__attribute__((fallthrough));
		case 1:
			strncat(bufout, "\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag open: lemma + tags (regular)
			if(united_lemmas[abs(array_ngrams1[0][t])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "\\", united_lemmas[abs(array_ngrams1[0][t])], united_tags[abs(array_ngrams1[0][t])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, REGULAR_HTML_OPEN, "", united_lemmas[abs(array_ngrams1[0][t])], united_tags[abs(array_ngrams1[0][t])]);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// escape if word is '"'
			if(united_words_case[abs(array_ngrams1[0][t])][0] == '"')
				strncat(bufout, "\\", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// take word itself
			strncat(bufout, united_words_case[abs(array_ngrams1[0][t])], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			// html tag close
			strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

			strncat(bufout, "\"]", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	}

	// Closing current section
	strncat(bufout, "},", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
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
	//const unsigned long long main_begin = thdata->start;
	//register const unsigned long long main_end = thdata->finish;
	register unsigned long long main_end = thdata->finish;
	//const unsigned int first_sentence = thdata->first_sentence;

	//main_begin = 0;
	main_end = 0;

	// search types for every token
	register unsigned long long search_types;

	// positions
	register unsigned int t;
	register unsigned long long z;
	unsigned int found_all;

	while(1) {
		// set threads to waiting state
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);

		// search types for every token
		search_types = morph_types;

		// distances
		/*
		progress = 0;
		sent_begin = 0;
		z1 = main_begin;
		last_pos = thdata->last_pos;
		curnt_sent = first_sentence;
		*/
		z = 0;
		found_all = 0;

		/*
		while(z < main_end) {
			switch(params) {
				case 6:
					if((united_mask[array_ngrams[z6]] & 0xFF0000000000) == (search_types & 0xFF0000000000)) {

					}
				case 5:
					if((united_mask[array_ngrams[z5]] & 0xFF00000000) == (search_types & 0xFF00000000)) {

					}
				case 4:
					if((united_mask[array_ngrams[z4]] & 0xFF000000) == (search_types & 0xFF000000)) {

					}
				case 3:
					if((united_mask[array_ngrams[z3]] & 0xFF0000) == (search_types & 0xFF0000)) {

					}
				case 2:
					if((united_mask[array_ngrams[z2]] & 0xFF00) == (search_types & 0xFF00)) {

					}
				case 1:
					if((united_mask[array_ngrams[z1]] & 0xFF) == (search_types & 0xFF)) {

					}
				default:
					break;
			}
		}
		*/

		switch(params) {
			case 6:
				main_end = NGRAMS6_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams6[0][z]] & 0xFF0000000000) == (search_types & 0xFF0000000000) &&	(t = array_ngrams6[2][z]) &&
						(united_mask[array_ngrams5[0][t]] & 0xFF00000000) == (search_types & 0xFF00000000) &&		(t = array_ngrams5[2][t]) &&
						(united_mask[array_ngrams4[0][t]] & 0xFF000000) == (search_types & 0xFF000000) &&		(t = array_ngrams4[2][t]) &&
						(united_mask[array_ngrams3[0][t]] & 0xFF0000) == (search_types & 0xFF0000) &&			(t = array_ngrams3[2][t]) &&
						(united_mask[array_ngrams2[0][t]] & 0xFF00) == (search_types & 0xFF00) &&			(t = array_ngrams2[2][t]) &&
						(united_mask[array_ngrams1[0][t]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			case 5:
				main_end = NGRAMS5_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams5[0][z]] & 0xFF00000000) == (search_types & 0xFF00000000) && 	(t = array_ngrams5[2][z]) &&
						(united_mask[array_ngrams4[0][t]] & 0xFF000000) == (search_types & 0xFF000000) &&	(t = array_ngrams4[2][t]) &&
						(united_mask[array_ngrams3[0][t]] & 0xFF0000) == (search_types & 0xFF0000) &&		(t = array_ngrams3[2][t]) &&
						(united_mask[array_ngrams2[0][t]] & 0xFF00) == (search_types & 0xFF00) && 		(t = array_ngrams2[2][t]) &&
						(united_mask[array_ngrams1[0][t]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			case 4:
				main_end = NGRAMS4_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams4[0][z]] & 0xFF000000) == (search_types & 0xFF000000) &&	(t = array_ngrams4[2][z]) &&
						(united_mask[array_ngrams3[0][t]] & 0xFF0000) == (search_types & 0xFF0000) &&		(t = array_ngrams3[2][t]) &&
						(united_mask[array_ngrams2[0][t]] & 0xFF00) == (search_types & 0xFF00) &&		(t = array_ngrams2[2][t]) &&
						(united_mask[array_ngrams1[0][t]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			case 3:
				main_end = NGRAMS3_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams3[0][z]] & 0xFF0000) == (search_types & 0xFF0000) &&	(t = array_ngrams3[2][z]) &&
						(united_mask[array_ngrams2[0][t]] & 0xFF00) == (search_types & 0xFF00) &&	(t = array_ngrams2[2][t]) &&
						(united_mask[array_ngrams1[0][t]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			case 2:
				main_end = NGRAMS2_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams2[0][z]] & 0xFF00) == (search_types & 0xFF00) &&	(t = array_ngrams2[2][z]) &&
						(united_mask[array_ngrams1[0][t]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			case 1:
				main_end = NGRAMS1_ARRAY_SIZE;
				while(z < main_end) {
					if(	(united_mask[array_ngrams1[0][z]] & 0xFF) == (search_types & 0xFF))
					{
						if(found_limit) {
							func_build_ngrams(z);
							//printf("\n->%d", array_ngrams1[0][z]);
							--found_limit;
						}
						++found_all;
					}
					++z;
				}
				break;
			default:
				break;
		}

		//thdata->last_pos = last_pos;
		thdata->found_num = found_all;
		//thdata->progress = progress;

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
					func_szWildMatch(wildmatch[x], x, united_words_case, WILD, thdata_united);
				} else {
					func_szWildMatch(wildmatch[x], x, united_words, WILD, thdata_united);
				}
			}
			if(wildmatch_lemma[x][0]) {
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

			// Preparing arrays
			memset(united_mask, 0, sizeof(united_mask));

			// Parsing incoming JSON string and setting global variables
			func_jsmn_json(bufin, rc);
			func_fill_search_mask();
			func_parse_last_pos();
			func_validate_distances();

			// Set the initial value of counters
			found_limit = return_ngrams;

			time_start(&tv2);
			// Find ids and set masks for all search words, lemmas, tags and patterns
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
	// main big array
	//array_united = malloc(sizeof(*array_united) * SIZE_ARRAY_MAIN);

	// Ngrams
	/*
	array_ngrams1 = malloc(sizeof(*array_ngrams1) * NGRAMS1_ARRAY_SIZE * (NGRAMS_COLUMNS - 1));
	array_ngrams2 = malloc(sizeof(*array_ngrams2) * NGRAMS2_ARRAY_SIZE * NGRAMS_COLUMNS);
	array_ngrams3 = malloc(sizeof(*array_ngrams3) * NGRAMS3_ARRAY_SIZE * NGRAMS_COLUMNS);
	array_ngrams4 = malloc(sizeof(*array_ngrams4) * NGRAMS4_ARRAY_SIZE * NGRAMS_COLUMNS);
	array_ngrams5 = malloc(sizeof(*array_ngrams5) * NGRAMS5_ARRAY_SIZE * NGRAMS_COLUMNS);
	array_ngrams6 = malloc(sizeof(*array_ngrams6) * NGRAMS6_ARRAY_SIZE * NGRAMS_COLUMNS);
	*/

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
	// sources
	//size_list_sources = SOURCES_ARRAY_SIZE * SOURCE_BUFFER_SIZE;
	//list_sources = malloc(sizeof(*list_sources) * size_list_sources);
	//memset(list_sources, 1, sizeof(*list_sources) * size_list_sources);

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
	*/

	func_read_mysql();
	//func_find_distances_for_threads();
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

	/*
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
	*/

	sem_init(&count_sem, 0, 1);

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

