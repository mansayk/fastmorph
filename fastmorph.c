/*
 * fastmorph.c - Fast corpus search engine.
 * Version v5.8.0 - 2018.12.13
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
//#include <sys/time.h>		/*   gettimeofday	   */
#include <mysql/mysql.h>	/*   MySQL and MariaDB	   */
#include <limits.h>		/*   LONG_MIN, ULLONG_MAX  */
#include <locale.h>		/*   for regcomp, regexec  */
#include <regex.h>		/*   regcomp, regexec	   */
#include "jsmn-master/jsmn.h"	/*   JSON		   */
#include "b64/b64.h"		/*   base64		   */

#include "point_of_time.h"	/*   TODO rename time.h to fast_time.h	   */

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
	char *ptr_nice;
	char *ptr_author;
	char *ptr_title;
	char *ptr_date;
	char *ptr_type;
	char *ptr_genre;
	char *ptr_source;
	char *ptr_url;
	char *ptr_meta;
	char *ptr_full;
	char *endptr;
	char text[SOURCE_BUFFER_SIZE];
	char mycommand[200];
	int i;
	int inice;
	int iauthor;
	int ititle;
	int idate;
	int itype;
	int igenre;
	int isource;
	int iurl;
	int imeta;
	int ifull;
	int x;
	
	int size;
	size = func_get_mysql_table_size("fastmorph_sources"); // TODO
	size = func_get_mysql_table_size("fastmorph_main"); // TODO
	
	if(func_connect_mysql(myconnect) == 0) {
		// load sources
		printf("\n    Sources: getting list...");
		fflush(stdout);
		ptr = list_sources;
		ptr_nice = list_sources_nice;
		ptr_author = list_sources_author;
		ptr_title = list_sources_title;
		ptr_date = list_sources_date;
		ptr_type = list_sources_type;
		ptr_genre = list_sources_genre;
		ptr_source = list_sources_source;
		ptr_url = list_sources_url;
		ptr_meta = list_sources_meta;
		ptr_full = list_sources_full;
		if(mysql_query(myconnect, "SELECT nice, author, title, date, type, genre, source, url, meta, full FROM fastmorph_sources ORDER BY id")) {
			printf("\n***ERROR: Cannot make query to MySQL!\n");
			exit(-1);
		} else {
			if((myresult = mysql_store_result(myconnect)) == NULL) {
				printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
				exit(-1);
			} else {
				i = 0;
				inice = 0;
				iauthor = 0;
				ititle = 0;
				idate = 0;
				itype = 0;
				igenre = 0;
				isource = 0;
				iurl = 0;
				imeta = 0;
				ifull = 0;
				printf("\33[2K\r    Sources: importing...");
				fflush(stdout);
				while((row = mysql_fetch_row(myresult))) {
					// full (author(s) + title)
					decrypt(row[1], text, sizeof(text));
					ptr_sources[i] = ptr;
					ptr = strncpy(ptr, text, SOURCE_BUFFER_SIZE - 1);
					ptr[SOURCE_BUFFER_SIZE - 1] = '\0';
					//printf("\nk: %s", ptr);
					ptr += strlen(text) + 1;
					//strncpy(source_types[i], row[1], SOURCE_TYPES_BUFFER_SIZE - 1);
					//source_types[i][SOURCE_TYPES_BUFFER_SIZE - 1] = '\0';
					++i;
					// nice
					decrypt(row[0], text, sizeof(text));
					ptr_sources_nice[inice] = ptr_nice;
					ptr_nice = strncpy(ptr_nice, text, SOURCE_BUFFER_SIZE - 1);
					ptr_nice[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_nice += strlen(text) + 1;
					++inice;
					// authors
					decrypt(row[1], text, sizeof(text));
					ptr_sources_author[iauthor] = ptr_author;
					ptr_author = strncpy(ptr_author, text, SOURCE_BUFFER_SIZE - 1);
					ptr_author[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_author += strlen(text) + 1;
					++iauthor;
					// titles
					decrypt(row[2], text, sizeof(text));
					ptr_sources_title[ititle] = ptr_title;
					ptr_title = strncpy(ptr_title, text, SOURCE_BUFFER_SIZE - 1);
					ptr_title[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_title += strlen(text) + 1;
					++ititle;
					// dates
					decrypt(row[3], text, sizeof(text));
					ptr_sources_date[idate] = ptr_date;
					ptr_date = strncpy(ptr_date, text, SOURCE_BUFFER_SIZE - 1);
					ptr_date[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_date += strlen(text) + 1;
					++idate;
					// type
					decrypt(row[4], text, sizeof(text));
					ptr_sources_type[itype] = ptr_type;
					ptr_type = strncpy(ptr_type, text, SOURCE_BUFFER_SIZE - 1);
					ptr_type[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_type += strlen(text) + 1;
					++itype;
					// genres
					decrypt(row[5], text, sizeof(text));
					ptr_sources_genre[igenre] = ptr_genre;
					ptr_genre = strncpy(ptr_genre, text, SOURCE_BUFFER_SIZE - 1);
					ptr_genre[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_genre += strlen(text) + 1;
					++igenre;
					// source
					decrypt(row[6], text, sizeof(text));
					ptr_sources_source[isource] = ptr_source;
					ptr_source = strncpy(ptr_source, text, SOURCE_BUFFER_SIZE - 1);
					ptr_source[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_source += strlen(text) + 1;
					++isource;
					// url
					decrypt(row[7], text, sizeof(text));
					ptr_sources_url[iurl] = ptr_url;
					ptr_url = strncpy(ptr_url, text, SOURCE_BUFFER_SIZE - 1);
					ptr_url[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_url += strlen(text) + 1;
					++iurl;
					// meta
					decrypt(row[8], text, sizeof(text));
					ptr_sources_meta[imeta] = ptr_meta;
					ptr_meta = strncpy(ptr_meta, text, SOURCE_BUFFER_SIZE - 1);
					ptr_meta[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_meta += strlen(text) + 1;
					++imeta;
					// full
					decrypt(row[9], text, sizeof(text));
					ptr_sources_full[ifull] = ptr_full;
					ptr_full = strncpy(ptr_full, text, SOURCE_BUFFER_SIZE - 1);
					ptr_full[SOURCE_BUFFER_SIZE - 1] = '\0';
					ptr_full += strlen(text) + 1;
					++ifull;
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

		//int iii = 0;

		for(int t = 0; t < SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - AMOUNT_SENTENCES; t = t + MYSQL_LOAD_LIMIT) {
			if(SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - AMOUNT_SENTENCES - t < MYSQL_LOAD_LIMIT) {
				n_mysql_load_limit = SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - AMOUNT_SENTENCES - t;
			} else {
				n_mysql_load_limit = MYSQL_LOAD_LIMIT;
			}
			sprintf(mycommand, "SELECT united, sentence, source FROM fastmorph_main WHERE id >= %d LIMIT %d", t, n_mysql_load_limit);
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



							//if(source_curnt > 6920 && sent_curnt == 3999999)	
							//	printf("\nID: %lld UNITED: %d SENT: %d SOURCE: %d", i, array_united[i], sent_curnt, source_curnt);


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
							array_united[i] = -sent_curnt; // unary "-"
							++i;
							//array_united[i] = (int) -(strtol(row[0], &endptr, 10)); // unary "-"
							array_united[i] = (int) strtol(row[0], &endptr, 10); // unary "-"
							if(errno == ERANGE)
								perror("\nstrtol");
							if(row[0] == endptr)
								fprintf(stderr, "\nNo digits were found");



							//if(source_curnt > 6920 && sent_curnt == 3999999)	
							//	printf("\nID: %lld UNITED: %d SENT: %d SENTi: %d SOURCE: %d", i, array_united[i], sent_curnt, iii, source_curnt);
							//iii++;


							sentence_source[m++] = (int) strtol(row[2], &endptr, 10); // source id
							sent_last = sent_curnt;
						}


						++i;
					}
					p += mysql_num_rows(myresult);
					printf("\33[2K\r    Main: %d / %d (of %d / %d) rows imported.", p, i, SIZE_ARRAY_MAIN - SOURCES_ARRAY_SIZE - AMOUNT_SENTENCES, SIZE_ARRAY_MAIN);
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
	mysql_close(myconnect);
	return 0;
}


/*
 * Finding search distances for each thread
 */
/*
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
*/
/*
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
			//thread_data_array[x].start = thread_data_array[x-1].finish + 1;
			thread_data_array[x].start = thread_data_array[x-1].finish + 1; /////////////////////////////////////////////////////////////
		}
		if(x == SEARCH_THREADS - 1) {
			thread_data_array[x].finish = SIZE_ARRAY_MAIN - 1;
		} else {
			while(array_united[thread_data_array[x].finish] > SOURCE_OFFSET) {
				++thread_data_array[x].finish;
			}
			//--thread_data_array[x].finish;
		}
		// sentences
		thread_data_array[x].first_sentence = sentence;
		y = thread_data_array[x].start;
		//while(y <= thread_data_array[x].finish) {
		while(y < thread_data_array[x].finish) {
			if(array_united[y] > SOURCE_OFFSET && array_united[y] < 0) {
				++sentence;
			}
			++y;
		}

		//--sentence; ///////////////////////////////////////////////////////////

		printf("\n  Thread #%d part:%llu start:%llu finish:%llu first_sentence:%u first_element:%d last_element:%d", x, min_part, thread_data_array[x].start, thread_data_array[x].finish, thread_data_array[x].first_sentence, array_united[thread_data_array[x].start], array_united[thread_data_array[x].finish]);
	}
	return 0;
}
*/
int func_find_distances_for_threads()
{
	printf("\n\nSearch distances for each thread:");
	unsigned int x, y;
	unsigned int sentence = 0;
	unsigned long long min_part = SIZE_ARRAY_MAIN / SEARCH_THREADS;

	// first
	thread_data_array[0].start = 0;
	thread_data_array[0].first_sentence = 0; // TODO: DON'T NEED ANYMORE

	for(x = 0; x < SEARCH_THREADS; x++) {
		// distances
		if(x != 0) {
			thread_data_array[x].start = thread_data_array[x-1].finish + 1;
			thread_data_array[x].first_sentence = sentence - 1;
		}
		if(x != SEARCH_THREADS - 1) {
			thread_data_array[x].finish = x * min_part + min_part - 1;
			while(array_united[thread_data_array[x].finish] > SOURCE_OFFSET) {
				++thread_data_array[x].finish;
			}
			--thread_data_array[x].finish;
		}
		// sentences
		y = thread_data_array[x].start;
		while(y <= thread_data_array[x].finish) {
			if(array_united[y] > SOURCE_OFFSET && array_united[y] < 0) {
				++sentence;
			}
			++y;
		}
		// last
		if(x == SEARCH_THREADS - 1)
			thread_data_array[SEARCH_THREADS - 1].finish = SIZE_ARRAY_MAIN - 1;
		printf("\n  Thread #%d part:%llu start:%llu finish:%llu first_sentence:%u first_element:%d last_element:%d", x, min_part, thread_data_array[x].start, thread_data_array[x].finish, thread_data_array[x].first_sentence, array_united[thread_data_array[x].start], array_united[thread_data_array[x].finish]);
	}
	/*
	thread_data_array[0].first_sentence = thread_data_array[0].first_sentence;
	*/
	return 0;
}


/*
 * Finding search distances for each thread united
 */
/*
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
*/
int func_find_distances_for_threads_united()
{
	printf("\n\nSearch distances for each thread united:");
	unsigned long long min_part = UNITED_ARRAY_SIZE / SEARCH_THREADS;
	
	// first
	thread_data_array_united[0].start = 0;

	for(int x = 0; x < SEARCH_THREADS; x++) {
		thread_data_array_united[x].finish = x * min_part + min_part - 1;
		if(x != 0) {
			thread_data_array_united[x].start = (thread_data_array_united[x-1].finish + 1);
		}
		printf("\n  Thread united #%d part:%llu start:%llu finish:%llu", x, min_part, thread_data_array_united[x].start, thread_data_array_united[x].finish);
	}
	// last
	thread_data_array_united[SEARCH_THREADS - 1].finish = UNITED_ARRAY_SIZE - 1;
	return 0;
}


/*
 * Build text sentences from array_main
 */
int func_build_sents(unsigned int end, unsigned int curnt_source, unsigned int curnt_sent, unsigned long long sent_begin, unsigned long long z1, unsigned long long z2, unsigned long long z3, unsigned long long z4, unsigned long long z5, unsigned long long z6)
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







	strncat(bufout, ",\"nice\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_nice[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_nice[curnt_source], SOCKET_BUFFER_SIZE - 1);
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




	strncat(bufout, ",\"author\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_author[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_author[curnt_source], SOCKET_BUFFER_SIZE - 1);
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

	strncat(bufout, ",\"title_id\":", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	bufout[SOCKET_BUFFER_SIZE - 1] = '\0';
	//snprintf(temp, 100, "%d", sentence_source[curnt_sent]);
	snprintf(temp, 100, "%d", curnt_source);
	strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	strncat(bufout, ",\"title\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_title[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_title[curnt_source], SOCKET_BUFFER_SIZE - 1);
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
	//strncpy(temp, ptr_sources_date[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_date[curnt_source], SOCKET_BUFFER_SIZE - 1);
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





	strncat(bufout, ",\"type\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	strncpy(temp, ptr_sources_type[curnt_source], SOCKET_BUFFER_SIZE - 1);
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
	//strncpy(temp, ptr_sources_genre[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_genre[curnt_source], SOCKET_BUFFER_SIZE - 1);
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










	strncat(bufout, ",\"source\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_source[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_source[curnt_source], SOCKET_BUFFER_SIZE - 1);
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



	strncat(bufout, ",\"url\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_url[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_url[curnt_source], SOCKET_BUFFER_SIZE - 1);
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



	strncat(bufout, ",\"meta\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_meta[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_meta[curnt_source], SOCKET_BUFFER_SIZE - 1);
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



	strncat(bufout, ",\"full\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncpy(temp, ptr_sources_full[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - 1);
	strncpy(temp, ptr_sources_full[curnt_source], SOCKET_BUFFER_SIZE - 1);
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
	
	//strncat(bufout, "\"source_type\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncat(bufout, source_types[sentence_source[curnt_sent]], SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//strncat(bufout, "\",", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	//printf("::1:: %s", bufout);
	//fflush(stdout);
	
	strncat(bufout, ",\"sentence\":\"", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);

	//printf("::2:: %s", bufout);
	//printf("::2len:: %d", strlen(bufout));
	//fflush(stdout);

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
	
		//printf("\n::2:: %s", bufout);
		//printf("\n::2len:: %ld", strlen(bufout));
		//fflush(stdout);

		//printf("\nsent_begin: %lld", sent_begin);
		//fflush(stdout);
		//printf("\nunited: %ld", array_united[sent_begin]);
		//fflush(stdout);
		//printf("\nulemma: %s", united_lemmas[array_united[sent_begin]]);
		//fflush(stdout);

		// html tag open
		if(z[y] == sent_begin) {
			// found: lemma + tags
			if(united_lemmas[abs(array_united[sent_begin])][0] == '"') // if lemma is '"'
				snprintf(temp, SOCKET_BUFFER_SIZE, FOUND_HTML_OPEN, y, "\\", united_lemmas[abs(array_united[z[y]])], united_tags[abs(array_united[z[y]])]);
			else
				snprintf(temp, SOCKET_BUFFER_SIZE, FOUND_HTML_OPEN, y, "", united_lemmas[abs(array_united[z[y]])], united_tags[abs(array_united[z[y]])]);
			//y++;
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
		if(z[y] == sent_begin) {
			//strncat(bufout, HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			snprintf(temp, SOCKET_BUFFER_SIZE, FOUND_HTML_CLOSE, y);
			strncat(bufout, temp, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
			y++;
		} else {
			strncat(bufout, REGULAR_HTML_CLOSE, SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
		}

		if(!rspace)
			lspace = rspace;
		else
			lspace = 1;

		//printf("\n::4:: %s", bufout);
		//printf("\n::4len:: %d", strlen(bufout));
		//fflush(stdout);

	} while(array_united[++sent_begin] >= 0 && sent_begin < end);

	strncat(bufout, "\"},", SOCKET_BUFFER_SIZE - strlen(bufout) - 1);
	//if(DEBUG)
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
	//const unsigned int first_sentence = thdata->first_sentence; // TODO: DON'T NEED ANYMORE

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
	//register int negative;
	unsigned int progress;
	unsigned int found_all;
	unsigned long long sent_begin = 0;
	register unsigned int curnt_sent;
	register unsigned int curnt_source;
	register unsigned long long last_pos;

	while(1) {
		// set threads to waiting state
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond, &mutex);
		pthread_mutex_unlock(&mutex);

		//printf("|RUN_CYCLE %d", thdata->id);
		//fflush(stdout);

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
		curnt_source = 0;
		valid_source = 0;
		z1 = main_begin;
		last_pos = thdata->last_pos;
		//curnt_sent = first_sentence;

		if(DEBUG)
			printf("\nz1=%lld, last_pos=%lld, curnt_sent=%d, sent_begin=%lld, found_limit=%d, thread=%d", z1, last_pos, curnt_sent, sent_begin, found_limit, thdata->id);

		// param1
		while(z1 <= main_end) {
			// Check if a new text and/or sentence beginning
			if(array_united[z1] < 0) {
				//negative = array_united[z1];
				// Calculate sentence id
				//if(z1) 
				//	++curnt_sent;
				// source_mask to check if this text(s) is selected (subcorpora)
				if(array_united[z1] <= SOURCE_OFFSET) {
					curnt_source = SOURCE_OFFSET - array_united[z1];
					//printf("\nCURNT_SOURCE: %d SENT: %d", curnt_source, curnt_sent);
					//fflush(stdout);
					//z1++;
					//z1++;
					//continue;
					//printf("\nsources_test: %d == %d", source_mask[SOURCE_OFFSET - negative], search_types_source);
					//if(source_mask[SOURCE_OFFSET - negative] == search_types_source)
					if(source_mask[curnt_source] == search_types_source)
						valid_source = 1;
					else
						valid_source = 0;
					//negative = -array_united[++z1];
					//negative = -array_united[++z1];
					//sent_begin = z1 + 1;
					++z1;
				}
				//else {
				//negative = -negative;
				curnt_sent = -array_united[z1];
				sent_begin = ++z1;
				//++z1;
				//}
//				continue;
			}
			if(valid_source) {
				if((united_mask[array_united[z1]] & 0xFF) == (search_types & 0xFF)) {
					// param2
					if(params > 1) {
						//z2 = z1 + dist1_start;
						//x2 = z1 + dist1_end;
						z2 = z1 + dist1_start;
						x2 = z1 + dist1_end;
						/*
						x2 = z1;
						// TODO: Check that we don't cross a sentence border on distance "3-10" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						while(x2 < z2) {
							if(array_united[x2] >= 0) {
								++x2;
							} else {
								z2 = z1 + dist1_end + 1;
								break;
							}
						}*/
						while(z2 <= main_end && z2 <= x2 && array_united[z2] >= 0)  {
							if((united_mask[array_united[z2]] & 0xFF00) == (search_types & 0xFF00)) {
								// param3
								if(params > 2) {
									z3 = z2 + dist2_start;
									x3 = z2 + dist2_end;
									while(z3 <= main_end && z3 <= x3 && array_united[z3] >= 0) {
										if((united_mask[array_united[z3]] & 0xFF0000) == (search_types & 0xFF0000)) {
											// param4
											if(params > 3) {
												z4 = z3 + dist3_start;
												x4 = z3 + dist3_end;
												while(z4 <= main_end && z4 <= x4 && array_united[z4] >= 0) {
													if((united_mask[array_united[z4]] & 0xFF000000) == (search_types & 0xFF000000)) {
														// param5
														if(params > 4) {
															z5 = z4 + dist4_start;
															x5 = z4 + dist4_end;
															while(z5 <= main_end && z5 <= x5 && array_united[z5] >= 0) {
																if((united_mask[array_united[z5]] & 0xFF00000000) == (search_types & 0xFF00000000)) {
																	// param6
																	if(params > 5) {
																		z6 = z5 + dist5_start;
																		x6 = z5 + dist5_end;
																		while(z6 <= main_end && z6 <= x6 && array_united[z6] >= 0) {
																			if((united_mask[array_united[z6]] & 0xFF0000000000) == (search_types & 0xFF0000000000)) {
																				if((z1 > last_pos || !last_pos) && found_limit) {
																					sem_wait(&count_sem);
																					if(found_limit) {
																						--found_limit;
																						sem_post(&count_sem);
																						func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, z2, z3, z4, z5, z6);
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
																				func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, z2, z3, z4, z5, 0);
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
																	func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, z2, z3, z4, 0, 0);
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
														//printf("\nBB: %lld, %lld, %lld, %d", z1, z2, z3, curnt_sent);
														//printf("\nCURNT_SOURCE: %d", curnt_source);
														//fflush(stdout);
														--found_limit;
														sem_post(&count_sem);
														func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, z2, z3, 0, 0, 0);
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
											func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, z2, 0, 0, 0, 0);
											// TODO: big distances search in the next sentence too...
											//printf("\nskkkkk: %d, %d : %s, %s;", z1, z2, united_words_case[abs(array_united[z1])], united_words_case[abs(array_united[z2])]);
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
								func_build_sents(main_end, curnt_source, curnt_sent, sent_begin, z1, 0, 0, 0, 0, 0);
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
	register unsigned int curnt_source = 0;
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
				curnt_source = SOURCE_OFFSET - array_united[z1];
				++z1;
			}
			curnt_sent = -array_united[z1];
			++z1;
			if(curnt_sent >= extend_min) {
				if(curnt_sent <= extend_max)
					func_build_sents(SIZE_ARRAY_MAIN - 1, curnt_source, curnt_sent, z1, -1, -1, -1, -1, -1, -1);
				else
					break;
			}
			//++curnt_sent;
		}
		++z1;
	}
	return 0;
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
 * 
 */
void func_on_socket_event(char *bufin)
{
	struct timeval tv1, tv2, tv3, tv4; // different counters
	//	struct timezone tz;
	unsigned int t;
	//char bufin[SOCKET_BUFFER_SIZE];
	char bufout[SOCKET_BUFFER_SIZE];
	unsigned int progress; // The position of last returned sentence in 'found_all'
	char temp[SOCKET_BUFFER_SIZE];

	
	
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

		// If 'source' isn't empty, do the source search
		if(strcmp(source, "") != 0) {
			if(regex) {
				func_regex_sources(source);
				printf("\n\nsz_regex_sources() time: %ld milliseconds.", time_stop(&tv2));
			} else {
				func_szWildMatchSource(source);
				printf("\n\nszWildMatchSource() time: %ld milliseconds.", time_stop(&tv2));
			}
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





/*
 * Main
 */
int main()
{
	// set locale for regex functions
	setlocale(LC_ALL, "ru_RU.UTF-8");

	// initial values
	extend = -1;

	// main big array
	array_united = malloc(sizeof(*array_united) * SIZE_ARRAY_MAIN);

	
	
	
	
	
	
	

	
	
	
	func_malloc();
	
	
	func_read_mysql_common();
	func_read_mysql();
	func_find_distances_for_threads();
	func_find_distances_for_threads_united();

	func_realloc();
	
	
	
	
	

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	sem_init(&count_sem, 0, 1);

	// run func_run_socket() in a thread
	printf("\n\nCreating socket listening thread...");
	pthread_t thread; // thread identifier
	pthread_attr_t attr; // thread attributes
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // run in detached mode
	int rc_thread = pthread_create(&thread, &attr, (void *) &func_run_socket, FASTMORPH_UNIX_DOMAIN_SOCKET);
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
