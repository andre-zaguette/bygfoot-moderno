/*
   competition.c

   Bygfoot Football Manager -- a small and simple GTK2-based
   football management game.

   http://bygfoot.sourceforge.net

   Copyright (C) 2023 Tom Stellard (tom@stellard.net)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "bygfoot.h"
#include "cup.h"
#include "competition.h"
#include "league.h"
#include "misc.h"

gboolean
clid_is_league(gint clid)
{
    return clid < ID_CUP_START;
}

gboolean
clid_is_cup(gint clid)
{
    return clid >= ID_CUP_START;
}

Competition *
competition_get_from_clid(gint clid)
{
    if (clid_is_league(clid))
        return &league_from_clid(clid)->c;
    return &cup_from_clid(clid)->c;
}

gboolean
competition_is_league(const Competition *c)
{
    return clid_is_league(c->id);
}

gboolean
competition_is_cup(const Competition *c)
{
    return clid_is_cup(c->id);
}

Team *
competition_teams_begin(const Competition *comp, CompetitionTeamsIterator *it)
{
    it->comp = comp;
    if (competition_is_cup(comp)) {
        return cup_teams_begin((const Cup *)comp, &it->it.cup);
    } else {
        return league_teams_begin((const League *)comp, &it->it.league);
    }
}

Team *
competition_teams_next(CompetitionTeamsIterator *it)
{
    if (competition_is_cup(it->comp)) {
        return cup_teams_next(&it->it.cup);
    } else {
        return league_teams_next(&it->it.league);
    }
}

gint
competition_get_num_teams(const Competition *c)
{
    if (competition_is_cup(c))
        return cup_get_num_teams((const Cup *)c);

    return ((const League *)c)->teams->len;
}

gboolean
competition_has_property(const Competition *c, const gchar *property)
{
    return query_misc_string_in_array(property, c->properties);
}
