# fastmorph
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
* RAM: 16Gb
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
If you are have any questions about using [fastmorph] (https://github.com/mansayk/fastmorph) in your projects, please contact us via [tatcorpus@gmail.com] (tatcorpus@gmail.com).  
Also we ask you to let us know where this search engine is used and, if you don't mind, we will publish links to those projects here.


## License
This software is distributed under GNU General Public License v3.0.


## ChangeLog:
18.11.2016 - The 4th version of fastmorph corpus search engine is released. List of changes:
   - case sensitive search option was added;
   - the memory (RAM) usage by the search system is reduced twice;
   - because of essential changes in the application architecture, search query performs now 3 - 5 times faster.
Technical info: version 4 uses about 2 Gb RAM for the same corpus.

19.07.2016 - Some improvements in the Complex morphological search engine "fastmorph":
   - in addition to the existing mask "\*", that matches any number of any symbols, the mask "?", that represents any single character, were added. More information about it you can find in the updated Guides;
   - in the technical plan memory usage by the search system is reduced up to 25%.
Technical info: version 3 uses about 4 Gb RAM for the same corpus.

13.06.2016 - Search by the middle part of a word functionality was added in the fastmorph module. For example, if you type \*әме\*, words like ярдәмендә, бәйрәмен, үткәрәмен, өйдәме will be found...

21.04.2016 - Because of implementation in "fastmorph" module some processor optimizations and multithreading support we achieved that complex morphological search now performs up to five times faster.

03.04.2016 - Complex morphological search system's features were significantly extended. You can get more info about them in [The Guides] (http://corpus.tatar/manual_en.htm) updated up to 3.0 and higher version.

22.02.2016 - Complex morphological search function appeared in The Corpus of Written Tatar, where you can use different combinations of such parameters as wordform, lemma, grammatical tags, beginning and end of words, distances between them.
Technical info: version 1 uses about 6 Gb RAM for the corpus, consisting of 116 mln word occurences. Its speed is quite high.

## MySQL database format

mysql> select * from morph6_main_apertium limit 10;  
| id | word_case | word   | lemma  | tags  | sentence | source |  
|----|-----------|--------|--------|-------|----------|--------|  
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

 

