// This code is part of Toolkit(FileHash)
// A useful and powerful toolkit(FileHash)
// Copyright (C) 2012-2016 Chengr28
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "BLAKE2.h"

//Cyclic right rotation.
#define ROTR64(x, y)  (((x) >> (y)) ^ ((x) << (64 - (y))))

//Little-endian byte access.
#define B2B_GET64(p)                              \
	(((uint64_t)((uint8_t *)(p))[0]) ^            \
	(((uint64_t)((uint8_t *)(p))[1U]) << 8U) ^    \
	(((uint64_t)((uint8_t *)(p))[2U]) << 16U) ^   \
	(((uint64_t)((uint8_t *)(p))[3U]) << 24U) ^   \
	(((uint64_t)((uint8_t *)(p))[4U]) << 32U) ^   \
	(((uint64_t)((uint8_t *)(p))[5U]) << 40U) ^   \
	(((uint64_t)((uint8_t *)(p))[6U]) << 48U) ^   \
	(((uint64_t)((uint8_t *)(p))[7U]) << 56U))

//G Mixing function.
#define B2B_G(a, b, c, d, x, y) {     \
	v[a] = v[a] + v[b] + x;           \
	v[d] = ROTR64(v[d] ^ v[a], 32);   \
	v[c] = v[c] + v[d];               \
	v[b] = ROTR64(v[b] ^ v[c], 24);   \
	v[a] = v[a] + v[b] + y;           \
	v[d] = ROTR64(v[d] ^ v[a], 16);   \
	v[c] = v[c] + v[d];               \
	v[b] = ROTR64(v[b] ^ v[c], 63); }

//Initialization Vector.
static const uint64_t blake2b_iv[8U] = 
{
	0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 
	0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1, 
	0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 
	0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
};

//Compression function. "last" flag indicates last block
static void blake2b_compress(
	blake2b_ctx * const ctx, 
	int last)
{
	const uint8_t sigma[12U][16U] = 
	{
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, 
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }, 
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 }, 
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 }, 
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 }, 
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }, 
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 }, 
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 }, 
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 }, 
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }, 
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, 
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
	};
	int i = 0;
	uint64_t v[16U]{0}, m[16U]{0};

//Init work variables
	for (i = 0;i < 8;++i)
	{
		v[i] = ctx->h[i];
		v[i + 8] = blake2b_iv[i];
	}

	v[12U] ^= ctx->t[0];                 //Low 64 bits of offset
	v[13U] ^= ctx->t[1U];                //High 64 bits

//Last block flag set ?
	if (last)
		v[14U] = ~v[14U];

//Get little-endian words
	for (i = 0;i < 16;++i)
		m[i] = B2B_GET64(&ctx->b[8 * i]);

//Twelve rounds
	for (i = 0;i < 12;++i)
	{
		B2B_G(0, 4, 8, 12, m[sigma[i][0]], m[sigma[i][1U]]);
		B2B_G(1, 5, 9, 13, m[sigma[i][2U]], m[sigma[i][3U]]);
		B2B_G(2, 6, 10, 14, m[sigma[i][4U]], m[sigma[i][5U]]);
		B2B_G(3, 7, 11, 15, m[sigma[i][6U]], m[sigma[i][7U]]);
		B2B_G(0, 5, 10, 15, m[sigma[i][8U]], m[sigma[i][9U]]);
		B2B_G(1, 6, 11, 12, m[sigma[i][10U]], m[sigma[i][11U]]);
		B2B_G(2, 7, 8, 13, m[sigma[i][12U]], m[sigma[i][13U]]);
		B2B_G(3, 4, 9, 14, m[sigma[i][14U]], m[sigma[i][15U]]);
	}

	for (i = 0;i < 8;++i)
		ctx->h[i] ^= v[i] ^ v[i + 8];

	return;
}

//Add "inlen" bytes from "in" into the hash
void blake2b_update(
	blake2b_ctx * const ctx, 
	const void * const in, 
	size_t inlen)                       //Data bytes
{
	size_t i = 0;
	for (i = 0;i < inlen;++i)
	{
	//Buffer full ?
		if (ctx->c == 128)
		{
			ctx->t[0] += ctx->c;        //Add counters
			if (ctx->t[0] < ctx->c)     //Carry overflow ?
				ctx->t[1U]++;           //High word
			blake2b_compress(ctx, 0);   //Compress (not last)
			ctx->c = 0;                 //Counter to zero
		}

		ctx->b[ctx->c++] = ((const uint8_t *)in)[i];
	}

	return;
}

//Initialize the hashing context "ctx" with optional key "key"
int blake2b_init(
	blake2b_ctx * const ctx, 
	size_t outlen, 
	const void * const key, 
	size_t keylen)                      //(keylen=0: no key)
{
	size_t i = 0;

//Illegal parameters
	if (outlen == 0 || outlen > 64 || keylen > 64)
		return -1;

//State, "param block"
	for (i = 0;i < 8;++i)
		ctx->h[i] = blake2b_iv[i];
	ctx->h[0] ^= 0x01010000 ^ (keylen << 8U) ^ outlen;

	ctx->t[0] = 0;                      //Input count low word
	ctx->t[1U] = 0;                     //Input count high word
	ctx->c = 0;                         //Pointer within buffer
	ctx->outlen = outlen;

//Zero input block
	for (i = keylen;i < 128;++i)
		ctx->b[i] = 0;
	if (keylen > 0)
	{
		blake2b_update(ctx, key, keylen);
		ctx->c = 128;                   //At the end
	}

	return 0;
}

//Generate the message digest (size given in init)
//Result placed in "out"
void blake2b_final(
	blake2b_ctx * const ctx, 
	void * const out)
{
	size_t i = 0;
	ctx->t[0] += ctx->c;                //Mark last block offset
	if (ctx->t[0] < ctx->c)             //Carry overflow
		ctx->t[1U]++;                   //High word

//Fill up with zeros
	while (ctx->c < 128)
		ctx->b[ctx->c++] = 0;
	blake2b_compress(ctx, 1);           //Final block flag = 1

//Little endian convert and store
	for (i = 0;i < ctx->outlen;++i)
		((uint8_t *)out)[i] = (ctx->h[i >> 3U] >> (8 * (i & 7))) & 0xFF;

	return;
}

//Cyclic right rotation.
#define ROTR32(x, y)  (((x) >> (y)) ^ ((x) << (32 - (y))))

//Little-endian byte access.
#define B2S_GET32(p)                              \
	(((uint32_t)((uint8_t *)(p))[0]) ^            \
	(((uint32_t)((uint8_t *)(p))[1U]) << 8U) ^    \
	(((uint32_t)((uint8_t *)(p))[2U]) << 16U) ^   \
	(((uint32_t)((uint8_t *)(p))[3U]) << 24U))

//Mixing function G.
#define B2S_G(a, b, c, d, x, y) {     \
	v[a] = v[a] + v[b] + x;           \
	v[d] = ROTR32(v[d] ^ v[a], 16);   \
	v[c] = v[c] + v[d];               \
	v[b] = ROTR32(v[b] ^ v[c], 12);   \
	v[a] = v[a] + v[b] + y;           \
	v[d] = ROTR32(v[d] ^ v[a], 8);    \
	v[c] = v[c] + v[d];               \
	v[b] = ROTR32(v[b] ^ v[c], 7); }

//Initialization Vector.
static const uint32_t blake2s_iv[8U] = 
{
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 
	0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

//Compression function. "last" flag indicates last block
static void blake2s_compress(
	blake2s_ctx * const ctx, 
	int last)
{
	const uint8_t sigma[10U][16U] = 
	{
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, 
		{ 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }, 
		{ 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 }, 
		{ 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 }, 
		{ 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 }, 
		{ 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }, 
		{ 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 }, 
		{ 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 }, 
		{ 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 }, 
		{ 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 }
	};
	int i = 0;
	uint32_t v[16U]{0}, m[16U]{0};

//Init work variables
	for (i = 0;i < 8;++i)
	{
		v[i] = ctx->h[i];
		v[i + 8] = blake2s_iv[i];
	}

	v[12U] ^= ctx->t[0];                //Low 32 bits of offset
	v[13U] ^= ctx->t[1U];               //High 32 bits

//Last block flag set ?
	if (last)
		v[14U] = ~v[14U];

//Get little-endian words
	for (i = 0;i < 16;++i)
		m[i] = B2S_GET32(&ctx->b[4 * i]);

//Ten rounds
	for (i = 0;i < 10;++i)        
	{
		B2S_G(0, 4, 8, 12, m[sigma[i][0]], m[sigma[i][1U]]);
		B2S_G(1, 5, 9, 13, m[sigma[i][2U]], m[sigma[i][3U]]);
		B2S_G(2, 6, 10, 14, m[sigma[i][4U]], m[sigma[i][5U]]);
		B2S_G(3, 7, 11, 15, m[sigma[i][6U]], m[sigma[i][7U]]);
		B2S_G(0, 5, 10, 15, m[sigma[i][8U]], m[sigma[i][9U]]);
		B2S_G(1, 6, 11, 12, m[sigma[i][10U]], m[sigma[i][11U]]);
		B2S_G(2, 7, 8, 13, m[sigma[i][12U]], m[sigma[i][13U]]);
		B2S_G(3, 4, 9, 14, m[sigma[i][14U]], m[sigma[i][15U]]);
	}

	for (i = 0;i < 8;++i)
		ctx->h[i] ^= v[i] ^ v[i + 8];

	return;
}

//Add "inlen" bytes from "in" into the hash
void blake2s_update(
	blake2s_ctx * const ctx, 
	const void * const in, 
	size_t inlen)                       //Data bytes
{
	size_t i = 0;
	for (i = 0;i < inlen;++i)
	{
	//Buffer full ?
		if (ctx->c == 64)
		{
			ctx->t[0] += (uint32_t)ctx->c;   //Add counters
			if (ctx->t[0] < ctx->c)          //Carry overflow ?
				ctx->t[1U]++;                //High word
			blake2s_compress(ctx, 0);        //Compress (not last)
			ctx->c = 0;                      //Counter to zero
		}

		ctx->b[ctx->c++] = ((const uint8_t *)in)[i];
	}
}

//Initialize the hashing context "ctx" with optional key "key"
int blake2s_init(
	blake2s_ctx * const ctx, 
	size_t outlen, 
	const void * const key, 
	size_t keylen)                      //(keylen=0: no key)
{
	size_t i = 0;

//Iillegal parameters
	if (outlen == 0 || outlen > 32 || keylen > 32)
		return -1;

//State, "param block"
	for (i = 0;i < 8;++i)
		ctx->h[i] = blake2s_iv[i];
	ctx->h[0] ^= 0x01010000 ^ (keylen << 8U) ^ outlen;

	ctx->t[0] = 0;                      //Input count low word
	ctx->t[1U] = 0;                     //Input count high word
	ctx->c = 0;                         //Pointer within buffer
	ctx->outlen = outlen;

//Zero input block
	for (i = keylen;i < 64;++i)
		ctx->b[i] = 0;
	if (keylen > 0)
	{
		blake2s_update(ctx, key, keylen);
		ctx->c = 64;                    //At the end
	}

	return 0;
}

//Generate the message digest (size given in init)
void blake2s_final(
	blake2s_ctx * const ctx, 
	void * const out)
{
	size_t i = 0;
	ctx->t[0] += (uint32_t)ctx->c;      //Mark last block offset
	if (ctx->t[0] < ctx->c)             //Carry overflow
		ctx->t[1U]++;                   //High word

//Fill up with zeros
	while (ctx->c < 64)
		ctx->b[ctx->c++] = 0;
	blake2s_compress(ctx, 1);           //Final block flag = 1

//Little endian convert and store
	for (i = 0;i < ctx->outlen;++i)
	{
		((uint8_t *)out)[i] =
			(ctx->h[i >> 2U] >> (8 * (i & 3))) & 0xFF;
	}
}

//////////////////////////////////////////////////
// Hash function
//
//Read commands(BLAKE2)
bool ReadCommands_BLAKE2(
#if defined(PLATFORM_WIN)
	std::wstring &Command)
#elif (defined(PLATFORM_LINUX) || defined(PLATFORM_MACX))
	std::string &Command)
#endif
{
//Hash function check
	if (Command == COMMAND_BLAKE2B_160) //BLAKE2B 160 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2B_160;
	}
	else if (Command == HASH_COMMAND_BLAKE2 || Command == COMMAND_BLAKE2B || Command == COMMAND_BLAKE2B_256) //BLAKE2B 256 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2B_256;
	}
	else if (Command == COMMAND_BLAKE2_384 || Command == COMMAND_BLAKE2B_384) //BLAKE2B 384 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2B_384;
	}
	else if (Command == COMMAND_BLAKE2_512 || Command == COMMAND_BLAKE2B_512) //BLAKE2B 512 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2B_512;
	}
	else if (Command == COMMAND_BLAKE2_128 || Command == COMMAND_BLAKE2S_128) //BLAKE2S 128 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2S_128;
	}
	else if (Command == COMMAND_BLAKE2S_160) //BLAKE2S 160 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2S_160;
	}
	else if (Command == COMMAND_BLAKE2_224 || Command == COMMAND_BLAKE2S_224) //BLAKE2S 224 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2S_224;
	}
	else if (Command == COMMAND_BLAKE2S || Command == COMMAND_BLAKE2S_256) //BLAKE2S 256 bits
	{
		BLAKE2_HashFunctionID = HASH_ID_BLAKE2S_256;
	}
	else { //Commands error
		fwprintf_s(stderr, L"[Error] Commands error.\n");
		return false;
	}

	return true;
}

//BLAKE2 hash function
bool BLAKE2_Hash(
	FILE * const FileHandle)
{
//Parameters check
	if (HashFamilyID != HASH_ID_BLAKE2 || FileHandle == nullptr)
	{
		fwprintf_s(stderr, L"[Error] Parameters error.\n");
		return false;
	}

//Initialization
	std::shared_ptr<uint8_t> Buffer(new uint8_t[FILE_BUFFER_SIZE]());
	memset(Buffer.get(), 0, FILE_BUFFER_SIZE);
	size_t ReadLength = 0, DigestSize = 0;

//BLAKE2 initialization
	blake2b_ctx CTX_2B;
	blake2s_ctx CTX_2S;
	memset(&CTX_2B, 0, sizeof(CTX_2B));
	memset(&CTX_2S, 0, sizeof(CTX_2S));
	if ((BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_160 && blake2b_init(&CTX_2B, BLAKE2_DIGEST_SIZE_160 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2B 160 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_256 && blake2b_init(&CTX_2B, BLAKE2_DIGEST_SIZE_256 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2B 256 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_384 && blake2b_init(&CTX_2B, BLAKE2_DIGEST_SIZE_384 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2B 384 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_512 && blake2b_init(&CTX_2B, BLAKE2_DIGEST_SIZE_512 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2B 512 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_128 && blake2s_init(&CTX_2S, BLAKE2_DIGEST_SIZE_128 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2S 128 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_160 && blake2s_init(&CTX_2S, BLAKE2_DIGEST_SIZE_160 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2S 160 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_224 && blake2s_init(&CTX_2S, BLAKE2_DIGEST_SIZE_224 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS) || //BLAKE2S 224 bits
		(BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_256 && blake2s_init(&CTX_2S, BLAKE2_DIGEST_SIZE_256 / BYTES_TO_BITS, nullptr, 0) != EXIT_SUCCESS)) //BLAKE2S 256 bits
	{
		fwprintf_s(stderr, L"[Error] Hash init error.\n");
		return false;
	}
	else {
		if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_128) //BLAKE2S 128 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_128;
		}
		else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_160 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_160) //BLAKE2B and BLAKE2S 160 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_160;
		}
		else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_224) //BLAKE2S 224 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_224;
		}
		else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_256 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_256) //BLAKE2B and BLAKE2S 256 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_256;
		}
		else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_384) //BLAKE2B 384 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_384;
		}
		else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_512) //BLAKE2B 512 bits
		{
			DigestSize = BLAKE2_DIGEST_SIZE_512;
		}
		else { //Commands error
			fwprintf_s(stderr, L"[Error] Commands error.\n");
			return false;
		}
	}

//Hash process
	while (!feof(FileHandle))
	{
		memset(Buffer.get(), 0, FILE_BUFFER_SIZE);
		_set_errno(0);
		ReadLength = fread_s(Buffer.get(), FILE_BUFFER_SIZE, sizeof(uint8_t), FILE_BUFFER_SIZE, FileHandle);
		if (ReadLength == 0 && errno != 0)
		{
			std::wstring Message(L"[Error] Read file error");
			ErrorCodeToMessage(errno, Message);
			if (errno == 0)
				fwprintf_s(stderr, Message.c_str());
			else 
				fwprintf_s(stderr, Message.c_str(), errno);

			return false;
		}
		else {
		//BLAKE2B
			if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_160 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_256 || 
				BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_384 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_512)
			{
				blake2b_update(&CTX_2B, Buffer.get(), ReadLength);
			}
		//BLAKE2S
			else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_128 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_160 || 
				BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_224 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_256)
			{
				blake2s_update(&CTX_2S, Buffer.get(), ReadLength);
			}
			else { //Commands error
				fwprintf_s(stderr, L"[Error] Commands error.\n");
				return false;
			}
		}
	}

//Finish hash process.
	std::shared_ptr<uint8_t> Result(new uint8_t[BLAKE2_DIGEST_SIZE_512 / BYTES_TO_BITS]());
	memset(Result.get(), 0, BLAKE2_DIGEST_SIZE_512 / BYTES_TO_BITS);
	if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_160 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_256 || 
		BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_384 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2B_512) //BLAKE2B
	{
		blake2b_final(&CTX_2B, Result.get());
	}
	else if (BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_128 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_160 || 
		BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_224 || BLAKE2_HashFunctionID == HASH_ID_BLAKE2S_256) //BLAKE2S
	{
		blake2s_final(&CTX_2S, Result.get());
	}
	else { //Commands error
		fwprintf_s(stderr, L"[Error] Commands error.\n");
		return false;
	}

//Binary to hex
	memset(Buffer.get(), 0, FILE_BUFFER_SIZE);
	if (sodium_bin2hex(Buffer.get(), FILE_BUFFER_SIZE, Result.get(), DigestSize / BYTES_TO_BITS) == nullptr)
	{
		fwprintf_s(stderr, L"[Error] Convert binary to hex error.\n");
		return false;
	}
	else {
		PrintToScreen(Buffer.get());
	}

	return true;
}
