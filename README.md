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

## System Requirements
- OS: tested on different Linux distributions.
- RAM: about 2 Gb for the 100 mln word corpus.
- CPU: multicore processors are recommended because of multithreading support.
- MySQL: program loads all data from MySQL database.





## Dependencies
* [jsmn] (https://github.com/zserge/jsmn) is a minimalistic JSON parser in C.
* MySQL C API is a C-based API that client applications written in C can use to communicate with MySQL Server.



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

03.04.2016 - Complex morphological search system's features were significantly extended. You can get more info about them in [The Guides] (http://corpus.tatar/en) updated up to 3.0 and higher version.

22.02.2016 - Complex morphological search function appeared in The Corpus of Written Tatar, where you can use different combinations of such parameters as wordform, lemma, grammatical tags, beginning and end of words, distances between them.
Technical info: version 1 uses about 6 Gb RAM for the corpus, consisting of 116 mln word occurences. Its speed is quite high.
