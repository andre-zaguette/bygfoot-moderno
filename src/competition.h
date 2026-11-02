/*
   competition.h

   Bygfoot Football Manager -- a small and simple GTK2-based
   football management game.

   http://bygfoot.sourceforge.net

   Copyright (C) 2023  Tom Stellard (tom@stellad.net)

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

#ifndef COMPETITION_H
#define COMPETITION_H

#include "competition_struct.h"
#include "cup_struct.h"
#include "league_struct.h"

typedef struct
{
    const Competition *comp;
    union {
        CupTeamsIterator cup;
        LeagueTeamsIterator league;
    } it;
} CompetitionTeamsIterator;

gboolean
clid_is_league(gint clid);

gboolean
clid_is_cup(gint clid);

Competition *
competition_get_from_clid(gint clid);

gboolean
competition_is_league(const Competition *c);

gboolean
competition_is_cup(const Competition *c);

GPtrArray *
competition_get_teams(const Competition *c);

Team *
competition_teams_begin(const Competition *comp, CompetitionTeamsIterator *it);

Team *
competition_teams_next(CompetitionTeamsIterator *it);

gint
competition_get_num_teams(const Competition *c);

gboolean
competition_has_property(const Competition *c, const gchar *property);
#endif
