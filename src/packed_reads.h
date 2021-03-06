/*
 *  MEGAHIT
 *  Copyright (C) 2014 - 2015 The University of Hong Kong & L3 Bioinformatics Limited
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* contact: Dinghua Li <dhli@cs.hku.hk> */


/**
 * Functions for packed reads or edges (ACGT -> 0123, packed by uint32_t)
 */

#ifndef PACKED_READS_H__
#define PACKED_READS_H__

#include <algorithm>
#include "definitions.h"
#include "sequence/kseq.h"
#include "safe_alloc_open-inl.h"
#include "kmlib/kmbit.h"

// 'spacing' is the strip length for read-word "coalescing"
/**
 * @brief copy src_read[offset...(offset+num_chars_to_copy-1)] to dest
 *
 * @param dest
 * @param src_read
 * @param offset
 * @param num_chars_to_copy
 */
inline void CopySubstring(uint32_t *dest, const uint32_t *src_read, int offset, int num_chars_to_copy,
                          int64_t spacing, int words_per_read, int words_per_substring) {
  // copy words of the suffix to the suffix pool
  int which_word = offset / kCharsPerEdgeWord;
  int word_offset = offset % kCharsPerEdgeWord;
  const uint32_t *src_p = src_read + which_word;

  if (!word_offset) { // special case (word aligned), easy
    int limit = std::min(words_per_read - which_word, words_per_substring);
    for (int i = 0; i < limit; ++i) {
      dest[i] = src_p[i];
    }
  } else {   // not word-aligned
    int bit_shift = word_offset * kBitsPerEdgeChar;

    int limit = std::min(words_per_read - which_word - 1, words_per_substring);

    for (int i = 0; i < limit; ++i) {
      dest[i] = (src_p[i] << bit_shift) | (src_p[i + 1] >> (kBitsPerEdgeWord - bit_shift));
    }
    if (limit != words_per_substring) {
      dest[limit] = src_p[limit] << bit_shift;
    }
  }

  {
    // now mask the extra bits (TODO can be optimized)
    int num_bits_to_copy = num_chars_to_copy * 2;
    int which_word = num_bits_to_copy / kBitsPerEdgeWord;
    uint32_t *p = dest + which_word * spacing;
    int bits_to_clear = kBitsPerEdgeWord - num_bits_to_copy % kBitsPerEdgeWord;

    if (bits_to_clear < kBitsPerEdgeWord) {
      *p >>= bits_to_clear;
      *p <<= bits_to_clear;
    } else if (which_word < words_per_substring) {
      *p = 0;
    }

    which_word++;

    while (which_word < words_per_substring) { // fill zero
      *(p += spacing) = 0;
      which_word++;
    }
  }
}

/**
 * @brief copy the reverse complement of src_read[offset...(offset+num_chars_to_copy-1)] to dest
 *
 * @param dest [description]
 * @param src_read [description]
 * @param offset [description]
 * @param num_chars_to_copy [description]
 */
inline void CopySubstringRC(uint32_t *dest, const uint32_t *src_read, int offset, int num_chars_to_copy,
                            int64_t spacing, int words_per_read, int words_per_substring) {
  int which_word = (offset + num_chars_to_copy - 1) / kCharsPerEdgeWord;
  int word_offset = (offset + num_chars_to_copy - 1) % kCharsPerEdgeWord;
  uint32_t *dest_p = dest;

  if (word_offset == kCharsPerEdgeWord - 1) { // uint32_t aligned
    int limit = std::min(words_per_substring, which_word + 1);
    for (int i = 0; i < limit; ++i) {
      dest[i] = src_read[which_word - i];
    }
    for (int i = 0; i < words_per_substring && i <= which_word; ++i) {
      dest[i] = kmlib::bit::ReverseComplement<2>(dest[i]);
    }
  } else {
    int bit_offset = (kCharsPerEdgeWord - 1 - word_offset) * kBitsPerEdgeChar;
    int i;
    uint32_t w;

    for (i = 0; i < words_per_substring - 1 && i < which_word; ++i) {
      w = (src_read[which_word - i] >> bit_offset) |
          (src_read[which_word - i - 1] << (kBitsPerEdgeWord - bit_offset));
      w = kmlib::bit::ReverseComplement<2>(w);
      *dest_p = w;
      dest_p += spacing;
    }

    // last word
    w = src_read[which_word - i] >> bit_offset;

    if (which_word >= i + 1) {
      w |= (src_read[which_word - i - 1] << (kBitsPerEdgeWord - bit_offset));
    }
    *dest_p = kmlib::bit::ReverseComplement<2>(w);
  }

  {
    // now mask the extra bits (TODO can be optimized)
    int num_bits_to_copy = num_chars_to_copy * 2;
    int which_word = num_bits_to_copy / kBitsPerEdgeWord;
    uint32_t *p = dest + which_word * spacing;
    int bits_to_clear = kBitsPerEdgeWord - num_bits_to_copy % kBitsPerEdgeWord;

    if (bits_to_clear < kBitsPerEdgeWord) {
      *p >>= bits_to_clear;
      *p <<= bits_to_clear;
    } else if (which_word < words_per_substring) {
      *p = 0;
    }

    which_word++;

    while (which_word < words_per_substring) { // fill zero
      *(p += spacing) = 0;
      which_word++;
    }
  }
}

#endif // PACKED_READS_H__
