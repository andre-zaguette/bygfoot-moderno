/*
   live_game.h

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

#ifndef LIVE_GAME_H
#define LIVE_GAME_H

#include "bygfoot.h"
#include "live_game_struct.h"

void
live_game_calculate_fixture(Fixture *fix, LiveGame *live_game, Bygfoot *bygfoot);

void
live_game_initialize(Fixture *fix, LiveGame *live_game, Bygfoot *bygfoot);

gboolean
query_live_game_event_is_break(LiveGame *live_game, gint minute, gint time);

gboolean
query_live_game_penalties_over(LiveGame *live_game);

gint
live_game_get_break(LiveGame *live_game);

gint
live_game_get_time(const LiveGameUnit *unit);

gint
live_game_get_minute(const LiveGame *live_game);

gint
live_game_get_minutes_remaining(const LiveGame *live_game, const LiveGameUnit *unit);

void
live_game_create_unit(LiveGame *live_game, GUI *gui);

void
live_game_fill_new_unit(LiveGame *live_game, LiveGameUnit *new);

void
live_game_create_start_unit(LiveGame *live_game);

void
live_game_evaluate_unit(LiveGame *live_game, GUI *gui, LiveGameUnit *unit);

void
live_game_event_foul(LiveGame *live_game, GUI *gui);

void
live_game_event_lost_possession(LiveGame *live_game, GUI *gui);

void
live_game_event_injury(LiveGame *live_game, GUI *gui, gint team, gint player, gboolean create_new);

void
live_game_event_stadium(LiveGame *live_game, GUI *gui);

void
live_game_event_scoring_chance(LiveGame *live_game, GUI *gui);

void
live_game_event_penalty(LiveGame *live_game, GUI *gui);

void
live_game_event_general(LiveGame *live_game, GUI *gui, gboolean create_new);

void
live_game_event_general_get_players(LiveGame *live_game);

void
live_game_event_free_kick(LiveGame *live_game, GUI *gui);

void
live_game_event_send_off(LiveGame *live_game, GUI *gui, gint team, Player *player, gboolean second_yellow);

void
live_game_event_duel(LiveGame *live_game, GUI *gui);

void
live_game_event_corner_kick(LiveGame *live_game, GUI *gui);

gboolean
query_live_game_second_yellow(LiveGame *live_game, gint team, gint player);

void
live_game_reset(LiveGame *live_game, Fixture *fix, gboolean free_variable);

gint
live_game_unit_get_minute(const LiveGame *live_game, const LiveGameUnit *unit);

gint
live_game_get_area(LiveGame *live_game, const LiveGameUnit *unit);

gfloat
live_game_pit_teams(LiveGame *live_game, const LiveGameUnit *unit, gfloat exponent);

void
live_game_injury_get_player(LiveGame *live_game);

void
live_game_resume(LiveGame *live_game, Bygfoot *bygfoot);

void
live_game_event_substitution(LiveGame *live_game, gint team_number, gint sub_in, gint sub_out);

void
live_game_event_team_change(LiveGame *live_game, gint team_number, gint event_type);

void
live_game_finish_unit(LiveGame *live_game);

LiveGameUnit*
live_game_unit_before(LiveGame *live_game, const LiveGameUnit* unit, gint gap);

gint
live_game_event_get_verbosity(gint event_type);

void
live_game_unit_result_to_buf(const LiveGameUnit *unit, gchar *buf, gboolean swap);

gint
live_game_penalties_taken(LiveGame *live_game);

gboolean
live_game_player_is_cautioned(const LiveGame *live_game, const Player *player);

gint
live_game_caution_player(LiveGame *live_game, Player *player);

gint
live_game_team_get_num_cautioned_players(const LiveGame *live_game,
                                         const Team *team);

gint
live_game_team_get_num_sent_off_players(const LiveGame *live_game,
                                        gint team_index);

#endif
