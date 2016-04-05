# JavelinPattern v0.1

JavelinPattern is a regular expression engine which aims to be fast *and* feature rich.

Currently only a pre-built OSX library is provided with a basic C API while I try and
clean up the code.

There is also a pre-built OSX binary for [ag (the_silver_searcher)](https://github.com/jthlim/the_silver_searcher) that uses JavelinPattern.

## Features

* JavelinPattern is *fast*
* Supports offline pattern compilation
* Supports sub-match capturing
* Multiple internal processing engines
* Supports unicode case folding
* JavelinPattern really is *fast*...
* JIT for back tracking x64 engine
* Automatic stack guarding (configurable auto-stack growth or fail match)
* Threadsafe -- patterns can be used on multiple threads concurrently
* ... did I mention it is *fast* ?

## Performance

JavelinPattern has a number of internal engines that it uses depending on the pattern
provided. To be feature rich, it has a back-tracking engine that provides look-aheads,
look-behinds, conditional regexes and other features. If you use one of these features,
the back tracking engine will automatically be chosen. In other situations, the pattern
compiler will try and determine the most effective engine to handle the supplied pattern.

Javelin-BT is the performance when the `JP_OPTION_PREFER_BACK_TRACKING` flag is set.

![Pattern Performance Graph](PatternPerformance.png)

Test machine: Macbook Pro Retina Mid 2012, 2.6 GHz Intel Core i7, OSX El Capitan 10.11.4 using code modified from here: [Performance comparison of regular expression engines](http://sljit.sourceforge.net/regex_perf.html)

## API

Check [JavelinPattern.h](JavelinPattern.h) to see the list of available functions (It's very brief)

Example code snippet:

```c
#include "JavelinPattern.h"

{
  jp_pattern_t pattern;
  int result = jp_pattern_compile(&pattern, "test(\\w+)", JP_OPTION_IGNORE_CASE | JP_OPTION_UTF8);
  if(result != 0) ...

  const void* captures[4];
  if(jp_partial_match(pattern, data, strlen(data), captures, 0))
  {
     // captures[0] contains start of match (inclusive)
	 // captures[1] contains end of match (exclusive)
	 // captures[2] contains start of \\w+ subgroup match
	 // captures[3] contains end of \\w+ subgroup match
  }

  jp_pattern_free(pattern);
}

```

## Supported Patterns
 
 Pattern        | BT Only | Meaning
----------------|---------|--------------------------------------------------
(...)           |         | Capture
(?:...)         |         | Cluster
(?...)          |         | Option change ('i', 's', 'm', 'u', 'U')
(?#...)         |         | Comment
(?=...)         |    Y    | Positive look-ahead
(?!...)         |    Y    | Negative look-ahead
(?\<=...)       |    Y    | Positive look-behind
(?\<!...)       |    Y    | Negative look-behind
(?\>...)        |    Y    | Atomic group
(?(*c*)*t*:*f*) |    Y    | Conditional regex
(?R)            |    Y    | Recursive regex
(?1)            |    Y    | Recursive to capture group 1
(?-*n*)         |    Y    | Recursive to relative capture group
(?+*n*)         |    Y    | Recursive to relative capture group
\|              |         | Alternation
.               |         | Any character
^               |         | Start of line
$               |         | End of line
\\a             |         | Alarm (ASCII 0x07)
\\A             |         | Start of input
\\b             |         | Word boundary
\\B             |         | Not word boundary
\\c*X*          |         | Control-*X*
\\C             |         | Any byte
\\d             |         | Digit
\\D             |         | Not digit
\\e             |         | Escape (ASCII 0x1b)
\\f             |         | Form feed (ASCII 0x0c)
\\G             |         | Start of search
\\h             |         | Horizontal whitespace
\\K             |         | Reset capture
\\n             |         | Newline (ASCII 0x0a)
\\r             |         | Carriage Return (ASCII 0x0d)
\\s             |         | Whitespace
\\S             |         | Not whitespace
\\t             |         | Horizontal tab (ASCII 0x09)
\\u*####*       |         | Unicode 4-hex digits
\\u{*###*}      |         | Unicode hex
\\v             |         | Vertical tab (ASCII 0x0b)
\\w             |         | Word character
\\W             |         | Not word character
\\x##           |         | 2 hex digits
\\x{*###*}      |         | Unicode hex
\\z             |         | End of input
\\1             |    Y    | Back reference
\\\\            |         | Backslash
[...]           |         | Character set (ranges supported)
[^...]          |         | Not character set (ranges supported)
[[:*xxx*:]]     |         | POSIX character class *xxx*, eg `lower`, `alpha`



 Quantifiers | BT Only | Meaning
-------------|---------|-----------------------------------------------------
\*           |         | 0 or more maximal
\*?          |         | 0 or more minimal
\*+          |    Y    | 0 or more possessive
\+           |         | 1 or more maximal
\+?          |         | 1 or more minimal
\++          |    Y    | 1 or more possessive
?            |         | 0 or 1 maximal
??           |         | 0 or 1 minimal
?+           |    Y    | 0 or 1 possessive
{###}        |         | Exactly ### times
{M,N}        |         | Between M and N times maximal
{M,N}?       |         | Between M and N times minimal
{M,N}+       |    Y    | Between M and N times possessive
{M,}         |         | At least M times maximal
{M,}?        |         | At least M times minimal
{M,}+        |    Y    | At least M times possessive`
{,N}         |         | At most M times maximal
{,N}?        |         | At most M times minimal
{,N}+        |    Y    | At most M times possessive`
