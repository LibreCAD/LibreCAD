/* fribidi-brackets.c - get bracketed character
 *
 * Copyright (C) 2004  Sharif FarsiWeb, Inc
 * Copyright (C) 2001, 2002, 2004  Behdad Esfahbod
 * Copyright (C) 1999, 2000, 2017  Dov Grobgeld
 *
 * This file is part of GNU FriBidi.
 * 
 * GNU FriBidi is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 * 
 * GNU FriBidi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with GNU FriBidi; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * For licensing issues, contact <fribidi.license@gmail.com> or write to
 * Sharif FarsiWeb, Inc., PO Box 13445-389, Tehran, Iran.
 */
/* 
 * Author(s):
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld, 1999, 2000, 2017
 */

#include "common.h"

#include <fribidi-brackets.h>

#include "brackets.tab.i"
#include "brackets-type.tab.i"
#include <stdio.h>

#define FRIBIDI_TYPE_BRACKET_OPEN 2

FRIBIDI_ENTRY FriBidiBracketType
fribidi_get_bracket (
  /* input */
  FriBidiChar ch
)
{
  FriBidiBracketType bracket_type;
  register uint8_t char_type = FRIBIDI_GET_BRACKET_TYPE (ch);

  /* The bracket type from the table may be:
        0 - Not a bracket
	1 - a bracket
	2 - closing.

     This will be recodeded into the FriBidiBracketType as having a
     bracket_id = 0 if the character is not a bracket.
   */
  fribidi_boolean is_open = false;

  if (char_type == 0)
    bracket_type = FRIBIDI_NO_BRACKET;
  else
  {
    is_open = (char_type & FRIBIDI_TYPE_BRACKET_OPEN) != 0;
    bracket_type = FRIBIDI_GET_BRACKETS (ch) & FRIBIDI_BRACKET_ID_MASK;
  }
  if (is_open)
    bracket_type |= FRIBIDI_BRACKET_OPEN_MASK;

  return bracket_type;
}

FRIBIDI_ENTRY void
fribidi_get_bracket_types (
  /* input */
  const FriBidiChar *str,
  const FriBidiStrIndex len,
  const FriBidiCharType *types,
  /* output */
  FriBidiBracketType *btypes
)
{
  FriBidiStrIndex i;
  for (i=0; i<len; i++)
    {
      /* Optimization that bracket must be of types ON */
      if (*types == FRIBIDI_TYPE_ON)
	*btypes = fribidi_get_bracket (*str);
      else
	*btypes = FRIBIDI_NO_BRACKET;

      btypes++;
      types++;
      str++;
    }
}

/* Editor directions:
 * Local Variables:
 *   mode: c
 *   c-basic-offset: 2
 *   indent-tabs-mode: t
 *   tab-width: 8
 * End:
 * vim: textwidth=78: autoindent: cindent: shiftwidth=2: tabstop=8:
 */
