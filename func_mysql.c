/*
 * MySQL related common functions
 */


/*
 * Connecting (or reconnecting) to MySQL (MariaDB)
 */
int func_connect_mysql(MYSQL *myconnect)
{
	printf("\nMySQL: ");
	mysql_init(myconnect);
	if(!mysql_real_connect(myconnect, DBHOST, DBUSER, DBPASSWORD, DB, 3306, NULL, 0)) {
		printf("\n  ***ERROR: Cannot connect to MySQL: \n");
		fprintf(stderr, "%s\n", mysql_error(myconnect));
		mysql_close(myconnect);
		exit(-1);
	}
	if(!mysql_set_character_set(myconnect, "utf8")) {
		printf("\n  MySQL Server Status: %s", mysql_stat(myconnect));
		printf("\n  New client character set: %s", mysql_character_set_name(myconnect));
	}
	return 0;
}

/*
 * Get size of MySQL (MariaDB) table
 */
int func_get_mysql_table_size(char *table)
{
	MYSQL connect;
	MYSQL *myconnect = &connect;
	MYSQL_RES *myresult;
	MYSQL_ROW row;
	char mycommand[200];
	char *endptr;
	int size;

	if(func_connect_mysql(myconnect)) {
		exit(-1);
	}
	
	sprintf(mycommand, "SELECT COUNT(*) FROM %s", table);
	if(mysql_query(myconnect, mycommand)) {
		printf("\n***ERROR: Cannot make query to MySQL!");
		exit(-1);
	} else {
		if((myresult = mysql_store_result(myconnect)) == NULL) {
			printf("\n***ERROR: MySQL - problem in 'myresult'!\n");
			exit(-1);
		} else {
			if((row = mysql_fetch_row(myresult))) {
				size = (int) strtol(row[0], &endptr, 10);
			}
			printf("\n    %s table size: %d", table, size);
			fflush(stdout);
		}
		if(mysql_errno(myconnect)) {
			printf("\n***ERROR: MySQL - some error occurred!\n");
			exit(-1);
		}
		mysql_free_result(myresult);
	}
	fflush(stdout);
	mysql_close(myconnect);
	return size;
}


/*
 * Reading data from MySQL (MariaDB) to arrays
 */
int func_read_mysql_common()
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
	size = func_get_mysql_table_size("fastmorph_tags_uniq"); // TODO
	size = func_get_mysql_table_size("fastmorph_tags"); // TODO
	size = func_get_mysql_table_size("fastmorph_words_case"); // TODO
	size = func_get_mysql_table_size("fastmorph_words"); // TODO
	size = func_get_mysql_table_size("fastmorph_lemmas"); // TODO
	size = func_get_mysql_table_size("fastmorph_united"); // TODO
	
	if(func_connect_mysql(myconnect)) {
		exit(-1);
	}
		
	printf("\n  Beginning data import...");

	// load tags_uniq table
	ptr = list_tags_uniq;
	if(mysql_query(myconnect, "SELECT id, tag FROM fastmorph_tags_uniq")) {
		printf("\n    ***ERROR: Cannot make query to MySQL!\n");
		exit(-1);
	} else {
		if((myresult = mysql_store_result(myconnect)) == NULL) {
			printf("\n    ***ERROR: MySQL - problem in 'myresult'!\n");
			exit(-1);
		} else {
			i = 0;
			while((row = mysql_fetch_row(myresult))) {
				decrypt(row[1], text, sizeof(text));
				ptr_tags_uniq[i] = ptr;
				ptr = strncpy(ptr, text, TAGS_UNIQ_BUFFER_SIZE - 1);
				ptr[TAGS_UNIQ_BUFFER_SIZE - 1] = '\0';
				ptr += strlen(text) + 1;
				++i;
			}
			printf("\n    Tags_uniq: %lld rows imported.", mysql_num_rows(myresult));
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
	if(mysql_query(myconnect, "SELECT id, combinations FROM fastmorph_tags")) {
		printf("\n    ***ERROR: Cannot make query to MySQL!\n");
		exit(-1);
	} else {
		if((myresult = mysql_store_result(myconnect)) == NULL) {
			printf("\n    ***ERROR: MySQL - problem in 'myresult'!\n");
			exit(-1);
		} else {
			i = 0;
			while((row = mysql_fetch_row(myresult))) {
				decrypt(row[1], text, sizeof(text));
				ptr_tags[i] = ptr;
				ptr = strncpy(ptr, text, WORDS_BUFFER_SIZE - 1);
				ptr[WORDS_BUFFER_SIZE - 1] = '\0';
				ptr += strlen(text) + 1;
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
	if(mysql_query(myconnect, "SELECT word_case FROM fastmorph_words_case")) {
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
				decrypt(row[0], text, sizeof(text));
				ptr_words_case[i] = ptr;
				ptr = strncpy(ptr, text, WORDS_BUFFER_SIZE - 1);
				ptr[WORDS_BUFFER_SIZE - 1] = '\0';
				ptr += strlen(text) + 1;
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
	if(mysql_query(myconnect, "SELECT word FROM fastmorph_words")) {
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
				decrypt(row[0], text, sizeof(text));
				ptr_words[i] = ptr;
				ptr = strncpy(ptr, text, WORDS_BUFFER_SIZE - 1);
				ptr[WORDS_BUFFER_SIZE - 1] = '\0';
				ptr += strlen(text) + 1;
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
	if(mysql_query(myconnect, "SELECT lemma FROM fastmorph_lemmas")) {
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
				decrypt(row[0], text, sizeof(text));
				ptr_lemmas[i] = ptr;
				ptr = strncpy(ptr, text, WORDS_BUFFER_SIZE - 1);
				ptr[WORDS_BUFFER_SIZE - 1] = '\0';
				ptr += strlen(text) + 1;
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
	if(mysql_query(myconnect, "SELECT word_case, word, lemma, tags FROM fastmorph_united")) {
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
	printf("\n    Data import's done!");
	mysql_close(myconnect);
	return 0;
}
