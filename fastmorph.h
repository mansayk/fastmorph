/*
 * fastmorph.h
 */

/**************************************************************************************
 *
 *                          Configuration
 *
 **************************************************************************************/

#ifndef FASTMORPH_H
#define FASTMORPH_H

#define VERSION				"Version v5.4.1 - 2017.09.30"		/*   Version and date								*/
#define DEBUG				0					/*   Output additional debugging info						*/

#define WORD				0					/*   id of words								*/
#define LEMMA				1					/*   id of lemmas								*/
#define TAGS				2					/*   id of tags									*/
#define WILD				3					/*   id of wild									*/
#define WILD_LEMMA			4					/*   id of wild									*/
#define SEARCH_TYPES_OFFSET		8					/*   search_types -> 0-7, 8-15, 16-23, 24-31, 32-39, 40-47 bits; 48-63 are free */
#define SOURCES_ARRAY_SIZE		2750					/*   Size of sources arrays							*/
#define AMOUNT_SENTENCES		10015946				/*   Amount of sentences							*/
//#define AMOUNT_SENTENCES		64900					/*   Amount of sentences	TEST						*/
#define SIZE_ARRAY_MAIN			(139991551 + SOURCES_ARRAY_SIZE)	/*   Size of array_main								*/
//#define SIZE_ARRAY_MAIN		(1000000 + SOURCES_ARRAY_SIZE)		/*   Size of array_main		TEST						*/
#define TAGS_ARRAY_SIZE			14864					/*   Size of tags combinations array						*/
#define TAGS_UNIQ_BUFFER_SIZE		32					/*   Length of buffer for uniq tags 						*/
#define TAGS_UNIQ_ARRAY_SIZE		134					/*   Amount of uniq tags	 						*/
#define WORDS_BUFFER_SIZE		256					/*   Buffer size for words (strings)						*/
#define WORDS_ARRAY_SIZE		1353045					/*   Size of words array							*/
#define WORDS_CASE_ARRAY_SIZE		1590290					/*   Size of words (case sensitive)						*/
#define LEMMAS_ARRAY_SIZE		963291					/*   Size of lemmas array							*/
#define UNITED_ARRAY_SIZE		1726045					/*   Size of united array							*/
//#define SOURCE_TYPES_BUFFER_SIZE	64					/*   Length of buffer for source types (book, www...ru)				*/
#define SOURCE_TYPES_BUFFER_SIZE	64					/*   Length of buffer for source types (book, www...ru)				*/
#define SOURCE_OFFSET			-2000000000				/*   Offset for the source value in the main array to differ it from sent begin */
#define MYSQL_LOAD_LIMIT		100000					/*   Portions to load from MySQL: SELECT ... LIMIT 100000			*/
#define FOUND_SENTENCES_LIMIT_DEFAULT	100					/*   Amount of found sentences to collect					*/
#define SOURCE_BUFFER_SIZE		10240					/*   Buffer for found sentences							*/
#define SOCKET_BUFFER_SIZE		10240					/*   Size of input and output buffers for socket				*/
#define SEARCH_THREADS			4					/*   Threads count to perform search: depends on CPU cores			*/
#define AMOUNT_TOKENS			6					/*   Max amount of words to search						*/

// HTML tags for highlighting words
#define FOUND_HTML_OPEN		"<span id='found_word_%d' class='found_word' title='(%s%s) %s'>"	/*   escaping '\' if lemma is '"', lemma utself, tag	*/
#define REGULAR_HTML_OPEN	"<span title='(%s%s) %s'>"						/*   escaping '\' if lemma is '"', lemma utself, tags	*/
#define HTML_CLOSE		"</span>"

// Path to socket file
#define UNIX_DOMAIN_SOCKET	"/tmp/fastmorph.socket"

#endif /* FASTMORPH_H */
