/*
   job.c

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
#include "cup.h"
#include "file.h"
#include "free.h"
#include "job.h"
#include "league.h"
#include "live_game.h"
#include "main.h"
#include "maths.h"
#include "option.h"
#include "start_end.h"
#include "team.h"
#include "transfer.h"
#include "user.h"
#include "variables.h"
#include "xml_country.h"

/** Update the job exchange: remove expired offers and add new ones. */
void
job_update(Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("job_update\n");
#endif

    gint i;
    gint new_offers, int_offers;
    gint national_teams = 0;

    for(i=jobs->len - 1; i >= 0; i--)
    {
	g_array_index(jobs, Job, i).time--;

	if(g_array_index(jobs, Job, i).time <= 0)
	    job_remove(&g_array_index(jobs, Job, i), TRUE);
    }

    if(week % const_int("int_job_update_interval") != 2)
	return;

    for (i = 0; i < country.leagues->len; i++) {
        national_teams += ((League*)g_ptr_array_index(country.leagues, i))->teams->len;
    }

    /* Limit the total number of jobs to the number of national teams.
     * Otherwise, we risk hitting an infinite loop in
     * job_pick_team_from_country();
     */

    new_offers = math_rndi(const_int("int_job_new_offers_lower"),
			   const_int("int_job_new_offers_upper"));

    new_offers = MIN(new_offers, national_teams - jobs->len);
    int_offers = (users->len == 1) ? 
	(gint)rint((gfloat)new_offers *
		   const_float("float_job_international_perc")) : 0;

    for(i=0;i<new_offers - int_offers;i++)
	job_add_new_national();

    job_add_new_international(int_offers);
}

/** Add some new international job offers to the job exchange. */
void
job_add_new_international(gint num_of_new)
{
#ifdef DEBUG
    printf("job_add_new_international\n");
#endif

    gint i, k, rndom;
    Team *tm = NULL;
    League *league = NULL;
    gint team_id = -1;
    Job new_job;

    k = 0;
    for(i=0;i<num_of_new;i++)
    {
        Country *job_country = NULL;
	do
	    rndom = math_rndi(0, country_list->len - 1);
	while(g_strrstr(((Country*)g_ptr_array_index(country_list, rndom))->sid,
			country.sid));
	
	job_country = g_ptr_array_index(country_list, rndom);

	job_pick_team_from_country(job_country, &tm, &league);

	new_job.country_file = 
	    g_strdup_printf("country_%s.xml", job_country->sid);
	new_job.time = math_rndi(const_int("int_job_update_interval") - 1,
				 const_int("int_job_update_interval") + 1);
	new_job.country_name = g_strdup(job_country->name);
	new_job.country_rating = job_country->rating;
	new_job.league_name = g_strdup(league->c.name);
	new_job.league_layer = league->layer;

	team_id = job_team_is_in_cup(tm);

	if(team_id == -1)
	{
	    new_job.team_id = tm->id;
	    new_job.type = JOB_TYPE_INTERNATIONAL;
	}
	else
	{
	    new_job.team_id = team_id;
	    new_job.type = JOB_TYPE_CUP;
	}

	new_job.talent_percent = 
	    (gint)rint((team_get_average_talent(tm) /
			league->average_talent) * 100);

	g_array_append_val(jobs, new_job);
    }
}

/** Find out whether the country file is already loaded and part
    of the country array.
    @param len The length of the array. */
gint
job_country_is_in_list(const gchar *country_file, 
		       const Country *countries, gint len)
{
#ifdef DEBUG
    printf("job_country_is_in_list\n");
#endif

    gint i;

    for(i=0;i<len;i++)
    {
	if(countries[i].sid == NULL)
	    return -1;
	else if(g_strrstr(country_file, countries[i].sid))
	    return i;
    }

    return -1;
}

/** Add a new national job offer to the job exchange. */
void
job_add_new_national(void)
{
#ifdef DEBUG
    printf("job_add_new_national\n");
#endif

    Job new_job;
    League *league = NULL;
    Team *tm = NULL;

    job_pick_team_from_country(&country, &tm, &league);

    new_job.type = JOB_TYPE_NATIONAL;
    new_job.time = math_rndi(const_int("int_job_update_interval") - 1,
			     const_int("int_job_update_interval") + 1);
    new_job.country_file = NULL;
    new_job.country_name = country.name;
    new_job.country_rating = -1;
    new_job.league_name = league->c.name;
    new_job.league_layer = league->layer;

    new_job.talent_percent = 
	(gint)rint((team_get_average_talent(tm) /
		    league->average_talent) * 100);
    new_job.team_id = tm->id;

    g_array_append_val(jobs, new_job);
}

void
job_pick_team_from_country(const Country *cntry, Team **tm, League **league)
{
#ifdef DEBUG
    printf("job_pick_team_from_country\n");
#endif

    gint i, rndom;
    gint team_lens[cntry->leagues->len];

    team_lens[0] = ((League*)g_ptr_array_index(cntry->leagues, 0))->teams->len;

    for(i=1;i<cntry->leagues->len;i++)
	team_lens[i] = team_lens[i - 1] + 
	    ((League*)g_ptr_array_index(cntry->leagues, i))->teams->len;

    do
    {
	rndom = math_rndi(0, team_lens[cntry->leagues->len - 1] - 1);

	for(i=0;i<cntry->leagues->len;i++)
	    if(rndom < team_lens[i])
	    {
		*tm = (i > 0) ?
		    g_ptr_array_index(((League*)g_ptr_array_index(
				       cntry->leagues, i))->teams, 
				   rndom - team_lens[i - 1]) :
		    g_ptr_array_index(((League*)g_ptr_array_index(
				       cntry->leagues, i))->teams, 
				   rndom);				
		*league = g_ptr_array_index(cntry->leagues, i);
		break;
	    }
    }
    while(team_is_user(*tm) != -1 ||
	  job_team_is_on_list((*tm)->id) != -1);
}

/** Find out whether the team with given id is already on the
    job exchange list.
    @return The index in the jobs array if the team is on the list or -1. */
gint
job_team_is_on_list(gint team_id)
{
#ifdef DEBUG
    printf("job_team_is_on_list\n");
#endif

    gint i;

    for(i=0;i<jobs->len;i++)
	if(g_array_index(jobs, Job, i).team_id == team_id)
	    return i;

    return -1;
}

/** Find out whether the team with given name is participating
    in an international cup (and thus doesn't have to be generated).
    @return The id of the team if found or -1. */
gint
job_team_is_in_cup(const Team *team)
{
#ifdef DEBUG
    printf("job_team_is_in_cup\n");
#endif

    gint i, j;

    for(i=0;i<country.allcups->len;i++) {
        Cup *cup = g_ptr_array_index(country.allcups, i);
        if (query_team_is_in_cup(team, cup))
            return team->id;
    }
    return -1;
}

/** Find the team going with the job offer. */
Team*
job_get_team(const Job *job)
{
#ifdef DEBUG
    printf("job_get_team\n");
#endif

    gint i, j;

    return team_of_id(job->team_id);
}

/** Find out whether the user's application for the job gets
    accepted. */
gboolean
query_job_application_successful(const Job *job, const User *user)
{
#ifdef DEBUG
    printf("query_job_application_successful\n");
#endif

    gfloat success_needed;
    const Team *tm = job_get_team(job);
    gfloat user_av_skill = team_get_average_skill(user->tm, FALSE),
	job_av_skill = team_get_average_skill(tm, FALSE);

    success_needed = (job_av_skill - user_av_skill) * 
	(gfloat)const_int("int_job_application_points_per_av_skill");

    success_needed += 
	((gfloat)(job->league_layer - user->tm->league->layer) *
	 (gfloat)const_int("int_job_application_points_per_layer"));

    if(job->type != JOB_TYPE_NATIONAL)
    {
	success_needed += 
	    (gfloat)const_int("int_job_application_points_international");

	success_needed +=
	    ((gfloat)(job->country_rating - country.rating) *
	     (gfloat)const_int("int_job_application_points_per_rating"));
    }

    return (user->counters[COUNT_USER_SUCCESS] >= success_needed);
}

/** Remove a job from the jobs array.
    @param free Whether to free memory occupied by the job
    and its team. */
void
job_remove(Job *job, gboolean free_tm)
{
#ifdef DEBUG
    printf("job_remove\n");
#endif

    gint i;

    for(i=0;i<jobs->len;i++)
	if(&g_array_index(jobs, Job, i) == job)
	{
	    free_job(job, free_tm);
	    g_array_remove_index(jobs, i);
	    break;
	}
}

/** Change the game so that the country is
    used that's specified in the job. */
void
job_change_country(Job *job, Bygfoot *bygfoot)
{
#ifdef DEBUG
    printf("job_change_country\n");
#endif

    gint i, j, k;
    Team tm = *(job_get_team(job));
    gint season_temp = season + 1;

    for(i=transfer_list->len - 1;i>=0;i--)
	transfer_remove_player(i);
    
    free_bets(TRUE);

    /* There's only one user (otherwise
       international job offers are disabled). */
    for(i=0;i<2;i++)
    {
	g_array_free(usr(0).bets[i], TRUE);
	usr(0).bets[i] = g_array_new(FALSE, FALSE, sizeof(BetUser));
    }

    live_game_reset(&usr(0).live_game, NULL, FALSE);

    free_country(&country, TRUE);

    xml_country_read(job->country_file, &country, bygfoot);

    stat5 = STATUS_GENERATE_TEAMS;
    for(i=0;i<country.leagues->len;i++) {
        League *league = g_ptr_array_index(country.leagues, i);
	for(j=0;j<league->teams->len;j++) {
	    Team *new_team = g_ptr_array_index(league->teams, j);
	    if(strcmp(new_team->name, tm.name) != 0)
		team_generate_players_stadium(new_team);
	    else
	    {
		tm.id = new_team->id;
                tm.league = new_team->league;
		job->team_id = tm.id;
		free_team(new_team);
		*new_team = tm;

                for (k = 0; k < new_team->players->len; k++) {
                    Player *player = g_ptr_array_index(new_team->players, k);
                    player->team = new_team;
                }
	    }
	}
    }
    stat5 = -1;

    /* Set season to 1 so that some special things
       in the start_new_season function don't get applied. */
    season = 1;
    start_new_season(bygfoot);
    season = season_temp;
}
