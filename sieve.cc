#include <cstdio>
#include <cstdint>
#include <vector>
#include "thread-pool.h"

constexpr int64_t N = 1000000;

void Sieve(bool print) {
  std::vector<bool> bits(N >> 1, true);
  for (int64_t i = 3; i * i < N; i += 2) {
    if (bits[i >> 1]) {
      for (int64_t j = i * i; j < N; j += 2 * i) bits[j >> 1] = false;
    }
  }
  if (print) {
    int64_t count = 1;
    for (int64_t i = 3; i < N; i += 2) if (bits[i >> 1]) ++count;
    printf("Count: %ld\n", count);
  }
}

int main() {
  ThreadPool pool(8);
  for (int i = 0; i < 7000; ++i) pool.Add([i]{Sieve(i == 0);});
}
