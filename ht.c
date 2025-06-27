#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ht.h"

#define HTSIZE 512
#define BYTEMAX 256

/*
    Optimizations completed:
        * Decompression algorithm is ~40 lines shorter, by including final byte logic in main loop
        * Compression now saves the count of nodes and "CompressedNode" - Saves ~7kB on compression of golfcore.ppm
        * Writes all of the nodes in one fwrite, decreases exe size and improves size

    OPTIMIZATIONS TO MAKE:
    * Change how compression writes HCodes to prepend bits instead of append
    *   Eliminating need to reverse N lower bits on decompression
    * Reduce API to just compress and decompress
    * Comprehensive devug/failure print function
    *
    *
*/

#pragma region Private Structs

/// @brief Struct to track frequency of value and if the value has been seen
typedef struct
{
    bool seen;
    int frequency;
} SymbolReader;

/// @brief Stores nodes in a way optimal for compression
typedef struct
{
    // saves 12 bytes per node
    unsigned char value;
    unsigned int hcode;
    unsigned char codelength;
} CompressedNode;

#pragma endregion Private Structs

#pragma region Private Functions

#pragma region Utilities

/// @brief Compares the values of two nodes within the huffman tree
/// @param node1 The first node to compare
/// @param node2 The second node to compare
/// @return 1 if N1 > N2, -1 if N1 < N2, 0 if N1 == N2
int CompareNodes(const void *node1, const void *node2)
{
    HuffmanNode *N1 = *((HuffmanNode **)node1);
    HuffmanNode *N2 = *((HuffmanNode **)node2);

    if (N1->frequency > N2->frequency)
        return 1;
    else if (N1->frequency < N2->frequency)
        return -1;
    else
        return 0;
}

/// @brief Append a singular bit to all of the huffman codes below the parent node [RECURSIVE]
/// @param parent The parent node to append below
/// @param appendbit The bit (0 or 1) to append to the huffman codes
/// @return 0 upon success
int UpdateHCodesBelow(HuffmanNode *parent, int appendbit)
{
    if (parent->left == NULL && parent->right == NULL)
    {
        return 0;
    }
    else
    {
        // append a 1 to the hcodes
        if (appendbit)
        {
            parent->left->hcode = (parent->left->hcode << 1) | 0x00000001;
            parent->right->hcode = (parent->right->hcode << 1) | 0x00000001;
        }
        else // append a 0 to the hcodes
        {
            parent->left->hcode = (parent->left->hcode << 1) & 0xFFFFFFFE;
            parent->right->hcode = (parent->right->hcode << 1) & 0xFFFFFFFE;
        }
        parent->left->codelength++;
        parent->right->codelength++;

        // recurse with the same appendature
        UpdateHCodesBelow(parent->left, appendbit);
        UpdateHCodesBelow(parent->right, appendbit);
    }
    return 0;
}

/// @brief Reverse the order of the bottom n bits in an integer
/// @param reverse The integer to reverse
/// @param n Number of bits to reverse
/// @return the reversed number
unsigned int ReverseLowerNBits(unsigned int num, unsigned int n)
{
    unsigned int result = 0;
    unsigned int mask = (1U << n) - 1; // Mask for the lower N bits
    // First, isolate the lower N bits
    unsigned int lowerBits = num & mask;
    // Reverse the lower N bits
    for (int i = 0; i < n; i++)
    {
        // If the i-th bit from right is set in original number
        if (lowerBits & (1U << i))
        {
            // Set the (n-1-i)-th bit in result
            result |= (1U << (n - 1 - i));
        }
    }
    // Clear the lower N bits in the original number and add the reversed bits
    return (num & ~mask) | result;
}

/// @brief Gets the index into ht->tree for the given symbol value
/// @param ht The huffman tree to search through
/// @param value Tee value (byte symbol) to find
/// @return If successful, the index to the symbol value. If not found, -1
int GetCodeFromCharacter(HuffmanTree *ht, unsigned char value)
{
    for (int i = 0; i < ht->count; i++)
    {
        if (ht->tree[i]->value == value && ht->tree[i]->left == NULL && ht->tree[i]->right == NULL)
        {
            return i;
        }
    }
    return -1;
}

/// @brief Gets the symbol value in the huffman tree, given the code and the length of the code
/// @param ht The huffman tree to search through
/// @param code The integer value of the code
/// @param len The length of the code
/// @return If successful, the symbol value (byte) from the code. -1 if not successful
int GetCharacterFromCode(HuffmanTree *ht, unsigned int code, unsigned char len)
{
    for (int i = 0; i < ht->count; i++)
    {
        // if the value and the length is correct
        if (ht->tree[i]->hcode == code && ht->tree[i]->codelength == len)
        { // and there are no left/right children (it is a symbol node)
            if (ht->tree[i]->left == NULL && ht->tree[i]->right == NULL)
            {
                return ht->tree[i]->value;
            }
        }
    }
    return -1;
}

#pragma endregion Utilities

#pragma region InitFree

/// @brief Initializes huffman tree object
/// @return pointer to huffman tree, or NULL on failure
HuffmanTree *InitHT()
{
    HuffmanTree *ht = (HuffmanTree *)calloc(1, sizeof(HuffmanTree));
    if (ht == NULL)
    {
        return NULL;
    }
    // set all values to null
    for (int i = 0; i < HTSIZE; i++)
    {
        ht->tree[i] = NULL;
    }

    return ht;
}

HuffmanTree *InitHTLight()
{
    HuffmanTree *ht = (HuffmanTree *)calloc(1, sizeof(HuffmanTree));
    if (ht == NULL)
    {
        return NULL;
    }
    return ht;
}

/// @brief Frees all memory associated with the huffmane tree
/// @param ht huffman tree to free
/// @return if successful, 0, if failed -1
int FreeHT(HuffmanTree *ht)
{
    if (ht == NULL)
    {
        printf("Cannot free null tree!\n");
        return -1;
    }

    for (int i = 0; i < ht->count; i++)
    {
        free(ht->tree[i]);
    }
    free(ht);
    return 0;
}

#pragma endregion InitFree

#pragma region Debug

/// @brief Prints basic tree information
/// @param ht Tree to print from
/// @param opening String used to set an opening preamble to tree info
void PrintTreeInformation(HuffmanTree *ht, const char *opening)
{
    printf("%s", opening);
    printf("Huffman Tree Stats:\n");
    printf("ByteCount: %u\n", ht->bytecount);
    printf("Count: %u\n", ht->count);
    printf("Max Freq: %u\n", ht->maxfreq);
    if (ht->root == NULL)
    {
        printf("\n");
        return;
    }
    printf("Root Node info:\n");
    printf("Value: %u\tFrequency: %u\n", ht->root->value, ht->root->frequency);
    printf("Left Child: %p\tRight Child: %p\n", (void *)ht->root->left, (void *)ht->root->right);
    printf("\n");
}

/// @brief Prints the information of all the nodes currently within the tree
/// @param ht The huffman tree to print
/// @return if success, 0; if failed, -1
int PrintNodes(HuffmanTree *ht)
{
    if (ht == NULL)
    {
        printf("Cannot print null tree!\n");
        return -1;
    }
    int scount = 0;

    printf("Printing tree from root down:\n\n");
    for (int i = ht->count - 1; i >= 0; i--)
    {
        unsigned int hcode = ht->tree[i]->hcode;
        printf("Node #%d @ addr: %p\t\tLeft: %p\t\tRight: %p\n", (ht->count - 1) - i, (void *)ht->tree[i], (void *)ht->tree[i]->left, (void *)ht->tree[i]->right);
        printf("HCode of len %d:\t%u =>\t", ht->tree[i]->codelength, hcode);
        for (int i = 31; i >= 0; i--)
        {
            printf("%u", (hcode >> i) & 0x01);
            scount++;
            if (scount == 4)
            {
                printf(" ");
                scount = 0;
            }
        }
        printf("\n");
        printf("Val: %u/%c   \tFreq: %u\n\n", ht->tree[i]->value, ht->tree[i]->value, ht->tree[i]->frequency);
    }
    printf("\n");

    return 0;
}

int PrintNode(HuffmanTree *ht, int index, const char *opening)
{
    if (index > 511 || index >= ht->count)
    {
        return -1;
    }
    int scount = 0;

    unsigned int hcode = ht->tree[index]->hcode;
    printf("%s", opening);
    printf("Node of index %d @ addr: %p\t\tLeft: %p\t\tRight: %p\n", index, (void *)ht->tree[index], (void *)ht->tree[index]->left, (void *)ht->tree[index]->right);
    printf("HCode of len %d:\t%u =>\t", ht->tree[index]->codelength, hcode);
    for (int i = 31; i >= 0; i--)
    {
        printf("%u", (hcode >> i) & 0x01);
        scount++;
        if (scount == 4)
        {
            printf(" ");
            scount = 0;
        }
    }
    printf("\n");
    printf("Val: %u   \tFreq: %u\n\n", ht->tree[index]->value, ht->tree[index]->frequency);
    return 0;
}

void HTFailure()
{
}

#pragma endregion Debug

#pragma region Compression

/// @brief Traverses through a file and counts the number of each byte. Resets input stream to start of file
/// @param inputFile The pointer to the input file stream
/// @param FreqTable Table of 256 unsigned ints
/// @return 0 if successful
int InitializeLeafNodes(FILE *inputFile, HuffmanTree *ht)
{
    unsigned char symbol;
    SymbolReader symbolTable[BYTEMAX];

    if (inputFile == NULL || ht == NULL)
    {
        printf("%p\t%p\n", inputFile, ht);

        printf("Cannot parse for null file or tree!\n");
        return -1;
    }

    for (int i = 0; i < BYTEMAX; i++)
    {
        symbolTable[i].seen = false;
        symbolTable[i].frequency = 0;
    }
    // just count symbols and then update table
    while (fread(&symbol, sizeof(unsigned char), 1, inputFile))
    {
        if (!symbolTable[symbol].seen)
        {
            symbolTable[symbol].seen = true;
        }
        symbolTable[(unsigned int)symbol].frequency++;
        ht->bytecount++;
    }
    ht->count = 0;

    for (int character = 0; character < BYTEMAX; character++)
    {
        // if the symbol was seen
        if (symbolTable[character].seen)
        {
            // init one leaf node
            ht->tree[ht->count] = (HuffmanNode *)calloc(1, sizeof(HuffmanNode)); // init
            ht->tree[ht->count]->value = (unsigned char)character;
            ht->tree[ht->count]->frequency = symbolTable[character].frequency;
            ht->tree[ht->count]->left = NULL;
            ht->tree[ht->count]->right = NULL;
            ht->tree[ht->count]->hcode = 0;
            ht->tree[ht->count]->codelength = 0;
            ht->count++;

            // Check if max
            if (symbolTable[character].frequency > ht->maxfreq)
            {
                ht->maxfreq = symbolTable[character].frequency;
            }
        }
    }
    rewind(inputFile); // rewind to start of fp
    return 0;
}

/// @brief Given tree with initialized symbols, builds tree and sets root node
/// @param ht The huffman tree object to build the tree within
/// @return 0 if successful, -1 if determined root frequency does not match size of file
int BuildHTFromFrequencies(HuffmanTree *ht)
{
    unsigned int numInternalNodes = 0, numLeafNodes = ht->count, numNodesProcessed = 0;

    qsort(ht->tree, ht->count, sizeof(HuffmanNode *), CompareNodes); // qsort each of the leaf nodes to sort from min to max freqs

    while (numInternalNodes < (numLeafNodes - 1))
    {
        // grab the two lowest freq nodes
        HuffmanNode *leftNode = ht->tree[numNodesProcessed];
        HuffmanNode *rightNode = ht->tree[numNodesProcessed + 1];

        // Insert a node at the end of the tree with frequency as the sum of the two children
        HuffmanNode *parentNode = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        parentNode->left = leftNode;
        parentNode->right = rightNode;
        parentNode->frequency = leftNode->frequency + rightNode->frequency;
        ht->tree[ht->count + numInternalNodes] = parentNode;

        leftNode->hcode = (leftNode->hcode << 1) & 0xFFFFFFFE;
        rightNode->hcode = (rightNode->hcode << 1) | 0x00000001;
        leftNode->codelength++;
        rightNode->codelength++;
        UpdateHCodesBelow(leftNode, 0);
        UpdateHCodesBelow(rightNode, 1);

        numInternalNodes++;
        numNodesProcessed += 2;

        qsort(&ht->tree[numNodesProcessed], ht->count - numInternalNodes, sizeof(HuffmanNode *), CompareNodes);
    }

    ht->count += numInternalNodes;
    ht->root = ht->tree[ht->count - 1];

    if (ht->root->frequency == ht->bytecount)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int WriteCompressedTreeToFile(HuffmanTree *ht, FILE *output)
{
    unsigned int nodeCount = 0;
    CompressedNode hnodes[BYTEMAX];
    for (int i = 0; i < ht->count; i++)
    {
        if (ht->tree[i]->left == NULL && ht->tree[i]->right == NULL)
        {
            hnodes[nodeCount].value = ht->tree[i]->value;
            hnodes[nodeCount].hcode = ht->tree[i]->hcode;
            hnodes[nodeCount].codelength = ht->tree[i]->codelength;
            nodeCount++;
        }
    }
    fwrite(&nodeCount, sizeof(unsigned int), 1, output);
    fwrite(&hnodes, sizeof(CompressedNode), nodeCount, output);

    return 0;
}

int WriteTreeToFile(HuffmanTree *ht, FILE *output)
{

    // Write the huffman tree to the file so that it can be used on decode
    fwrite(ht, sizeof(HuffmanTree), 1, output);

    for (int i = 0; i < ht->count; i++)
    {
        fwrite(ht->tree[i], sizeof(HuffmanNode), 1, output);
    }

    return 0;
}

int WriteDataToFile(HuffmanTree *ht, FILE *input, FILE *output)
{
    unsigned char inbyte = 0, outbyte = 0, bitsinbyte = 0, thisbytebits = 0;
    unsigned int hcode = 0, index = 0, counter = 0;

    // read bytes from file
    while (fread(&inbyte, sizeof(unsigned char), 1, input))
    {
        // get code and shift so MSB is first bit in code
        index = GetCodeFromCharacter(ht, inbyte);
        hcode = ht->tree[index]->hcode;

        // printf("Character %c with hcode: %d\n", ht->tree[index]->value, hcode);

        // for the length of the code
        for (int i = 0; i < ht->tree[index]->codelength; i++)
        {
            // move LSB of hcode to MSB of outbyte
            outbyte = (outbyte & 0x7F) | ((hcode & 1) << 7);
            bitsinbyte++;
            thisbytebits++;

            if (bitsinbyte == 8)
            {
                counter++;
                // printf("Wrote char #%d: %u with %d bits from char %c to file\n\n", counter, outbyte, thisbytebits, outbyte);
                // write byte to file
                fwrite(&outbyte, sizeof(unsigned char), 1, output);
                bitsinbyte = 0;
                outbyte = 0; // rst outbyte
            }
            // shift hcode and outbyte one to the right
            hcode >>= 1;
            outbyte >>= 1;
        }
        thisbytebits = 0;
    }
    // printf("Bits in byte: %d\n", bitsinbyte);

    // write the remaining bits
    if (bitsinbyte != 0)
    {
        counter++;
        outbyte <<= 1; // shift back since we shifted, but didn't add new bit to byte
        // printf("Wrote final char #%d: %u with char %c to file\n\n", counter, outbyte, outbyte);
        fwrite(&outbyte, sizeof(unsigned char), 1, output);
    }
    // last char is always bitsinbyte, tells you # bits in final byte are valid
    // printf("Wrote bitsinbyte: %u with char %c to file\n\n", bitsinbyte, bitsinbyte);
    fwrite(&bitsinbyte, sizeof(unsigned char), 1, output);

    return 0;
}

#pragma endregion Compression

#pragma region Decompression

HuffmanTree *ReadCompressedTreeFromFile(FILE *input)
{
    HuffmanTree *ht = InitHTLight();
    if (ht == NULL)
        return NULL;

    fread(&ht->count, sizeof(unsigned int), 1, input);
    for (int i = 0; i < ht->count; i++)
    {
        CompressedNode cnode;
        HuffmanNode *hnode = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        fread(&cnode, sizeof(CompressedNode), 1, input);
        hnode->value = cnode.value;
        hnode->codelength = cnode.codelength;
        hnode->hcode = cnode.hcode;
        ht->tree[i] = hnode;
    }
    return ht;
}

/// @brief Reads the Huffman tree from a file and restores it's structure, except for the pointers between nodes
/// @param input
/// @return
HuffmanTree *ReadTreeFromFile(FILE *input)
{
    // Write the huffman tree to the file so that it can be used on decode
    HuffmanTree *ht = InitHT();
    if (ht == NULL)
    {
        return NULL;
    }

    fread(ht, sizeof(HuffmanTree), 1, input);

    // for each node that was in the tree, add it
    for (int i = 0; i < ht->count; i++)
    {
        HuffmanNode *hnode = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        fread(hnode, sizeof(HuffmanNode), 1, input);
        ht->tree[i] = hnode;
    }
    // This does NOT restore the pointers for the connections between the nodes
    // but that does not matter, cause you just loop through them to get values from keys
    ht->root = ht->tree[ht->count - 1];

    return ht;
}

int ReadDataFromFile(HuffmanTree *ht, FILE *input, FILE *output)
{
    unsigned char inbyte = 0, bb = 0;
    unsigned int hcode = 0, revhcode = 0;
    int idx = 0, hlen = 0, hvalue = 0, bit = 0, bitc = 8;
    unsigned long start, len, count = 0;

    start = ftell(input);
    fseek(input, 0, SEEK_END);
    len = ftell(input) - start - 1;
    fseek(input, start, SEEK_SET);

    for (count = 0; count < len; count++)
    {
        fread(&inbyte, sizeof(unsigned char), 1, input);
        if (count == len - 1)
        {
            fread(&bb, sizeof(unsigned char), 1, input);
            if (bb != 0)
            {
                inbyte >>= (8 - bb);
                bitc = bb;
            }
        }
        for (bit = 0; bit < bitc; bit++)
        {
            hcode = (inbyte & 0x01) | (hcode & 0xFFFFFFFE);
            hlen++;
            revhcode = 0;
            revhcode = ReverseLowerNBits(hcode, hlen);
            hvalue = GetCharacterFromCode(ht, revhcode, hlen);
            if (hvalue == -1)
            {
                hcode <<= 1;
                inbyte >>= 1;
            }
            else
            {
                fwrite(&hvalue, sizeof(unsigned char), 1, output);
                hlen = 0;
                hcode = 0;
                inbyte >>= 1;
            }
        }
    }
    return 0;
}

#pragma endregion Decompression

#pragma endregion Private Functions

#pragma region Public Functions

/// @brief Does huffman tree based compression on an input file. The compressed file is output with the suffix .hf
/// @param input THe file to compress.
/// @return 0 upon success, failure on any other
int DoHTCompression(FILE *input)
{

    return 0;
}

/// @brief Does huffman tree based decompression on an input file. THe decompressed file is output with the suffix .u
/// @param input The file to decompress
/// @return 0 upon success, failure on any other
int DoHTDecompression(FILE *input)
{

    return 0;
}

#pragma endregion Public Functions
