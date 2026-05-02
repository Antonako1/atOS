#ifndef ATZP_H
#define ATZP_H
#include <STD/TYPEDEF.h>

/// @brief Decompresses data using LZ4 algorithm.
/// @param src Pointer to the source compressed data.
/// @param src_len Length of the source data.
/// @param dst Pointer to the destination buffer for decompressed data.
/// @param dst_capacity Capacity of the destination buffer.
/// @return The number of bytes decompressed, or 0 if an error occurred.
U32 LZ4_DECOMPRESS(PU8 src, U32 src_len, PU8 dst, U32 dst_capacity);

/// @brief Compresses data using LZ4 algorithm.
/// @param src Pointer to the source data to be compressed.
/// @param src_len Length of the source data.
/// @param dst Pointer to the destination buffer for compressed data.
/// @param dst_capacity Capacity of the destination buffer.
/// @return The number of bytes compressed, or 0 if an error occurred.
U32 LZ4_COMPRESS(PU8 src, U32 src_len, PU8 dst, U32 dst_capacity);

/// @brief Returns the maximum compressed size for a given input length.
/// Use this to allocate the dst buffer before calling LZ4_COMPRESS.
#define LZ4_COMPRESS_BOUND(n) ((n) + ((n) / 255) + 16)

#endif // ATZP_H