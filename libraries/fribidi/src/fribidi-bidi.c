/* FriBidi
 * fribidi-bidi.c - bidirectional algorithm
 *
 * Authors:
 *   Behdad Esfahbod, 2001, 2002, 2004
 *   Dov Grobgeld, 1999, 2000, 2017
 *
 * Copyright (C) 2004 Sharif FarsiWeb, Inc
 * Copyright (C) 2001,2002 Behdad Esfahbod
 * Copyright (C) 1999,2000,2017 Dov Grobgeld
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library, in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA
 *
 * For licensing issues, contact <fribidi.license@gmail.com>.
 */

#include "common.h"

#include <fribidi-bidi.h>
#include <fribidi-mirroring.h>
#include <fribidi-brackets.h>
#include <fribidi-unicode.h>

#include "bidi-types.h"
#include "run.h"

/*
 * This file implements most of Unicode Standard Annex #9, Tracking Number 13.
 */

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

/* Some convenience macros */
#define RL_TYPE(list) ((list)->type)
#define RL_LEN(list) ((list)->len)
#define RL_LEVEL(list) ((list)->level)

/* "Within this scope, bidirectional types EN and AN are treated as R" */
#define RL_TYPE_AN_EN_AS_RTL(list) ( \
 (((list)->type == FRIBIDI_TYPE_AN) || ((list)->type == FRIBIDI_TYPE_EN) | ((list)->type == FRIBIDI_TYPE_RTL)) ? FRIBIDI_TYPE_RTL : (list)->type)
#define RL_BRACKET_TYPE(list) ((list)->bracket_type)
#define RL_ISOLATE_LEVEL(list) ((list)->isolate_level)
#define RL_FSI_BASE_LEVEL(list) ((list)->fsi_base_level)

#define LOCAL_BRACKET_SIZE 16

/* A pairing entry holds a pair of open/close brackets as described in
   BD16. They are collected (in an arbitrary order determined by the
   bracket-matching scan) into a flat, growable array rather than a
   malloc-per-pair linked list, then qsort()ed by the open bracket's
   position for the N0 pass below. Real-world text has few brackets, so
   the array typically stays within its small initial allocation. */
typedef struct {
  FriBidiRun *open;
  FriBidiRun *close;
} FriBidiPairingNode;

typedef struct {
  FriBidiPairingNode *nodes;
  int count;
  int capacity;
} FriBidiPairingNodeArray;

#define FRIBIDI_PAIRING_ARRAY_MIN_CAPACITY 16

static FriBidiRun *
merge_with_prev (
  FriBidiRun *second
)
{
  FriBidiRun *first;

  fribidi_assert (second);
  fribidi_assert (second->next);
  first = second->prev;
  fribidi_assert (first);

  first->next = second->next;
  first->next->prev = first;
  RL_LEN (first) += RL_LEN (second);
  if (second->next_isolate)
    second->next_isolate->prev_isolate = second->prev_isolate;
  /* The following edge case typically shouldn't happen, but fuzz
     testing shows it does, and the assignment protects against
     a dangling pointer. */
  else if (second->next->prev_isolate == second)
    second->next->prev_isolate = second->prev_isolate;  
  if (second->prev_isolate)
    second->prev_isolate->next_isolate = second->next_isolate;
  first->next_isolate = second->next_isolate;

  /* 'second' is pool-owned; it just becomes unreachable garbage within
     the pool instead of being freed individually (see run.h). */
  return first;
}
static void
compact_list (
  FriBidiRun *list
)
{
  fribidi_assert (list);

  if (list->next)
    for_run_list (list, list)
      if (RL_TYPE (list->prev) == RL_TYPE (list)
	  && RL_LEVEL (list->prev) == RL_LEVEL (list)
          && RL_ISOLATE_LEVEL (list->prev) == RL_ISOLATE_LEVEL (list)
          && RL_BRACKET_TYPE(list) == FRIBIDI_NO_BRACKET /* Don't join brackets! */
          && RL_BRACKET_TYPE(list->prev) == FRIBIDI_NO_BRACKET
          )
      list = merge_with_prev (list);
}

static void
compact_neutrals (
  FriBidiRun *list
)
{
  fribidi_assert (list);

  if (list->next)
    {
      for_run_list (list, list)
      {
	if (RL_LEVEL (list->prev) == RL_LEVEL (list)
            && RL_ISOLATE_LEVEL (list->prev) == RL_ISOLATE_LEVEL (list)
	    &&
	    ((RL_TYPE (list->prev) == RL_TYPE (list)
	      || (FRIBIDI_IS_NEUTRAL (RL_TYPE (list->prev))
		  && FRIBIDI_IS_NEUTRAL (RL_TYPE (list)))))
            && RL_BRACKET_TYPE(list) == FRIBIDI_NO_BRACKET /* Don't join brackets! */
            && RL_BRACKET_TYPE(list->prev) == FRIBIDI_NO_BRACKET
            )
	  list = merge_with_prev (list);
      }
    }
}

/* Search for an adjacent run in the forward or backward direction.
   It uses the next_isolate and prev_isolate run for short circuited searching.
 */

/* The static sentinel is used to signal the end of an isolating
   sequence */
static FriBidiRun sentinel = { NULL, NULL, 0,0, FRIBIDI_TYPE_SENTINEL, -1,-1,FRIBIDI_NO_BRACKET, NULL, NULL, 0 };

static FriBidiRun *get_adjacent_run(FriBidiRun *list, fribidi_boolean forward, fribidi_boolean skip_neutral)
{
  FriBidiRun *ppp = forward ? list->next_isolate : list->prev_isolate;
  if (!ppp)
    return &sentinel;

  while (ppp)
    {
      FriBidiCharType ppp_type = RL_TYPE (ppp);

      if (ppp_type == FRIBIDI_TYPE_SENTINEL)
        break;

      /* Note that when sweeping forward we continue one run
         beyond the PDI to see what lies behind. When looking
         backwards, this is not necessary as the leading isolate
         run has already been assigned the resolved level. */
      if (ppp->isolate_level > list->isolate_level   /* <- How can this be true? */
          || (forward && ppp_type == FRIBIDI_TYPE_PDI)
          || (skip_neutral && !FRIBIDI_IS_STRONG(ppp_type)))
        {
          ppp = forward ? ppp->next_isolate : ppp->prev_isolate;
          if (!ppp)
            ppp = &sentinel;

          continue;
        }
      break;
    }

  return ppp;
}

#ifdef DEBUG
/*======================================================================
 *  For debugging, define some functions for printing the types and the
 *  levels.
 *----------------------------------------------------------------------*/

static char char_from_level_array[] = {
  '$',				/* -1 == FRIBIDI_SENTINEL, indicating
				 * start or end of string. */
  /* 0-61 == 0-9,a-z,A-Z are the the only valid levels before resolving
   * implicits.  after that the level @ may be appear too. */
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
  'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D',
  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z',

  /* TBD - insert another 125-64 levels */

  '@',				/* 62 == only must appear after resolving
				 * implicits. */

  '!',				/* 63 == FRIBIDI_LEVEL_INVALID, internal error,
				 * this level shouldn't be seen.  */

  '*', '*', '*', '*', '*'	/* >= 64 == overflows, this levels and higher
				 * levels show a real bug!. */
};

#define fribidi_char_from_level(level) char_from_level_array[(level) + 1]

static void
print_types_re (
  const FriBidiRun *pp
)
{
  fribidi_assert (pp);

  MSG ("  Run types  : ");
  for_run_list (pp, pp)
  {
    MSG6 ("%d:%d(%s)[%d,%d] ",
	  pp->pos, pp->len, fribidi_get_bidi_type_name (pp->type), pp->level, pp->isolate_level);
  }
  MSG ("\n");
}

static void
print_resolved_levels (
  const FriBidiRun *pp
)
{
  fribidi_assert (pp);

  MSG ("  Res. levels: ");
  for_run_list (pp, pp)
  {
    register FriBidiStrIndex i;
    for (i = RL_LEN (pp); i; i--)
      MSG2 ("%c", fribidi_char_from_level (RL_LEVEL (pp)));
  }
  MSG ("\n");
}

static void
print_resolved_types (
  const FriBidiRun *pp
)
{
  fribidi_assert (pp);

  MSG ("  Res. types : ");
  for_run_list (pp, pp)
  {
    FriBidiStrIndex i;
    for (i = RL_LEN (pp); i; i--)
      MSG2 ("%s ", fribidi_get_bidi_type_name (pp->type));
  }
  MSG ("\n");
}

static void
print_bidi_string (
  /* input */
  const FriBidiCharType *bidi_types,
  const FriBidiStrIndex len
)
{
  register FriBidiStrIndex i;

  fribidi_assert (bidi_types);

  MSG ("  Org. types : ");
  for (i = 0; i < len; i++)
    MSG2 ("%s ", fribidi_get_bidi_type_name (bidi_types[i]));
  MSG ("\n");
}

static void print_pairing_nodes(FriBidiPairingNodeArray *nodes)
{
  int i;
  MSG ("Pairs: ");
  for (i = 0; i < nodes->count; i++)
    MSG3 ("(%d, %d) ", nodes->nodes[i].open->pos, nodes->nodes[i].close->pos);
  MSG ("\n");
}
#endif /* DEBUG */


/*=========================================================================
 * define macros for push and pop the status in to / out of the stack
 *-------------------------------------------------------------------------*/

/* There are a few little points in pushing into and popping from the status
   stack:
   1. when the embedding level is not valid (more than
   FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL=125), you must reject it, and not to push
   into the stack, but when you see a PDF, you must find the matching code,
   and if it was pushed in the stack, pop it, it means you must pop if and
   only if you have pushed the matching code, the over_pushed var counts the
   number of rejected codes so far.

   2. there's a more confusing point too, when the embedding level is exactly
   FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL-1=124, an LRO, LRE, or LRI is rejected
   because the new level would be FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL+1=126, that
   is invalid; but an RLO, RLE, or RLI is accepted because the new level is
   FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL=125, that is valid, so the rejected codes
   may be not continuous in the logical order, in fact there are at most two
   continuous intervals of codes, with an RLO, RLE, or RLI between them.  To
   support this case, the first_interval var counts the number of rejected
   codes in the first interval, when it is 0, means that there is only one
   interval.

*/

/* a. If this new level would be valid, then this embedding code is valid.
   Remember (push) the current embedding level and override status.
   Reset current level to this new level, and reset the override status to
   new_override.
   b. If the new level would not be valid, then this code is invalid. Don't
   change the current level or override status.
*/
#define PUSH_STATUS \
    FRIBIDI_BEGIN_STMT \
      if LIKELY(over_pushed == 0 \
                && isolate_overflow == 0 \
                && new_level <= FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL)   \
        { \
          if UNLIKELY(level == FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL - 1) \
            first_interval = over_pushed; \
          status_stack[stack_size].level = level; \
          status_stack[stack_size].isolate_level = isolate_level; \
          status_stack[stack_size].isolate = isolate; \
          status_stack[stack_size].override = override; \
          stack_size++; \
          level = new_level; \
          override = new_override; \
        } else if LIKELY(isolate_overflow == 0) \
	  over_pushed++; \
    FRIBIDI_END_STMT

/* If there was a valid matching code, restore (pop) the last remembered
   (pushed) embedding level and directional override.
*/
#define POP_STATUS \
    FRIBIDI_BEGIN_STMT \
      if (stack_size) \
      { \
        if UNLIKELY(over_pushed > first_interval) \
          over_pushed--; \
        else \
          { \
            if LIKELY(over_pushed == first_interval) \
              first_interval = 0; \
            stack_size--; \
            level = status_stack[stack_size].level; \
            override = status_stack[stack_size].override; \
            isolate = status_stack[stack_size].isolate; \
            isolate_level = status_stack[stack_size].isolate_level; \
          } \
      } \
    FRIBIDI_END_STMT


/* Return the type of previous run or the SOR, if already at the start of
   a level run. */
#define PREV_TYPE_OR_SOR(pp) \
    ( \
      RL_LEVEL(pp->prev) == RL_LEVEL(pp) ? \
        RL_TYPE(pp->prev) : \
        FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(pp->prev), RL_LEVEL(pp))) \
    )

/* Return the type of next run or the EOR, if already at the end of
   a level run. */
#define NEXT_TYPE_OR_EOR(pp) \
    ( \
      RL_LEVEL(pp->next) == RL_LEVEL(pp) ? \
        RL_TYPE(pp->next) : \
        FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(pp->next), RL_LEVEL(pp))) \
    )


/* Return the embedding direction of a link. */
#define FRIBIDI_EMBEDDING_DIRECTION(link) \
    FRIBIDI_LEVEL_TO_DIR(RL_LEVEL(link))


FRIBIDI_ENTRY FriBidiParType
fribidi_get_par_direction (
  /* input */
  const FriBidiCharType *bidi_types,
  const FriBidiStrIndex len
)
{
  int valid_isolate_count = 0;
  register FriBidiStrIndex i;

  fribidi_assert (bidi_types);

  for (i = 0; i < len; i++)
    {
      if (bidi_types[i] == FRIBIDI_TYPE_PDI)
        {
          /* Ignore if there is no matching isolate */
          if (valid_isolate_count>0)
            valid_isolate_count--;
        }
      else if (FRIBIDI_IS_ISOLATE(bidi_types[i]))
        valid_isolate_count++;
      else if (valid_isolate_count==0 && FRIBIDI_IS_LETTER (bidi_types[i]))
        return FRIBIDI_IS_RTL (bidi_types[i]) ? FRIBIDI_PAR_RTL :
          FRIBIDI_PAR_LTR;
    }

  return FRIBIDI_PAR_ON;
}

static void pairing_node_array_init(FriBidiPairingNodeArray *arr)
{
  arr->nodes = NULL;
  arr->count = 0;
  arr->capacity = 0;
}

/* Append a new (open, close) pair, growing the backing array (by
   doubling, geometric growth) if needed. */
static fribidi_boolean pairing_node_array_push(FriBidiPairingNodeArray *arr,
                                               FriBidiRun *open,
                                               FriBidiRun *close)
{
  if UNLIKELY
    (arr->count == arr->capacity)
    {
      int new_capacity = arr->capacity ? arr->capacity * 2 :
        FRIBIDI_PAIRING_ARRAY_MIN_CAPACITY;
      FriBidiPairingNode *new_nodes =
        fribidi_malloc (new_capacity * sizeof (FriBidiPairingNode));
      if UNLIKELY
        (!new_nodes) return false;
      if (arr->nodes)
        {
          memcpy (new_nodes, arr->nodes, arr->count * sizeof (FriBidiPairingNode));
          fribidi_free (arr->nodes);
        }
      arr->nodes = new_nodes;
      arr->capacity = new_capacity;
    }

  arr->nodes[arr->count].open = open;
  arr->nodes[arr->count].close = close;
  arr->count++;
  return true;
}

static int pairing_node_compare (const void *a, const void *b)
{
  const FriBidiPairingNode *na = a, *nb = b;
  return (na->open->pos > nb->open->pos) - (na->open->pos < nb->open->pos);
}

static void sort_pairing_nodes(FriBidiPairingNodeArray *arr)
{
  if (arr->count > 1)
    qsort (arr->nodes, arr->count, sizeof (FriBidiPairingNode), pairing_node_compare);
}

static void free_pairing_nodes(FriBidiPairingNodeArray *arr)
{
  fribidi_free (arr->nodes);
  arr->nodes = NULL;
  arr->count = arr->capacity = 0;
}

FRIBIDI_ENTRY FriBidiLevel
fribidi_get_par_embedding_levels_ex (
  /* input */
  const FriBidiCharType *bidi_types,
  const FriBidiBracketType *bracket_types,
  const FriBidiStrIndex len,
  /* input and output */
  FriBidiParType *pbase_dir,
  /* output */
  FriBidiLevel *embedding_levels
)
{
  FriBidiLevel base_level, max_level = 0;
  FriBidiParType base_dir;
  FriBidiRun *main_run_list = NULL, *explicits_list = NULL, *pp;
  fribidi_boolean has_isolate = false;
  FriBidiRunPool *run_pool = NULL;
  fribidi_boolean status = false;
  int max_iso_level = 0;

  if UNLIKELY
    (!len)
    {
      status = true;
      goto out;
    }

  DBG ("in fribidi_get_par_embedding_levels");

  fribidi_assert (bidi_types);
  fribidi_assert (pbase_dir);
  fribidi_assert (embedding_levels);

  /* All FriBidiRun nodes needed to resolve this paragraph's embedding
     levels are carved out of this single pool and released together at
     'out', rather than being malloc'd/freed one at a time. */
  run_pool = fribidi_run_pool_new (len);
  if UNLIKELY
    (!run_pool) goto out;

  /* Determinate character types */
  {
    /* Get run-length encoded character types */
    main_run_list = run_list_encode_bidi_types (bidi_types, bracket_types, len, run_pool, &has_isolate);
    if UNLIKELY
      (!main_run_list) goto out;
  }

  /* Find base level */
  /* If no strong base_dir was found, resort to the weak direction
     that was passed on input. */
  base_level = FRIBIDI_DIR_TO_LEVEL (*pbase_dir);
  if (!FRIBIDI_IS_STRONG (*pbase_dir))
    /* P2. P3. Search for first strong character and use its direction as
       base direction */
    {
      int valid_isolate_count = 0;
      for_run_list (pp, main_run_list)
        {
          if (RL_TYPE(pp) == FRIBIDI_TYPE_PDI)
            {
              /* Ignore if there is no matching isolate */
              if (valid_isolate_count>0)
                valid_isolate_count--;
            }
          else if (FRIBIDI_IS_ISOLATE(RL_TYPE(pp)))
            valid_isolate_count++;
          else if (valid_isolate_count==0 && FRIBIDI_IS_LETTER (RL_TYPE (pp)))
            {
              base_level = FRIBIDI_DIR_TO_LEVEL (RL_TYPE (pp));
              *pbase_dir = FRIBIDI_LEVEL_TO_DIR (base_level);
              break;
            }
        }
    }
  base_dir = FRIBIDI_LEVEL_TO_DIR (base_level);
  DBG2 ("  base level : %c", fribidi_char_from_level (base_level));
  DBG2 ("  base dir   : %s", fribidi_get_bidi_type_name (base_dir));

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_types_re (main_run_list);
    }
# endif	/* DEBUG */

  /* Explicit Levels and Directions */
  DBG ("explicit levels and directions");
  {
    FriBidiLevel level, new_level = 0;
    int isolate_level = 0;
    FriBidiCharType override, new_override;
    FriBidiStrIndex i;
    int stack_size, over_pushed, first_interval;
    int valid_isolate_count = 0;
    int isolate_overflow = 0;
    int isolate = 0; /* The isolate status flag */
    struct
    {
      FriBidiCharType override;	/* only LTR, RTL and ON are valid */
      FriBidiLevel level;
      int isolate;
      int isolate_level;
    } status_stack[FRIBIDI_BIDI_MAX_RESOLVED_LEVELS];
    FriBidiRun temp_link;
    FriBidiRun *run_per_isolate_level[FRIBIDI_BIDI_MAX_RESOLVED_LEVELS];
    int prev_isolate_level = 0; /* When running over the isolate levels, remember the previous level */

    memset(run_per_isolate_level, 0, sizeof(run_per_isolate_level[0])
           * FRIBIDI_BIDI_MAX_RESOLVED_LEVELS);

/* explicits_list is a list like main_run_list, that holds the explicit
   codes that are removed from main_run_list, to reinsert them later by
   calling the shadow_run_list.
*/
    explicits_list = new_run_list (run_pool);
    if UNLIKELY
      (!explicits_list) goto out;

    /* X5c preprocessing: resolve the effective direction of every FSI
       up front, in a single linear pass over the (still untouched)
       run list, using an explicit stack to skip over nested isolates.
       This replaces rescanning the tail of the run list from every FSI
       individually, which made paragraphs with many FSIs -- or with no
       strong character before an FSI's matching PDI -- quadratic. Only
       the direct content of each isolate initiator counts towards its
       own resolution; content of a nested isolate is skipped entirely,
       which a stack captures naturally: a strong character only ever
       resolves the isolate currently on top of the stack. Skipped
       entirely when the text has no isolate-initiator at all, which is
       the common case and would otherwise cost a full, pointless pass
       over the run list. */
    if (has_isolate)
    {
      FriBidiRun *local_fsi_stack[LOCAL_BRACKET_SIZE];
      FriBidiRun **fsi_stack = local_fsi_stack;
      int fsi_stack_capacity = LOCAL_BRACKET_SIZE;
      int fsi_stack_size = 0;
      fribidi_boolean fsi_stack_heap = false;
      FriBidiRun *fsi_pp;

      for_run_list (fsi_pp, main_run_list)
        {
          FriBidiCharType fsi_this_type = RL_TYPE (fsi_pp);

          if (fsi_this_type == FRIBIDI_TYPE_PDI)
            {
              if (fsi_stack_size)
                fsi_stack_size--;
            }
          else if (FRIBIDI_IS_ISOLATE (fsi_this_type))
            {
              if UNLIKELY
                (fsi_stack_size == fsi_stack_capacity)
                {
                  int new_capacity = fsi_stack_capacity * 2;
                  FriBidiRun **new_stack =
                    fribidi_malloc (new_capacity * sizeof (*new_stack));
                  if UNLIKELY
                    (!new_stack) break;
                  memcpy (new_stack, fsi_stack,
                          fsi_stack_size * sizeof (*new_stack));
                  if (fsi_stack_heap)
                    fribidi_free (fsi_stack);
                  fsi_stack = new_stack;
                  fsi_stack_capacity = new_capacity;
                  fsi_stack_heap = true;
                }

              if (fsi_this_type == FRIBIDI_TYPE_FSI)
                {
                  RL_FSI_BASE_LEVEL (fsi_pp) = 0;
                  fsi_stack[fsi_stack_size++] = fsi_pp;
                }
              else
                fsi_stack[fsi_stack_size++] = NULL;
            }
          else if (fsi_stack_size && FRIBIDI_IS_LETTER (fsi_this_type))
            {
              FriBidiRun *pending = fsi_stack[fsi_stack_size - 1];
              if (pending)
                {
                  RL_FSI_BASE_LEVEL (pending) =
                    FRIBIDI_DIR_TO_LEVEL (fsi_this_type);
                  /* Found; stop this isolate from being resolved again
                     by a later, incorrect, character. */
                  fsi_stack[fsi_stack_size - 1] = NULL;
                }
            }
        }

      if (fsi_stack_heap)
        fribidi_free (fsi_stack);
    }

    /* X1. Begin by setting the current embedding level to the paragraph
       embedding level. Set the directional override status to neutral,
       and directional isolate status to false.

       Process each character iteratively, applying rules X2 through X8.
       Only embedding levels from 0 to 123 are valid in this phase. */

    level = base_level;
    override = FRIBIDI_TYPE_ON;
    /* stack */
    stack_size = 0;
    over_pushed = 0;
    first_interval = 0;
    valid_isolate_count = 0;
    isolate_overflow = 0;

    for_run_list (pp, main_run_list)
    {
      FriBidiCharType this_type = RL_TYPE (pp);
      RL_ISOLATE_LEVEL (pp) = isolate_level;

      if (FRIBIDI_IS_EXPLICIT_OR_BN (this_type))
	{
	  if (FRIBIDI_IS_STRONG (this_type))
	    {			/* LRE, RLE, LRO, RLO */
	      /* 1. Explicit Embeddings */
	      /*   X2. With each RLE, compute the least greater odd
	         embedding level. */
	      /*   X3. With each LRE, compute the least greater even
	         embedding level. */
	      /* 2. Explicit Overrides */
	      /*   X4. With each RLO, compute the least greater odd
	         embedding level. */
	      /*   X5. With each LRO, compute the least greater even
	         embedding level. */
	      new_override = FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR (this_type);
	      for (i = RL_LEN (pp); i; i--)
		{
		  new_level =
		    ((level + FRIBIDI_DIR_TO_LEVEL (this_type) + 2) & ~1) -
		    FRIBIDI_DIR_TO_LEVEL (this_type);
                  isolate = 0;
		  PUSH_STATUS;
		}
	    }
	  else if (this_type == FRIBIDI_TYPE_PDF)
	    {
	      /* 3. Terminating Embeddings and overrides */
	      /*   X7. With each PDF, determine the matching embedding or
	         override code. */
              for (i = RL_LEN (pp); i; i--)
                {
                  if (stack_size && status_stack[stack_size-1].isolate != 0)
                    break;
                  POP_STATUS;
                }
	    }

	  /* X9. Remove all RLE, LRE, RLO, LRO, PDF, and BN codes. */
	  /* Remove element and add it to explicits_list */
	  RL_LEVEL (pp) = FRIBIDI_SENTINEL;
	  temp_link.next = pp->next;
	  move_node_before (pp, explicits_list);
	  pp = &temp_link;
	}
      else if (this_type == FRIBIDI_TYPE_PDI)
        /* X6a. pop the direction of the stack */
        {
          for (i = RL_LEN (pp); i; i--)
            {
              if (isolate_overflow > 0)
                {
                  isolate_overflow--;
                  RL_LEVEL (pp) = level;
                }

              else if (valid_isolate_count > 0)
                {
                  /* Pop away all LRE,RLE,LRO, RLO levels
                     from the stack, as these are implicitly
                     terminated by the PDI */
                  while (stack_size && !status_stack[stack_size-1].isolate)
                    POP_STATUS;
                  over_pushed = 0; /* The PDI resets the overpushed! */
                  POP_STATUS;
                  if (isolate_level>0)
                    isolate_level--;
                  valid_isolate_count--;
                  RL_LEVEL (pp) = level;
                  RL_ISOLATE_LEVEL (pp) = isolate_level;
                }
              else
                {
                  /* Ignore isolated PDI's by turning them into ON's */
                  RL_TYPE (pp) = FRIBIDI_TYPE_ON;
                  RL_LEVEL (pp) = level;
                }
            }
        }
      else if (FRIBIDI_IS_ISOLATE(this_type))
        {
          /* TBD support RL_LEN > 1 */
          new_override = FRIBIDI_TYPE_ON;
          isolate = 1;
          if (this_type == FRIBIDI_TYPE_LRI)
            new_level = level + 2 - (level%2);
          else if (this_type == FRIBIDI_TYPE_RLI)
            new_level = level + 1 + (level%2);
          else if (this_type == FRIBIDI_TYPE_FSI)
            {
              /* The effective direction was already resolved by the
                 single-pass X5c preprocessing above; just look it up. */
              FriBidiLevel fsi_base_level = RL_FSI_BASE_LEVEL (pp);

              /* Same behavior like RLI and LRI above */
              if (FRIBIDI_LEVEL_IS_RTL (fsi_base_level))
                new_level = level + 1 + (level%2);
              else
                new_level = level + 2 - (level%2);
            }

	  RL_LEVEL (pp) = level;
          RL_ISOLATE_LEVEL (pp) = isolate_level;
          if (isolate_level < FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL-1)
              isolate_level++;

	  if (!FRIBIDI_IS_NEUTRAL (override))
	    RL_TYPE (pp) = override;

          if (new_level <= FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL)
            {
              valid_isolate_count++;
              PUSH_STATUS;
              level = new_level;
            }
          else
            isolate_overflow += 1;
        }
      else if (this_type == FRIBIDI_TYPE_BS)
	{
	  /* X8. All explicit directional embeddings and overrides are
	     completely terminated at the end of each paragraph. Paragraph
	     separators are not included in the embedding. */
	  break;
	}
      else
	{
	  /* X6. For all types besides RLE, LRE, RLO, LRO, and PDF:
	     a. Set the level of the current character to the current
	     embedding level.
	     b. Whenever the directional override status is not neutral,
	     reset the current character type to the directional override
	     status. */
	  RL_LEVEL (pp) = level;
	  if (!FRIBIDI_IS_NEUTRAL (override))
	    RL_TYPE (pp) = override;
	}
    }

    /* Build the isolate_level connections */
    prev_isolate_level = 0;
    for_run_list (pp, main_run_list)
    {
      int isolate_level = RL_ISOLATE_LEVEL (pp);
      int i;

      /* When going from an upper to a lower level, zero out all higher levels
         in order not erroneous connections! */
      if (isolate_level<prev_isolate_level)
        for (i=isolate_level+1; i<=prev_isolate_level; i++)
          run_per_isolate_level[i]=0;
      prev_isolate_level = isolate_level;
      
      if (run_per_isolate_level[isolate_level])
        {
          run_per_isolate_level[isolate_level]->next_isolate = pp;
          pp->prev_isolate = run_per_isolate_level[isolate_level];
        }
      run_per_isolate_level[isolate_level] = pp;
    }

    /* Implementing X8. It has no effect on a single paragraph! */
    level = base_level;
    override = FRIBIDI_TYPE_ON;
    stack_size = 0;
    over_pushed = 0;
  }
  /* X10. The remaining rules are applied to each run of characters at the
     same level. For each run, determine the start-of-level-run (sor) and
     end-of-level-run (eor) type, either L or R. This depends on the
     higher of the two levels on either side of the boundary (at the start
     or end of the paragraph, the level of the 'other' run is the base
     embedding level). If the higher level is odd, the type is R, otherwise
     it is L. */
  /* Resolving Implicit Levels can be done out of X10 loop, so only change
     of Resolving Weak Types and Resolving Neutral Types is needed. */

  compact_list (main_run_list);

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_types_re (main_run_list);
      print_bidi_string (bidi_types, len);
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

  /* 4. Resolving weak types. Also calculate the maximum isolate level */
  max_iso_level = 0;
  DBG ("4a. resolving weak types");
  {
    int last_strong_stack[FRIBIDI_BIDI_MAX_RESOLVED_LEVELS];
    FriBidiCharType prev_type_orig;
    fribidi_boolean w4;

    last_strong_stack[0] = base_dir;

    for_run_list (pp, main_run_list)
    {
      register FriBidiCharType prev_type, this_type, next_type;
      FriBidiRun *ppp_prev, *ppp_next;
      int iso_level;

      ppp_prev = get_adjacent_run(pp, false, false);
      ppp_next = get_adjacent_run(pp, true, false);

      this_type = RL_TYPE (pp);
      iso_level = RL_ISOLATE_LEVEL(pp);

      if (iso_level > max_iso_level)
        max_iso_level = iso_level;

      if (RL_LEVEL(ppp_prev) == RL_LEVEL(pp))
        prev_type = RL_TYPE(ppp_prev);
      else
        prev_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_prev), RL_LEVEL(pp)));

      if (RL_LEVEL(ppp_next) == RL_LEVEL(pp))
        next_type = RL_TYPE(ppp_next);
      else
        next_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_next), RL_LEVEL(pp)));

      if (FRIBIDI_IS_STRONG (prev_type))
	last_strong_stack[iso_level] = prev_type;

      /* W1. NSM
         Examine each non-spacing mark (NSM) in the level run, and change the
         type of the NSM to the type of the previous character. If the NSM
         is at the start of the level run, it will get the type of sor. */
      /* Implementation note: it is important that if the previous character
         is not sor, then we should merge this run with the previous,
         because of rules like W5, that we assume all of a sequence of
         adjacent ETs are in one FriBidiRun. */
      if (this_type == FRIBIDI_TYPE_NSM)
	{
          /* New rule in Unicode 6.3 */
          if (FRIBIDI_IS_ISOLATE (RL_TYPE (pp->prev)))
              RL_TYPE(pp) = FRIBIDI_TYPE_ON;

	  if (RL_LEVEL (ppp_prev) == RL_LEVEL (pp))
            {
              if (ppp_prev == pp->prev)
                pp = merge_with_prev (pp);
            }
	  else
	    RL_TYPE (pp) = prev_type;

	  if (prev_type == next_type && RL_LEVEL (pp) == RL_LEVEL (pp->next))
	    {
              if (ppp_next == pp->next)
                pp = merge_with_prev (pp->next);
	    }
	  continue;		/* As we know the next condition cannot be true. */
	}

      /* W2: European numbers. */
      if (this_type == FRIBIDI_TYPE_EN && last_strong_stack[iso_level] == FRIBIDI_TYPE_AL)
	{
	  RL_TYPE (pp) = FRIBIDI_TYPE_AN;

	  /* Resolving dependency of loops for rules W1 and W2, so we
	     can merge them in one loop. */
	  if (next_type == FRIBIDI_TYPE_NSM)
	    RL_TYPE (ppp_next) = FRIBIDI_TYPE_AN;
	}
    }

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

    /* The last iso level is used to invalidate the the last strong values when going from
       a higher to a lower iso level. When this occur, all "last_strong" values are
       set to the base_dir. */
    last_strong_stack[0] = base_dir;

    DBG ("4b. resolving weak types. W4 and W5");

    /* Resolving dependency of loops for rules W4 and W5, W5 may
       want to prevent W4 to take effect in the next turn, do this
       through "w4". */
    w4 = true;
    /* Resolving dependency of loops for rules W4 and W5 with W7,
       W7 may change an EN to L but it sets the prev_type_orig if needed,
       so W4 and W5 in next turn can still do their works. */
    prev_type_orig = FRIBIDI_TYPE_ON;

    /* Each isolate level has its own memory of the last strong character */
    for_run_list (pp, main_run_list)
    {
      register FriBidiCharType prev_type, this_type, next_type;
      int iso_level;
      FriBidiRun *ppp_prev, *ppp_next;

      this_type = RL_TYPE (pp);
      iso_level = RL_ISOLATE_LEVEL(pp);

      ppp_prev = get_adjacent_run(pp, false, false);
      ppp_next = get_adjacent_run(pp, true, false);

      if (RL_LEVEL(ppp_prev) == RL_LEVEL(pp))
        prev_type = RL_TYPE(ppp_prev);
      else
        prev_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_prev), RL_LEVEL(pp)));

      if (RL_LEVEL(ppp_next) == RL_LEVEL(pp))
        next_type = RL_TYPE(ppp_next);
      else
        next_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_next), RL_LEVEL(pp)));

      if (FRIBIDI_IS_STRONG (prev_type))
	last_strong_stack[iso_level] = prev_type;

      /* W2 ??? */

      /* W3: Change ALs to R. */
      if (this_type == FRIBIDI_TYPE_AL)
	{
	  RL_TYPE (pp) = FRIBIDI_TYPE_RTL;
	  w4 = true;
	  prev_type_orig = FRIBIDI_TYPE_ON;
	  continue;
	}

      /* W4. A single european separator changes to a european number.
         A single common separator between two numbers of the same type
         changes to that type. */
      if (w4
	  && RL_LEN (pp) == 1 && FRIBIDI_IS_ES_OR_CS (this_type)
	  && FRIBIDI_IS_NUMBER (prev_type_orig)
	  && prev_type_orig == next_type
	  && (prev_type_orig == FRIBIDI_TYPE_EN
	      || this_type == FRIBIDI_TYPE_CS))
	{
	  RL_TYPE (pp) = prev_type;
	  this_type = RL_TYPE (pp);
	}
      w4 = true;

      /* W5. A sequence of European terminators adjacent to European
         numbers changes to All European numbers. */
      if (this_type == FRIBIDI_TYPE_ET
	  && (prev_type_orig == FRIBIDI_TYPE_EN
	      || next_type == FRIBIDI_TYPE_EN))
	{
	  RL_TYPE (pp) = FRIBIDI_TYPE_EN;
	  w4 = false;
	  this_type = RL_TYPE (pp);
	}

      /* W6. Otherwise change separators and terminators to other neutral. */
      if (FRIBIDI_IS_NUMBER_SEPARATOR_OR_TERMINATOR (this_type))
	RL_TYPE (pp) = FRIBIDI_TYPE_ON;

      /* W7. Change european numbers to L. */
      if (this_type == FRIBIDI_TYPE_EN && last_strong_stack[iso_level] == FRIBIDI_TYPE_LTR)
	{
	  RL_TYPE (pp) = FRIBIDI_TYPE_LTR;
	  prev_type_orig = (RL_LEVEL (pp) == RL_LEVEL (pp->next) ?
			    FRIBIDI_TYPE_EN : FRIBIDI_TYPE_ON);
	}
      else
	prev_type_orig = PREV_TYPE_OR_SOR (pp->next);
    }
  }

  compact_neutrals (main_run_list);

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

  /* 5. Resolving Neutral Types */

  DBG ("5. resolving neutral types - N0");
  {
    /*  BD16 - Build list of all pairs*/
    int num_iso_levels = max_iso_level + 1;
    FriBidiPairingNodeArray pairing_nodes;
    fribidi_boolean pairing_alloc_failed = false;
    FriBidiRun *local_bracket_stack[FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL][LOCAL_BRACKET_SIZE];
    FriBidiRun **bracket_stack[FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL];
    int bracket_stack_size[FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL];
    int last_level = RL_LEVEL(main_run_list);
    int last_iso_level = 0;

    pairing_node_array_init (&pairing_nodes);
    memset(bracket_stack, 0, sizeof(bracket_stack[0])*num_iso_levels);
    memset(bracket_stack_size, 0, sizeof(bracket_stack_size[0])*num_iso_levels);

    /* populate the bracket_size. The first LOCAL_BRACKET_SIZE entries
       of the stack are one the stack. Allocate the rest of the entries.
     */
    {
      int iso_level;
      for (iso_level=0; iso_level < LOCAL_BRACKET_SIZE; iso_level++)
        bracket_stack[iso_level] = local_bracket_stack[iso_level];

      for (iso_level=LOCAL_BRACKET_SIZE; iso_level < num_iso_levels; iso_level++)
        bracket_stack[iso_level] = fribidi_malloc (sizeof (bracket_stack[0])
                                                       * FRIBIDI_BIDI_MAX_NESTED_BRACKET_PAIRS);
    }

    /* Build the bd16 pair stack. */
    for_run_list (pp, main_run_list)
      {
        int level = RL_LEVEL(pp);
        int iso_level = RL_ISOLATE_LEVEL(pp);
        FriBidiBracketType brack_prop = RL_BRACKET_TYPE(pp);

        /* Interpret the isolating run sequence as such that they
           end at a change in the level, unless the iso_level has been
           raised. */
        if (level != last_level && last_iso_level == iso_level)
          bracket_stack_size[last_iso_level] = 0;

        if (brack_prop!= FRIBIDI_NO_BRACKET
            && RL_TYPE(pp)==FRIBIDI_TYPE_ON)
          {
            if (FRIBIDI_IS_BRACKET_OPEN(brack_prop))
              {
                if (bracket_stack_size[iso_level]==FRIBIDI_BIDI_MAX_NESTED_BRACKET_PAIRS)
                  break;

                /* push onto the pair stack */
                bracket_stack[iso_level][bracket_stack_size[iso_level]++] = pp;
              }
            else
              {
                int stack_idx = bracket_stack_size[iso_level] - 1;
                while (stack_idx >= 0)
                  {
                    FriBidiBracketType se_brack_prop = RL_BRACKET_TYPE(bracket_stack[iso_level][stack_idx]);
                    if (FRIBIDI_BRACKET_ID(se_brack_prop) == FRIBIDI_BRACKET_ID(brack_prop))
                      {
                        bracket_stack_size[iso_level] = stack_idx;

                        if UNLIKELY
                          (!pairing_node_array_push (&pairing_nodes,
                                                     bracket_stack[iso_level][stack_idx],
                                                     pp))
                          pairing_alloc_failed = true;
                        break;
                    }
                    stack_idx--;
                  }
              }
          }
        if UNLIKELY
          (pairing_alloc_failed) break;
        last_level = level;
        last_iso_level = iso_level;
      }

    if UNLIKELY
      (pairing_alloc_failed)
      {
        free_pairing_nodes (&pairing_nodes);
        if (num_iso_levels >= LOCAL_BRACKET_SIZE)
          {
            int i;
            for (i=LOCAL_BRACKET_SIZE; i<num_iso_levels; i++)
              fribidi_free(bracket_stack[i]);
          }
        status = false;
        goto out;
      }

    /* The list must now be sorted for the next algo to work! */
    sort_pairing_nodes(&pairing_nodes);

# if DEBUG
    if UNLIKELY
    (fribidi_debug_status ())
      {
        print_pairing_nodes (&pairing_nodes);
      }
# endif	/* DEBUG */

    /* Start the N0 */
    {
      int ppairs_idx;
      /* Track, per isolate level, the level of the most recently seen
         strong character while sweeping the run list forward exactly
         once (strong_scan_pp only ever moves forward, it is never
         rewound). This lets N0c below look up the preceding strong
         character for each bracket pair in amortized O(1) instead of
         rescanning backwards toward the start of the string for every
         pair, which made runs of many bracket pairs with no strong
         content quadratic. */
      int last_strong_level[FRIBIDI_BIDI_MAX_EXPLICIT_LEVEL];
      FriBidiRun *strong_scan_pp = main_run_list->next;
      memset (last_strong_level, 0xFF, sizeof (int) * num_iso_levels);

      for (ppairs_idx = 0; ppairs_idx < pairing_nodes.count; ppairs_idx++)
        {
          FriBidiPairingNode *ppairs = &pairing_nodes.nodes[ppairs_idx];
          int embedding_level = ppairs->open->level;

          /* Find matching strong. */
          fribidi_boolean found = false;
          FriBidiRun *ppn;
          for (ppn = ppairs->open; ppn!= ppairs->close; ppn = ppn->next)
            {
              FriBidiCharType this_type = RL_TYPE_AN_EN_AS_RTL(ppn);

              /* Calculate level like in resolve implicit levels below to prevent
                 embedded levels not to match the base_level */
              int this_level = RL_LEVEL (ppn) +
                (FRIBIDI_LEVEL_IS_RTL (RL_LEVEL(ppn)) ^ FRIBIDI_DIR_TO_LEVEL (this_type));

              /* N0b */
              if (FRIBIDI_IS_STRONG (this_type) && this_level == embedding_level)
                {
                  RL_TYPE(ppairs->open) = RL_TYPE(ppairs->close) = this_level%2 ? FRIBIDI_TYPE_RTL : FRIBIDI_TYPE_LTR;
                  found = true;
                  break;
                }
            }

          /* N0c */
          /* Search for any strong type preceding and within the bracket pair */
          if (!found)
            {
              /* Search for a preceding strong. Catch the sweep cursor up
                 to this pair's opening bracket, recording the level of
                 every strong character passed along the way, indexed by
                 its isolate level. */
              int iso_level = RL_ISOLATE_LEVEL(ppairs->open);
              int prec_strong_level;

              for (; strong_scan_pp != ppairs->open; strong_scan_pp = strong_scan_pp->next)
                {
                  FriBidiCharType this_type = RL_TYPE_AN_EN_AS_RTL(strong_scan_pp);
                  if (FRIBIDI_IS_STRONG (this_type))
                    {
                      last_strong_level[RL_ISOLATE_LEVEL(strong_scan_pp)] = RL_LEVEL (strong_scan_pp) +
                        (FRIBIDI_LEVEL_IS_RTL (RL_LEVEL(strong_scan_pp)) ^ FRIBIDI_DIR_TO_LEVEL (this_type));
                    }
                }

              prec_strong_level = last_strong_level[iso_level] >= 0 ?
                last_strong_level[iso_level] : embedding_level; /* TBDov! Extract from Isolate level in effect */

              for (ppn = ppairs->open; ppn!= ppairs->close; ppn = ppn->next)
                {
                  FriBidiCharType this_type = RL_TYPE_AN_EN_AS_RTL(ppn);
                  if (FRIBIDI_IS_STRONG (this_type) && RL_ISOLATE_LEVEL(ppn) == iso_level)
                    {
                      /* By constraint this is opposite the embedding direction,
                         since we did not match the N0b rule. We must now
                         compare with the preceding strong to establish whether
                         to apply N0c1 (opposite) or N0c2 embedding */
                      RL_TYPE(ppairs->open) = RL_TYPE(ppairs->close) = prec_strong_level % 2 ? FRIBIDI_TYPE_RTL : FRIBIDI_TYPE_LTR;
                      found = true;
                      break;
                    }
                }
            }
        }

      free_pairing_nodes(&pairing_nodes);

      if (num_iso_levels >= LOCAL_BRACKET_SIZE)
        {
          int i;
          /* Only need to free the non static members */
          for (i=LOCAL_BRACKET_SIZE; i<num_iso_levels; i++)
            fribidi_free(bracket_stack[i]);
        }

      /* Remove the bracket property and re-compact */
      {
        const FriBidiBracketType NoBracket = FRIBIDI_NO_BRACKET;
        for_run_list (pp, main_run_list)
          pp->bracket_type = NoBracket;
        compact_neutrals (main_run_list);
      }
    }

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */
  }

  DBG ("resolving neutral types - N1+N2");
  {
    for_run_list (pp, main_run_list)
    {
      FriBidiCharType prev_type, this_type, next_type;
      FriBidiRun *ppp_prev, *ppp_next;

      ppp_prev = get_adjacent_run(pp, false, false);
      ppp_next = get_adjacent_run(pp, true, false);

      /* "European and Arabic numbers are treated as though they were R"
         FRIBIDI_CHANGE_NUMBER_TO_RTL does this. */
      this_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (RL_TYPE (pp));

      if (RL_LEVEL(ppp_prev) == RL_LEVEL(pp))
        prev_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (RL_TYPE(ppp_prev));
      else
        prev_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_prev), RL_LEVEL(pp)));

      if (RL_LEVEL(ppp_next) == RL_LEVEL(pp))
        next_type = FRIBIDI_CHANGE_NUMBER_TO_RTL (RL_TYPE(ppp_next));
      else
        next_type = FRIBIDI_LEVEL_TO_DIR(MAX(RL_LEVEL(ppp_next), RL_LEVEL(pp)));

      if (FRIBIDI_IS_NEUTRAL (this_type))
	RL_TYPE (pp) = (prev_type == next_type) ?
	  /* N1. */ prev_type :
	  /* N2. */ FRIBIDI_EMBEDDING_DIRECTION (pp);
    }
  }

  compact_list (main_run_list);

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

  /* 6. Resolving implicit levels */
  DBG ("resolving implicit levels");
  {
    max_level = base_level;

    for_run_list (pp, main_run_list)
    {
      FriBidiCharType this_type;
      int level;

      this_type = RL_TYPE (pp);
      level = RL_LEVEL (pp);

      /* I1. Even */
      /* I2. Odd */
      if (FRIBIDI_IS_NUMBER (this_type))
	RL_LEVEL (pp) = (level + 2) & ~1;
      else
	RL_LEVEL (pp) =
	  level +
	  (FRIBIDI_LEVEL_IS_RTL (level) ^ FRIBIDI_DIR_TO_LEVEL (this_type));

      if (RL_LEVEL (pp) > max_level)
	max_level = RL_LEVEL (pp);
    }
  }

  compact_list (main_run_list);

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_bidi_string (bidi_types, len);
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

/* Reinsert the explicit codes & BN's that are already removed, from the
   explicits_list to main_run_list. */
  DBG ("reinserting explicit codes");
  if UNLIKELY
    (explicits_list->next != explicits_list)
    {
      register FriBidiRun *p;
      register fribidi_boolean stat =
	shadow_run_list (main_run_list, explicits_list, true, run_pool);
      explicits_list = NULL;
      if UNLIKELY
	(!stat) goto out;

      /* Set level of inserted explicit chars to that of their previous
       * char, such that they do not affect reordering. */
      p = main_run_list->next;
      if (p != main_run_list && p->level == FRIBIDI_SENTINEL)
	p->level = base_level;
      for_run_list (p, main_run_list) if (p->level == FRIBIDI_SENTINEL)
	p->level = p->prev->level;
    }

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_types_re (main_run_list);
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

  DBG ("reset the embedding levels, 1, 2, 3.");
  {
    register int j, state, pos;
    register FriBidiCharType char_type;
    register FriBidiRun *p, *q, *list;

    /* L1. Reset the embedding levels of some chars:
       1. segment separators,
       2. paragraph separators,
       3. any sequence of whitespace characters preceding a segment
          separator or paragraph separator, and
       4. any sequence of whitespace characters and/or isolate formatting
          characters at the end of the line.
       ... (to be continued in fribidi_reorder_line()). */
    list = new_run_list (run_pool);
    if UNLIKELY
      (!list) goto out;
    q = list;
    state = 1;
    pos = len - 1;
    for (j = len - 1; j >= -1; j--)
      {
	/* close up the open link at the end */
	if (j >= 0)
	  char_type = bidi_types[j];
	else
	  char_type = FRIBIDI_TYPE_ON;
	if (!state && FRIBIDI_IS_SEPARATOR (char_type))
	  {
	    state = 1;
	    pos = j;
	  }
	else if (state &&
                 !(FRIBIDI_IS_EXPLICIT_OR_SEPARATOR_OR_BN_OR_WS(char_type)
                   || FRIBIDI_IS_ISOLATE(char_type)))
	  {
	    state = 0;
	    p = new_run (run_pool);
	    if UNLIKELY
	      (!p)
	      {
		free_run_list (list);
		goto out;
	      }
	    p->pos = j + 1;
	    p->len = pos - j;
	    p->type = base_dir;
	    p->level = base_level;
	    move_node_before (p, q);
	    q = p;
	  }
      }
    if UNLIKELY
      (!shadow_run_list (main_run_list, list, false, run_pool)) goto out;
  }

# if DEBUG
  if UNLIKELY
    (fribidi_debug_status ())
    {
      print_types_re (main_run_list);
      print_resolved_levels (main_run_list);
      print_resolved_types (main_run_list);
    }
# endif	/* DEBUG */

  {
    FriBidiStrIndex pos = 0;
    for_run_list (pp, main_run_list)
    {
      register FriBidiStrIndex l;
      register FriBidiLevel level = pp->level;
      for (l = pp->len; l; l--)
	embedding_levels[pos++] = level;
    }
  }

  status = true;

out:
  DBG ("leaving fribidi_get_par_embedding_levels");

  fribidi_run_pool_free (run_pool);

  return status ? max_level + 1 : 0;
}


static void
bidi_string_reverse (
  FriBidiChar *str,
  const FriBidiStrIndex len
)
{
  FriBidiStrIndex i;

  fribidi_assert (str);

  for (i = 0; i < len / 2; i++)
    {
      FriBidiChar tmp = str[i];
      str[i] = str[len - 1 - i];
      str[len - 1 - i] = tmp;
    }
}

static void
index_array_reverse (
  FriBidiStrIndex *arr,
  const FriBidiStrIndex len
)
{
  FriBidiStrIndex i;

  fribidi_assert (arr);

  for (i = 0; i < len / 2; i++)
    {
      FriBidiStrIndex tmp = arr[i];
      arr[i] = arr[len - 1 - i];
      arr[len - 1 - i] = tmp;
    }
}

/* A maximal run of consecutive characters that share the same resolved
 * embedding level, as used by the linear-time implementation of L2 below. */
typedef struct
{
  FriBidiStrIndex pos, len;
  FriBidiLevel level;
  FriBidiStrIndex next;	/* index into the runs array, or -1 */
} FriBidiL2Run;

/* A range groups one or more adjacent runs (in *visual* order) that have
 * already been merged together, along with the highest level seen among
 * the runs that produced it. */
typedef struct
{
  FriBidiLevel level;
  FriBidiStrIndex left, right;	/* indices into the runs array */
  FriBidiStrIndex previous;	/* index into the ranges array, or -1 */
} FriBidiL2Range;

/* Merges ranges[top] with ranges[ranges[top].previous], frees the top
 * range by returning its slot to the caller, and returns the index of
 * the surviving (previous) range. */
static FriBidiStrIndex
fribidi_l2_merge_range_with_previous (
  FriBidiL2Run *runs,
  FriBidiL2Range *ranges,
  FriBidiStrIndex top
)
{
  FriBidiStrIndex previous = ranges[top].previous;
  FriBidiStrIndex left, right;

  fribidi_assert (previous != -1);
  fribidi_assert (ranges[previous].level < ranges[top].level);

  if (FRIBIDI_LEVEL_IS_RTL (ranges[previous].level))
    {
      /* Odd, previous goes to the right of range. */
      left = top;
      right = previous;
    }
  else
    {
      /* Even, previous goes to the left of range. */
      left = previous;
      right = top;
    }
  /* Stitch them. */
  runs[ranges[left].right].next = ranges[right].left;

  ranges[previous].left = ranges[left].left;
  ranges[previous].right = ranges[right].right;

  return previous;
}

/* A one-pass linear-time implementation of UAX#9 rule L2, operating on
 * maximal same-level runs instead of individual characters.  This avoids
 * the O(len * max_level) cost of repeatedly rescanning the whole line for
 * each embedding level from max_level down to 1.
 *
 * Reorders runs[0..num_runs-1], which must be given in logical order and
 * linked via ->next accordingly, and returns the index of the left-most
 * (i.e. first in visual order) run.  The runs array is used both as
 * storage for the run list and as a preallocated stack for ranges sized
 * to num_runs, since neither structure can exceed num_runs entries.
 */
static FriBidiStrIndex
fribidi_l2_linear_reorder (
  FriBidiL2Run *runs,
  FriBidiL2Range *ranges,
  const FriBidiStrIndex num_runs
)
{
  FriBidiStrIndex top = -1;
  FriBidiStrIndex range_count = 0;
  FriBidiStrIndex i;

  for (i = 0; i < num_runs; i++)
    {
      while (top != -1 && ranges[top].level > runs[i].level &&
	     ranges[top].previous != -1 &&
	     ranges[ranges[top].previous].level >= runs[i].level)
	top = fribidi_l2_merge_range_with_previous (runs, ranges, top);

      if (top != -1 && ranges[top].level >= runs[i].level)
	{
	  /* Attach run to the range. */
	  if (FRIBIDI_LEVEL_IS_RTL (runs[i].level))
	    {
	      /* Odd, range goes to the right of run. */
	      runs[i].next = ranges[top].left;
	      ranges[top].left = i;
	    }
	  else
	    {
	      /* Even, range goes to the left of run. */
	      runs[ranges[top].right].next = i;
	      ranges[top].right = i;
	    }
	  ranges[top].level = runs[i].level;
	}
      else
	{
	  /* Push new range for run. */
	  FriBidiStrIndex r = range_count++;
	  ranges[r].left = ranges[r].right = i;
	  ranges[r].level = runs[i].level;
	  ranges[r].previous = top;
	  top = r;
	}
    }

  fribidi_assert (top != -1);
  while (ranges[top].previous != -1)
    top = fribidi_l2_merge_range_with_previous (runs, ranges, top);

  /* Terminate. */
  runs[ranges[top].right].next = -1;

  return ranges[top].left;
}


FRIBIDI_ENTRY FriBidiLevel
fribidi_reorder_line (
  /* input */
  FriBidiFlags flags, /* reorder flags */
  const FriBidiCharType *bidi_types,
  const FriBidiStrIndex len,
  const FriBidiStrIndex off,
  const FriBidiParType base_dir,
  /* input and output */
  FriBidiLevel *embedding_levels,
  FriBidiChar *visual_str,
  /* output */
  FriBidiStrIndex *map
)
{
  fribidi_boolean status = false;
  FriBidiLevel max_level = 0;

  if UNLIKELY
    (len == 0)
    {
      status = true;
      goto out;
    }

  DBG ("in fribidi_reorder_line");

  fribidi_assert (bidi_types);
  fribidi_assert (embedding_levels);

  DBG ("reset the embedding levels, 4. whitespace at the end of line");
  {
    register FriBidiStrIndex i;

    /* L1. Reset the embedding levels of some chars:
       4. any sequence of white space characters at the end of the line. */
    for (i = off + len - 1; i >= off &&
	 FRIBIDI_IS_EXPLICIT_OR_BN_OR_WS (bidi_types[i]); i--)
      embedding_levels[i] = FRIBIDI_DIR_TO_LEVEL (base_dir);
  }

  /* 7. Reordering resolved levels */
  {
    register FriBidiStrIndex i;

    /* Reorder both the outstring and the order array */
    {
      if (FRIBIDI_TEST_BITS (flags, FRIBIDI_FLAG_REORDER_NSM))
	{
	  register FriBidiLevel level;

	  /* L3. Reorder NSMs. */
	  for (i = off + len - 1; i >= off; i--)
	    if (FRIBIDI_LEVEL_IS_RTL (embedding_levels[i])
		&& bidi_types[i] == FRIBIDI_TYPE_NSM)
	      {
		register FriBidiStrIndex seq_end = i;
		level = embedding_levels[i];

		for (i--; i >= off &&
		     FRIBIDI_IS_EXPLICIT_OR_BN_OR_NSM (bidi_types[i])
		     && embedding_levels[i] == level; i--)
		  ;

		if (i < off || embedding_levels[i] != level)
		  {
		    i++;
		    DBG ("warning: NSM(s) at the beginning of level run");
		  }

		if (visual_str)
		  {
		    bidi_string_reverse (visual_str + i, seq_end - i + 1);
		  }
		if (map)
		  {
		    index_array_reverse (map + i, seq_end - i + 1);
		  }
	      }
	}

      /* L2. Reorder, and along the way find max_level of the line.  We
       * don't reuse the paragraph max_level, both for a cleaner API, and
       * that the line max_level may be far less than paragraph max_level.
       *
       * This is done in linear time by grouping the line into maximal
       * same-level runs and reordering the runs, rather than repeatedly
       * rescanning the whole line once per embedding level as the naive
       * algorithm from the standard does. */
      {
	/* Count the runs first, so the runs/ranges arrays below can be
	 * sized to the actual run count rather than to len: real-world
	 * lines usually consist of a handful of long runs, so this keeps
	 * the common case cheap. */
	FriBidiStrIndex num_runs = 0;
	FriBidiStrIndex pos;

	for (pos = off; pos < off + len; num_runs++)
	  {
	    FriBidiStrIndex run_len = 1;
	    while (pos + run_len < off + len &&
		   embedding_levels[pos + run_len] == embedding_levels[pos])
	      run_len++;
	    pos += run_len;
	  }

	{
	  char *runs_and_ranges =
	    fribidi_malloc (num_runs * (sizeof (FriBidiL2Run) +
					 sizeof (FriBidiL2Range)));
	  FriBidiL2Run *runs = (FriBidiL2Run *) runs_and_ranges;
	  FriBidiL2Range *ranges =
	    (FriBidiL2Range *) (runs_and_ranges +
				 num_runs * sizeof (FriBidiL2Run));
	  FriBidiStrIndex run_idx = 0;

	  for (pos = off; pos < off + len; run_idx++)
	    {
	      FriBidiStrIndex run_len = 1;
	      while (pos + run_len < off + len &&
		     embedding_levels[pos + run_len] == embedding_levels[pos])
		run_len++;

	      runs[run_idx].pos = pos;
	      runs[run_idx].len = run_len;
	      runs[run_idx].level = embedding_levels[pos];
	      runs[run_idx].next = -1;

	      if (runs[run_idx].level > max_level)
		max_level = runs[run_idx].level;

	      /* The run's own content must be reversed exactly once if it
	       * is an RTL run; further reordering only ever moves whole
	       * runs around, it never touches what is inside them. */
	      if (FRIBIDI_LEVEL_IS_RTL (runs[run_idx].level))
		{
		  if (visual_str)
		    bidi_string_reverse (visual_str + pos, run_len);
		  if (map)
		    index_array_reverse (map + pos, run_len);
		}

	      pos += run_len;
	    }

	  if (num_runs > 1)
	    {
	      FriBidiStrIndex left =
		fribidi_l2_linear_reorder (runs, ranges, num_runs);

	      if (visual_str || map)
		{
		  char *tmp =
		    fribidi_malloc (len * ((visual_str ? sizeof (FriBidiChar) :
					     0) +
					    (map ? sizeof (FriBidiStrIndex) :
					     0)));
		  FriBidiChar *tmp_visual = visual_str ? (FriBidiChar *) tmp :
		    NULL;
		  FriBidiStrIndex *tmp_map =
		    map ? (FriBidiStrIndex *) (tmp +
						(visual_str ?
						 len * sizeof (FriBidiChar) :
						 0)) : NULL;
		  FriBidiStrIndex out_pos = 0;
		  FriBidiStrIndex idx;

		  for (idx = left; idx != -1; idx = runs[idx].next)
		    {
		      if (tmp_visual)
			memcpy (tmp_visual + out_pos,
				visual_str + runs[idx].pos,
				runs[idx].len * sizeof (FriBidiChar));
		      if (tmp_map)
			memcpy (tmp_map + out_pos, map + runs[idx].pos,
				runs[idx].len * sizeof (FriBidiStrIndex));
		      out_pos += runs[idx].len;
		    }

		  if (tmp_visual)
		    memcpy (visual_str + off, tmp_visual,
			    len * sizeof (FriBidiChar));
		  if (tmp_map)
		    memcpy (map + off, tmp_map, len * sizeof (FriBidiStrIndex));

		  fribidi_free (tmp);
		}
	    }

	  fribidi_free (runs_and_ranges);
	}
      }
    }

  }

  status = true;

out:

  return status ? max_level + 1 : 0;
}

/* Editor directions:
 * vim:textwidth=78:tabstop=8:shiftwidth=2:autoindent:cindent
 */
