/*
   start_end.c

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

#include "bet.h"
#include "competition.h"
#include "cup.h"
#include "file.h"
#include "finance.h"
#include "fixture.h"
#include "free.h"
#include "game_gui.h"
#include "gui.h"
#include "job.h"
#include "league.h"
#include "live_game.h"
#include "load_save.h"
#include "main.h"
#include "maths.h"
#include "misc.h"
#include "name.h"
#include "news.h"
#include "option.h"
#include "player.h"
#include "start_end.h"
#include "stat.h"
#include "table.h"
#include "team.h"
#include "transfer.h"
#include "user.h"
#include "variables.h"
#include "xml_country.h"
#include "xml_cup.h"
#include "xml_name.h"
#include "youth_academy.h"

/** Prototype of a function called at the start or
    end of a week round. */
typedef void(*WeekFunc)(void);
typedef void(*WeekFuncBygfoot)(Bygfoot *);

/** Array of functions called when a week round
    is ended. */
WeekFuncBygfoot end_week_round_funcs[] =
{end_week_round_results, end_week_round_sort_tables,
 end_week_round_generate_news, end_week_round_update_fixtures, NULL};

/** Array of functions called when a week round
    is started. */
WeekFunc start_week_round_funcs[] =
{start_week_round_update_teams, bet_update, NULL};

/** Array of functions called when a week
    is started. */
WeekFuncBygfoot start_week_funcs[] =
{start_week_update_leagues, start_week_add_cups, start_week_update_users,
 start_week_update_teams, start_week_update_user_finances,
 youth_academy_update_weekly, transfer_update, job_update,
 finance_update_current_interest, NULL};

WeekFunc end_week_funcs[] = {stat_update_leagues, end_week_hide_cups, NULL};

/** Generate the teams etc. */
void
start_new_game(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_new_game\n");
#endif
    gint i;

    start_write_variables();

    start_generate_league_teams();

    start_load_other_countries(bygfoot);

    start_load_international_cups(bygfoot);

    start_generate_cup_history(country.cups);

    for (i = 0; i < country_list->len; i++) {
        Country *c = g_ptr_array_index(country_list, i);
        start_generate_cup_history(c->cups);
    }

    start_generate_cup_history(bygfoot->international_cups);

    start_new_season(bygfoot);
}

/** Load other countries that the user isn't playing, so we can easily reference
 * the cups and leagues.
 */
void
start_load_other_countries(Bygfoot *bygfoot)
{
    GPtrArray *country_files = file_get_country_files();
    country_list = g_ptr_array_new();
    gint i;

    for (i = 0; i < country_files->len; i++) {
        gint j;
        const gchar *country_file = g_ptr_array_index(country_files, i);
        Country *new_country = g_malloc0(sizeof(Country));

        xml_country_read(country_file, new_country, bygfoot);
        if (!strcmp(new_country->sid, country.sid))
            continue;
        for (j = 0; j < new_country->leagues->len; j++) {
            League *league = g_ptr_array_index(new_country->leagues, j);
            gint k;
            for (k = 0; k < league->teams->len; k++) {
                Team *team = g_ptr_array_index(league->teams, k);
	            team_generate_players_stadium(team);
            }
	    /* We need to add the tables because they are used by
	     * ChooseTeams that select teams for international cups. */
            league_add_table(league);
        }
        g_ptr_array_add(country_list, new_country);
    }
}

void
start_load_international_cups(Bygfoot *bygfoot)
{
    gint i;
    GPtrArray *cup_files = file_get_international_cup_files();

    for (i = 0; i < cup_files->len; i++) {
        const gchar *cup_file = g_ptr_array_index(cup_files, i);
        xml_cup_read(cup_file, bygfoot->international_cups, bygfoot);
    }
    free_gchar_array(&cup_files);
}

static void
generate_cup_history_leagues(Cup *cup)
{
    gint j, k, m, t;
    gint number_of_teams;

    for(j = cup->rounds->len - 1; j  >= 0; j--) {
        CupRound *round = &g_array_index(cup->rounds, CupRound, j);
        for (k = 0; k < round->choose_teams->len; k++) {
            gint start_idx, end_idx;
            GPtrArray *teams = g_ptr_array_new();
            CupChooseTeam *ct = &g_array_index(round->choose_teams, CupChooseTeam, k);
            League *league = bygfoot_get_league_sid(ct->sid);
            Cup *ct_cup;
            if (!league)
                continue;
            for (t = 0; t < league->teams->len; t++) {
                g_ptr_array_add(teams, g_ptr_array_index(league->teams, t));
            }
            number_of_teams = ct->number_of_teams == -1 ? teams->len : ct->number_of_teams;
            t = 0;
            start_idx = cup_choose_team_compute_start_idx(ct);
            end_idx = cup_choose_team_compute_end_idx(ct, teams->len);
            for (m = start_idx; m <= end_idx && m < teams->len && t < number_of_teams; m++) { 
                Team *team = g_ptr_array_index(teams, m);
                if(!ct->skip_group_check && query_team_is_in_cups(team, cup->group))
                    continue;
                g_ptr_array_add(round->team_ptrs, team);
                t++;
            }
            g_ptr_array_free(teams, TRUE);
        }
    }
}

static void
generate_cup_history_cups(Cup *cup)
{
    int j, k, t, number_of_teams, m;
    GPtrArray *cup_teams = g_ptr_array_new();
    CupTeamsIterator it;
    Team *team;
    for(j = cup->rounds->len - 1; j  >= 0; j--) {
        CupRound *round = &g_array_index(cup->rounds, CupRound, j);
        for (k = 0; k < round->choose_teams->len; k++) {
            gint start_idx, end_idx;
            GPtrArray *teams = g_ptr_array_new();
            CupChooseTeam *ct = &g_array_index(round->choose_teams, CupChooseTeam, k);
            Cup *ct_cup = bygfoot_get_cup_sid(ct->sid);
            if (!ct_cup)
                continue;
            for (team = cup_teams_begin(ct_cup, &it); team;
                 team = cup_teams_next(&it)) {
                g_ptr_array_add(teams, team);
            }
            number_of_teams = ct->number_of_teams == -1 ? teams->len : ct->number_of_teams;
            t = 0;
            start_idx = cup_choose_team_compute_start_idx(ct);
            end_idx = cup_choose_team_compute_end_idx(ct, teams->len);
            for (m = start_idx; m <= end_idx && m < teams->len && t < number_of_teams; m++) { 
                Team *team = g_ptr_array_index(teams, m);
                if(!ct->skip_group_check && query_team_is_in_cups(team, cup->group))
                    continue;
                g_ptr_array_add(round->team_ptrs, team);
                t++;
            }
            g_ptr_array_free(teams, TRUE);
        }
    }

    for (team = cup_teams_begin(cup, &it); team; team = cup_teams_next(&it)) {
        g_ptr_array_add(cup_teams, team);
    }
    cup_teams = misc_randomise_g_pointer_array(cup_teams);
    g_ptr_array_add(cup->history, cup_teams);
}

/** Generate cup results so that in the first season we can select the cup
 * winners using the same method we do for the later seasons.
 *
 */
void
start_generate_cup_history(GPtrArray *cups)
{
    gint i;

    /* First pass: Collect all the teams that qualify for a cup from a league. */
    for (i = cups->len - 1; i >=0; i--) {
        Cup *cup = g_ptr_array_index(cups, i);
        generate_cup_history_leagues(cup);
    }

    /* Second pass: Collect all the teams that qualify for a cup from another cup. */
    for (i = cups->len - 1; i >=0; i--) {
        Cup *cup = g_ptr_array_index(cups, i);
        generate_cup_history_cups(cup);
    }
}

/** Make new fixtures, nullify things etc. */
void
start_new_season(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_new_season\n");
#endif

    gint i, j;
    gchar buf[SMALL];

    week = week_round = 1;
    free_names(TRUE);
    stat5 = STATUS_GENERATE_TEAMS;

    /* Randomize league lists and cup results for non-user countries so we get
     * different teams in the international cups.  Adding the extra history
     * value to the cups also makes it possible for cup_get_last_season_results
     * to return the correct value for non-user country's cups.  */
    for (i = 0; i < country_list->len; i++) {
        Country *country = g_ptr_array_index(country_list, i);
        for (j = 0; j < country->leagues->len; j++) {
            League *league = g_ptr_array_index(country->leagues, j);
            league->teams = misc_randomise_g_pointer_array(league->teams);
        }

        for (j = 0; j < country->cups->len; j++) {
            Cup *cup = g_ptr_array_index(country->cups, j);
            GPtrArray *new_history =
                misc_copy_ptr_array(g_ptr_array_index(cup->history, 0));
            g_ptr_array_add(cup->history,
                            misc_randomise_g_pointer_array(new_history));
        }
    }

    /* Generate a new cup history for each cup for non-user countries. */

    /* Reset all cups.  We need to make sure all cups get reset
     * before the start of the next season.  Otherwise, the fixtures
     * from last year's cup will interfere with scheduling the league
     * fixtures.  Also, cup_reset() saves this season's results in the
     * history list, and it makes it more simple to fetch last season's
     * results if all cups are reset at the same time. */
    for (i = 0; i < country.cups->len; i++) {
        Cup *cup = g_ptr_array_index(country.cups, i);
        cup_reset(cup);
    }

    for (i = 0; i < bygfoot->international_cups->len; i++) {
        Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        cup_reset(cup);
    }

    if(season == 1)
    {
        for(i=0;i<country.leagues->len;i++)
        {
            League *league = g_ptr_array_index(country.leagues, i);
            league_add_table(league);
            league_check_new_tables(league);
        }

	for(i=0;i<country.cups->len;i++) {
            Cup *cup = g_ptr_array_index(country.cups, i);
	    if(cup->add_week <= 0)
		g_ptr_array_add(country.allcups, cup);
        }

        for(i = 0; i < bygfoot->international_cups->len; i++) {
            Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
            if (cup->add_week <= 0)
                g_ptr_array_add(country.allcups, cup);
        }
    }

    /* Remove cups that don't start at the beginning of season. */
    for(i = country.allcups->len - 1; i >= 0; i--)
    {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	if(cup->add_week > 0)
	    g_ptr_array_remove_index(country.allcups, i);
    }

    for(i=country.cups->len - 1; i >= 0; i--) {
        Cup *cup = g_ptr_array_index(country.cups, i);

        /* Deal with cups that have to take place before promotion/relegation. */
	if(cup->add_week == -1)
	    fixture_write_cup_fixtures(cup);
    }

    for (i = bygfoot->international_cups->len - 1; i >=0; i--) {
        Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        if (cup->add_week == -1) 
            fixture_write_cup_fixtures(cup);
    }
    if(season > 1)
    {
	for(i=0;i<users->len;i++)
	{
	    sprintf(buf, "%d", team_get_league_rank(usr(i).tm, NULL));
	    user_history_add(&usr(i), USER_HISTORY_END_SEASON,
			     usr(i).tm->name, 
			     usr(i).tm->league->c.name, 
			     buf, NULL);

	    usr(i).counters[COUNT_USER_TRAININGS_LEFT_SEASON] =
		const_int("int_training_camps_per_season");
	}

        /* Promotion/relegation, mainly. */
	start_new_season_league_changes();

	for(i=0;i<users->len;i++)
	{
	    usr(i).youth_academy.tm = usr(i).tm;
	    for(j=0;j<usr(i).youth_academy.players->len;j++)
	    {
                Player *player =
                    g_ptr_array_index(usr(i).youth_academy.players, j);
                player->team = usr(i).tm;
                player_season_start(player, 0);
            }

	    live_game_reset(&usr(i).live_game, NULL, TRUE);
	}

	start_new_season_reset_ids();
    }
    
    /* We have to reset all fixture arrays beforehand because
       of interleague scheduling (see joined_league). */
    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(query_league_active(league))
        {
            g_array_free(league->fixtures, TRUE);
            league->fixtures = g_array_new(FALSE, FALSE, sizeof(Fixture));            
        }
    }

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(query_league_active(league))
	    fixture_write_league_fixtures(league);
    }

    /* Loop backwards because the national cup is usually
       the first the results are often still 
       needed for the international cups. */
    for (i = bygfoot->international_cups->len - 1; i >=0; i--) {
        Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        if (cup->add_week == 0)
            fixture_write_cup_fixtures(cup);
    }

    for(i=country.cups->len - 1; i >= 0; i--)
    {
        Cup *cup = g_ptr_array_index(country.cups, i);
        if(cup->add_week == 0)
            fixture_write_cup_fixtures(cup);
    }

    for(i = country.allcups->len - 1; i >= 0; i--) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
        if(!cup_check_fixtures(cup))
        {
            g_ptr_array_remove_index(country.allcups, i);
        }
    }

    stat5 = -1;

    if(season == 1)
	bet_update();
}

/** Fill some global variables with default values at the
    beginning of a new game. */
void
start_write_variables(void)
{
#ifdef DEBUG
    printf("start_write_variables\n");
#endif

    gint i;

    season = 1;
    cur_user = 0;

    for(i=0;i<users->len;i++)
	file_load_user_conf_file(&usr(i));
}

/** Generate the teams in the leagues. */
void
start_generate_league_teams(void)
{
#ifdef DEBUG
    printf("start_generate_league_teams\n");
#endif

    gint i, j;    

    stat5 = STATUS_GENERATE_TEAMS;

    if(country.leagues->len == 0)
	main_exit_program(EXIT_NO_LEAGUES,
			  "start_generate_league_teams: no leagues found. There must be at least one league in the game.\n");

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->teams->len;j++)
	    team_generate_players_stadium(g_ptr_array_index(league->teams, j));
    }

    stat5 = -1;
}

/** Reset the cup and league ids to the smallest possible
    value to avoid an overflow (gotta admit, the id system
    isn't perfect). */
void
start_new_season_reset_ids(void)
{
#ifdef DEBUG
    printf("start_new_season_reset_ids\n");
#endif

    gint i, max;

    max = -1;
    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(league->c.id > max)
	    max = league->c.id;
    }

    counters[COUNT_LEAGUE_ID] = max + 1;

    max = -1;
    for(i=0;i<country.cups->len;i++) {
        Cup *cup = g_ptr_array_index(country.cups, i);
	if(cup->c.id > max)
	    max = cup->c.id;
    }

    for (i = 0; i < country.bygfoot->international_cups->len; i++) {
        Cup *cup = g_ptr_array_index(country.bygfoot->international_cups, i);
        if (cup->c.id > max)
            max = cup->c.id;
    }
    counters[COUNT_CUP_ID] = max + 1;
}

/** End a week round. */
void
end_week_round(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("end_week_round\n");
#endif

    gint i = 0;
    gboolean new_week = TRUE;
    WeekFuncBygfoot *end_func = end_week_round_funcs;

    if(debug > 100)
	g_print("End w %d r %d \n", week, week_round);

    while(*end_func != NULL)
    {
	(*end_func)(bygfoot);
	end_func++;
    }

    week_round++;

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(query_fixture_in_week_round(league->c.id, week, week_round))
	{
	    new_week = FALSE;
	    break;
	}
    }

    for(i=0;i<country.allcups->len;i++) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	if(query_fixture_in_week_round(cup->c.id, week, week_round))
	{
	    new_week = FALSE;
	    break;
	}
    }

    if(new_week)
    {
	end_week();

	if(query_start_end_season_end())
	{
	    end_season();

	    season++;
	    week = 1;
	    
	    start_new_season(bygfoot);
	}
	else
	    week++;

	week_round = 1;
	start_week(bygfoot);
    }

    start_week_round(bygfoot);

    bygfoot->random_seed = g_rand_int(rand_generator);
    g_rand_set_seed(rand_generator, bygfoot->random_seed);
}

/** Calculate the match results of a week round. */
void
end_week_round_results(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("end_week_round_results\n");
#endif

    gint i, j, done = 0;
    LiveGame live_game;
    gchar buf[SMALL], buf2[SMALL];
    gfloat num_matches =
	(gfloat)fixture_get_number_of_matches(week, week_round);
    gint usr_idx;

    /** Free the matches from last week. */
    for(i = 0; i < live_games->len; i++)
        free_live_game(&g_array_index(live_games, LiveGame, i));
    g_array_free(live_games, TRUE);
    live_games = g_array_new(FALSE, FALSE, sizeof(LiveGame));
    
    for(i=0;i<country.leagues->len;i++)
    {
        League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->fixtures->len;j++)
	    if(g_array_index(league->fixtures, Fixture, j).week_number == week &&
	       g_array_index(league->fixtures, Fixture, j).week_round_number == week_round &&
	       g_array_index(league->fixtures, Fixture, j).attendance == -1)
	    {                
                usr_idx = fixture_user_team_involved(&g_array_index(league->fixtures, Fixture, j));

                if(usr_idx == -1)
                {
                    g_array_append_val(live_games, live_game);
                    live_game_calculate_fixture(&g_array_index(league->fixtures, Fixture, j),
                                                &g_array_index(live_games, LiveGame, live_games->len - 1), bygfoot);
                }
                else
                    live_game_calculate_fixture(&g_array_index(league->fixtures, Fixture, j),
                                                &usr(usr_idx).live_game, bygfoot);

		done++;
		fixture_result_to_buf(&g_array_index(league->fixtures, Fixture, j),
				      buf, FALSE);
		sprintf(buf2, "%s %s %s",
			g_array_index(league->fixtures, Fixture, j).teams[0]->name,
			buf,
			g_array_index(league->fixtures, Fixture, j).teams[1]->name);
		bygfoot_show_progress(bygfoot, (gfloat)done / num_matches, buf2,
				  PIC_TYPE_MATCHPIC);

		if(debug > 120)
		    g_print("%s \n", buf2);
	    }
    }

    for(i=0;i<country.allcups->len;i++)
    {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	for(j=0;j<cup->fixtures->len;j++)
	{
	    if(g_array_index(cup->fixtures, Fixture, j).week_number == week &&
	       g_array_index(cup->fixtures, Fixture, j).week_round_number == week_round &&
	       g_array_index(cup->fixtures, Fixture, j).attendance == -1)
	    {

                usr_idx = fixture_user_team_involved(&g_array_index(cup->fixtures, Fixture, j));

                if(usr_idx == -1)
                {
                    g_array_append_val(live_games, live_game);
                    live_game_calculate_fixture(&g_array_index(cup->fixtures, Fixture, j),
                                                &g_array_index(live_games, LiveGame, live_games->len - 1), bygfoot);
                }
                else
                    live_game_calculate_fixture(&g_array_index(cup->fixtures, Fixture, j),
                                                &usr(usr_idx).live_game, bygfoot);

		done++;
		fixture_result_to_buf(&g_array_index(cup->fixtures, Fixture, j), 
				      buf, FALSE);
		sprintf(buf2, "%s %s %s",
			g_array_index(cup->fixtures, Fixture, j).teams[0]->name,
			buf,
			g_array_index(cup->fixtures, Fixture, j).teams[1]->name);
		bygfoot_show_progress(bygfoot, (gfloat)done / num_matches, buf2,
				  PIC_TYPE_MATCHPIC);
		if(debug > 120)
		    g_print("%s \n", buf2);
	    }
	}
    }
    
    bygfoot_show_progress(bygfoot, -1, "", PIC_TYPE_MATCHPIC);
}

/** Sort league and cup tables. */
void
end_week_round_sort_tables(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("end_week_round_sort_tables\n");
#endif

    gint i, j, k;

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(query_fixture_in_week_round(league->c.id, week, week_round))
	{
            for(k = 0; k < league->tables->len; k++)
            {
                for(j=0;j<g_array_index(league->tables, Table, k).elements->len;j++)
                    g_array_index(g_array_index(league->tables, Table, k).elements, TableElement, j).old_rank = j;
                
                g_array_sort_with_data(g_array_index(league->tables, Table, k).elements,
                                       (GCompareDataFunc)table_element_compare_func,
                                       &league->c);
            }
	}
    }

    for(i=0;i<country.allcups->len;i++) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	if(query_fixture_in_week_round(cup->c.id, week, week_round) &&
	   g_array_index(cup->fixtures, Fixture, cup->fixtures->len - 1).round ==
	   cup_has_tables(cup))
	    for(j=0;j<cup_get_last_tables(cup)->len;j++)
	    {
		for(k=0;k<g_array_index(cup_get_last_tables(cup), Table, j).elements->len;k++)
		    g_array_index(
			g_array_index(cup_get_last_tables(cup), Table, j).elements,
			TableElement, k).old_rank = k;

		g_array_sort_with_data(
		    g_array_index(cup_get_last_tables(cup), Table, j).elements,
		    (GCompareDataFunc)table_element_compare_func,
		    &cup->c);
	    }
    }
}

/** Update cup fixtures. */
void
end_week_round_update_fixtures(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("end_week_round_update_fixtures\n");
#endif

    gint i;

    for(i=0;i<country.allcups->len;i++) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	if(cup->next_fixture_update_week <= week &&
	   cup->next_fixture_update_week_round <= week_round)
	    fixture_update(cup);
    }      

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	if(query_league_active(league) && week == 
	   g_array_index(
	       league->fixtures, Fixture, league->fixtures->len - 1).week_number && 
	   week_round == 1 &&
	   team_is_user(g_array_index(league_table((league))->elements, TableElement, 0).team) != -1)
           user_history_add(&usr(team_is_user(g_array_index(league_table((league))->elements, TableElement, 0).team)),
                            USER_HISTORY_CHAMPION, 
                            g_array_index(league_table((league))->elements, 
                                          TableElement, 0).team->name,
                            league->c.name,
                            NULL, NULL);
    }
    
    for(i=0;i<country.cups->len;i++)
    {
        Cup *cup = g_ptr_array_index(country.cups, i);
	if(cup->add_week == 1000 && 
	   query_cup_hidden(cup) &&
	   query_cup_begins(cup))
	{
	    cup->last_week = cup_get_last_week_from_first(cup, week + 1);
            
	    if(fixture_write_cup_fixtures(cup))
                g_ptr_array_add(country.allcups, cup);
	}
    }
    for (i = 0; i < bygfoot->international_cups->len; i++) {
        Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        if (cup->add_week == 1000 &&
            query_cup_hidden(cup) &&
            query_cup_begins(cup))
        {
            cup->last_week = cup_get_last_week_from_first(cup, week + 1);
	        if(fixture_write_cup_fixtures(cup)) {
                g_ptr_array_add(country.allcups, cup);
            }
        }
    }
}

/** Write newspaper articles after week round. */
void
end_week_round_generate_news(Bygfoot *bygfoot)
{
    gint i;

    /** News for user matches. */
    for(i = 0; i < users->len; i++)
        if(usr(i).live_game.fix != NULL &&
           usr(i).live_game.fix->week_number == week &&
           usr(i).live_game.fix->week_round_number == week_round &&
           news_check_match_relevant(&usr(i).live_game))
            news_generate_match(&usr(i).live_game);

    /** News for other matches. */
    for(i = 0; i < live_games->len; i++)
        if(news_check_match_relevant(&g_array_index(live_games, LiveGame, i)))
            news_generate_match(&g_array_index(live_games, LiveGame, i));

    /* Get rid of older news. */
    while(newspaper.articles->len > const_int("int_news_history_length"))
    {
        free_newspaper_article(&g_array_index(newspaper.articles, NewsPaperArticle, 0));
        g_array_remove_index(newspaper.articles, 0);
    }
}

/** Start a new week round. */
void
start_week_round(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_round\n");
#endif

    WeekFunc *start_func = start_week_round_funcs;

    if(debug > 100)
	g_print("Start w %d r %d \n", week, week_round);

    while(*start_func != NULL)
    {
	(*start_func)();
	start_func++;
    }

    if(!query_start_end_season_end() &&
	opt_int("int_opt_skip") &&
       !query_user_games_this_week_round() &&
       !query_user_events() &&
       ((week_round == 1 && 
	 !query_user_games_in_week_round(week - 1, fixture_get_last_week_round(week - 1))) ||
	(week_round > 1 && 
	 !query_user_games_in_week_round(week, week_round - 1)))) 
    {
	end_week_round(bygfoot);
    }
    else
    {
        cur_user = 0;
    }
}

/** Start a new week. */
void
start_week(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week\n");
#endif

    WeekFuncBygfoot *start_func = start_week_funcs;

    while(*start_func != NULL)
    {
	(*start_func)(bygfoot);
	start_func++;
    }
}

/** Finish a week. */
void
end_week(void)
{
#ifdef DEBUG
    printf("end_week\n");
#endif

    WeekFunc *end_func = end_week_funcs;

    while(*end_func != NULL)
    {
	(*end_func)();
	end_func++;
    }

    if(debug > 150)
	stat_show_av_league_goals();
}

/** Hide some not-so-important cups that
    are already finished. */
void
end_week_hide_cups(void)
{
#ifdef DEBUG
    printf("end_week_hide_cups\n");
#endif

    gint i;

    for(i=country.allcups->len - 1; i >= 0; i--) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	if(competition_has_property(&cup->c, "hide") &&
	   g_array_index(cup->fixtures, Fixture, cup->fixtures->len - 1).attendance > 0)
	    g_ptr_array_remove_index(country.allcups, i);
    }
}

/** Add the cups that begin later in the season to the acps array. */
void
start_week_add_cups(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_add_cups\n");
#endif

    gint i;

    for(i=0;i<country.cups->len;i++) {
        Cup *cup = g_ptr_array_index(country.cups, i);
	if(cup->add_week == week)
	{
	    if(fixture_write_cup_fixtures(cup))
                g_ptr_array_add(country.allcups, cup);            
	}
    }

    for (i = 0; i < country.bygfoot->international_cups->len; i++) {
        Cup *cup = g_ptr_array_index(country.bygfoot->international_cups, i);
        if (cup->add_week == week) {
            if (fixture_write_cup_fixtures(cup))
                g_ptr_array_add(country.allcups, cup);
        }
    }
}

static void
update_teams(void (*update_func)(Team*))
{
    gint i, j;
    GHashTable *visited = g_hash_table_new(g_direct_hash, g_direct_equal);

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->teams->len;j++) {
            Team *team = g_ptr_array_index(league->teams, j);
	    update_func(team);
            g_hash_table_insert(visited, team, team);
        }
    }

    /* Update international teams participating in cups, but ignore teams
     * in the user's country, because these have already been updated above.
     */
    for(i=0;i<country.bygfoot->international_cups->len;i++) {
        Cup *cup = g_ptr_array_index(country.bygfoot->international_cups, i);
        CupTeamsIterator it;
        Team *team;
        for (team = cup_teams_begin(cup, &it); team;
             team = cup_teams_next(&it)) {
            if (g_hash_table_lookup(visited, team))
                continue;
	    update_func(team);
            g_hash_table_insert(visited, team, team);
        }
    }
    g_hash_table_unref(visited);
}

/** Age increase etc. of players.
    CPU teams get updated at the end of their matches
    (to avoid cup teams getting updated too often). */
void
start_week_update_teams(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_update_teams\n");
#endif

    update_teams(team_update_team_weekly);
}

/** Do some things at the beginning of each new round for
    the user teams. */
void
start_week_round_update_teams(void)
{
#ifdef DEBUG
    printf("start_week_round_update_teams\n");
#endif

    update_teams(team_update_team_week_roundly);
}

/** Deduce wages etc. */
void
start_week_update_user_finances(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_update_user_finances\n");
#endif

    gint i;

    if(sett_int("int_opt_disable_finances"))
	return;

    for(i=0;i<users->len;i++)
	finance_update_user_weekly(&usr(i));
}

/** Some general user update. */
void
start_week_update_users(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_update_users\n");
#endif

    gint i;

    for(i=0;i<users->len;i++)
    {
	if(usr(i).scout >= 100)
	    usr(i).scout = math_get_place(usr(i).scout, 2);
	
	if(usr(i).physio >= 100)
	    usr(i).physio = math_get_place(usr(i).physio, 2);

	if(usr(i).youth_academy.coach >= 100)
	    usr(i).youth_academy.coach = math_get_place(usr(i).youth_academy.coach, 2);

	user_weekly_update_counters(&usr(i));
    }
}

/** Create new tables if necessary etc. */
void
start_week_update_leagues(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("start_week_update_leagues\n");
#endif

    gint i;

    for(i = 0; i < country.leagues->len; i++) {
        League *league = g_ptr_array_index(country.leagues, i);
        league_check_new_tables(league);
    }
}

/** Check whether the season has ended. */
gboolean
query_start_end_season_end(void)
{
#ifdef DEBUG
    printf("query_start_end_season_end\n");
#endif

    gint i, j;

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->fixtures->len;j++)
	    if(g_array_index(league->fixtures, Fixture, j).week_number > week - 1)
		return FALSE;
    }

    for(i=0;i<country.allcups->len;i++) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
	for(j=0;j<cup->fixtures->len;j++)
	    if(g_array_index(cup->fixtures, Fixture, j).week_number > week - 1)
		return FALSE;
    }

    return TRUE;
}

/** Manage promotions and relegations at the beginning of a new season. */
void
start_new_season_league_changes(void)
{
#ifdef DEBUG
    printf("start_new_season_league_changes\n");
#endif

    gint i, j, k;
    gint league_size[country.leagues->len];
    GArray *team_movements = g_array_new(FALSE, FALSE, sizeof(TeamMove));

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	league_get_team_movements(league, team_movements);
    }

    if (country.reserve_promotion_rules)
        country_apply_reserve_prom_rules(&country, team_movements);

    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	league_size[i] = league->teams->len;
    }

    for(i=0;i<team_movements->len;i++)
	league_remove_team_with_id(
	    g_array_index(team_movements, TeamMove, i).tm->league,
	    g_array_index(team_movements, TeamMove, i).tm->id);

    league_team_movements_destinations(team_movements, league_size);

    for(i = team_movements->len - 1; i >= 0; i--) {
        TeamMove *move = &g_array_index(team_movements, TeamMove, i);
	if(move->prom_rel_type == PROM_REL_RELEGATION) {
            League *league = g_ptr_array_index(country.leagues, g_array_index(move->dest_idcs, gint, 0));
	    misc_g_ptr_array_insert(league->teams, 0, move->tm);
        }
    }
    
    for(i=1;i<team_movements->len;i++) {
        TeamMove *move = &g_array_index(team_movements, TeamMove, i);
	if(move->prom_rel_type != PROM_REL_RELEGATION) {
            League *league = g_ptr_array_index(country.leagues, g_array_index(move->dest_idcs, gint, 0));
	    g_ptr_array_add(league->teams, move->tm);
        }
    }

    for(i=0;i<team_movements->len;i++)
	g_array_free(g_array_index(team_movements, TeamMove, i).dest_idcs, TRUE);
    g_array_free(team_movements, TRUE);
    
    for(i=0;i<country.leagues->len;i++)
    {
	League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->teams->len;j++)
	{
            Team *team = g_ptr_array_index(league->teams, j);
	    team_set_league(team, league);
            for (k = 0; k < team->players->len; k++) {
                Player *player = g_ptr_array_index(team->players, k);
                player->team = team;
            }
        }

	league_season_start(league);
    }
}


/** End a season (store stats etc.) */
void
end_season(void)
{
#ifdef DEBUG
    printf("end_season\n");
#endif

    stat_create_season_stat();
}
