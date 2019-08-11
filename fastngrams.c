/*
 * fastngrams.c - Fast search engine for n-grams.
 * Version v5.8.0 - 2018.12.13
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
//#include <sys/time.h>		/*   gettimeofday	   */
#include <mysql/mysql.h>	/*   MySQL and MariaDB	   */
#include <limits.h>		/*   LONG_MIN, ULLONG_MAX  */
#include <locale.h>		/*   for regcomp, regexec  */
#include <regex.h>		/*   regcomp, regexec	   */
#include "jsmn-master/jsmn.h"	/*   JSON		   */
#include "b64/b64.h"		/*   base64		   */

#include "time.h"		/*   			   */

#include "fastmorph.h"
#include "credentials.h"	/*   DB login, password... */

/**************************************************************************************
 *
 *                                  Functions
 *
 **************************************************************************************/

//#include "func_time.c"		/*    */
#include "func_jsmn.c"		/*    */
#include "func_crypt.c"		/*    */
#include "func_mysql.c"		/*    */
#include "func_cmp.c"		/*    */
#include "func_sort.c"		/*    */
#include "func_other.c"		/*    */
#include "func_malloc.c"	/*    */
#include "func_resize.c"	/*    */


/*
 * Reading data from MySQL (MariaDB) to arrays
 */
int func_read_mysql()
{
	MYSQL connect;
	MYSQL *myconnect = &connect;
	MYSQL_RES *myresult;
	MYSQL_ROW row;
	char *ptr;
	//char *ptr_author;
	//char *ptr_title;
	//char *ptr_date;
	//char *ptr_genre;
	char *endptr;
	char text[SOURCE_BUFFER_SIZE];
	char mycommand[200];
	int i;
	//int iauthor;
	//int ititle;
	//int idate;
	//int igenre;
	int x;

	int size;
	size = func_get_mysql_table_size("fastngrams_ngrams"); // TODO

	if(func_connect_mysql(myconnect) == 0) {
		printf("\n  Beginning data import...");

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
			sprintf(mycommand, "SELECT level, node, parentnode, united, freq FROM fastngrams_ngrams WHERE id >= %d LIMIT %d", t, n_mysql_load_limit);
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
	return 0;
}


/*
 * Finding search distances for each thread united
 */
int func_find_distances_for_threads_united()
{
	return 0;
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
 * RegEx
 */
/*int func_regex(const char pattern[WORDS_BUFFER_SIZE], const int mask_offset, char *united_x[UNITED_ARRAY_SIZE], const int type, struct thread_data_united *thdata_united)
{
	// Setting search distances for current (by each) thread
	const unsigned long long united_begin = thdata_united->start;
	register const unsigned long long united_end = thdata_united->finish;
	regex_t start_state;

	//if(regcomp(&start_state, pattern, REG_EXTENDED|REG_ICASE)) { // TODO: Case sensitive
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
}*/


/*
 * Run search united
 */
/*void * func_run_united(struct thread_data_united *thdata_united)
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
}*/


/*
 * Parsing last found positions for every thread
 */
/*int func_parse_last_pos()
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
}*/


/*
 * 
 */
void func_on_socket_event(char *bufin)
{
	struct timeval tv1, tv2, tv3, tv4; // different counters
	struct timezone tz;
	unsigned int t;
	//char bufin[SOCKET_BUFFER_SIZE];
	char bufout[SOCKET_BUFFER_SIZE];
	char temp[SOCKET_BUFFER_SIZE];
	unsigned int progress; // The position of last returned sentence in 'found_all'

	
	
	
	// Initial search time value
	time_start(&tv1, &tz);

	printf("\n-----------------------------------------------------------------");
	printf("\nRead %u bytes: %.*s\n", rc, rc, bufin);

	// Preparing arrays
	memset(united_mask, 0, sizeof(united_mask));

	// Parsing incoming JSON string and setting global variables
	func_jsmn_json(bufin, rc);
	func_fill_search_mask();
	//func_parse_last_pos();
	//func_validate_distances();

	// Set the initial value of counters
	found_limit = return_ngrams;

	time_start(&tv2, &tz);
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
	printf("\n\nszExactMatch() time: %ld milliseconds.", time_stop(&tv2, &tz));

	time_start(&tv3, &tz);
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
	printf("\nThreads func_run_united time: %ld milliseconds.", time_stop(&tv3, &tz));

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

	time_start(&tv4, &tz);
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
	printf("\nThreads func_run_cycle time: %ld milliseconds.", time_stop(&tv4, &tz));

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
	printf("\nQuery processing time: %ld milliseconds.\n", time_stop(&tv1, &tz));
	
	
	
	
			
	
	
}

/*
 * Create UNIX domain socket
 * 	netstat -ap unix |grep fastmorph.socket
 * 	ncat -U /tmp/fastmorph.socket
 */
//void * func_run_socket(/*int argc, char *argv[]*/)
/*{
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
			func_on_socket_event();
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
}*/


/*
 * Main
 */
int main()
{
	// set locale for regex functions
	setlocale(LC_ALL, "ru_RU.UTF-8");

	// Ngrams
	/*
	int array_ngrams1[NGRAMS_COLUMNS-1][NGRAMS1_ARRAY_SIZE];
	int array_ngrams2[NGRAMS_COLUMNS][NGRAMS2_ARRAY_SIZE];
	int array_ngrams3[NGRAMS_COLUMNS][NGRAMS3_ARRAY_SIZE];
	int array_ngrams4[NGRAMS_COLUMNS][NGRAMS4_ARRAY_SIZE];
	int array_ngrams5[NGRAMS_COLUMNS][NGRAMS5_ARRAY_SIZE];
	int array_ngrams6[NGRAMS_COLUMNS][NGRAMS6_ARRAY_SIZE];
	*/
	for(int i = 0; i < NGRAMS_COLUMNS; i++) {
		if(i < NGRAMS_COLUMNS-1)
			array_ngrams1[i] = malloc(sizeof(*array_ngrams1) * NGRAMS1_ARRAY_SIZE);
		array_ngrams2[i] = malloc(sizeof(*array_ngrams2) * NGRAMS2_ARRAY_SIZE);
		array_ngrams3[i] = malloc(sizeof(*array_ngrams3) * NGRAMS3_ARRAY_SIZE);
		array_ngrams4[i] = malloc(sizeof(*array_ngrams4) * NGRAMS4_ARRAY_SIZE);
		array_ngrams5[i] = malloc(sizeof(*array_ngrams5) * NGRAMS5_ARRAY_SIZE);
		array_ngrams6[i] = malloc(sizeof(*array_ngrams6) * NGRAMS6_ARRAY_SIZE);
	}
	
	
	
	
	
	
	
	
	func_malloc();

	func_read_mysql_common();
	func_read_mysql();
	//func_find_distances_for_threads();
	func_find_distances_for_threads_united();

	func_realloc();
	
	
	
	
	
	
	

	
	
	
	
	
	
	

	sem_init(&count_sem, 0, 1);

	// run func_run_socket() in a thread
	printf("\n\nCreating socket listening thread...");
	pthread_t thread; // thread identifier
	pthread_attr_t attr; // thread attributes
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // run in detached mode
	int rc_thread = pthread_create(&thread, &attr, (void *) &func_run_socket, FASTNGRAMS_UNIX_DOMAIN_SOCKET);
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
