# CChain

A Bitcoin-style blockchain built from scratch in C. 100% handwritten, no AI code. 

CChain implements the main data structures behind Bitcoin's ledger directly in C, using only the standard library and a small SHA-256 implementation: UTXO-based transactions, hash-linked blocks, and Merkle trees with inclusion proofs. The aim is to understand how these pieces fit together at the level of pointers, memory and hashes, without relying on a framework.

> **Status: in progress.** The transaction model, UTXO set, block chaining and Merkle proofs work. Digital signatures and Proof-of-Work are **not implemented yet** (see the [Roadmap](#roadmap)).

---

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Example Session](#example-session)
- [Project Structure](#project-structure)
- [Design Notes](#design-notes)
- [Known Limitations](#known-limitations)
- [Roadmap](#roadmap)
- [Acknowledgements](#acknowledgements)

---

## Features

### Implemented

| Component | Description |
|---|---|
| **UTXO model** | Balances are not stored anywhere. They are worked out from a set of unspent transaction outputs (UTXOs), as in Bitcoin. |
| **Coinbase transactions** | Every block starts with a coinbase transaction that mints new coins for the miner's wallet. |
| **Transactions with change** | Inputs are selected from the sender's UTXOs. If they add up to more than the amount sent, a change output goes back to the sender. |
| **Double-spend protection** | Spent outputs are removed from the UTXO set as soon as a transaction is accepted. Transactions with insufficient funds or a non-positive amount are rejected and the UTXO set is left unchanged. |
| **Transaction IDs** | Each `txid` is the SHA-256 hash of the transaction's inputs, outputs and block height. |
| **Hash-linked blocks** | Each block stores the hash of the previous block and links to its neighbours in a doubly linked chain. |
| **Merkle trees** | A binary Merkle tree is built over each block's transactions. An odd node at any level is paired with itself, as in Bitcoin. |
| **Merkle proofs** | A Merkle inclusion proof can be generated for any transaction and checked against the block's Merkle root, which is the basis of SPV (light-client) verification. |
| **Balance queries** | A wallet's balance is the sum of the UTXOs it owns. |

### Not Yet Implemented

- **Digital signatures:** transaction inputs are not yet signed or verified (planned: ECDSA over secp256k1).
- **Proof-of-Work:** blocks have no nonce or difficulty target and are not mined yet.

---

## Architecture

```
         Block N-1                        Block N
┌────────────────────────┐       ┌────────────────────────┐
│ hash                   │◄──────│ prevHash               │
│ prevHash               │       │ hash                   │
│ txs: Tx → Tx → Tx → …  │       │ txs: Tx → Tx → Tx → …  │
│ Merkle root            │       │ Merkle root            │
└────────────────────────┘       └────────────────────────┘
            ◄───── prev / next pointers ─────►

                    Merkle tree (per block)
                          root
                       /        \
                   H(AB)          H(CD)
                  /     \        /     \
               H(A)    H(B)   H(C)    H(D)     ◄── leaves = txids
```

### Core Data Structures (`structures.h`)

| Struct | Role |
|---|---|
| `Wallet` | Identifies a participant by ID. |
| `TxInput` | Refers to a previous output by `(prevTxid, outputIndex)`. |
| `TxOutput` | Assigns an `amount` to an `owner`. |
| `Transaction` | Holds its inputs, outputs and computed `txid`. Transactions are chained as a linked list inside a block. |
| `UTXO` | One entry in the global unspent-output set (a linked list). |
| `Block` | Stores the block hash, the previous block's hash, the transaction list, the Merkle root and pointers to the Merkle leaves. |
| `MerkleNode` | A tree node holding a hash, with parent and child pointers. |
| `MerkleProof` | The list of sibling hashes, and which side each sibling is on, needed to rebuild the root. |

### Key Functions (`blockchain.c`)

| Function | Purpose |
|---|---|
| `createCoinbase` | Mints the block reward and adds it to the UTXO set. |
| `createTransaction` | Selects inputs, builds outputs (including change), computes the `txid`, updates the UTXO set and appends the transaction to the ledger. |
| `createBlock` | Links a new block to the chain and computes its hash. |
| `createMerkleTree` | Builds the Merkle tree from the bottom up and stores the root on the block. |
| `getMerkleProof` | Walks from a leaf to the root and collects the sibling hashes. |
| `verifyMerkleProof` | Rebuilds the root from a `txid` and a proof and compares it with the expected root. |
| `calculateBalance` | Adds up all UTXOs owned by a wallet. |

---

## Getting Started

### Prerequisites

- A C11-compatible compiler (`gcc` or `clang`)

### Build

```bash
git clone https://github.com/Asood-CS/cchain.git
cd cchain
cc -std=c11 -Wall -Wextra -O2 blockchain.c sha256.c -o blockchain
```

### Run

```bash
./blockchain
```

The program:

1. Creates two wallets, **Alice** (`0`) and **Bob** (`1`).
2. Mints a 100-coin coinbase reward to Alice.
3. Asks for transactions until the block is full (`BLOCK_SIZE = 4` transactions, including the coinbase). Each transaction is entered as:
   ```
   <sender> <receiver> <amount>
   ```
4. Seals the block, builds its Merkle tree, and generates and verifies a Merkle proof for the coinbase transaction.
5. Prints the remaining UTXO set and Alice's balance.

The number of blocks and the number of transactions per block are set by `NUMBLOCKS` and `BLOCK_SIZE` at the top of `blockchain.c`.

---

## Example Session

Input: Alice sends Bob 30, Bob sends Alice 10, Alice sends Bob 20.

```bash
printf "0 1 30\n1 0 10\n0 1 20\n" | ./blockchain
```

```
Current hash: 8bb11590b0f0e6a52061e89b36e5a7a6a28a8e4f15e0b1212710bbb1832d8b24
Previous hash:
Transaction #1 | ID: c1bb01379219b984dfe492cb69179bc14ee90abc6a1d02706371fafaf2030d7b
Transaction #2 | ID: 88a9462c9743cd59dec10ab8d38a909c49c293adeda80f83a6b1aa6e3c7f4d88
Transaction #3 | ID: 3a460df5e94053cdc2fa93f5f03a9b7791190023f12e5fff9a25f53badec21ad
Transaction #4 | ID: 73f63fd6768a36a1c802114ec4c5c5719c1c2b2bac1bd8a843da8bd028a83b80

Merkle proof verified successfully
Unspent output #1: 10 coins owned by Alice
Unspent output #2: 20 coins owned by Bob
Unspent output #3: 20 coins owned by Bob
Unspent output #4: 50 coins owned by Alice

Alice's balance: 60 coins
```

The UTXO set shows how change is handled. Alice's 100-coin coinbase output is spent in full in the first transaction, producing 30 for Bob and 70 in change for Alice. Later transactions consume and split outputs in the same way. The final set adds up to 100 coins, the total supply.

---

## Project Structure

```
.
├── blockchain.c       # Blockchain logic and interactive driver (main)
├── structures.h       # Core data structure definitions
├── sha256.c           # SHA-256 implementation (third-party, see Acknowledgements)
├── sha256.h           # SHA-256 public interface
├── sha256.pro         # Qt Creator project file shipped with the SHA-256 library
├── sha256_c_cpp.zip   # Original SHA-256 library archive
└── README.md
```

---

## Design Notes

- **Balances come from the UTXO set.** There are no account balances. Ownership is the set of unspent outputs, which matches Bitcoin and means double-spend checking only needs to look at the current UTXO set.
- **Coin selection** is a greedy, first-fit scan over the sender's UTXOs. Selected outputs are marked first and only removed once the transaction is known to be valid, so a rejected transaction leaves the UTXO set as it was.
- **The block height is part of each `txid`.** Coinbase transactions in different blocks therefore get different IDs even when they pay the same miner the same amount. Bitcoin solved the same problem in BIP 34.
- **Odd Merkle levels** are padded by pairing the last node with itself, which follows Bitcoin's Merkle tree construction.
- **Proof direction** is stored in `MerkleProof.position`: `1` means the sibling is on the right, `0` means it is on the left. The verifier uses this to concatenate the hashes in the right order at each level.

---

## Known Limitations

CChain is a learning project and not production software. Current limitations include:

- **No signatures:** anyone can spend any wallet's outputs. Ownership is checked only by comparing wallet ID strings.
- **No mining:** block hashes are computed directly, with no nonce or difficulty target.
- **The block hash does not include the Merkle root.** It is computed from the previous hash and the `txid`s, and the Merkle tree is built afterwards. In Bitcoin, the Merkle root is part of the block header that gets hashed.
- **Fixed-size buffers:** hash inputs are concatenated into 1000-byte buffers, which limits how many inputs, outputs or transactions a single hash can cover.
- **Memory is not fully freed:** most heap allocations (transactions, blocks, Merkle nodes, proofs) are never released.
- **Single process, in memory:** there is no persistence, networking or peer-to-peer consensus.
- **Wallets are hard-coded** to two participants in the interactive driver.

---

## Roadmap

- [x] UTXO-based transaction model with change outputs
- [x] Coinbase transactions
- [x] Hash-linked blocks
- [x] Merkle tree construction
- [x] Merkle inclusion proofs and verification
- [ ] Key-pair wallets and digital signatures (ECDSA / secp256k1)
- [ ] Signature verification on transaction inputs
- [ ] Proof-of-Work mining with an adjustable difficulty target
- [ ] Block header containing the Merkle root, timestamp and nonce
- [ ] Full chain validation (hash links, PoW and UTXO replay)
- [ ] Dynamic buffers and full memory cleanup
- [ ] Makefile and automated tests

---

## Acknowledgements

- **SHA-256:** `sha256.c` and `sha256.h` come from the open-source [983/SHA-256](https://github.com/983/SHA-256) library, released under [The Unlicense](https://unlicense.org/). All other code in this repository is original.
- The design follows the [Bitcoin whitepaper](https://bitcoin.org/bitcoin.pdf) by Satoshi Nakamoto.
