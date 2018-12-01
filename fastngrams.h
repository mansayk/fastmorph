/*
 * fastngrams.h
 */

/**************************************************************************************
 *
 *                          Configuration
 *
 **************************************************************************************/

#ifndef FASTNGRAMS_H
#define FASTNGRAMS_H

#define VERSION				"Version v5.7.1 - 2018.12.01"		/*   Version and date								*/
#define DEBUG				0					/*   Output additional debugging info						*/

#define WORD				0					/*   id of words								*/
#define LEMMA				1					/*   id of lemmas								*/
#define TAGS				2					/*   id of tags									*/
#define WILD				3					/*   id of wildmatch words							*/
#define WILD_LEMMA			4					/*   id of wildmatch lemmas							*/
#define SEARCH_TYPES_OFFSET		8					/*   search_types -> 0-7, 8-15, 16-23, 24-31, 32-39, 40-47 bits; 48-63 are free */
#define SOURCES_ARRAY_SIZE		2750					/*   Size of sources arrays							*/
#define AMOUNT_SENTENCES		10015946				/*   Amount of sentences							*/
//#define AMOUNT_SENTENCES		64900					/*   Amount of sentences	TEST						*/
#define SIZE_ARRAY_MAIN			(139991551 + SOURCES_ARRAY_SIZE)	/*   Size of array_main								*/
//#define SIZE_ARRAY_MAIN		(1000000 + SOURCES_ARRAY_SIZE)		/*   Size of array_main		TEST						*/
#define TAGS_ARRAY_SIZE			23603					/*   Size of tags combinations array						*/
#define TAGS_UNIQ_BUFFER_SIZE		32					/*   Length of buffer for uniq tags 						*/
#define TAGS_UNIQ_ARRAY_SIZE		139					/*   Amount of uniq tags	 						*/
#define WORDS_BUFFER_SIZE		140					/*   Buffer size for words (strings)						*/
#define WORDS_ARRAY_SIZE		4279435					/*   Size of words array							*/
#define WORDS_CASE_ARRAY_SIZE		4979752					/*   Size of words (case sensitive)						*/
#define LEMMAS_ARRAY_SIZE		3347482					/*   Size of lemmas array							*/
#define UNITED_ARRAY_SIZE		5704397					/*   Size of united array							*/

// WITH_PUNCT:
//#define NGRAMS_ARRAY_SIZE		829273910				/*   Size of the whole ngrams array						*/
////#define NGRAMS_ARRAY_SIZE		8292739					/*   Size of the whole ngrams array						*/
//#define NGRAMS1_ARRAY_SIZE		5704397					/*   Size of 1-grams array							*/
//#define NGRAMS2_ARRAY_SIZE		73260916				/*   Size of 2-grams array							*/
//#define NGRAMS3_ARRAY_SIZE		164210291				/*   Size of 3-grams array							*/
//#define NGRAMS4_ARRAY_SIZE		200849446				/*   Size of 4-grams array							*/
//#define NGRAMS5_ARRAY_SIZE		199620256				/*   Size of 5-grams array							*/
//#define NGRAMS6_ARRAY_SIZE		185628604				/*   Size of 6-grams array							*/
//#define NGRAMS_COLUMNS			3					/*   Number of columns for 2-6-grams arrays (-1 for 1-grams table)		*/

// WO_PUNCT:
#define NGRAMS_ARRAY_SIZE		433958261				/*   Size of the whole ngrams array						*/
//#define NGRAMS_ARRAY_SIZE							/*   Size of the whole ngrams array						*/
#define NGRAMS1_ARRAY_SIZE		5704346					/*   Size of 1-grams array							*/
#define NGRAMS2_ARRAY_SIZE		66265427				/*   Size of 2-grams array							*/
#define NGRAMS3_ARRAY_SIZE		110879617				/*   Size of 3-grams array							*/
#define NGRAMS4_ARRAY_SIZE		103755860				/*   Size of 4-grams array							*/
#define NGRAMS5_ARRAY_SIZE		83269742				/*   Size of 5-grams array							*/
#define NGRAMS6_ARRAY_SIZE		64083269				/*   Size of 6-grams array							*/
#define NGRAMS_COLUMNS			3					/*   Number of columns for 2-6-grams arrays (-1 for 1-grams table)		*/

//#define SOURCE_TYPES_BUFFER_SIZE	64					/*   Length of buffer for source types (book, www...ru)				*/
#define SOURCE_TYPES_BUFFER_SIZE	10240					/*   Length of buffer for source types (book, www...ru)				*/
#define SOURCE_OFFSET			-2000000000				/*   Offset for the source value in the main array to differ it from sent begin */
#define MYSQL_LOAD_LIMIT		1000000					/*   Portions to load from MySQL: SELECT ... LIMIT 100000			*/
#define FOUND_NGRAMS_LIMIT_DEFAULT	100					/*   Amount of found sentences to collect					*/
#define SOURCE_BUFFER_SIZE		10240					/*   Buffer for found sentences							*/
#define SOCKET_BUFFER_SIZE		10240					/*   Size of input and output buffers for socket				*/
#define SEARCH_THREADS			1					/*   Threads count to perform search: depends on CPU cores			*/
#define AMOUNT_TOKENS			6					/*   Max amount of words to search						*/

// HTML tags for highlighting words
#define FOUND_HTML_OPEN		"<span id='found_word_%d' class='found_word' title='(%s%s) %s'>"	/*   escaping '\' if lemma is '"', lemma utself, tag	*/
#define REGULAR_HTML_OPEN	"<span title='(%s%s) %s'>"						/*   escaping '\' if lemma is '"', lemma utself, tags	*/
#define HTML_CLOSE		"</span>"

// Path to socket file
#define UNIX_DOMAIN_SOCKET	"/var/run/fastmorph/fastngrams.socket"

#endif /* FASTNGRAMS_H */
