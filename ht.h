#ifndef HUFFTREE_H
#define HUFFTREE_H

#include <stdbool.h>

typedef struct HuffmanNode HuffmanNode;
typedef struct HuffmanTree HuffmanTree;

typedef struct HuffmanNode
{
    unsigned char value;    // the character value of
    unsigned int frequency; // Freq of the byte within the file
    unsigned int hcode;     // the code of the node represented in the huffman tree
    unsigned char codelength;
    HuffmanNode *left;  // NULL if dne
    HuffmanNode *right; // NULL if dne
} HuffmanNode;

typedef struct HuffmanTree
{
    unsigned int bytecount; // total # of bytes in read file
    unsigned int count;     // total number of nodes within the tree
    unsigned int maxfreq;   // Largest frequency of a byte present within the file
    HuffmanNode *tree[512]; // Huffman tree can be statically declared
    HuffmanNode *root;
} HuffmanTree;

// funcs
HuffmanTree *InitHT();
int FreeHT(HuffmanTree *ht);

int InitializeLeafNodes(FILE *inputFile, HuffmanTree *ht);
int BuildHTFromFrequencies(HuffmanTree *ht);

int GetCodeFromCharacter(HuffmanTree *ht, unsigned char value);
int GetCharacterFromCode(HuffmanTree *ht, unsigned int code, unsigned char len);

int WriteTreeToFile(HuffmanTree *ht, FILE *output);
int WriteDataToFile(HuffmanTree *ht, FILE *input, FILE *output);

HuffmanTree *ReadTreeFromFile(FILE *input);
int ReadDataFromFile(HuffmanTree *ht, FILE *input, FILE *output);

int PrintNodes(HuffmanTree *ht);
int PrintNode(HuffmanTree *ht, int index, const char *opening);
void PrintTreeInformation(HuffmanTree *ht, const char *opening);

#endif