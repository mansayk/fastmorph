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

#define VERSION				"Version v5.7.1 - 2018.12.01"		/*   Version and date								*/
#define DEBUG				0					/*   Output additional debugging info						*/
//#define TEST_MODE								/*   TEST mode									*/

#define SOURCE				0					/*   id of sources full								*/
#define WORD				0					/*   id of words								*/
#define LEMMA				1					/*   id of lemmas								*/
#define TAGS				2					/*   id of tags									*/
#define WILD				3					/*   id of wild words								*/
#define WILD_LEMMA			4					/*   id of wildmatch lemmas							*/
#define SEARCH_TYPES_OFFSET		8					/*   search_types -> 0-7, 8-15, 16-23, 24-31, 32-39, 40-47 bits; 48-63 are free */
#define SOURCE_TYPES_BUFFER_SIZE	64					/*   Length of buffer for source types (book, www...ru)				*/
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

/********** CORPUS SPECIFIC *************************************************************************************************************************************/

#define SEARCH_THREADS			1					/*   Threads count to perform search: depends on CPU cores.			*/
#define SOURCES_ARRAY_SIZE		16787					/*   Size of sources arrays							*/
#define TAGS_ARRAY_SIZE			23603					/*   Size of tags combinations array						*/
#define TAGS_UNIQ_ARRAY_SIZE		139					/*   Amount of uniq tags	 						*/
#define WORDS_ARRAY_SIZE		4279435					/*   Size of words array							*/
#define WORDS_CASE_ARRAY_SIZE		4979752					/*   Size of words (case sensitive)						*/
#define LEMMAS_ARRAY_SIZE		3347482					/*   Size of lemmas array							*/
#define UNITED_ARRAY_SIZE		5704397					/*   Size of united array							*/

#if defined TEST_MODE
#define AMOUNT_SENTENCES		10211					/*   Amount of sentences	TEST						*/
#define SIZE_ARRAY_MAIN			(99996 + SOURCES_ARRAY_SIZE + AMOUNT_SENTENCES)		/*   Size of array_main		TEST						*/
#else
#define AMOUNT_SENTENCES		40443091				/*   Amount of sentences							*/
#define SIZE_ARRAY_MAIN			(430972027 + SOURCES_ARRAY_SIZE + AMOUNT_SENTENCES)	/*   Size of array_main								*/
#endif

// Path to socket file
#define UNIX_DOMAIN_SOCKET	"/var/run/fastmorph/fastmorph.socket"

/****************************************************************************************************************************************************************/

#endif /* FASTMORPH_H */
