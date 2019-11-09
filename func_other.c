/*
 * Some other functions
 */


/*
 * Finding search distances for each thread united
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
 * Create UNIX domain socket
 * 	netstat -ap unix |grep fastmorph.socket
 * 	ncat -U /tmp/fastmorph.socket
 */
void * func_run_socket(char *socket_path)
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
			func_on_socket_event(bufin);
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
