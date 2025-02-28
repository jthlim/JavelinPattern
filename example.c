#include "Javelin/JavelinPattern.h"
#include <stdio.h>
#include <string.h>

int main(int argc, const char **argv) {
  if (argc != 3) {
    printf("Usage: %s <pattern> <data>\n", argv[0]);
    return 1;
  }

  jp_pattern_t pattern;
  int err = jp_pattern_compile(&pattern, argv[1], JP_OPTION_UTF8);
  if (err != 0) {
    printf("Bytecode compile error: %d\n", err);
    return 2;
  }

  const char *data = argv[2];
  const size_t data_length = strlen(data);
  const int capture_count = jp_get_number_of_captures(pattern);
  const void *captures[2 * capture_count];

  printf("Number of captures: %d\n", capture_count);

  printf("  has_full_match: %s\n",
         jp_has_full_match(pattern, data, data_length) ? "true" : "false");
  if (jp_full_match(pattern, data, data_length, captures)) {
    for (size_t i = 0; i < capture_count; ++i) {
      printf("    %zu: offset %zu -> %zu\n", i,
             (const char *)captures[2 * i] - data,
             (const char *)captures[2 * i + 1] - data);
    }
  }
  const bool has_partial_match =
      jp_has_partial_match(pattern, data, data_length, 0);
  printf("  has_partial_match: %s\n", has_partial_match ? "true" : "false");
  if (has_partial_match) {
    printf("  count_partial_match: %zu\n",
           jp_count_partial_matches(pattern, data, data_length, 0));

    size_t offset = 0;
    while (offset <= data_length &&
           jp_partial_match(pattern, data, data_length, captures, offset)) {
      printf("Partial match starting at %zu:\n", offset);
      for (size_t i = 0; i < capture_count; ++i) {
        printf("  %zu: offset %zu -> %zu\n", i,
               (const char *)captures[2 * i] - data,
               (const char *)captures[2 * i + 1] - data);
      }
      const size_t end_offset = (const char *)captures[1] - data;
      offset = (offset == end_offset) ? offset + 1 : end_offset;
    }
  }

  jp_pattern_free(pattern);

  return 0;
}