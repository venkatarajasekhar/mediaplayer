/* otfdrive.c -- OpenType font driver.

Copyright (C) 2003, 2004
  National Institute of Advanced Industrial Science and Technology (AIST)
  Registration Number H15PRO167

This file is part of libotf.

Libotf is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

Libotf is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library, in a file named COPYING; if not,
write to the Free Software Foundation, Inc., 59 Temple Place, Suite
330, Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#include "otf.h"
#include "otferror.h"

/* Return nonzero (-1 if ID is zero, 1 otherwise) if OTF_Glyph *G
   should be ignored according to LookupFlag FLAG.  */
#define IGNORED_GLYPH(g, flag)				\
  ((g)->glyph_id == 0 ? -1				\
   : (((flag) & (1 << (g)->GlyphClass))			\
      || (((flag) & OTF_MarkAttachmentType)		\
	  && (g)->GlyphClass == OTF_GlyphClassMark	\
	  && ((flag) >> 8) != (g)->MarkAttachClass)))

#define GSTRING_DELETE(gstring, from, len)				\
  do {									\
    memmove (gstring->glyphs + from, gstring->glyphs + from + len,	\
	     sizeof (OTF_Glyph) * (gstring->used - from - len));	\
    gstring->used -= len;						\
  } while (0)


#define GSTRING_INSERT(gstring, pos, len)				\
  do {									\
    if (gstring->used + len > gstring->size)				\
      {									\
	char *errfmt = "GSTRING%s";					\
									\
	gstring->size = gstring->used + len;				\
	gstring->glyphs							\
	  = (OTF_Glyph *) realloc (gstring->glyphs,			\
				   sizeof (OTF_Glyph) * gstring->size);	\
	if (! gstring->glyphs)						\
	  OTF_ERROR (OTF_ERROR_MEMORY, "");				\
      }									\
    memmove (gstring->glyphs + pos + len, gstring->glyphs + pos,	\
	     sizeof (OTF_Glyph) * (gstring->used - pos));		\
    gstring->used += len;						\
  } while (0)


static unsigned get_class_def (OTF_ClassDef *, OTF_GlyphID);

static int
gstring_subst (OTF *otf, OTF_GlyphString *gstring, int from, int to, int flag,
	       OTF_GlyphID *ids, int num)
{
  int errret = -1;
  int len = to - from;
  int i;
  int from_idx = gstring->glyphs[from].f.index.from;
  int to_idx = gstring->glyphs[to - 1].f.index.to;
  int non_ignored_idx;

  for (i = non_ignored_idx = to - 1; i >= from; i--)
    {
      OTF_Glyph *g = gstring->glyphs + i;

      if (IGNORED_GLYPH (g, flag) == 1)
	{
	  /* Move this glyph to the next of the current target of
	     substitution.  */
	  OTF_Glyph temp = *g;

	  memmove (g, g + 1, sizeof (OTF_Glyph) * (non_ignored_idx - i));
	  temp.f.index.from = from_idx;
	  temp.f.index.to = to_idx;
	  gstring->glyphs[non_ignored_idx--] = temp;
	  len--;
	}
    }

  if (len < num)
    GSTRING_INSERT (gstring, from, (num - len));
  else if (len > num)
    GSTRING_DELETE (gstring, from, (len - num));
  for (i = 0; i < num; i++)
    {
      if (gstring->glyphs[from + i].glyph_id != ids[i])
	{
	  gstring->glyphs[from + i].c = 0;
	  if (otf->gdef)
	    gstring->glyphs[from + i].GlyphClass
	      = get_class_def (&otf->gdef->glyph_class_def, ids[i]);
	  else
	    gstring->glyphs[from + i].GlyphClass = 0;
	}
      gstring->glyphs[from + i].glyph_id = ids[i];
      gstring->glyphs[from + i].positioning_type = 0;
      gstring->glyphs[from + i].f.index.from = from_idx;
      gstring->glyphs[from + i].f.index.to = to_idx;
    }
  return 0;
}


static int
get_coverage_index (OTF_Coverage *coverage, OTF_GlyphID id)
{
  int i;

  if (coverage->CoverageFormat == 1)
    {
      for (i = 0; i < coverage->Count; i++)
	if (coverage->table.GlyphArray[i] == id)
	  return i;
    }
  else
    {
      for (i = 0; i < coverage->Count; i++)
	if (coverage->table.RangeRecord[i].Start <= id
	    && coverage->table.RangeRecord[i].End >= id)
	  return (coverage->table.RangeRecord[i].StartCoverageIndex
		  + (id - coverage->table.RangeRecord[i].Start));
    }
  return -1;
}

static unsigned
get_class_def (OTF_ClassDef *class_def, OTF_GlyphID glyph_id)
{
  if (class_def->ClassFormat == 1)
    {
      int idx = (int) glyph_id - (int) class_def->f.f1.StartGlyph;

      if (idx >= 0 && idx < class_def->f.f1.GlyphCount)
	return class_def->f.f1.ClassValueArray[idx];
    }
  else
    {
      int i;

      for (i = 0; i < class_def->f.f2.ClassRangeCount; i++)
	if (glyph_id >= class_def->f.f2.ClassRangeRecord[i].Start
	    && glyph_id <= class_def->f.f2.ClassRangeRecord[i].End)
	  return class_def->f.f2.ClassRangeRecord[i].Class;
    }
  return 0;
}

static OTF_LangSys *
get_langsys (OTF_ScriptList *script_list,
	     const char *script, const char *language)
{

  OTF_Tag script_tag = OTF_tag (script);
  OTF_Tag langsys_tag = OTF_tag (language);
  int i, j;
  OTF_Tag dflt_tag = OTF_tag ("DFLT");
  OTF_Script *dflt = NULL;

  for (i = 0; i < script_list->ScriptCount; i++)
    {
      OTF_Script *script = script_list->Script + i;

      if (script_list->Script[i].ScriptTag == dflt_tag)
	dflt = script;
      if (script_list->Script[i].ScriptTag == script_tag)
	{
	  if (! langsys_tag)
	    return &script->DefaultLangSys;
	  for (j = 0; j < script->LangSysCount; j++)
	    if (script->LangSysRecord[j].LangSysTag == langsys_tag)
	      return script->LangSys + j;
	  return &script->DefaultLangSys;	
	}
    }

  if (! dflt)
    dflt = script_list->Script;
  if (! langsys_tag)
    return &dflt->DefaultLangSys;
  for (j = 0; j < dflt->LangSysCount; j++)
    if (dflt->LangSysRecord[j].LangSysTag == langsys_tag)
      return dflt->LangSys + j;
  return &dflt->DefaultLangSys;	
}

static int
setup_lookup_flags (OTF_LookupList *LookupList, OTF_FeatureList *FeatureList,
		    OTF_LangSys *LangSys,
		    const char *features, char *lookup_flags)
{
  int i, j, n = 0;
  OTF_Feature *feature;
  int *feature_table = alloca (sizeof (int) * FeatureList->FeatureCount);

  if (! feature_table)
    return -1;
  for (i = 0; i < FeatureList->FeatureCount; i++)
    feature_table[i] = 0;
  memset (lookup_flags, 0, LookupList->LookupCount);

  while (*features)
    {
      char tagname[4];
      OTF_Tag tag;
      int use_it = 1;

      if (*features == '*')
	{
	  /* Consume all remaining features.  */
	  for (i = 0; i < LangSys->FeatureCount; i++) 
	    {
	      int index = LangSys->FeatureIndex[i];

	      if (! feature_table[index])
		{
		  feature = FeatureList->Feature + index;
		  for (j = 0; j < feature->LookupCount; j++)
		    lookup_flags[feature->LookupListIndex[j]] = 1;
		}
	    }
	  break;
	}

      if (*features == '~')
	use_it = -1, features++;
      for (i = 0; *features && *features != ','; i++, features++)
	tagname[i] = *features;
      if (*features)
	/* Skip ',' */
	features++;
      for (; i < 4; i++)
	tagname[i] = '\0';
      tag = OTF_tag (tagname);
      for (i = 0; i < LangSys->FeatureCount; i++)
	{
	  feature = FeatureList->Feature + LangSys->FeatureIndex[i];
	  if (tag == feature->FeatureTag)
	    {
	      if (feature_table[i])
		break;
	      if (use_it > 0)
		for (j = 0; j < feature->LookupCount; j++)
		  lookup_flags[feature->LookupListIndex[j]] = 1;
	      feature_table[i] = use_it;
	      break;
	    }
	}
    }
  return 0;
}

static int
match_ids (OTF_GlyphString *gstring, int gidx, int flag,
	   int count, OTF_GlyphID *ids)
{
  OTF_Glyph *gbeg = gstring->glyphs + gidx;
  OTF_Glyph *gend = gstring->glyphs + gstring->used;
  OTF_Glyph *g;
  int i;

  for (g = gbeg, i = 0; g < gend && i < count; g++)
    if (! IGNORED_GLYPH (g, flag) && g->glyph_id != ids[i++])
      return -1;
  return (i < count ? -1 : g - gbeg);
}

static int
match_chain_ids (OTF_GlyphString *gstring, int gidx, int flag,
		 OTF_ChainRule *rule)
{
  int i = rule->BacktrackGlyphCount;

  if (i > 0)
    {
      int j;
      OTF_Glyph *g;

      for (j = gidx - 1, g = gstring->glyphs + j; j >= 0; j--, g--)
	if (! IGNORED_GLYPH (g, flag) && --i == 0)
	  break;
      if (i > 0)
	return -1;
      if (match_ids (gstring, j, flag,
		     rule->BacktrackGlyphCount, rule->Backtrack)
	  < 0)
	return -1;
    }
  gidx++;
  i = match_ids (gstring, gidx, flag,
		 rule->InputGlyphCount - 1, rule->Input);
  if (i < 0)
    return -1;
  gidx += i;
  i = match_ids (gstring, gidx, flag,
		 rule->LookaheadGlyphCount, rule->LookAhead);
  if (i < 0)
    return -1;
  return 0;
}

static int
match_classes (OTF_ClassDef *class_def, OTF_GlyphString *gstring, int gidx,
	       int flag, int count, unsigned *classes)
{
  OTF_Glyph *gbeg = gstring->glyphs + gidx;
  OTF_Glyph *gend = gstring->glyphs + gstring->used;
  OTF_Glyph *g;
  int i;

  for (g = gbeg, i = 0; g < gend && i < count; g++)
    if (! IGNORED_GLYPH (g, flag)
	&& get_class_def (class_def, g->glyph_id) != classes[i++])
      return -1;
  return (i < count ? -1 : g - gbeg);
}

static int
match_chain_classes (OTF_GlyphString *gstring, int gidx, int flag,
		     OTF_ClassDef *BacktrackClassDef,
		     OTF_ClassDef *InputClassDef,
		     OTF_ClassDef *LookaheadClassDef,
		     OTF_ChainClassRule *rule)
{
  int i = rule->BacktrackGlyphCount;

  if (i > 0)
    {
      int j;
      OTF_Glyph *g;

      for (j = gidx - 1, g = gstring->glyphs + j; j >= 0; j--, g--)
	if (! IGNORED_GLYPH (g, flag) && i-- == 0)
	  break;
      if (i > 0)
	return -1;
      if (match_classes (BacktrackClassDef, gstring, j, flag,
			 rule->BacktrackGlyphCount, rule->Backtrack) < 0);
      return -1;
    }
  gidx++;
  i = match_classes (InputClassDef, gstring, gidx, flag,
		     rule->InputGlyphCount - 1, rule->Input);
  if (i < 0)
    return -1;
  gidx += i;
  i = match_classes (LookaheadClassDef, gstring, gidx, flag,
		     rule->LookaheadGlyphCount, rule->LookAhead);
  if (i < 0)
    return -1;
  return 0;
}


static int
match_coverages (OTF_GlyphString *gstring, int gidx, int flag, int count,
		 OTF_Coverage *coverages)
{
  OTF_Glyph *gbeg = gstring->glyphs + gidx;
  OTF_Glyph *gend = gstring->glyphs + gstring->used;
  OTF_Glyph *g;
  int i;

  for (g = gbeg, i = 0; g < gend && i < count; g++)
    if (! IGNORED_GLYPH (g, flag)
	&& get_coverage_index (coverages + i++, g->glyph_id) < 0)
      return -1;
  return (i < count ? -1 : g - gbeg);
}

static int
match_chain_coverages (OTF_GlyphString *gstring, int gidx, int flag,
		       OTF_GSUB_ChainContext3 *context3)
{
  int i = context3->BacktrackGlyphCount;

  if (i > 0)
    {
      int j;
      OTF_Glyph *g;

      for (j = gidx - 1, g= gstring->glyphs +j; j >= 0; j--, g--)
	if (! IGNORED_GLYPH (g, flag) && --i == 0)
	  break;
      if (i > 0)
	return -1;
      if (match_coverages (gstring, j, flag, context3->BacktrackGlyphCount,
			   context3->Backtrack) < 0)
	return -1;
    }
  gidx++;
  if (context3->InputGlyphCount > 1)
    {
      i = match_coverages (gstring, gidx, flag, context3->InputGlyphCount - 1,
			   context3->Input + 1);
      if (i < 0)
	return -1;
      gidx += i;
    }
  if (match_coverages (gstring, gidx, flag, context3->LookaheadGlyphCount,
		       context3->LookAhead) < 0)
    return -1;
  return 0;
}

static int
lookup_gsub (OTF *otf, OTF_LookupList *lookup_list, unsigned lookup_list_index,
	     OTF_GlyphString *gstring, int gidx, int alternate_subst)
{
  char *errfmt = "GSUB Looking up%s";
  int errret = -1;
  OTF_Lookup *lookup = lookup_list->Lookup + lookup_list_index;
  unsigned int flag = (lookup->LookupFlag
		       & (OTF_LookupFlagIgnoreMask | OTF_MarkAttachmentType));
  int orig_gidx = gidx;
  OTF_Glyph *g = gstring->glyphs + gidx;
  int i;

  if (IGNORED_GLYPH (g, flag))
    return (gidx + 1);

  /* Try all subtables until one of them handles the current glyph.  */
  for (i = 0; i < lookup->SubTableCount && gidx == orig_gidx; i++)
    {
      unsigned lookup_type = lookup->LookupType;
      OTF_LookupSubTableGSUB *subtable = lookup->SubTable.gsub + i;
      int coverage_idx;

      if (lookup_type == 7)
	{
	  OTF_GSUB_Extension1 *extension1 = &subtable->u.extension1;

	  lookup_type = extension1->ExtensionLookupType;
	  subtable = extension1->ExtensionSubtable;
	}

      if (alternate_subst
	  ? (lookup_type != 3 && lookup_type != 5 && lookup_type != 6)
	  : (lookup_type == 3))
	continue;

      if (subtable->Coverage.offset)
	{
	  coverage_idx = get_coverage_index (&subtable->Coverage,
					     g->glyph_id);
	  if (coverage_idx < 0)
	    continue;
	}

      switch (lookup_type)
	{
	case 1:
	  if (subtable->Format == 1)
	    g->glyph_id += subtable->u.single1.DeltaGlyphID;
	  else
	    g->glyph_id = subtable->u.single2.Substitute[coverage_idx];
	  gidx++;
	  break;

	case 2:
	  if (subtable->Format == 1)
	    {
	      OTF_GSUB_Multiple1 *multiple1 = &subtable->u.multiple1;
	      OTF_Sequence *seq = multiple1->Sequence + coverage_idx;

	      gstring_subst (otf, gstring, gidx, gidx + 1, flag,
			     seq->Substitute, seq->GlyphCount);
	      gidx += seq->GlyphCount;
	    }
	  else
	    OTF_ERROR (OTF_ERROR_GSUB_DRIVE, " (invalid SubFormat)");
	  break;

	case 3:
	  if (subtable->Format == 1)
	    {
	      OTF_GSUB_Alternate1 *alt1 = &subtable->u.alternate1;
	      OTF_AlternateSet *altset = alt1->AlternateSet + coverage_idx;

	      gstring_subst (otf, gstring, gidx, gidx + 1, flag,
			     altset->Alternate, altset->GlyphCount);
	      gidx += altset->GlyphCount;;
	    }
	  else
	    OTF_ERROR (OTF_ERROR_GSUB_DRIVE, " (invalid SubFormat)");
	  break;

	case 4:
	  if (subtable->Format == 1)
	    {
	      OTF_GSUB_Ligature1 *lig1 = &subtable->u.ligature1;
	      OTF_LigatureSet *ligset = lig1->LigatureSet + coverage_idx;
	      OTF_Ligature *lig;
	      int j;

	      for (j = 0; j < ligset->LigatureCount; j++)
		{
		  int n;

		  lig = ligset->Ligature + j;
		  n = match_ids (gstring, gidx + 1, flag,
				 lig->CompCount - 1, lig->Component);
		  if (n < 0)
		    continue;
		  gstring_subst (otf, gstring, gidx, gidx + 1 + n, flag,
				 &lig->LigGlyph, 1);
		  gidx++;
		  break;
		}
	    }
	  else
	    OTF_ERROR (OTF_ERROR_GSUB_DRIVE, " (invalid SubFormat)");
	  break;

	case 5:
	  if (subtable->Format == 1)
	    {
	      OTF_GSUB_Context1 *context1 = &subtable->u.context1;
	      OTF_RuleSet *set = context1->RuleSet + coverage_idx;
	      OTF_Rule *rule;
	      int orig_used;
	      int j, k;

	      for (j = 0; j < set->RuleCount; j++)
		{
		  rule = set->Rule + j;
		  if (match_ids (gstring, gidx + 1, flag,
				 rule->GlyphCount - 1, rule->Input) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gsub (otf, lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 alternate_subst);
		  gidx += rule->GlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else if (subtable->Format == 2)
	    {
	      OTF_GSUB_Context2 *context2 = &subtable->u.context2;
	      OTF_ClassSet *set;
	      OTF_ClassRule *rule;
	      unsigned class;
	      int orig_used;
	      int j, k;

	      class = get_class_def (&context2->ClassDef, g->glyph_id);
	      set = context2->ClassSet + class;
	      if (set)
		for (j = 0; j < set->ClassRuleCnt; j++)
		  {
		    rule = set->ClassRule + j;
		    if (match_classes (&context2->ClassDef,
				       gstring, gidx + 1, flag,
				       rule->GlyphCount - 1, rule->Class)
			< 0)
		      continue;
		    orig_used = gstring->used;
		    for (k = 0; k < rule->LookupCount; k++)
		      lookup_gsub (otf, lookup_list,
				   rule->LookupRecord[k].LookupListIndex,
				   gstring,
				   gidx + rule->LookupRecord[k].SequenceIndex,
				   alternate_subst);
		    gidx += rule->GlyphCount + (gstring->used - orig_used);
		    break;
		  }
	    }
	  else 			/* subtable->Format == 3 */
	    {
	      OTF_GSUB_Context3 *context3 = &subtable->u.context3;
	      int orig_used;
	      int j;

	      if (match_coverages (gstring, gidx + 1, flag,
				   context3->GlyphCount - 1,
				   context3->Coverage + 1) < 0)
		continue;
	      orig_used = gstring->used;
	      for (j = 0; j < context3->LookupCount; j++)
		lookup_gsub (otf, lookup_list,
			     context3->LookupRecord[j].LookupListIndex,
			     gstring,
			     gidx + context3->LookupRecord[j].SequenceIndex,
			     alternate_subst);
	      gidx += context3->GlyphCount + (gstring->used - orig_used);
	    }
	  break;

	case 6:
	  if (subtable->Format == 1)
	    {
	      OTF_GSUB_ChainContext1 *context1 = &subtable->u.chain_context1;
	      OTF_ChainRuleSet *set = context1->ChainRuleSet + coverage_idx;
	      int orig_used;
	      int j, k;
	      
	      for (j = 0; j < set->ChainRuleCount; j++)
		{
		  OTF_ChainRule *rule = set->ChainRule + j;

		  if (gidx < rule->BacktrackGlyphCount
		      || (gidx + rule->InputGlyphCount
			  + rule->LookaheadGlyphCount) > gstring->used)
		    continue;
		  if (match_chain_ids (gstring, gidx, flag, rule) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gsub (otf, lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 alternate_subst);
		  gidx += rule->InputGlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else if (subtable->Format == 2)
	    {
	      OTF_GSUB_ChainContext2 *context2 = &subtable->u.chain_context2;
	      OTF_ChainClassSet *set;
	      unsigned class;
	      int j;
	      int orig_used;

	      class = get_class_def (&context2->InputClassDef, g->glyph_id);
	      set = context2->ChainClassSet + class;
	      for (j = 0; j < set->ChainClassRuleCnt; j++)
		{
		  OTF_ChainClassRule *rule = set->ChainClassRule + j;
		  int k;

		  if (gidx < rule->BacktrackGlyphCount
		      || (gidx + rule->InputGlyphCount
			  + rule->LookaheadGlyphCount) > gstring->used)
		    continue;
		  if (match_chain_classes (gstring, gidx, flag,
					   &context2->BacktrackClassDef,
					   &context2->InputClassDef,
					   &context2->LookaheadClassDef,
					   rule) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gsub (otf, lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 alternate_subst);
		  gidx += rule->InputGlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else
	    {
	      OTF_GSUB_ChainContext3 *context3 = &subtable->u.chain_context3;
	      int orig_used;
	      int j;

	      if (gidx < context3->BacktrackGlyphCount
		  || (gidx + context3->InputGlyphCount
		      + context3->LookaheadGlyphCount) > gstring->used)
		continue;
	      if (match_chain_coverages (gstring, gidx, flag, context3) < 0)
		continue;
	      orig_used = gstring->used;
	      for (j = 0; j < context3->LookupCount; j++)
		lookup_gsub (otf, lookup_list,
			     context3->LookupRecord[j].LookupListIndex,
			     gstring,
			     gidx + context3->LookupRecord[j].SequenceIndex,
			     alternate_subst);
	      gidx += context3->InputGlyphCount + (gstring->used - orig_used);
	    }
	  break;

	case 8:
	  {
	    OTF_GSUB_ReverseChain1 *reverse = &subtable->u.reverse_chain1;
	    int back_gidx = gidx + 1 + reverse->BacktrackGlyphCount;
	    int ahead_gidx = gidx - reverse->LookaheadGlyphCount;
	    int j;

	    if (back_gidx > gstring->used || ahead_gidx < 0)
	      break;

	    for (j = 0; j < reverse->BacktrackGlyphCount; j++)
	      if (get_coverage_index (reverse->Backtrack + j,
				      gstring->glyphs[gidx + 1 + j].glyph_id)
		  < 0)
		break;
	    if (j < reverse->BacktrackGlyphCount)
	      continue;
	    for (j = 0; j < reverse->LookaheadGlyphCount; j++)
	      if (get_coverage_index (reverse->LookAhead + j,
				      gstring->glyphs[gidx - 1 - j].glyph_id)
		  < 0)
		break;
	    if (j < reverse->LookaheadGlyphCount)
	      continue;
	    g->glyph_id = reverse->Substitute[coverage_idx];
	    gidx--;
	  }

	default:
	  continue;
	}
    }
  if (gidx == orig_gidx)
    gidx++;
  return gidx;
}



/* GPOS */
unsigned
get_anchor (OTF_Anchor *anchor, OTF_ValueRecord *rec)
{
  unsigned value_format = OTF_XPlacement | OTF_YPlacement;

  rec->XPlacement = anchor->XCoordinate;
  rec->YPlacement = anchor->YCoordinate;
  if (anchor->AnchorFormat == 1)
    /* Nothing to do */
    ;
  else if (anchor->AnchorFormat == 2)
    /* Not yet implemented */
    ;
  else if (anchor->AnchorFormat == 3)
    /* Not yet implemented */
    ;
  return value_format;
}

static int
gstring_insert_for_gpos (OTF_GlyphString *gstring, int gidx)
{
  int errret = -1;
  int orig_gidx = gidx++;

  while (gidx < gstring->used
	 && ! gstring->glyphs[gidx].glyph_id
	 && gstring->glyphs[gidx].positioning_type)
    gidx++;
  GSTRING_INSERT (gstring, gidx, 1);
  gstring->glyphs[gidx] = gstring->glyphs[orig_gidx];
  gstring->glyphs[gidx].glyph_id = 0;
  return gidx;
}

static int
lookup_gpos (OTF_LookupList *lookup_list, unsigned lookup_list_index,
	     OTF_GlyphString *gstring, int gidx, int accumulate)
{
  char *errfmt = "GPOS Looking up%s";
  int errret = -1;
  OTF_Lookup *lookup = lookup_list->Lookup + lookup_list_index;
  unsigned int flag = (lookup->LookupFlag
		       & (OTF_LookupFlagIgnoreMask | OTF_MarkAttachmentType));
  int orig_gidx = gidx;
  OTF_Glyph *g = gstring->glyphs + gidx;
  int i;

  if (IGNORED_GLYPH (g, flag))
    return (gidx + 1);

  /* Try all subtables until one of them handles the current glyph.  */
  for (i = 0; i < lookup->SubTableCount && gidx == orig_gidx; i++)
    {
      unsigned lookup_type = lookup->LookupType;
      OTF_LookupSubTableGPOS *subtable = lookup->SubTable.gpos + i;
      int coverage_idx;
      int positioning_type;
      enum OTF_ValueFormat format;
      OTF_ValueRecord *value;
      OTF_Anchor *anchor1, *anchor2;

      if (lookup_type == 9)
	{
	  OTF_GPOS_Extension1 *extension1 = &subtable->u.extension1;

	  lookup_type = extension1->ExtensionLookupType;
	  subtable = extension1->ExtensionSubtable;
	}

      if (subtable->Coverage.offset)
	{
	  coverage_idx = get_coverage_index (&subtable->Coverage,
					     g->glyph_id);
	  if (coverage_idx < 0)
	    continue;
	}

      switch (lookup_type)
	{
	case 1:
	  positioning_type = lookup_type;
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_Single1 *single1 = &subtable->u.single1;

	      format = single1->ValueFormat;
	      value = &single1->Value;
	    }
	  else if (subtable->Format == 2)
	    {
	      OTF_GPOS_Single2 *single2 = &subtable->u.single2;

	      format = single2->ValueFormat;
	      value = single2->Value + coverage_idx;
	    }
	  if (accumulate && g->positioning_type)
	    {
	      gidx = gstring_insert_for_gpos (gstring, gidx);	      
	      g = gstring->glyphs + gidx;
	    }
	  g->positioning_type = positioning_type;
	  g->f.f1.format = format;
	  g->f.f1.value = value;
	  gidx++;
	  break;

	case 2:
	  {
	    int next_gidx;
	    OTF_Glyph *nextg;

	    for (next_gidx = gidx + 1, nextg = gstring->glyphs + next_gidx;
		 next_gidx < gstring->used && IGNORED_GLYPH (nextg, flag);
		 next_gidx++, nextg++);

	    if (next_gidx >= gstring->used
		|| nextg->positioning_type)
	      continue;
	    if (subtable->Format == 1)
	      {
		OTF_GPOS_Pair1 *pair1 = &subtable->u.pair1;
		OTF_PairSet *set = pair1->PairSet + coverage_idx;
		int j;

		for (j = 0; j < set->PairValueCount; j++)
		  if (set->PairValueRecord[j].SecondGlyph == nextg->glyph_id)
		    {
		      if (pair1->ValueFormat1)
			{
			  if (accumulate && g->positioning_type)
			    {
			      gidx = gstring_insert_for_gpos (gstring, gidx);
			      g = gstring->glyphs + gidx;
			      next_gidx += gidx - orig_gidx;
			      nextg = gstring->glyphs + next_gidx;
			    }
			  g->positioning_type = lookup_type;
			  g->f.f2.format = pair1->ValueFormat1;
			  g->f.f2.value = &set->PairValueRecord[j].Value1;
			}
		      gidx = next_gidx;
		      g = nextg;
		      if (pair1->ValueFormat2)
			{
			  if (accumulate && g->positioning_type)
			    {
			      gidx = gstring_insert_for_gpos (gstring, gidx);
			      g = gstring->glyphs + gidx;
			    }
			  g->positioning_type = lookup_type;
			  g->f.f2.format = pair1->ValueFormat2;
			  g->f.f2.value = &set->PairValueRecord[j].Value2;
			  gidx++;
			}
		      break;
		    }
	      }
	    else if (subtable->Format == 2)
	      {
		OTF_GPOS_Pair2 *pair2 = &subtable->u.pair2;
		unsigned class1, class2;

		class1 = get_class_def (&pair2->ClassDef1, g->glyph_id);
		class2 = get_class_def (&pair2->ClassDef2, nextg->glyph_id);
		if (pair2->ValueFormat1)
		  {
		    if (accumulate && g->positioning_type)
		      {
			gidx = gstring_insert_for_gpos (gstring, gidx);
			g = gstring->glyphs + gidx;
			next_gidx += gidx - orig_gidx;
			nextg = gstring->glyphs + next_gidx;
		      }
		    g->positioning_type = lookup_type;
		    g->f.f2.format = pair2->ValueFormat1;
		    g->f.f2.value
		      = &pair2->Class1Record[class1].Class2Record[class2].Value1;
		  }
		gidx = next_gidx;
		g = nextg;
		if (pair2->ValueFormat2)
		  {
		    if (accumulate && g->positioning_type)
		      {
			gidx = gstring_insert_for_gpos (gstring, gidx);
			g = gstring->glyphs + gidx;
		      }
		    g->positioning_type = lookup_type;
		    g->f.f2.format = pair2->ValueFormat2;
		    g->f.f2.value
		      = &pair2->Class1Record[class1].Class2Record[class2].Value2;
		    gidx++;
		  }
	      }
	  }
	  break;

	case 3:
	  {
	    OTF_GPOS_Cursive1 *cursive1 = &subtable->u.cursive1;
	  
	    if (accumulate && g->positioning_type)
	      {
		gidx = gstring_insert_for_gpos (gstring, gidx);
		g = gstring->glyphs + gidx;
	      }
	    g->positioning_type = lookup_type;
	    g->f.f3.entry_anchor
	      = &cursive1->EntryExitRecord[coverage_idx].EntryAnchor;
	    g->f.f3.exit_anchor
	      = &cursive1->EntryExitRecord[coverage_idx].ExitAnchor;
	  }
	  break;

	case 4:
	  if (gidx < 1)
	    continue;
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_MarkBase1 *mark_base1 = &subtable->u.mark_base1;
	      OTF_MarkRecord *mark_record;
	      OTF_AnchorRecord *base_record;
	      OTF_Glyph *baseg;
	      int coverage_idx_base;
	      unsigned int this_flag = flag | OTF_IgnoreMarks;

	      for (baseg = g - 1;
		   baseg >= gstring->glyphs && IGNORED_GLYPH (baseg, this_flag);
		   baseg--);
	      if (baseg < gstring->glyphs)
		continue;
	      coverage_idx_base
		= get_coverage_index (&mark_base1->BaseCoverage,
				      baseg->glyph_id);
	      if (coverage_idx_base < 0)
		continue;
	      mark_record = mark_base1->MarkArray.MarkRecord + coverage_idx;
	      base_record
		= mark_base1->BaseArray.AnchorRecord + coverage_idx_base;
	      if (accumulate && g->positioning_type)
		{
		  gidx = gstring_insert_for_gpos (gstring, gidx);
		  g = gstring->glyphs + gidx;
		}
	      g->f.f4.mark_anchor = &mark_record->MarkAnchor;
	      g->f.f4.base_anchor
		= &base_record->Anchor[mark_record->Class];
	      g->positioning_type = lookup_type;
	    }
	  break;

	case 5:
	  if (gidx < 1)
	    continue;
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_MarkLig1 *mark_lig1 = &subtable->u.mark_lig1;
	      OTF_Glyph *ligg;
	      int coverage_idx_lig;
	      OTF_MarkRecord *mark_record;
	      OTF_LigatureAttach *attach;
	      int *num_class = alloca (sizeof (int) * mark_lig1->ClassCount);
	      int j;
				       
	      for (j = 0; j < mark_lig1->ClassCount; j++)
		num_class[j] = 0;

	      for (ligg = g - 1;
		   (ligg >= gstring->glyphs
		    && (IGNORED_GLYPH (ligg, flag)
			|| ligg->GlyphClass > OTF_GlyphClassLigature));
		   ligg--)
		if (ligg->positioning_type == 5
		    && ligg->MarkAttachClass < mark_lig1->ClassCount)
		  num_class[ligg->MarkAttachClass]++;
	      if (ligg < gstring->glyphs)
		continue;
	      coverage_idx_lig
		= get_coverage_index (&mark_lig1->LigatureCoverage,
				      ligg->glyph_id);
	      if (coverage_idx_lig < 0)
		continue;
	      mark_record = mark_lig1->MarkArray.MarkRecord + coverage_idx;
	      g->MarkAttachClass = mark_record->Class;
	      attach = (mark_lig1->LigatureArray.LigatureAttach
			+ coverage_idx_lig);
	      for (j = 0; j < attach->ComponentCount; j++)
		{
		  OTF_Anchor *lig_anchor
		    = attach->ComponentRecord[j].LigatureAnchor;

		  if (lig_anchor[mark_record->Class].AnchorFormat
		      && num_class[mark_record->Class]-- == 0)
		    {
		      if (accumulate && g->positioning_type)
			{
			  gidx = gstring_insert_for_gpos (gstring, gidx);
			  g = gstring->glyphs + gidx;
			}
		      g->positioning_type = lookup_type;
		      g->f.f5.mark_anchor = &mark_record->MarkAnchor;
		      g->f.f5.ligature_anchor = lig_anchor + mark_record->Class;
		      break;
		    }
		}
	    }
	  break;

	case 6:
	  if (gidx < 1)
	    continue;
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_MarkMark1 *mark_mark1 = &subtable->u.mark_mark1;
	      OTF_MarkRecord *mark1_record;
	      OTF_AnchorRecord *mark2_record;
	      OTF_Glyph *prevg;
	      int coverage_idx_base;

	      for (prevg = g - 1;
		   prevg >= gstring->glyphs && IGNORED_GLYPH (prevg, flag);
		   prevg--);
	      if (prevg < gstring->glyphs)
		continue;
	      coverage_idx_base
		= get_coverage_index (&mark_mark1->Mark2Coverage,
				      prevg->glyph_id);
	      if (coverage_idx_base < 0)
		continue;
	      mark1_record = mark_mark1->Mark1Array.MarkRecord + coverage_idx;
	      mark2_record
		= mark_mark1->Mark2Array.AnchorRecord + coverage_idx_base;
	      if (accumulate && g->positioning_type)
		{
		  gidx = gstring_insert_for_gpos (gstring, gidx);
		  g = gstring->glyphs + gidx;
		}
	      g->f.f6.mark1_anchor = &mark1_record->MarkAnchor;
	      g->f.f6.mark2_anchor
		= &mark2_record->Anchor[mark1_record->Class];
	      g->positioning_type = lookup_type;
	      break;
	    }
	  break;

	case 7:
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_Context1 *context1 = &subtable->u.context1;
	      OTF_RuleSet *set = context1->RuleSet + coverage_idx;
	      OTF_Rule *rule;
	      int orig_used;
	      int j, k;

	      for (j = 0; j < set->RuleCount; j++)
		{
		  rule = set->Rule + j;
		  if (match_ids (gstring, gidx + 1, flag,
				 rule->GlyphCount - 1, rule->Input) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gpos (lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 accumulate);
		  gidx += rule->GlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else if (subtable->Format == 2)
	    {
	      OTF_GPOS_Context2 *context2 = &subtable->u.context2;
	      OTF_ClassSet *set;
	      OTF_ClassRule *rule;
	      unsigned class;
	      int orig_used;
	      int j, k;

	      class = get_class_def (&context2->ClassDef, g->glyph_id);
	      set = context2->ClassSet + class;
	      if (set)
		for (j = 0; j < set->ClassRuleCnt; j++)
		  {
		    rule = set->ClassRule + j;
		    if (match_classes (&context2->ClassDef,
				       gstring, gidx + 1, flag,
				       rule->GlyphCount - 1, rule->Class)
			< 0)
		      continue;
		    orig_used = gstring->used;
		    for (k = 0; k < rule->LookupCount; k++)
		      lookup_gpos (lookup_list,
				   rule->LookupRecord[k].LookupListIndex,
				   gstring,
				   gidx + rule->LookupRecord[k].SequenceIndex,
				   accumulate);
		    gidx += rule->GlyphCount + (gstring->used - orig_used);
		    break;
		  }
	    }
	  else 			/* subtable->Format == 3 */
	    {
	      OTF_GPOS_Context3 *context3 = &subtable->u.context3;
	      int orig_used;
	      int j;

	      if (match_coverages (gstring, gidx + 1, flag,
				   context3->GlyphCount - 1,
				   context3->Coverage + 1) < 0)
		continue;
	      orig_used = gstring->used;
	      for (j = 0; j < context3->LookupCount; j++)
		lookup_gpos (lookup_list,
			     context3->LookupRecord[j].LookupListIndex,
			     gstring,
			     gidx + context3->LookupRecord[j].SequenceIndex,
			     accumulate);
	      gidx += context3->GlyphCount + (gstring->used - orig_used);
	    }
	  break;

	case 8:
	  if (subtable->Format == 1)
	    {
	      OTF_GPOS_ChainContext1 *context1 = &subtable->u.chain_context1;
	      OTF_ChainRuleSet *set = context1->ChainRuleSet + coverage_idx;
	      int orig_used;
	      int j, k;
	      
	      for (j = 0; j < set->ChainRuleCount; j++)
		{
		  OTF_ChainRule *rule = set->ChainRule + j;

		  if (gidx < rule->BacktrackGlyphCount
		      || (gidx + rule->InputGlyphCount
			  + rule->LookaheadGlyphCount) > gstring->used)
		    continue;
		  if (match_chain_ids (gstring, gidx, flag, rule) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gpos (lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 accumulate);
		  gidx += rule->InputGlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else if (subtable->Format == 2)
	    {
	      OTF_GPOS_ChainContext2 *context2 = &subtable->u.chain_context2;
	      OTF_ChainClassSet *set;
	      unsigned class;
	      int j;
	      int orig_used;

	      class = get_class_def (&context2->InputClassDef, g->glyph_id);
	      set = context2->ChainClassSet + class;
	      for (j = 0; j < set->ChainClassRuleCnt; j++)
		{
		  OTF_ChainClassRule *rule = set->ChainClassRule + j;
		  int k;

		  if (gidx < rule->BacktrackGlyphCount
		      || (gidx + rule->InputGlyphCount
			  + rule->LookaheadGlyphCount) > gstring->used)
		    continue;
		  if (match_chain_classes (gstring, gidx, flag,
					   &context2->BacktrackClassDef,
					   &context2->InputClassDef,
					   &context2->LookaheadClassDef,
					   rule) < 0)
		    continue;
		  orig_used = gstring->used;
		  for (k = 0; k < rule->LookupCount; k++)
		    lookup_gpos (lookup_list,
				 rule->LookupRecord[k].LookupListIndex,
				 gstring,
				 gidx + rule->LookupRecord[k].SequenceIndex,
				 accumulate);
		  gidx += rule->InputGlyphCount + (gstring->used - orig_used);
		  break;
		}
	    }
	  else if (subtable->Format == 3)
	    {
	      OTF_GPOS_ChainContext3 *context3 = &subtable->u.chain_context3;
	      int orig_used;
	      int j;

	      if (gidx < context3->BacktrackGlyphCount
		  || (gidx + context3->InputGlyphCount
		      + context3->LookaheadGlyphCount) > gstring->used)
		continue;
	      if (match_chain_coverages (gstring, gidx, flag, context3) < 0)
		continue;
	      orig_used = gstring->used;
	      for (j = 0; j < context3->LookupCount; j++)
		lookup_gpos (lookup_list,
			     context3->LookupRecord[j].LookupListIndex,
			     gstring,
			     gidx + context3->LookupRecord[j].SequenceIndex,
			     accumulate);
	      gidx += context3->InputGlyphCount + (gstring->used - orig_used);
	    }
	  else
	    OTF_ERROR (OTF_ERROR_GPOS_DRIVE, " (invalid subformat)");
	  break;

	default:
	  continue;
	}
    }
  if (gidx == orig_gidx)
    gidx++;
  return gidx;
}

static int
lookup_encoding_0 (OTF_EncodingSubtable0 *sub0, OTF_GlyphString *gstring)
{
  int i, c;

  for (i = 0; i < gstring->used; i++)
    {
      c = gstring->glyphs[i].c;
      if (c < 0 || c >= 256)
	gstring->glyphs[i].glyph_id = 0;
      else
	gstring->glyphs[i].glyph_id = sub0->glyphIdArray[c];
    }
  return 0;
}

static int
lookup_encoding_2 (OTF_EncodingSubtable2 *sub2, OTF_GlyphString *gstring)
{
  return 0;
}

static int
lookup_encoding_4 (OTF_EncodingSubtable4 *sub4, OTF_GlyphString *gstring)
{
  int i, j, c;
  int segCount = sub4->segCountX2 / 2;

  for (i = 0; i < gstring->used; i++)
    {
      c = gstring->glyphs[i].c;
      if (c < 0)
	gstring->glyphs[i].glyph_id = 0;
      for (j = 0; j < segCount; j++)
	{
	  OTF_cmapSegment *seg = sub4->segments + i;

	  if (c >= seg->startCount && c <= seg->endCount)
	    {
	      if (seg->idRangeOffset == 0xFFFF)
		gstring->glyphs[i].glyph_id = c + seg->idDelta;
	      else
		gstring->glyphs[i].glyph_id
		  = sub4->glyphIdArray[seg->idRangeOffset
				       + (c - seg->startCount)];
	      break;
	    }
	}
    }

  return 0;
}

static int
lookup_encoding_6 (OTF_EncodingSubtable6 *sub6, OTF_GlyphString *gstring)
{
  return 0;
}

static int
lookup_encoding_8 (OTF_EncodingSubtable8 *sub8, OTF_GlyphString *gstring)
{
  return 0;
}

static int
lookup_encoding_10 (OTF_EncodingSubtable10 *sub10, OTF_GlyphString *gstring)
{
  return 0;
}

static int
lookup_encoding_12 (OTF_EncodingSubtable12 *sub12, OTF_GlyphString *gstring)
{
  return 0;
}

static OTF_GlyphID
get_uvs_glyph (OTF_cmap *cmap, OTF_EncodingSubtable14 *sub14, int c1, int c2)
{
  unsigned nRecords = sub14->nRecords;
  OTF_VariationSelectorRecord *record;
  unsigned i;

  for (i = 0; i < nRecords; i++)
    {
      record = &sub14->Records[i];
      if (record->varSelector == c2)
	{
	  if (record->defaultUVSOffset)
	    {
	      OTF_UnicodeValueRange *uVRs = record->unicodeValueRanges;
	      unsigned numUVRs = record->numUnicodeValueRanges;
	      unsigned top = numUVRs, bottom = 0, middle;

	      if (uVRs[0].startUnicodeValue <= c1)
		{
		  unsigned additionalCount, startUnicodeValue;

		  for (;;)
		    {
		      middle = (top + bottom) / 2;
		      if (c1 < uVRs[middle].startUnicodeValue)
			top = middle;
		      else if (bottom == middle)
			break;
		      else
			bottom = middle;
		    }
		  startUnicodeValue = uVRs[bottom].startUnicodeValue;
		  additionalCount = uVRs[bottom].additionalCount;
		  if (c1 <= startUnicodeValue + additionalCount)
		    return cmap->unicode_table[c1];
		}
	    }
	  if (record->nonDefaultUVSOffset)
	    {
	      OTF_UVSMapping *uvsMappings = record->uvsMappings;
	      unsigned numUVSMs = record->numUVSMappings;
	      unsigned top = numUVSMs, bottom = 0, middle;

	      if (uvsMappings[0].unicodeValue <= c1)
		{
		  for (;;)
		    {
		      middle = (top + bottom) / 2;
		      if (c1 < uvsMappings[middle].unicodeValue)
			top = middle;
		      else if (bottom == middle)
			break;
		      else
			bottom = middle;
		    }
		  if (uvsMappings[bottom].unicodeValue == c1)
		    return uvsMappings[bottom].glyphID;
		}
	    }
	  return 0;
	}
    }
  return 0;
}

static void
check_cmap_uvs (OTF_cmap *cmap, OTF_GlyphString *gstring, int idx)
{  
  OTF_EncodingSubtable14 *sub14;
  int c1 = gstring->glyphs[idx - 1].c;
  int c2 = gstring->glyphs[idx].c;
  OTF_GlyphID code;
  int i;

  gstring->glyphs[idx].glyph_id = 0;
  for (i = 0; i < cmap->numTables; i++)
    if (cmap->EncodingRecord[i].subtable.format == 14)
      break;
  if (i == cmap->numTables)
    return;
  code = get_uvs_glyph (cmap, cmap->EncodingRecord[i].subtable.f.f14, c1, c2);
  if (code == 0)
    return;
  gstring->glyphs[idx - 1].glyph_id = code;
  gstring->glyphs[idx - 1].f.index.to = gstring->glyphs[idx].f.index.to;
  gstring->used--;
  memmove (gstring->glyphs + idx, gstring->glyphs + idx + 1,
	   sizeof (OTF_Glyph) * (gstring->used - idx));
}



/* GDEF */
/* Table of GlyphClass and MarkAttackClass.

   For the Nth element CHAR, CHAR and the succeeding characters
   (before CHAR of the next element) has GlyphClass C (= (N % 2) ? 3 : 1).

   This table is generated from the General Category (GC) property of
   characters defined in the Unicode Character Database.  */

static int glyph_class_table[] =
  { 0x00000, 0x00300, 0x00370, 0x00483, 0x00487, 0x00488, 0x0048A, 0x00591,
    0x005BE, 0x005BF, 0x005C0, 0x005C1, 0x005C3, 0x005C4, 0x005C6, 0x005C7,
    0x005C8, 0x00610, 0x00616, 0x0064B, 0x0065F, 0x00670, 0x00671, 0x006D6,
    0x006DD, 0x006DE, 0x006E5, 0x006E7, 0x006E9, 0x006EA, 0x006EE, 0x00711,
    0x00712, 0x00730, 0x0074B, 0x007A6, 0x007B1, 0x007EB, 0x007F4, 0x00901,
    0x00904, 0x0093C, 0x0093D, 0x0093E, 0x0094E, 0x00951, 0x00955, 0x00962,
    0x00964, 0x00981, 0x00984, 0x009BC, 0x009BD, 0x009BE, 0x009C5, 0x009C7,
    0x009CE, 0x009D7, 0x009D8, 0x009E2, 0x009E4, 0x00A01, 0x00A04, 0x00A3C,
    0x00A3D, 0x00A3E, 0x00A4E, 0x00A70, 0x00A72, 0x00A81, 0x00A84, 0x00ABC,
    0x00ABD, 0x00ABE, 0x00ACE, 0x00AE2, 0x00AE4, 0x00B01, 0x00B04, 0x00B3C,
    0x00B3D, 0x00B3E, 0x00B44, 0x00B47, 0x00B58, 0x00B82, 0x00B83, 0x00BBE,
    0x00BCE, 0x00BD7, 0x00BD8, 0x00C01, 0x00C04, 0x00C3E, 0x00C45, 0x00C46,
    0x00C57, 0x00C82, 0x00C84, 0x00CBC, 0x00CBD, 0x00CBE, 0x00CC5, 0x00CC6,
    0x00CCE, 0x00CD5, 0x00CD7, 0x00CE2, 0x00CE4, 0x00D02, 0x00D04, 0x00D3E,
    0x00D44, 0x00D46, 0x00D4E, 0x00D57, 0x00D58, 0x00D82, 0x00D84, 0x00DCA,
    0x00DCB, 0x00DCF, 0x00DD7, 0x00DD8, 0x00DF4, 0x00E31, 0x00E32, 0x00E34,
    0x00E3B, 0x00E47, 0x00E4F, 0x00EB1, 0x00EB2, 0x00EB4, 0x00EBD, 0x00EC8,
    0x00ECE, 0x00F18, 0x00F1A, 0x00F35, 0x00F36, 0x00F37, 0x00F38, 0x00F39,
    0x00F3A, 0x00F3E, 0x00F40, 0x00F71, 0x00F85, 0x00F86, 0x00F88, 0x00F90,
    0x00FBD, 0x00FC6, 0x00FC7, 0x0102C, 0x0103A, 0x01056, 0x0105A, 0x0135F,
    0x01360, 0x01712, 0x01715, 0x01732, 0x01735, 0x01752, 0x01754, 0x01772,
    0x01774, 0x017B6, 0x017D4, 0x017DD, 0x017DE, 0x0180B, 0x0180E, 0x018A9,
    0x018AA, 0x01920, 0x0193C, 0x019B0, 0x019C1, 0x019C8, 0x019CA, 0x01A17,
    0x01A1C, 0x01B00, 0x01B05, 0x01B34, 0x01B45, 0x01B6B, 0x01B74, 0x01DC0,
    0x01E00, 0x020D0, 0x020F0, 0x0302A, 0x03030, 0x03099, 0x0309B, 0x0A802,
    0x0A803, 0x0A806, 0x0A807, 0x0A80B, 0x0A80C, 0x0A823, 0x0A828, 0x0FB1E,
    0x0FB1F, 0x0FE00, 0x0FE10, 0x0FE20, 0x0FE24, 0x10A01, 0x10A10, 0x10A38,
    0x10A40, 0x1D165, 0x1D16A, 0x1D16D, 0x1D173, 0x1D17B, 0x1D183, 0x1D185,
    0x1D18C, 0x1D1AA, 0x1D1AE, 0x1D242, 0x1D245, 0xE0100, 0xE01F0 };

int get_class_def_auto (int c)
{
  static int table_size
    = sizeof glyph_class_table / sizeof glyph_class_table[0];
  int low, high, mid;

  if (c >= glyph_class_table[table_size - 1])
    return 0;
  low = 0;
  high = table_size - 1;
  while (1)
    {
      mid = (low + high) / 2;
      if (c < glyph_class_table[mid])
	high = mid - 1;
      else if (c >= glyph_class_table[mid + 1])
	low = mid + 1;
      else
	break;
    }
  return ((mid % 2) ? 3 : 1);
}



/* API */

#define UVS_P(C)	\
  (((C) >= 0xFE00 && (C) <= 0xFE0F) || ((C) >= 0xE0100 && (C) <= 0xE01EF))

int
OTF_drive_cmap (OTF *otf, OTF_GlyphString *gstring)
{
  OTF_cmap *cmap;
  int i;

  if (! otf->cmap
      && OTF_get_table (otf, "cmap") < 0)
    return -1;

  cmap = otf->cmap;
  for (i = 0; i < gstring->used; i++)
    if (! gstring->glyphs[i].glyph_id)
      {
	int c = gstring->glyphs[i].c;
	if (c < 32 || ! cmap->unicode_table)
	  gstring->glyphs[i].glyph_id = 0;
	else if (UVS_P (c) && i > 0)
	  check_cmap_uvs (cmap, gstring, i);
	else
	  gstring->glyphs[i].glyph_id = cmap->unicode_table[c];
      }
  return 0;
}


int
OTF_drive_cmap2 (OTF *otf, OTF_GlyphString *gstring,
		 int platform_id, int encoding_id)
{
  OTF_cmap *cmap;
  int i;
  char *errfmt = "CMAP Looking up%s";
  int errret = -1;
  OTF_EncodingRecord *enc;

  if (! otf->cmap
      && OTF_get_table (otf, "cmap") < 0)
    return -1;

  cmap = otf->cmap;
  for (i = 0; i < cmap->numTables; i++)
    if (cmap->EncodingRecord[i].platformID == platform_id
	&& cmap->EncodingRecord[i].encodingID == encoding_id)
      break;
  if (i == cmap->numTables)
    OTF_ERROR (OTF_ERROR_CMAP_DRIVE, " (unknown platformID/encodingID)");
  enc = cmap->EncodingRecord + i;
  switch (enc->subtable.format)
    {
    case 0: return lookup_encoding_0 (enc->subtable.f.f0, gstring);
    case 2: return lookup_encoding_2 (enc->subtable.f.f2, gstring);
    case 4: return lookup_encoding_4 (enc->subtable.f.f4, gstring);
    case 6: return lookup_encoding_6 (enc->subtable.f.f6, gstring);
    case 8: return lookup_encoding_8 (enc->subtable.f.f8, gstring);
    case 10: return lookup_encoding_10 (enc->subtable.f.f10, gstring);
    case 12: return lookup_encoding_12 (enc->subtable.f.f12, gstring);
    }
  OTF_ERROR (OTF_ERROR_CMAP_DRIVE, " (invalid format)");
}


int
OTF_get_unicode (OTF *otf, OTF_GlyphID code)
{
  if (! otf->cmap
      && OTF_get_table (otf, "cmap") < 0)
    return 0;
  if (code == 0
      || code > otf->cmap->max_glyph_id
      || ! otf->cmap->decode_table)
    return 0;
  return otf->cmap->decode_table[code];
}

int
OTF_get_variation_glyphs (OTF *otf, int c, OTF_GlyphID code[256])
{
  int i, n;
  OTF_cmap *cmap;
  OTF_EncodingSubtable14 *sub14;

  memset (code, 0, sizeof (OTF_GlyphID) * 256);
  if (! otf->cmap
      && OTF_get_table (otf, "cmap") < 0)
    return 0;
  cmap = otf->cmap;
  for (i = 0; i < cmap->numTables; i++)
    if (cmap->EncodingRecord[i].subtable.format == 14)
      break;
  if (i == cmap->numTables)
    return 0;
  sub14 = cmap->EncodingRecord[i].subtable.f.f14;
  for (i = 0, n = 0; i < 256; i++)
    {
      int uvs = (i < 16 ? 0xFE00 + i : 0xE0100 + (i - 16));

      if ((code[i] = get_uvs_glyph (cmap, sub14, c, uvs)))
	n++;
    }
  return n;
}


int
OTF_drive_gdef (OTF *otf, OTF_GlyphString *gstring)
{
  OTF_GDEF *gdef;
  int i;

  if (! otf->gdef
      && OTF_get_table (otf, "GDEF") < 0)
    return -1;
  gdef = otf->gdef;

  if (gdef->glyph_class_def.offset)
    for (i = 0; i < gstring->used; i++)
      gstring->glyphs[i].GlyphClass
	= get_class_def (&gdef->glyph_class_def,
			 gstring->glyphs[i].glyph_id);
  else
    for (i = 0; i < gstring->used; i++)
      gstring->glyphs[i].GlyphClass
	= get_class_def_auto (gstring->glyphs[i].c);

  if (gdef->mark_attach_class_def.offset)
    for (i = 0; i < gstring->used; i++)
      gstring->glyphs[i].MarkAttachClass
	= get_class_def (&gdef->mark_attach_class_def,
			 gstring->glyphs[i].glyph_id);

  return 0;
}

static int
OTF_drive_gsub_internal (OTF *otf, OTF_GlyphString *gstring,
			 const char *script, const char *language,
			 const char *features,
			 int alternate_subst)
{
  char *errfmt = "GSUB driving%s";
  int errret = -1;
  OTF_GSUB *gsub;
  OTF_LangSys *LangSys;
  char *lookup_flags;
  int i;

  for (i = 0; i < gstring->used; i++)
    {
      gstring->glyphs[i].positioning_type = 0;
      gstring->glyphs[i].f.index.from = gstring->glyphs[i].f.index.to = i;
    }

  if (OTF_get_table (otf, "GSUB") < 0)
    return errret;
  gsub = otf->gsub;
  if (gsub->FeatureList.FeatureCount == 0
      || gsub->LookupList.LookupCount == 0)
    return 0;

  LangSys = get_langsys (&gsub->ScriptList, script, language);
  if (! LangSys)
    return errret;

  lookup_flags = alloca (gsub->LookupList.LookupCount);
  if (! lookup_flags
      || setup_lookup_flags (&gsub->LookupList, &gsub->FeatureList, LangSys,
			     features, lookup_flags) < 0)
    OTF_ERROR (OTF_ERROR_MEMORY, " feature list");

  for (i = 0; i < gsub->LookupList.LookupCount; i++)
    {
      int gidx;

      if (! lookup_flags[i]) continue;

      if (gsub->LookupList.Lookup[i].LookupType != 8)
	{
	  gidx = 0;
	  while (gidx < gstring->used)
	    {
	      gidx = lookup_gsub (otf, &gsub->LookupList, i, gstring, gidx,
				  alternate_subst);
	      if (gidx < 0)
		return errret;
	    }
	}
      else
	{
	  gidx = gstring->used - 1;
	  while (gidx >= 0)
	    {
	      gidx = lookup_gsub (otf, &gsub->LookupList, i, gstring, gidx,
				  alternate_subst);
	      if (gidx < 0)
		return errret;
	    }
	}
    }

  return 0;
}

int
OTF_drive_gsub (OTF *otf, OTF_GlyphString *gstring,
		const char *script, const char *language, const char *features)
{
  if (! otf->cmap)
    OTF_get_table (otf, "cmap");
  return OTF_drive_gsub_internal (otf, gstring, script, language, features, 0);
}

int
OTF_drive_gpos_internal (OTF *otf, OTF_GlyphString *gstring,
			 const char *script, const char *language,
			 const char *features,
			 int accumulate)
{
  char *errfmt = "GPOS driving%s";
  int errret = -1;
  OTF_GPOS *gpos;
  OTF_LangSys *LangSys;
  char *lookup_flags;
  int i, n;

  for (i = 0; i < gstring->used; i++)
    gstring->glyphs[i].positioning_type = 0;

  if (OTF_get_table (otf, "GPOS") < 0)
    return errret;
  gpos = otf->gpos;
  if (gpos->FeatureList.FeatureCount == 0
      || gpos->LookupList.LookupCount == 0)
    return 0;

  LangSys = get_langsys (&gpos->ScriptList, script, language);
  if (! LangSys)
    return errret;

  lookup_flags = alloca (gpos->LookupList.LookupCount);
  if (! lookup_flags
      || setup_lookup_flags (&gpos->LookupList, &gpos->FeatureList, LangSys,
			     features, lookup_flags) < 0)
    OTF_ERROR (OTF_ERROR_MEMORY, " feature list");

  for (i = 0; i < gpos->LookupList.LookupCount; i++)
    {
      int gidx = 0;

      if (! lookup_flags[i]) continue;

      while (gidx < gstring->used)
	{
	  gidx = lookup_gpos (&gpos->LookupList, i, gstring, gidx, accumulate);
	  if (gidx < 0)
	    return errret;
	}
    }

  return 0;
}

int
OTF_drive_gpos (OTF *otf, OTF_GlyphString *gstring,
		const char *script, const char *language, const char *features)
{
  if (! otf->cmap)
    OTF_get_table (otf, "cmap");
  return OTF_drive_gpos_internal (otf, gstring, script, language, features, 0);
}

int
OTF_drive_gpos2 (OTF *otf, OTF_GlyphString *gstring,
		const char *script, const char *language, const char *features)
{
  if (! otf->cmap)
    OTF_get_table (otf, "cmap");
  return OTF_drive_gpos_internal (otf, gstring, script, language, features, 1);
}

int
OTF_drive_tables (OTF *otf, OTF_GlyphString *gstring,
		  const char *script, const char *language,
		  const char *gsub_features, const char *gpos_features)
{
  if (OTF_drive_cmap (otf, gstring) < 0)
    return -1;
  if (OTF_drive_gdef (otf, gstring) < 0)
    return -1;
  if (gsub_features
      && OTF_drive_gsub (otf, gstring, script, language, gsub_features) < 0)
    return -1;
  if (gpos_features
      && OTF_drive_gpos (otf, gstring, script, language, gpos_features) < 0)
    return -1;
  return 0;
}

int
OTF_drive_gsub_alternate (OTF *otf, OTF_GlyphString *gstring,
			  const char *script, const char *language,
			  const char *features)
{
  return OTF_drive_gsub_internal (otf, gstring, script, language, features, 1);
}
