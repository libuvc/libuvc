#include <string.h>
#include <stdlib.h>

#if __APPLE__
char *strndup(const char *s, size_t n) {
  size_t src_n = 0;
  const char *sp = s;
  char *d;

  while (*sp++)
    src_n++;

  if (src_n < n)
    n = src_n;

  d = malloc(n + 1);

  memcpy(d, s, n);
  
  d[n] = '\0';

  return d;
}
#endif

