/*
   cup.h

   Bygfoot Football Manager -- a small and simple GTK2-based
   football management game.

   http://bygfoot.sourceforge.net

   Copyright (C) 2005  Gyözö Both (gyboth@bygfoot.com)

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

#ifndef CUP_H
#define CUP_H

#include "bygfoot.h"
#include "cup_struct.h"
#include "fixture_struct.h"
#include "league_struct.h"

#define cup_get_last_tables_round(cup) &g_array_index((cup)->rounds, CupRound, cup_has_tables((cup)))
#define cup_get_last_tables(cup) g_array_index((cup)->rounds, CupRound, cup_has_tables((cup))).tables

/* Helper macro to iterate through national and international cups. */

#define __get_cup(bygfoot, i) \
    (i < cps->len ? cp(i) : (i - cps->len < bygfoot->international_cups->len ? \
                           g_ptr_array_index(bygfoot->international_cups, i - cps->len) : \
                           NULL))

#define foreach_available_cup(bygfoot, i, cup) \
    for (i = 0, cup = __get_cup(bygfoot, i); cup; i++, cup = __get_cpu(bygfoot, i))

Cup
cup_new(gboolean new_id, Bygfoot *bygfoot);

CupChooseTeam
cup_choose_team_new(void);

CupRound
cup_round_new(void);

void
cup_reset(Cup *cup);

void
cup_get_team_pointers(Cup *cup, gint round, gboolean preload);

void
cup_load_choose_team(Cup *cup, GPtrArray *teams, const CupChooseTeam *ct);

void
cup_load_choose_team_from_league(Cup *cup, const League *league,
                                 GPtrArray *teams, const CupChooseTeam *ct);

void
cup_load_choose_team_from_cup(Cup *cup, const Cup *cup_team, GPtrArray *teams, 
                              const CupChooseTeam *ct);

gint
cup_get_first_week_of_cup_round(Cup *cup, gint cup_round, gboolean with_delay);

gint
cup_get_matchdays_in_cup_round(const Cup *cup, gint round);

gint
cup_round_get_number_of_teams(const Cup *cup, gint round);

gint
cup_round_get_byes(const Cup *cup, gint round);

gint
cup_round_get_new_teams(const CupRound *cup_round);

Cup*
cup_from_clid(gint clid);

Cup*
cup_from_sid(const gchar* sid);

void
cup_round_get_name(const Fixture *fix, gchar *buf);

GPtrArray*
cup_get_teams_sorted(const Cup *cup);

gint
cup_compare_success_tables(const Team *tm1, const Team *tm2, Cup *cup, gint round);

gint
cup_compare_success(gconstpointer a, gconstpointer b, gpointer data);

gint
cup_compare_success_knockout(const Team *tm1, const Team *tm2, const GArray *fixtures);

gint
cup_get_round_reached(const Team *tm, const GArray *fixtures);

gboolean
query_cup_begins(const Cup *cup);

gint
cup_choose_team_compute_start_idx(const CupChooseTeam *ct);

gint
cup_choose_team_compute_end_idx(const CupChooseTeam *ct, gint num_teams);

void
cup_get_choose_team_league_cup(const CupChooseTeam *ct, 
			       const League **league, const Cup **cup);

gint
cup_get_last_week_from_first(const Cup *cup, gint first_week);

void
cup_get_round_name(const Cup *cup, gint round, gchar *buf);

gint
cup_has_tables(const Cup *cup);

Team*
cup_get_winner(const Cup *cup);

gchar*
cup_get_highlight_colour(const Cup *cup);

gboolean
cup_check_fixtures(const Cup *cup);

gboolean
cup_round_check_waits(const CupRound *cup_round);

gboolean
query_cup_self_referential(const Cup *cup);

gboolean
query_cup_hidden(const Cup *cup);

GPtrArray *
cup_get_last_season_results(const Cup *cup);

GPtrArray *
cup_get_most_recent_results(const Cup *cup);

gboolean
cup_is_international(const Cup *cup);

Cup *
cup_get_next_cup(gint clid);

Team *
cup_teams_begin(const Cup *cup, CupTeamsIterator *it);

Team *
cup_teams_next(CupTeamsIterator *it);

gint
cup_get_num_teams(const Cup *cup);

gint
cup_get_last_completed_round(const Cup *cup);

#endif
