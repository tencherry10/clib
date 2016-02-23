
#ifndef WILDCARDCMP_H
#define WILDCARDCMP_H 1

/**
 * Check if the given `string` matches `pattern`,
 * using `*` as a wildcard.  Will return `1` if
 * the two match.
 */

int wildcardcmp(const char *pattern, const char *string);

//This function compares text strings, one of which can have wildcards ('*').
int better_wildcardcmp (
  const char  *pTameText,       // A string without wildcards
  const char  *pWildText,       // A (potentially) corresponding string with wildcards
  int         bCaseSensitive,   // By default, match on 'X' vs 'x'
  char        cAltTerminator);  // For function names, for example, you can stop at the first '('
  
#endif
