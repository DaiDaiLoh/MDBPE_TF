/*
 * xxHash - Extremely Fast Hash algorithm
 * Header File
 * Copyright (c) 2012-2020, Yann Collet, Facebook, Inc.
 *
 * You can contact the author at :
 * - xxHash source repository : https://github.com/Cyan4973/xxHash
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
*/

/* Notice extracted from xxHash homepage :

xxHash is an extremely fast Hash algorithm, running at RAM speed limits.
It also successfully passes all tests from the SMHasher suite.

Comparison (single thread, Windows Seven 32 bits, using SMHasher on a Core 2 Duo @3GHz)

Name            Speed       Q.Score   Author
xxHash          5.4 GB/s     10
CrapWow         3.2 GB/s      2       Andrew
MumurHash 3a    2.7 GB/s     10       Austin Appleby
SpookyHash      2.0 GB/s     10       Bob Jenkins
SBox            1.4 GB/s      9       Bret Mulvey
Lookup3         1.2 GB/s      9       Bob Jenkins
SuperFastHash   1.2 GB/s      1       Paul Hsieh
CityHash64      1.05 GB/s    10       Pike & Alakuijala
FNV             0.55 GB/s     5       Fowler, Noll, Vo
CRC32           0.43 GB/s     9
MD5-32          0.33 GB/s    10       Ronald L. Rivest
SHA1-32         0.28 GB/s    10

Q.Score is a measure of quality of the hash function.
It depends on successfully passing SMHasher test set.
10 is a perfect score.

A 64-bits version, named ZSTD_XXH64, is available since r35.
It offers much better speed, but for 64-bits applications only.
Name     Speed on 64 bits    Speed on 32 bits
ZSTD_XXH64       13.8 GB/s            1.9 GB/s
ZSTD_XXH32        6.8 GB/s            6.0 GB/s
*/

#if defined (__cplusplus)
extern "C" {
#endif

#ifndef ZSTD_XXHASH_H_5627135585666179
#define ZSTD_XXHASH_H_5627135585666179 1


/* ****************************
*  Definitions
******************************/
#include "zstd_deps.h"
typedef enum { ZSTD_XXH_OK=0, ZSTD_XXH_ERROR } ZSTD_XXH_errorcode;


/* ****************************
*  API modifier
******************************/
/** ZSTD_XXH_PRIVATE_API
*   This is useful if you want to include xxhash functions in `static` mode
*   in order to inline them, and remove their symbol from the public list.
*   Methodology :
*     #define ZSTD_XXH_PRIVATE_API
*     #include "xxhash.h"
*   `xxhash.c` is automatically included.
*   It's not useful to compile and link it as a separate module anymore.
*/
#ifdef ZSTD_XXH_PRIVATE_API
#  ifndef ZSTD_XXH_STATIC_LINKING_ONLY
#    define ZSTD_XXH_STATIC_LINKING_ONLY
#  endif
#  if defined(__GNUC__)
#    define ZSTD_XXH_PUBLIC_API static __inline __attribute__((unused))
#  elif defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */)
#    define ZSTD_XXH_PUBLIC_API static inline
#  elif defined(_MSC_VER)
#    define ZSTD_XXH_PUBLIC_API static __inline
#  else
#    define ZSTD_XXH_PUBLIC_API static   /* this version may generate warnings for unused static functions; disable the relevant warning */
#  endif
#else
#  define ZSTD_XXH_PUBLIC_API   /* do nothing */
#endif /* ZSTD_XXH_PRIVATE_API */

/*!ZSTD_XXH_NAMESPACE, aka Namespace Emulation :

If you want to include _and expose_ xxHash functions from within your own library,
but also want to avoid symbol collisions with another library which also includes xxHash,

you can use ZSTD_XXH_NAMESPACE, to automatically prefix any public symbol from xxhash library
with the value of ZSTD_XXH_NAMESPACE (so avoid to keep it NULL and avoid numeric values).

Note that no change is required within the calling program as long as it includes `xxhash.h` :
regular symbol name will be automatically translated by this header.
*/
#ifdef ZSTD_XXH_NAMESPACE
#  define ZSTD_XXH_CAT(A,B) A##B
#  define ZSTD_XXH_NAME2(A,B) ZSTD_XXH_CAT(A,B)
#  define ZSTD_XXH32 ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32)
#  define ZSTD_XXH64 ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64)
#  define ZSTD_XXH_versionNumber ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH_versionNumber)
#  define ZSTD_XXH32_createState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_createState)
#  define ZSTD_XXH64_createState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_createState)
#  define ZSTD_XXH32_freeState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_freeState)
#  define ZSTD_XXH64_freeState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_freeState)
#  define ZSTD_XXH32_reset ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_reset)
#  define ZSTD_XXH64_reset ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_reset)
#  define ZSTD_XXH32_update ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_update)
#  define ZSTD_XXH64_update ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_update)
#  define ZSTD_XXH32_digest ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_digest)
#  define ZSTD_XXH64_digest ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_digest)
#  define ZSTD_XXH32_copyState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_copyState)
#  define ZSTD_XXH64_copyState ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_copyState)
#  define ZSTD_XXH32_canonicalFromHash ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_canonicalFromHash)
#  define ZSTD_XXH64_canonicalFromHash ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_canonicalFromHash)
#  define ZSTD_XXH32_hashFromCanonical ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH32_hashFromCanonical)
#  define ZSTD_XXH64_hashFromCanonical ZSTD_XXH_NAME2(ZSTD_XXH_NAMESPACE, ZSTD_XXH64_hashFromCanonical)
#endif


/* *************************************
*  Version
***************************************/
#define ZSTD_XXH_VERSION_MAJOR    0
#define ZSTD_XXH_VERSION_MINOR    6
#define ZSTD_XXH_VERSION_RELEASE  2
#define ZSTD_XXH_VERSION_NUMBER  (ZSTD_XXH_VERSION_MAJOR *100*100 + ZSTD_XXH_VERSION_MINOR *100 + ZSTD_XXH_VERSION_RELEASE)
ZSTD_XXH_PUBLIC_API unsigned ZSTD_XXH_versionNumber (void);


/* ****************************
*  Simple Hash Functions
******************************/
typedef unsigned int       ZSTD_XXH32_hash_t;
typedef unsigned long long ZSTD_XXH64_hash_t;

ZSTD_XXH_PUBLIC_API ZSTD_XXH32_hash_t ZSTD_XXH32 (const void* input, size_t length, unsigned int seed);
ZSTD_XXH_PUBLIC_API ZSTD_XXH64_hash_t ZSTD_XXH64 (const void* input, size_t length, unsigned long long seed);

/*!
ZSTD_XXH32() :
    Calculate the 32-bits hash of sequence "length" bytes stored at memory address "input".
    The memory between input & input+length must be valid (allocated and read-accessible).
    "seed" can be used to alter the result predictably.
    Speed on Core 2 Duo @ 3 GHz (single thread, SMHasher benchmark) : 5.4 GB/s
ZSTD_XXH64() :
    Calculate the 64-bits hash of sequence of length "len" stored at memory address "input".
    "seed" can be used to alter the result predictably.
    This function runs 2x faster on 64-bits systems, but slower on 32-bits systems (see benchmark).
*/


/* ****************************
*  Streaming Hash Functions
******************************/
typedef struct ZSTD_XXH32_state_s ZSTD_XXH32_state_t;   /* incomplete type */
typedef struct ZSTD_XXH64_state_s ZSTD_XXH64_state_t;   /* incomplete type */

/*! State allocation, compatible with dynamic libraries */

ZSTD_XXH_PUBLIC_API ZSTD_XXH32_state_t* ZSTD_XXH32_createState(void);
ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode  ZSTD_XXH32_freeState(ZSTD_XXH32_state_t* statePtr);

ZSTD_XXH_PUBLIC_API ZSTD_XXH64_state_t* ZSTD_XXH64_createState(void);
ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode  ZSTD_XXH64_freeState(ZSTD_XXH64_state_t* statePtr);


/* hash streaming */

ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode ZSTD_XXH32_reset  (ZSTD_XXH32_state_t* statePtr, unsigned int seed);
ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode ZSTD_XXH32_update (ZSTD_XXH32_state_t* statePtr, const void* input, size_t length);
ZSTD_XXH_PUBLIC_API ZSTD_XXH32_hash_t  ZSTD_XXH32_digest (const ZSTD_XXH32_state_t* statePtr);

ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode ZSTD_XXH64_reset  (ZSTD_XXH64_state_t* statePtr, unsigned long long seed);
ZSTD_XXH_PUBLIC_API ZSTD_XXH_errorcode ZSTD_XXH64_update (ZSTD_XXH64_state_t* statePtr, const void* input, size_t length);
ZSTD_XXH_PUBLIC_API ZSTD_XXH64_hash_t  ZSTD_XXH64_digest (const ZSTD_XXH64_state_t* statePtr);

/*
These functions generate the xxHash of an input provided in multiple segments.
Note that, for small input, they are slower than single-call functions, due to state management.
For small input, prefer `ZSTD_XXH32()` and `ZSTD_XXH64()` .

ZSTD_XXH state must first be allocated, using ZSTD_XXH*_createState() .

Start a new hash by initializing state with a seed, using ZSTD_XXH*_reset().

Then, feed the hash state by calling ZSTD_XXH*_update() as many times as necessary.
Obviously, input must be allocated and read accessible.
The function returns an error code, with 0 meaning OK, and any other value meaning there is an error.

Finally, a hash value can be produced anytime, by using ZSTD_XXH*_digest().
This function returns the nn-bits hash as an int or long long.

It's still possible to continue inserting input into the hash state after a digest,
and generate some new hashes later on, by calling again ZSTD_XXH*_digest().

When done, free ZSTD_XXH state space if it was allocated dynamically.
*/


/* **************************
*  Utils
****************************/
#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))   /* ! C99 */
#  define restrict   /* disable restrict */
#endif

ZSTD_XXH_PUBLIC_API void ZSTD_XXH32_copyState(ZSTD_XXH32_state_t* restrict dst_state, const ZSTD_XXH32_state_t* restrict src_state);
ZSTD_XXH_PUBLIC_API void ZSTD_XXH64_copyState(ZSTD_XXH64_state_t* restrict dst_state, const ZSTD_XXH64_state_t* restrict src_state);


/* **************************
*  Canonical representation
****************************/
/* Default result type for ZSTD_XXH functions are primitive unsigned 32 and 64 bits.
*  The canonical representation uses human-readable write convention, aka big-endian (large digits first).
*  These functions allow transformation of hash result into and from its canonical format.
*  This way, hash values can be written into a file / memory, and remain comparable on different systems and programs.
*/
typedef struct { unsigned char digest[4]; } ZSTD_XXH32_canonical_t;
typedef struct { unsigned char digest[8]; } ZSTD_XXH64_canonical_t;

ZSTD_XXH_PUBLIC_API void ZSTD_XXH32_canonicalFromHash(ZSTD_XXH32_canonical_t* dst, ZSTD_XXH32_hash_t hash);
ZSTD_XXH_PUBLIC_API void ZSTD_XXH64_canonicalFromHash(ZSTD_XXH64_canonical_t* dst, ZSTD_XXH64_hash_t hash);

ZSTD_XXH_PUBLIC_API ZSTD_XXH32_hash_t ZSTD_XXH32_hashFromCanonical(const ZSTD_XXH32_canonical_t* src);
ZSTD_XXH_PUBLIC_API ZSTD_XXH64_hash_t ZSTD_XXH64_hashFromCanonical(const ZSTD_XXH64_canonical_t* src);

#endif /* ZSTD_XXHASH_H_5627135585666179 */



/* ================================================================================================
   This section contains definitions which are not guaranteed to remain stable.
   They may change in future versions, becoming incompatible with a different version of the library.
   They shall only be used with static linking.
   Never use these definitions in association with dynamic linking !
=================================================================================================== */
#if defined(ZSTD_XXH_STATIC_LINKING_ONLY) && !defined(ZSTD_XXH_STATIC_H_3543687687345)
#define ZSTD_XXH_STATIC_H_3543687687345

/* These definitions are only meant to allow allocation of ZSTD_XXH state
   statically, on stack, or in a struct for example.
   Do not use members directly. */

   struct ZSTD_XXH32_state_s {
       unsigned total_len_32;
       unsigned large_len;
       unsigned v1;
       unsigned v2;
       unsigned v3;
       unsigned v4;
       unsigned mem32[4];   /* buffer defined as U32 for alignment */
       unsigned memsize;
       unsigned reserved;   /* never read nor write, will be removed in a future version */
   };   /* typedef'd to ZSTD_XXH32_state_t */

   struct ZSTD_XXH64_state_s {
       unsigned long long total_len;
       unsigned long long v1;
       unsigned long long v2;
       unsigned long long v3;
       unsigned long long v4;
       unsigned long long mem64[4];   /* buffer defined as U64 for alignment */
       unsigned memsize;
       unsigned reserved[2];          /* never read nor write, will be removed in a future version */
   };   /* typedef'd to ZSTD_XXH64_state_t */


#  ifdef ZSTD_XXH_PRIVATE_API
#    include "xxhash.c"   /* include xxhash functions as `static`, for inlining */
#  endif

#endif /* ZSTD_XXH_STATIC_LINKING_ONLY && ZSTD_XXH_STATIC_H_3543687687345 */


#if defined (__cplusplus)
}
#endif
