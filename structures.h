#include "sha256.h"
struct TxInput
{
    char* prevTxid;
    int outputIndex;
};

struct TxOutput
{
    char *owner;
    int amount; 
};

struct Transaction
{
    char* txid;
    struct TxInput *inputs;
    int inputCount;
    struct TxOutput *outputs;
    int outputCount;
    struct Transaction * next;  
};

struct UTXO {
    char *txid;
    int outputIndex;
    char *owner;
    int amount;
    struct UTXO *next;
    int tracker; 
};

struct Wallet
{
    char* id;
};

struct MerkleNode
{
    char hash[SHA256_HEX_SIZE];
    struct MerkleNode *parent; 
    struct MerkleNode *left;
    struct MerkleNode *right; 
};

struct MerkleProof
{
    char **hashes;
    int *position;
    int count; 
};

struct Block
{
    char hash[SHA256_HEX_SIZE];
    char prevHash[SHA256_HEX_SIZE];
    struct Block *prev;
    struct Block *next; 
    struct Transaction *head;
    struct MerkleNode *root;
    struct MerkleNode **leaves; 
};