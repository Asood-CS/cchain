#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "structures.h"
#include "sha256.h"
#define BLOCK_SIZE 4
#define NUMBLOCKS 1
char* getHash(char *ptr)
{
    char * hash = (char *)calloc(SHA256_HEX_SIZE, sizeof(char));
    sha256_hex(ptr, strlen(ptr), hash);
    return hash; 
}

void createTransactionHash(struct Transaction *transaction, int blockNumber)
{
    char *concat = (char *)malloc(1000*sizeof(char));
    concat[0] = '\0'; 
    for(int i = 0; i<transaction->inputCount; i++)
    {
        strcat(concat, transaction->inputs[i].prevTxid);
        char tempIndex[20];
        snprintf(tempIndex, sizeof(tempIndex), "%d", transaction->inputs[i].outputIndex);
        strcat(concat, tempIndex);
    }
    for(int j = 0; j<transaction->outputCount; j++)
    {
        strcat(concat, transaction->outputs[j].owner);
        char tempIndex[20];
        snprintf(tempIndex, sizeof(tempIndex), "%d", transaction->outputs[j].amount);
        strcat(concat, tempIndex);
    }
    char blockId[20];
    snprintf(blockId, sizeof(blockId), "%d", blockNumber);
    strcat(concat, blockId);
    transaction->txid = getHash(concat);
    free(concat);
}
void appendUTXO(struct UTXO **utxo, char *txid, int outputIndex, char *owner, int amount)
{
    struct UTXO *temp, *r;
    if(*utxo == NULL)
    {
        temp = (struct UTXO *)malloc(sizeof(struct UTXO));
        temp->txid = txid;
        temp->outputIndex = outputIndex;
        temp->owner = owner;
        temp->amount = amount;
        temp->tracker = 0;
        temp->next = NULL;
        *utxo = temp;
    }else
    {
        temp = *utxo; 
        while(temp->next!= NULL)
        {
            temp = temp->next; 
        }
        r = (struct UTXO *)malloc(sizeof(struct UTXO));
        r->txid = txid;
        r->outputIndex = outputIndex;
        r->owner = owner;
        r->amount = amount;
        r->tracker = 0; 
        r->next = NULL; 
        temp->next = r; 
    }

}
void removeMarkedNodes(struct UTXO **utxo)
{
    struct UTXO **temp = utxo;
    while(*temp != NULL)
    {
        struct UTXO *node = *temp; 

        if(node->tracker == 1)
        {
            *temp = node->next; 
            free(node);
        }else
        {
            temp = &node->next;
        }
    }
}
void createCoinbase(struct Wallet *miner, int amount, struct Transaction **ledger, int blockNumber, struct UTXO **utxo)
{
    struct Transaction *transaction = (struct Transaction*) malloc(sizeof(struct Transaction));
    transaction->inputCount = 0;
    transaction->inputs = NULL;
    transaction->outputCount = 1;
    transaction->outputs = malloc(sizeof(struct TxOutput));
    transaction->outputs[0].owner = miner->id;
    transaction->outputs[0].amount = amount;
    transaction->next = NULL;
    createTransactionHash(transaction, blockNumber);
    appendUTXO(utxo, transaction->txid, 0, transaction->outputs[0].owner, transaction->outputs[0].amount);
    *ledger = transaction;
}

bool createTransaction(struct Wallet *sender, struct Wallet *receiver, int amount, struct Transaction **ledger, struct UTXO **utxo, int blockNumber)
{
    if(*ledger == NULL)
    {
        printf("Error: create coinbase before adding transactions\n");
        return false; 
    }
    if(amount <=0)
    {
        printf("Input a positive amount\n");
        return false; 
    }
    struct Transaction *transaction = (struct Transaction*) malloc(sizeof(struct Transaction));
    int tracker = 0;
    struct UTXO *temp = *utxo;
    transaction->inputCount = 0; 
    while(temp != NULL)
    {
        if(tracker>=amount)
        {
            break;
        }
        if(strcmp(temp->owner, sender->id) == 0)
        {
            tracker+= temp->amount;
            temp->tracker = 1;
            transaction->inputCount++;  
        }
        temp = temp->next;
    }
    if(tracker < amount)
    {
        printf("Insufficient funds for transaction\n");
        struct UTXO *r = *utxo; 
        while(r != NULL)
        {
            r->tracker = 0; 
            r = r->next; 
        }
        free(transaction);
        return false; 
    }else if(tracker == amount)
    {
        transaction->outputCount = 1;
        transaction->outputs = malloc(sizeof(struct TxOutput));
        transaction->outputs[0].amount = amount;
        transaction->outputs[0].owner = receiver->id; 

    }else
    {
        transaction->outputCount = 2; 
        transaction->outputs = malloc(2*sizeof(struct TxOutput));
        transaction->outputs[0].amount = amount;
        transaction->outputs[0].owner = receiver->id;
        int remaining = tracker - amount; 
        transaction->outputs[1].amount = remaining;
        transaction->outputs[1].owner = sender->id;
    }
    struct UTXO *tempTwo = *utxo;
    transaction->inputs = malloc((transaction->inputCount)*sizeof(struct TxInput));
    int count = 0; 
    while(tempTwo != NULL)
    {
        if(tempTwo->tracker == 1)
        {
            transaction->inputs[count].prevTxid = tempTwo->txid;
            transaction->inputs[count].outputIndex = tempTwo->outputIndex;
            count++; 
        }
        tempTwo = tempTwo->next;
    }
    createTransactionHash(transaction, blockNumber);
    removeMarkedNodes(utxo);
    transaction->next = NULL;
    for(int j = 0; j<(transaction->outputCount); j++)
    {
        appendUTXO(utxo, transaction->txid, j, transaction->outputs[j].owner, transaction->outputs[j].amount);
    }
    struct Transaction **q = ledger; 
    while((*q) -> next != NULL)
    {
        q = &((*q) -> next); 
    }
    (*q) -> next = transaction;
    return true;
}

void getBlockHash(struct Block *block, char *ptr)
{
    char* hash = (char*)malloc(1000*sizeof(char));
    hash[0] = '\0';
    if(block->prev != NULL)
    {
        strcat(hash, block->prevHash);
    }
    struct Transaction *temp = block->head; 
    while(temp != NULL)
    {
        strcat(hash, temp->txid);
        temp = temp->next; 
    }
    sha256_hex(hash, strlen(hash), ptr);
    free(hash);    
}
struct Block* createBlock(struct Transaction **ledger, struct Block *prev)
{
    struct Block *block = (struct Block*) malloc(sizeof(struct Block));
    block->head = *ledger;
    block->prev = prev; 
    block->next = NULL;
    if(prev != NULL)
    {
        memcpy(block->prevHash, prev->hash, SHA256_HEX_SIZE);
        prev->next = block; 
    }else
    {
        memset(block->prevHash,0,64);
        strcat(block->prevHash, "\0");
    }
    char hash[SHA256_HEX_SIZE];
    getBlockHash(block, hash);
    memcpy(block->hash, hash, SHA256_HEX_SIZE);
    return block; 
}
int traverseBlock(struct Block *block)
{
    int count = 0;
    printf("Current hash: %s\n", block->hash);
    printf("Previous hash: %s\n", block->prevHash); 
    struct Transaction *temp = block->head; 
    while(temp != NULL)
    {
        count++;  
        printf("Transaction #%d | ID: %s\n", count, temp->txid);
        temp = temp->next;
    }
    return count; 
}

void createMerkleTree(struct Block *block)
{
    int size = traverseBlock(block);
    block->leaves = malloc(size*sizeof(struct MerkleNode*)); 
    struct Transaction *temp = block->head;
    for(int i = 0; i<size; i++)
    {
        block->leaves[i] = malloc(sizeof(struct MerkleNode));
        strcpy((block->leaves[i]->hash), temp->txid);
        block->leaves[i]->left = NULL;
        block->leaves[i]->right = NULL; 
        block->leaves[i]->parent = NULL;
        temp = temp->next;
    }
    struct MerkleNode **arr = malloc(size*sizeof(struct MerkleNode*)); 
    for(int tracker = 0; tracker<size; tracker++)
    {
        arr[tracker] = block->leaves[tracker];
    }
    while(size > 1)
    {
        int n = (size+1)/2;
        for(int j = 0; j<n; j++)
        {
            struct MerkleNode *parent = malloc(sizeof(struct MerkleNode));
            parent->parent = NULL; 
            char *concat = (char *)malloc(1000*sizeof(char));
            if((2*j+1)<size)
            {
                parent->left = arr[2*j];
                (parent->left)->parent = parent;
                parent->right = arr[2*j + 1];
                (parent->right)->parent = parent;
            }else
            {
                parent->left = parent->right = arr[2*j];
                (parent->left)->parent = (parent->right)->parent = parent;
            }
            strcpy(concat, parent->left->hash);
            strcat(concat, parent->right->hash);
            concat = getHash(concat);
            strcpy(parent->hash, concat);
            arr[j] = parent;
            free(concat);  
        }
        size = n; 
    }
    block->root = arr[0];
}
void getMerkleProof(struct MerkleProof *proof, struct Block *block, struct Transaction *transaction)
{
    int index = 0;
    int size = traverseBlock(block);
    struct Transaction *temp = block->head; 
    while(temp != NULL)
    {
        if(temp == transaction)
        {
            break; 
        }
        index++;
        temp = temp->next; 
    }
    if(temp == NULL)
    {
        proof->count = 0;
        proof->hashes = NULL;
        proof->position = NULL;
        printf("Transaction not found in given block\n");
        return; 
    }
    int n = size, levels = 0;
    while(n>1)
    {
        levels++; 
        n = (n+1)/2;
    }
    proof->position = (int *)malloc(levels*sizeof(int));
    int i = index;
    proof->hashes = malloc(levels*sizeof(char *));
    struct MerkleNode* tracker = block->leaves[index];
    for(int level = 0; level<levels; level++)
    {
        struct MerkleNode *sibling;
        if(i%2 == 0)
        {
            sibling = tracker->parent->right;
            proof->position[level] = 1;
        }else
        {
            sibling = tracker->parent->left; 
            proof->position[level] = 0;
        }
        proof->hashes[level] = (char *)malloc(SHA256_HEX_SIZE*sizeof(char));
        strcpy(proof->hashes[level], sibling->hash);
        i = i/2;
        tracker = tracker->parent;
    }
    proof->count = levels; 
}

bool verifyMerkleProof(struct MerkleProof *proof, struct Transaction *transaction, const char *rootHash)
{
    char current[SHA256_HEX_SIZE];
    strcpy(current, transaction->txid);
    for(int i = 0; i<proof->count; i++)
    {
        char combined[2 * SHA256_HEX_SIZE];
        if(proof->position[i] == 0)
        {
            strcpy(combined, proof->hashes[i]);
            strcat(combined, current);
        }else
        {
            strcpy(combined, current);
            strcat(combined, proof->hashes[i]);
        }
        sha256_hex(combined, strlen(combined), current);
    }
    return strcmp(current, rootHash)==0;
}
void traverseUTXO(struct UTXO *utxo)
{
    struct UTXO *temp = utxo; 
    int count = 1; 
    while(temp != NULL)
    {
        printf("Unspent output #%d: ", count);
        printf("%d coins owned by %s\n", temp->amount, temp->owner);
        temp = temp->next; 
        count++; 
    }
}
int calculateBalance(struct Wallet wallet, struct UTXO *utxo)
{
    int balance = 0;
    struct UTXO *temp = utxo; 
    while(temp != NULL)
    {
        if(strcmp(temp->owner, wallet.id) == 0)
        {
            balance += temp->amount;
        }
        temp = temp->next;
    }
    return balance;
}
int main()
{
    struct Wallet alice, bob; 
    alice.id = "Alice";
    bob.id = "Bob";
    struct Block *block;
    struct Block *prev = NULL;
    struct UTXO *utxo = NULL;   
    for(int i = 0; i<NUMBLOCKS; i++)
    {
        printf("Inputting transactions for block %d\n\n", i+1);
        int transactionCount = 0;
        struct Transaction *ledger = NULL;
        while(transactionCount < BLOCK_SIZE)
        {
            if(transactionCount == 0)
            {
                createCoinbase(&alice, 100, &ledger, i+1, &utxo);
                transactionCount++;
                continue;
            }
            int sendInput, receiveInput;
            int amount; 
            printf("\nInput sender, receiver, and amount each separated by spaces (0 for Alice, 1 for Bob): ");
            scanf("%d %d %d", &sendInput, &receiveInput, &amount);
            struct Wallet sender, receiver;
            if(sendInput == 0)
            {
                sender = alice;
            }else if(sendInput == 1)
            {
                sender = bob;
            }else
            {
                printf("Invalid input\n");
                continue;
            }

            if(receiveInput == 0)
            {
                receiver = alice;
            }else if(receiveInput == 1)
            {
                receiver = bob;
            }else
            {
                printf("Invalid input\n");
                continue;
            }
            if(createTransaction(&sender, &receiver, amount, &ledger, &utxo, i+1) == false)
            {
                continue;
            }
            transactionCount++; 
        }
        block = createBlock(&ledger, prev);
        createMerkleTree(block);
        prev = block; 
    }
    struct MerkleProof *proof = (struct MerkleProof*) malloc(sizeof(struct MerkleProof));
    getMerkleProof(proof, block, block->head);
    if(verifyMerkleProof(proof, block->head, block->root->hash) == true)
    {
        printf("\nMerkle proof verified successfully\n");
    }else
    {
        printf("\nMerkle proof returned false\n");
    }
    traverseUTXO(utxo);
    printf("\nAlice's balance: %d coins\n", calculateBalance(alice, utxo));
}