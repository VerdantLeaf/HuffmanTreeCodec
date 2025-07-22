# Academic Integrity Notice

This repository is provided solely as an example of my personal programming work. It is not intended to be used, in whole or in part, to complete academic assignments, projects, or assessments. Using this code in a way that violates academic integrity policies—such as submitting it as your own work—may result in serious consequences from your institution. By accessing, using, or adapting any part of this code, you accept full responsibility for your actions and agree that I am not liable for any academic integrity violations. You also waive any right to request that I remove this repository as a result of your misuse.

# State of Development:

This project is currently under semi-active development and improvement. I have previous working versions that are not on GitHub, but no longer have linux machine access to do easy testing and development. The improvements to the program/algorithm are mostly to be more efficient in compression and faster progromatically. Still however, no guarantees when it comes to active maintenance and development (but hopefully completion).

# Description
`ht.c` provides a C implementation of the huffman tree compression and decompression algorithms. `ht.h` provides the header implementation and information regarding the different functions and structs used within `ht.c`. Ideally this will become the compression/decompression functions at the end of the day.

## Implementation
The implementation of the Huffman coding compression algorithm uses a byte as the root symbol and utilizes a two-pass method to achieve compression. Much like two-pass compilation, the first pass scans the file for its contents (what symbols are present within the file, and what are their frequencies). The second pass then writes the data from the input file to the compressed file. This is done with bit shifting into a byte that is then written to the file.

![huffman tree from wikipedia](https://upload.wikimedia.org/wikipedia/commons/thumb/8/82/Huffman_tree_2.svg/500px-Huffman_tree_2.svg.png)

Using the frequency information, the algorithm builds the huffman tree and codes before the second pass of the file. This implementation uses a statically allocated sorted priority queue. It uses the C stdlib qsort function to build the Huffman tree. Upon further analysis/research, a min heap would provide better performance than my current implementation. Nevertheless, I am proud of this more *unique* implementation, and I think it's a little novel, and at the least, interesting. Below you can find a demonstration gif of the Huffman tree algorithm. The corresponding code can be found in `BuildHTFromFrequencies()`

![Compression demo from wikimedia](https://upload.wikimedia.org/wikipedia/commons/a/ac/Huffman_huff_demo.gif)

## Improvements to make:
- Return project to fully working shape (need Linux machine for testing (current problem))
- Optimize tree building algorithm to use heap based pq where insertion of parent huffman node does not use quick sort to resort in O(n*log(n)) time but can insert into pq in O(log(n)) time.
- Load huffman codes from other side of the integer to not require de-reversal upon decompression, improving algorithm speed
- See if compression ratio can be improved by using current compressed nodes or storing frequency + byte value to header of file
- Explore 32 bit code implementation (writing ints to the file instead of chars would decrease total writes to file needed)
- Reduce API to simple compress/decompress
- Build comprehensive debug/failure print function
- Figure out how to make newly created files "real" files like the old file it was compressed from?
