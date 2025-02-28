//============================================================================

#include "Javelin/Type/String.h"

#include "Javelin/Pattern/Internal/PatternTypes.h"
#include "Javelin/Pattern/Pattern.h"

#include "Javelin/Stream/StandardWriter.h"

#include <unistd.h>

//============================================================================

using namespace Javelin;
using namespace Javelin::PatternInternal;

//============================================================================

const String String::EMPTY_STRING;

//==========================================================================

const CharacterRangeList CharacterRangeList::WORD_CHARACTERS =
    []() -> CharacterRangeList {
  CharacterRangeList result;
  result.Append('0', '9');
  result.Append('A', 'Z');
  result.Append('_');
  result.Append('a', 'z');
  return result;
}();

const CharacterRangeList CharacterRangeList::NOT_WORD_CHARACTERS =
    CharacterRangeList::WORD_CHARACTERS.CreateComplement();

const CharacterRangeList CharacterRangeList::WHITESPACE_CHARACTERS =
    []() -> CharacterRangeList {
  CharacterRangeList result;
  result.Append('\t', '\n');
  result.Append('\f', '\r');
  result.Append(' ');
  return result;
}();

const CharacterRangeList CharacterRangeList::HORIZONTAL_WHITESPACE_CHARACTERS =
    []() -> CharacterRangeList {
  CharacterRangeList result;
  result.Append(0x0009); // Horizontal tab (HT)
  result.Append(0x0020); // Space
  result.Append(0x00A0); // Non-break space
  result.Append(0x1680); // Ogham space mark
  result.Append(0x180E); // Mongolian vowel separator
  // result.Append(0x2000);     // En quad
  // result.Append(0x2001);     // Em quad
  // result.Append(0x2002);     // En space
  // result.Append(0x2003);     // Em space
  // result.Append(0x2004);     // Three-per-em space
  // result.Append(0x2005);     // Four-per-em space
  // result.Append(0x2006);     // Six-per-em space
  // result.Append(0x2007);     // Figure space
  // result.Append(0x2008);     // Punctuation space
  // result.Append(0x2009);     // Thin space
  // result.Append(0x200A);     // Hair space
  result.Append(0x2000, 0x200A);
  result.Append(0x202F); // Narrow no-break space
  result.Append(0x205F); // Medium mathematical space
  result.Append(0x3000); // Ideographic space
  return result;
}();

const CharacterRangeList CharacterRangeList::VERTICAL_WHITESPACE_CHARACTERS =
    []() -> CharacterRangeList {
  CharacterRangeList result;

  // result.Append(0x000A);     // Linefeed (LF)
  // result.Append(0x000B);     // Vertical tab (VT)
  // result.Append(0x000C);     // Form feed (FF)
  // result.Append(0x000D);     // Carriage return (CR)
  result.Append(0x000A, 0x000D);
  result.Append(0x0085); // Next line (NEL)
  // result.Append(0x2028);     // Line separator
  // result.Append(0x2029);     // Paragraph separator
  result.Append(0x2028, 0x2029);

  return result;
}();

//============================================================================

StandardWriter Javelin::StandardOutput(STDOUT_FILENO);
StandardWriter Javelin::StandardError(STDERR_FILENO);

//============================================================================
