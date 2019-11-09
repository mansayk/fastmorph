/*
 * Sorting functions
 */


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
