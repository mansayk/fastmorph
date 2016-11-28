# fastmorph v4
Fast corpus search engine originally made for the [Corpus of Written Tatar] (http://corpus.tatar/en) language.

You can try it [here] (http://corpus.tatar/index_en.php?openinframe=search/index_en.html?stype=6&lang=en#top). 

Source code is available at https://github.com/mansayk/fastmorph.


## Features
- Advanced search options based on any combination of different search parameters:
   * word form
   * lemma
   * set of morphological tags
   * pattern matching (currently "*" and "?" masks are supported)
   * case matching
   * distance to the next word
- It receives search queries over UNIX Domain Socket file in JSON format.


## Some speed tests
### Tests performed on machine with following characteristics:
* CPU: AMD FX-4100 Quad-Core Processor
* RAM: 16 Gb
* OS: CentOS release 6.8 (Final)
* fastmorph: compiled with 4 threads support, x64

### Test results for different types of queries:
```
Query:
   Word 1: китап
Number of occurences: 32209
Query processing time: 0,17 sec. 
```
```
Query:
   Word 1 (case sensitive, distance to the next word up to 3 words): Китап
   Word 2 (if in brackets - lemma): (бир)
Number of occurences: 15
Query processing time: 0,121 sec.
```
```
Query:
   Word 1 (case sensitive, distance range to the next word is from 1 to 100): \*ы
   Word 2 (case sensitive, distance range to the next word is from 1 to 100): \*а
   Word 3 (case sensitive, distance range to the next word is from 1 to 100): \*м
   Word 4 (case sensitive, distance range to the next word is from 1 to 100): \*с
   Word 5 (case sensitive): \*е
Number of occurences: 276778
Query processing time: 1,704 sec.
```


## System Requirements
- OS: tested on different Linux distributions.
- RAM: about 2 Gb for the 100 mln word corpus.
- CPU: multicore processors are recommended because of multithreading support.
- MySQL: program loads all data from MySQL database.
- UNIX Domain Socket support by OS.


## Dependencies
* [jsmn] (https://github.com/zserge/jsmn) is a minimalistic JSON parser in C.
* MySQL C API is a C-based API that client applications written in C can use to communicate with MySQL Server.


## Using
If you are have any questions about using [fastmorph] (https://github.com/mansayk/fastmorph) in your projects, please contact us by [tatcorpus@gmail.com] (tatcorpus@gmail.com).  
Also we ask you to let us know where this search engine is used and, if you don't mind, we will publish links to those projects here.


## License
This software is distributed under GNU General Public License v3.0.


## JSON
Search query: 
```
Schematical view: {<adj>}(0) 1-5 {ке*<n>}(1) 1-1 {(кил)}(0) 1-1 {}(0) 1-1 {}(0)  
Detailed:  
   Word 1 (distance range to the next word is from 1 to 5): <adj>  
   Word 2 (case sensitive): ке*<n>  
   Word 3: (кил)  
   Word 4:  
   Word 5:  
```
### Input format
```
{  
  "word": [  
    "",  
    "",  
    "",  
    "",  
    ""  
  ],  
  "lemma": [  
    "",  
    "",  
    "кил",  
    "",  
    ""  
  ],  
  "tags": [  
    "<adj>",  
    "<n>",  
    "",  
    "",  
    ""  
  ],  
  "wildmatch": [  
    "",  
    "ке*",  
    "",  
    "",  
    ""  
  ],  
  "dist_from": [  
    1,  
    1,  
    1,  
    1  
  ],  
  "dist_to": [  
    5,  
    1,  
    1,  
    1  
  ],  
  "types": 4218888,  
  "params": 3,  
  "last_pos": "0"  
}  
```
**"types"** is used as a set of bits according to these rules:  
```
#define WORD_CASE			0				/*   id of words (case sensitive)						*/
#define WORD				1				/*   id of words								*/
#define LEMMA				2				/*   id of lemmas								*/
#define TAGS				3				/*   id of tags									*/
#define WILD_CASE			4				/*   id of wild (case sensitive)						*/
#define WILD				5				/*   id of wild									*/
#define SEARCH_TYPES_OFFSET		10				/*   search_types -> 0-9, 10-19, 20-29, 30-39, 40-49 bits; 50-63 are free	*/
```
**"params"** is just an amount of words to search (1-5).  

**Warning!** You should normalize and verify input data before passing it to fastmorph:  
- remove all not allowed symbols;
- check string legths and so on.

### Output format
```
{  
  "example": [  
    {  
      "id": 15853,  
      "source": "\"2013 Универсиадасы блогы\" (web-сайт)",  
      "source_type": "kazan2013.ru",  
      "sentence": "Универсиада кебек зур проектның бер өлеше булу өчен, Казанга Россиянең төрле  
        төбәкләреннән һәм Дөньяның  
        <span id='found_word_0' class='found_word' title='(төрле) <adj>'>төрле</span>  
        илләреннән бик күп  
        <span id='found_word_1' class='found_word' title='(кеше) <n>,<nom>,<sg>'>кеше</span>  
        <span id='found_word_2' class='found_word' title='(кил) <ifi>,<iv>,<p3>,<sg>,<v>'>килде</span>."  
    },  
    {  
      "id": -1  
    }  
  ],  
  "last_pos": "892447x39311905x75980782x114356633",  
  "found_all": 1359  
}  
```
As you see, each word matching the search query is returned in the following HTML tags:  
```
<span id='found_word_0' class='found_word' title='(LEMMA) <TAG1><TAG2>'>FOUND_WORD</span>
```
so, for example, you can use CSS for highlighting them...


## MySQL database format

You can find CREATE TABLE examples [here] (https://github.com/mansayk/fastmorph/blob/master/mysql_create_table.txt).

mysql> select * from morph6_main_apertium limit 10;  
  
| id | word_case | word   | lemma  | tags  | sentence | source |  
| ---: | -------: | -----: | -----: | ----: | -------: | -----: |  
|  0 |     89304 | 137624 | 103580 | 11189 |        1 |      1 |  
|  1 |    197781 | 390168 | 291788 |  6598 |        1 |      1 |  
|  2 |        21 |     21 |     15 | 14861 |        1 |      1 |  
|  3 |     82146 | 166720 | 121016 | 11029 |        1 |      1 |  
|  4 |    134768 | 245422 | 177813 |  1870 |        1 |      1 |  
|  5 |        19 |     19 |     13 | 11354 |        1 |      1 |  
|  6 |    405188 | 852194 | 632380 |  2160 |        1 |      1 |  
|  7 |    405343 | 857977 | 637594 | 14724 |        1 |      1 |  
|  8 |        19 |     19 |     13 | 11354 |        1 |      1 |  
|  9 |    451227 | 978676 | 717918 | 14608 |        1 |      1 |  
  
mysql> select * from morph6_words_case_apertium where id > 200000 limit 10;  
  
| id     | freq | word_case                |  
| -----: | ---: | ------------------------ |  
| 200001 |    4 | Идарәсендәге             |  
| 200002 |    1 | Идарәсендәме             |  
| 200003 |    3 | Идарәсене                |  
| 200004 |  290 | Идарәсенең               |  
| 200005 |   14 | Идарәсеннән              |  
| 200006 |    1 | Идарәсеның               |  
| 200007 |   79 | Идарәсенә                |  
| 200008 |    1 | Идарәснең                |  
| 200009 |    1 | Идарәсәнең               |  
| 200010 |    1 | Идарәханә                |  
  
mysql> select * from morph6_words_apertium where id > 100000 limit 10;  
  
| id     | freq | word                                    |  
| -----: | ---: | --------------------------------------- |  
| 100001 |  975 | артистларны                             |  
| 100002 |    7 | артистларны гына                        |  
| 100003 |   74 | артистларны да                          |  
| 100004 |    1 | артистларныкы                           |  
| 100005 |    1 | артистларныкыдай                        |  
| 100006 |    8 | артистларныкыннан                       |  
| 100007 |    1 | артистларныкыннан да                    |  
| 100008 |    1 | артистларныкыча                         |  
| 100009 | 1408 | артистларның                            |  
| 100010 |    3 | артистларның гына                       |  
  
mysql> select * from morph6_lemmas_apertium where id > 300000 limit 10;  
  
| id     | freq | lemma                      |  
| -----: | ---: | -------------------------- |  
| 300001 |    1 | иярүчелек                  |  
| 300002 |  130 | иярүчеләр                  |  
| 300003 |    8 | иярүчеләргә                |  
| 300004 |    2 | иярүчеләрдә                |  
| 300005 |    3 | иярүчеләрдән               |  
| 300006 |    9 | иярүчеләре                 |  
| 300007 |    2 | иярүчеләрегез              |  
| 300008 |    2 | иярүчеләрен                |  
| 300009 |    1 | иярүчеләренең              |  
| 300010 |   12 | иярүчеләрне                |  
  
mysql> select * from morph6_tags_apertium where id > 11100 limit 10;  
  
| id    | freq  | combinations                       |  
| ----: | ----: | ---------------------------------- |  
| 11101 |     4 | \<ant\>,\<dat\>,\<f\>,\<frm\>,\<np\>,\<px2sg\> |  
| 11102 | 17141 | \<ant\>,\<dat\>,\<f\>,\<np\>               |  
| 11103 |   387 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<pl\>          |  
| 11104 |     1 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<pl\>,\<px1pl\>  |  
| 11105 |     1 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<pl\>,\<px1sg\>  |  
| 11106 |    12 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<pl\>,\<px3sp\>  |  
| 11107 |     1 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<pl\>,\<px\>     |  
| 11108 |    40 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<px1pl\>       |  
| 11109 |   101 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<px1sg\>       |  
| 11110 |    41 | \<ant\>,\<dat\>,\<f\>,\<np\>,\<px2sg\>       |  
  
mysql> select * from sources where col1 > 300 limit 3;  
  
| col1 |col2 | col3 |
| ---: | --- | ---- |
|  301 | "miras.belem.ru" (web-сайт)                                                                   | miras.belem.ru |
|  302 | Абдулла Алиш. Куршеләр                                                                        | book           |
|  303 | Дәүләт хакимияте һәм җирле үзидарә органнарының бердәм "Рәсми Татарстан" порталы (web-сайт)   | tatarstan.ru   |


## Apertium
If you use [Apertium's] (http://wiki.apertium.org/wiki/Publications) tagger to morphologically annotate the corpus, then you can use our Python script to generate tables from Apertium's output. (Script will be available soon.)  
This script also needs "inv_so" text file in the following format:
  
| sentence id | source id |  
| ------: | ------: |  
|    1 |    1 |  
|    2 |    1 |  
|    3 |    1 |  
|    4 |    1 |  
|    5 |    1 |  
|    6 |    1 |  
|    7 |    1 |  
|    8 |    1 |  
|    9 |    1 |  
|   10 |    1 |  


## ChangeLog:
18.11.2016 - The 4th version of fastmorph corpus search engine is released. List of changes:
   - case sensitive search option was added;
   - the memory (RAM) usage by the search system is reduced twice;
   - because of essential changes in the application architecture, search query performs now 3 - 5 times faster.
Technical info: version 4 consumes about 2 Gb RAM for the same corpus.

19.07.2016 - Some improvements in the Complex morphological search engine "fastmorph":
   - in addition to the existing mask "\*", that matches any number of any symbols, the mask "?", that represents any single character, were added. More information about it you can find in the updated Guides;
   - in the technical plan memory usage by the search system is reduced up to 25%.
Technical info: version 3 consumes about 4 Gb RAM for the same corpus.

13.06.2016 - Search by the middle part of a word functionality was added in the fastmorph module. For example, if you type \*әме\*, words like ярдәмендә, бәйрәмен, үткәрәмен, өйдәме will be found...

21.04.2016 - Because of implementation in "fastmorph" module some processor optimizations and multithreading support we achieved that complex morphological search now performs up to five times faster.

03.04.2016 - Complex morphological search system's features were significantly extended. You can get more info about them in [The Guides] (http://corpus.tatar/manual_en.htm) updated up to 3.0 and higher version.

22.02.2016 - Complex morphological search function appeared in The Corpus of Written Tatar, where you can use different combinations of such parameters as wordform, lemma, grammatical tags, beginning and end of words, distances between them.
Technical info: version 1 consumes about 6 Gb RAM for the corpus, consisting of 116 mln word occurences. Its speed is quite high.

