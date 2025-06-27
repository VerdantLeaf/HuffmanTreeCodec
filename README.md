# PLEASE READ:

This project is currently under semi-active development and improvement. I have previous working versions that are not on GitHub, but no longer have linux machine access to do easy testing and development. The improvements to the program/algorithm are mostly to be more efficient in compression and faster progromatically. Still however, no guarantees when it comes to active maintenance and development.

# Description
ht.c provides a C implementation of the huffman tree compression and decompression algorithms. ht.h provides the header implementation and information regarding the different functions and structs used within ht.c. Ideally this will become the compression/decompression functions at the end of the day. The compression algorithm uses a statically allocated sorted priority queue with the C stdlib qsort function to build the Huffman tree, something that I am proud of. While it is not the *most* efficient implementation, I think it is a little novel, and at the leastm, interesting.

## Improvements to make:
- Return project to fully working shape (need Linux machine for testing (current problem))
- Optimize tree building algorithm to use heap based pq where insertion of parent huffman node does not use quick sort to resort in O(n*log(n)) time but can insert into pq in O(log(n)) time.
- Load huffman codes from other side of the integer to not require de-reversal upon decompression, improving algorithm speed
- See if compression ratio can be improved by using current compressed nodes or storing frequency + byte value to header of file
- Explore 32 bit code implementation
- Reduce API to simple compress/decompress
- Build comprehensive debug/failure print function
- Figure out how to make newly created files "real" files like the old file it was compressed from?
