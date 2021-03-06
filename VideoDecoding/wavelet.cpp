#include "wavelet.h"

#include <vector>

// Returns a normalized index in the given range by ping-ponging
// across boundaries...
//
// e.g. for an array of size 5, ABCDE, we interpret 'idx' as an index
// into the infinite array .....CBABCDEDCBABCDE... such that idx 0 is
// at the start of ABCDE. Hence, f(-1, 5) = B, and f(-6, 5) = C, etc
static int NormalizeIndex(int idx, int range) {
  const int x = idx - (int)(idx >= range) * (idx - range + 2);
  const int mask = (x >> (8 * sizeof(int) - 1));
  return (x ^ mask) + (mask & 1);
}

static void Transpose(int16_t *img, size_t dim, size_t rowbytes) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(img);
  for (size_t y = 0; y < dim; ++y) {
    for (size_t x = y + 1; x < dim; ++x) {
      int16_t *v1 = reinterpret_cast<int16_t *>(bytes + y * rowbytes);
      int16_t *v2 = reinterpret_cast<int16_t *>(bytes + x * rowbytes);
      std::swap(v1[x], v2[y]);
    }
  }
}

namespace MPTC {

size_t ForwardWavelet1D(const int16_t *src, int16_t *dst, size_t len) {
  if (len == 0) {
    return 0;
  }

  if (len == 1) {
    dst[0] = src[0];
    return 0;
  }

  // First set the output buffer to zero:
  memset(dst, 0, len * sizeof(dst[0]));

  // Deinterleave everything
  const size_t mid_pt = len - (len / 2);
  const int range = static_cast<int>(len);

  // Do the odd coefficients first
  for (int i = 1; i < range; i += 2) {
    int next = NormalizeIndex(i + 1, range);
    int prev = NormalizeIndex(i - 1, range);
    dst[mid_pt + i / 2] = src[i] - (src[prev] + src[next]) / 2;
  }

  // Do the even coefficients second
  for (int i = 0; i < range; i += 2) {
    int next = static_cast<int>(mid_pt) + NormalizeIndex(i + 1, range) / 2;
    int prev = static_cast<int>(mid_pt) + NormalizeIndex(i - 1, range) / 2;
    dst[i / 2] = src[i] + (dst[prev] + dst[next] + 2) / 4;
  }

  return mid_pt;
}

void InverseWavelet1D(const int16_t *src, int16_t *dst, size_t len) {
  if (len == 0) {
    return;
  }

  if (len == 1) {
    dst[0] = src[0];
    return;
  }

  // First set the output buffer to zero:
  memset(dst, 0, len * sizeof(dst[0]));

  // Interleave everything
  const size_t mid_pt = len - (len / 2);
  const int range = static_cast<int>(len);

  // Do the even coefficients first
  for (int i = 0; i < range; i += 2) {
    int prev = static_cast<int>(mid_pt) + NormalizeIndex(i - 1, range) / 2;
    int next = static_cast<int>(mid_pt) + NormalizeIndex(i + 1, range) / 2;
    dst[i] = src[i / 2] - (src[prev] + src[next] + 2) / 4;
  }

  // Do the odd coefficients second
  for (int i = 1; i < range; i += 2) {
    int src_idx = static_cast<int>(mid_pt) + i / 2;
    int prev = NormalizeIndex(i - 1, range);
    int next = NormalizeIndex(i + 1, range);
    dst[i] = src[src_idx] + (dst[prev] + dst[next]) / 2;
  }
}

void ForwardWavelet2D(const int16_t *src, size_t src_rowbytes,
                      int16_t *dst, size_t dst_rowbytes, size_t dim) {
  // Allocate a bit of scratch memory
  std::vector<int16_t> scratch(src, src + dim * dim);
  const uint8_t *src_bytes = reinterpret_cast<const uint8_t *>(src);
  uint8_t *dst_bytes = reinterpret_cast<uint8_t *>(dst);

  // Copy src into scratch
  for (size_t row = 0; row < dim; ++row) {
    const int16_t *img = reinterpret_cast<const int16_t *>(src_bytes + row * src_rowbytes);
    memcpy(scratch.data() + row*dim, img, sizeof(scratch[0]) * dim);
  }

  // Do all the columns, store into dst
  Transpose(scratch.data(), dim, sizeof(scratch[0]) * dim);

  for (size_t col = 0; col < dim; ++col) {
    int16_t *img = reinterpret_cast<int16_t *>(dst_bytes + col*dst_rowbytes);
    ForwardWavelet1D(scratch.data() + col*dim, img, dim);
  }

  // Copy dst back into scratch
  for (size_t row = 0; row < dim; ++row) {
    const int16_t *img = reinterpret_cast<const int16_t *>(dst_bytes + row * dst_rowbytes);
    memcpy(scratch.data() + row*dim, img, sizeof(scratch[0]) * dim);
  }

  Transpose(scratch.data(), dim, sizeof(scratch[0]) * dim);

  // Go through and do all the rows
  for (size_t row = 0; row < dim; ++row) {
    int16_t *dst_img = reinterpret_cast<int16_t *>(dst_bytes + row*dst_rowbytes);
    ForwardWavelet1D(scratch.data() + row * dim, dst_img, dim);
  }
}

extern void InverseWavelet2D(const int16_t *src, size_t src_rowbytes,
                             int16_t *dst, size_t dst_rowbytes, size_t dim) {
  // Allocate a bit of scratch memory
  std::vector<int16_t> scratch(dim * dim);
  const uint8_t *src_bytes = reinterpret_cast<const uint8_t *>(src);
  uint8_t *dst_bytes = reinterpret_cast<uint8_t *>(dst);

  // Do all the rows, store into scratch
  for (size_t row = 0; row < dim; ++row) {
    const int16_t *src_img = reinterpret_cast<const int16_t *>(src_bytes + row * src_rowbytes);
    InverseWavelet1D(src_img, scratch.data() + row * dim, dim);
  }

  Transpose(scratch.data(), dim, sizeof(scratch[0]) * dim);

  // Do all the cols, store into dst
  for (size_t col = 0; col < dim; ++col) {
    int16_t *dst_img = reinterpret_cast<int16_t *>(dst_bytes + col * dst_rowbytes);
    InverseWavelet1D(scratch.data() + col * dim, dst_img, dim);
  }

  Transpose(dst, dim, dst_rowbytes);
}

}
