/*
 * fastmorph.h
 */

/*****************************************************************************************************************************************************************
 *
 *                          Configuration
 *
 ****************************************************************************************************************************************************************/

#ifndef FASTMORPH_H
#define FASTMORPH_H

/********** GLOBAL **********************************************************************************************************************************************/

#define VERSION				"Version v5.8.0 - 2018.12.13"		/*   Version and date								*/
#define DEBUG				0					/*   Output additional debugging info						*/
#define TEST_MODE			1					/*   TEST mode									*/

#define SOURCE				0					/*   id of sources full								*/
#define WORD				0					/*   id of words								*/
#define LEMMA				1					/*   id of lemmas								*/
#define TAGS				2					/*   id of tags									*/
#define WILD				3					/*   id of wild words								*/
#define WILD_LEMMA			4					/*   id of wildmatch lemmas							*/
#define SEARCH_TYPES_OFFSET		8					/*   search_types -> 0-7, 8-15, 16-23, 24-31, 32-39, 40-47 bits; 48-63 are free */
#define SOURCE_TYPES_BUFFER_SIZE	64					/*   Length of buffer for source types (book, www...ru)				*/
//#define SOURCES_ARRAY_SIZE		2750					/*   Size of sources arrays							*/
#define SOURCE_OFFSET			-2000000000				/*   Offset for the source value in the main array to differ it from sent begin */
#define MYSQL_LOAD_LIMIT		1000000					/*   Portions to load from MySQL: SELECT ... LIMIT 100000			*/
#define FOUND_SENTENCES_LIMIT_DEFAULT	100					/*   Amount of found sentences to collect					*/
#define SOURCE_BUFFER_SIZE		10240					/*   Buffer for found sentences							*/
#define SOCKET_BUFFER_SIZE		102400					/*   Size of input and output buffers for socket				*/
#define AMOUNT_TOKENS			6					/*   Max amount of words to search						*/
#define EXTEND_RANGE_MAX		10					/*   Max limit for expanding context						*/
#define EXTEND_RANGE_DEFAULT		2					/*   Default value for expanding context					*/
#define TAGS_UNIQ_BUFFER_SIZE		32					/*   Length of buffer for uniq tags 						*/
#define WORDS_BUFFER_SIZE		140					/*   Buffer size for words (strings)						*/

// HTML tags for highlighting words
#define FOUND_HTML_OPEN		"<span class='found_word' id='found_word_%d' title='(%s%s) %s'>"	/*   escaping '\' if lemma is '"', lemma utself, tag	*/
#define REGULAR_HTML_OPEN	"<span class='regular_word' title='(%s%s) %s'>"				/*   escaping '\' if lemma is '"', lemma utself, tags	*/
#define FOUND_HTML_CLOSE	"</span><!-- found_word_%d -->"
#define REGULAR_HTML_CLOSE	"</span><!-- regular_word -->"

// HTML tags for highlighting words FASTNGRAMS
#define HTML_CLOSE		"</span>"


/********** CORPUS SPECIFIC *************************************************************************************************************************************/

#define SEARCH_THREADS			2					/*   Threads count to perform search: depends on CPU cores.			*/






#define SOURCES_ARRAY_SIZE		16787					/*   Size of sources arrays							*/
#define TAGS_ARRAY_SIZE			23603					/*   Size of tags combinations array						*/
#define TAGS_UNIQ_ARRAY_SIZE		139					/*   Amount of uniq tags	 						*/
#define WORDS_ARRAY_SIZE		4279435					/*   Size of words array							*/
#define WORDS_CASE_ARRAY_SIZE		4979752					/*   Size of words (case sensitive)						*/
#define LEMMAS_ARRAY_SIZE		3347482					/*   Size of lemmas array							*/
#define UNITED_ARRAY_SIZE		5704397					/*   Size of united array							*/

#if defined TEST_MODE
#define AMOUNT_SENTENCES		10211					/*   Amount of sentences	TEST						*/
#define SIZE_ARRAY_MAIN			(99996 + SOURCES_ARRAY_SIZE + AMOUNT_SENTENCES)
										/*   Size of array_main		TEST						*/
#else
#define AMOUNT_SENTENCES		40443091				/*   Amount of sentences							*/
#define SIZE_ARRAY_MAIN			(430972027 + SOURCES_ARRAY_SIZE + AMOUNT_SENTENCES)
										/*   Size of array_main								*/
#endif

#if defined TEST_MODE
// WITH_PUNCT:
//#define NGRAMS_ARRAY_SIZE		829273910				/*   Size of the whole ngrams array	TEST					*/
////#define NGRAMS_ARRAY_SIZE		8292739					/*   Size of the whole ngrams array	TEST					*/
//#define NGRAMS1_ARRAY_SIZE		5704397					/*   Size of 1-grams array		TEST					*/
//#define NGRAMS2_ARRAY_SIZE		73260916				/*   Size of 2-grams array		TEST					*/
//#define NGRAMS3_ARRAY_SIZE		164210291				/*   Size of 3-grams array		TEST					*/
//#define NGRAMS4_ARRAY_SIZE		200849446				/*   Size of 4-grams array		TEST					*/
//#define NGRAMS5_ARRAY_SIZE		199620256				/*   Size of 5-grams array		TEST					*/
//#define NGRAMS6_ARRAY_SIZE		185628604				/*   Size of 6-grams array		TEST					*/
// WO_PUNCT:
#define NGRAMS_ARRAY_SIZE		2000000					/*   Size of the whole ngrams array	TEST					*/
#define NGRAMS1_ARRAY_SIZE		1000000					/*   Size of 1-grams array		TEST					*/
#define NGRAMS2_ARRAY_SIZE		1000000					/*   Size of 2-grams array		TEST					*/
#define NGRAMS3_ARRAY_SIZE		1000000					/*   Size of 3-grams array		TEST					*/
#define NGRAMS4_ARRAY_SIZE		1000000					/*   Size of 4-grams array		TEST					*/
#define NGRAMS5_ARRAY_SIZE		1000000					/*   Size of 5-grams array		TEST					*/
#define NGRAMS6_ARRAY_SIZE		1000000					/*   Size of 6-grams array		TEST					*/
#else
// WITH_PUNCT:
//#define NGRAMS_ARRAY_SIZE		829273910				/*   Size of the whole ngrams array						*/
////#define NGRAMS_ARRAY_SIZE		8292739					/*   Size of the whole ngrams array						*/
//#define NGRAMS1_ARRAY_SIZE		5704397					/*   Size of 1-grams array							*/
//#define NGRAMS2_ARRAY_SIZE		73260916				/*   Size of 2-grams array							*/
//#define NGRAMS3_ARRAY_SIZE		164210291				/*   Size of 3-grams array							*/
//#define NGRAMS4_ARRAY_SIZE		200849446				/*   Size of 4-grams array							*/
//#define NGRAMS5_ARRAY_SIZE		199620256				/*   Size of 5-grams array							*/
//#define NGRAMS6_ARRAY_SIZE		185628604				/*   Size of 6-grams array							*/
// WO_PUNCT:
#define NGRAMS_ARRAY_SIZE		433958261				/*   Size of the whole ngrams array						*/
#define NGRAMS1_ARRAY_SIZE		5704346					/*   Size of 1-grams array							*/
#define NGRAMS2_ARRAY_SIZE		66265427				/*   Size of 2-grams array							*/
#define NGRAMS3_ARRAY_SIZE		110879617				/*   Size of 3-grams array							*/
#define NGRAMS4_ARRAY_SIZE		103755860				/*   Size of 4-grams array							*/
#define NGRAMS5_ARRAY_SIZE		83269742				/*   Size of 5-grams array							*/
#define NGRAMS6_ARRAY_SIZE		64083269				/*   Size of 6-grams array							*/
#endif








#define NGRAMS_COLUMNS			3					/*   Number of columns for 2-6-grams arrays (-1 for 1-grams table)	TEST	*/
#define FOUND_NGRAMS_LIMIT_DEFAULT	100					/*   Amount of found sentences to collect					*/

// Path to socket file
#define FASTMORPH_UNIX_DOMAIN_SOCKET	"/tmp/fastmorph.socket"
#define FASTNGRAMS_UNIX_DOMAIN_SOCKET	"/tmp/fastngrams.socket"

/********** GLOBAL VARIABLES TODO: that should be minimized! ****************************************************************************************************/

// For socket file
int cl, rc;

// Semaphore
static sem_t count_sem;









// int SOURCES_ARRAY_SIZE;					/*   Size of sources arrays							*/
// int TAGS_ARRAY_SIZE;					/*   Size of tags combinations array						*/
// int TAGS_UNIQ_ARRAY_SIZE;				/*   Amount of uniq tags	 						*/
// int WORDS_ARRAY_SIZE;					/*   Size of words array							*/
// int WORDS_CASE_ARRAY_SIZE;				/*   Size of words (case sensitive)						*/
// int LEMMAS_ARRAY_SIZE;					/*   Size of lemmas array							*/
// int UNITED_ARRAY_SIZE;					/*   Size of united array							*/
// 
// int AMOUNT_SENTENCES;					/*   Amount of sentences	TEST						*/
// int SIZE_ARRAY_MAIN;
// 
// int NGRAMS_ARRAY_SIZE;					/*   Size of the whole ngrams array	TEST					*/
// int NGRAMS1_ARRAY_SIZE;					/*   Size of 1-grams array		TEST					*/
// int NGRAMS2_ARRAY_SIZE;					/*   Size of 2-grams array		TEST					*/
// int NGRAMS3_ARRAY_SIZE;					/*   Size of 3-grams array		TEST					*/
// int NGRAMS4_ARRAY_SIZE;					/*   Size of 4-grams array		TEST					*/
// int NGRAMS5_ARRAY_SIZE;					/*   Size of 5-grams array		TEST					*/
// int NGRAMS6_ARRAY_SIZE;					/*   Size of 6-grams array		TEST					*/








// main array for fastmorph
int *array_united;

// main arrays for fastngrams
int *array_ngrams1[NGRAMS_COLUMNS-1];
int *array_ngrams2[NGRAMS_COLUMNS];
int *array_ngrams3[NGRAMS_COLUMNS];
int *array_ngrams4[NGRAMS_COLUMNS];
int *array_ngrams5[NGRAMS_COLUMNS];
int *array_ngrams6[NGRAMS_COLUMNS];

// additional
char *list_words_case;							/*   Long string for words (case sensitive): "Кеше\0алманы\0табага\0"   */
int size_list_words_case;

char *list_words;							/*   Long string for words   */
int size_list_words;

char *list_lemmas;							/*   Long string for lemmas   */
int size_list_lemmas;

char *list_tags;							/*   Long string for tags   */
int size_list_tags;

//char array_tags_uniq[TAGS_UNIQ_ARRAY_SIZE][WORDS_BUFFER_SIZE];	/*   Array of uniq tags   */
//int size_array_tags_uniq;
char *list_tags_uniq;							/*   Array of uniq tags   */
int size_list_tags_uniq;

char *list_sources;							/*   Long string for sources   */
int size_list_sources;

char *list_sources_nice;						/*   Long string for sources nice   */
int size_list_sources_nice;

char *list_sources_author;						/*   Long string for sources authors   */
int size_list_sources_author;

char *list_sources_title;						/*   Long string for sources titles   */
int size_list_sources_title;

char *list_sources_date;						/*   Long string for sources dates   */
int size_list_sources_date;

char *list_sources_type;						/*   Long string for sources type   */
int size_list_sources_type;

char *list_sources_genre;						/*   Long string for sources genres   */
int size_list_sources_genre;

char *list_sources_source;						/*   Long string for sources source   */
int size_list_sources_source;

char *list_sources_url;							/*   Long string for sources url   */
int size_list_sources_url;

char *list_sources_meta;						/*   Long string for sources meta   */
int size_list_sources_meta;

char *list_sources_full;						/*   Long string for sources full   */
int size_list_sources_full;

//char source_types[SOURCES_ARRAY_SIZE][SOURCE_TYPES_BUFFER_SIZE];	/*   source types: book, www...com   */

char source[SOURCE_BUFFER_SIZE];					/*   search string for sources: (*еш*) -> еш (кеше, ешрак, теш)   */

int sentence_source[AMOUNT_SENTENCES];					/*   Sentence-source associations. TODO: remove this (160Mb of RAM) and make through source in main array!   */

char *ptr_sources[SOURCES_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_sources_nice[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_author[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_title[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_date[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_type[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_genre[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_source[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_url[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_meta[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */
char *ptr_sources_full[SOURCES_ARRAY_SIZE];				/*   Array of pointers...   */

unsigned int source_mask[SOURCES_ARRAY_SIZE];				/*   For search in subcorpora and particular texts (bitmask)   */
//int source_mask[SOURCES_ARRAY_SIZE][3];				/*   TODO: For subcorpora. Texts: bitmask, begin, end   */

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

char *ptr_words_case[WORDS_CASE_ARRAY_SIZE];				/*   Array of pointers to list_words_case elements   */
char *ptr_words[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_lemmas[WORDS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_tags[TAGS_ARRAY_SIZE];					/*   Array of pointers...   */
char *ptr_tags_uniq[TAGS_UNIQ_ARRAY_SIZE];				/*   Array of pointers...   */

char wildmatch[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: *еш* -> еш (кеше, ешрак, теш)   */
char wildmatch_lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];			/*   search string: (*еш*) -> еш (кеше, ешрак, теш)   */
char word[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кешегә, ешрак, тешнең   */
char lemma[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: кеше, еш, теш   */
char tags[AMOUNT_TOKENS][WORDS_BUFFER_SIZE];				/*   search string: *<n>*<nom>*<pl>*   */

char *united_words_case[UNITED_ARRAY_SIZE];				/*   Array of non uniq pointers to list_words_case elements   */
char *united_words[UNITED_ARRAY_SIZE];					/*   Array of non uniq pointers   */
char *united_lemmas[UNITED_ARRAY_SIZE];					/*   Array of non uniq pointers   */
char *united_tags[UNITED_ARRAY_SIZE];					/*   Array of non uniq pointers   */

unsigned long long united_mask[UNITED_ARRAY_SIZE];			/*   Array of bits fields   */

unsigned int dist_from[AMOUNT_TOKENS];					/*   search distances to the next word   */
unsigned int dist_to[AMOUNT_TOKENS];

unsigned int size_array_found_sents_all_summ;				/*   For statistics: ...found all summary...   */

unsigned int found_limit;						/*   Counter from return_ngrams to 0   */

unsigned long long morph_types;						/*   Bits with search data   */
unsigned int morph_types_source;					/*   Bits with search data for sources   */
unsigned int case_sensitive[AMOUNT_TOKENS];				/*   Array for case sensitive (1) or not (0)   */
char morph_last_pos[WORDS_BUFFER_SIZE];					/*   The id of last found token to continue from   */
unsigned int return_sentences;						/*   Number of sentences to return   */
unsigned int return_ngrams;						/*   Number of ngrams to return   */
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

/********** FUNCTIONS *******************************************************************************************************************************************/

// Common functions
int func_read_mysql_commons();
static inline int func_cmp_strings(const void *, const void *);
static inline int func_cmp_strptrs(const void *, const void *);
static inline int func_cmp_ints(const void *, const void *);
int func_regex(const char *, const int, char **, const int, struct thread_data_united *);
int func_regex_normalization(const char *);
int func_regex_sources(const char *);
int func_szWildMatch(const char *, const int, char **, const int, struct thread_data_united *);
int func_szWildMatchSource(const char *);
int func_szExactMatch(char **, char *, const int, const int, char **, const int);
int func_fill_search_mask(void);
void decrypt(char *, char *, size_t);
static int jsoneq(const char *, jsmntok_t *, const char *);
int func_jsmn_json(char *, int);
int func_connect_mysql(MYSQL *);
void * func_run_united(struct thread_data_united *);
int prompt(void);
void * func_run_socket(char *);
int func_sort_tags(char *);
void func_realloc(void);
void func_malloc(void);

// Specific functions
int func_read_mysql(void);
int func_find_distances_for_threads(void);
int func_find_distances_for_threads_united(void);
int func_build_sents(unsigned int, unsigned int, unsigned int, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long, unsigned long long);
int func_build_ngrams(unsigned int);
void * func_run_cycle(struct thread_data *);
int func_extend_context(unsigned int);
int func_parse_last_pos(void);
int func_validate_distances(void);
void func_on_socket_event(char *);

/****************************************************************************************************************************************************************/

#endif /* FASTMORPH_H */
