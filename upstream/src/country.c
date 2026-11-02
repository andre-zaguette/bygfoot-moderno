/*
   country.c

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

#include "country.h"
#include "competition.h"
#include "league.h"

static gint
league_allcups_get_index(const Country *country, gint clid)
{
    int i;
    for (i = 0; i < country->leagues->len; i++) {
        League *league = g_ptr_array_index(country->leagues, i);
        if (league->c.id == clid)
            return i;
    }

    for (i = 0; i < country->allcups->len; i++) {
        Cup *cup = g_ptr_array_index(country->allcups, i);
        if (cup->c.id == clid)
            return i;
    }
    return 0;
}

Competition *
country_get_next_competition(const Country *country, gint clid, gboolean count_inactive)
{
    League *league;
    gint index = league_allcups_get_index(country, clid);

    /* Get the next league. */
    if (clid_is_league(clid) && index + 1 < country->leagues->len) {
         league = g_ptr_array_index(country->leagues, index + 1);
         if (!count_inactive && !query_league_active(league))
            return country_get_next_competition(country, league->c.id, count_inactive);
         return &league->c;
    }

    /* Get the next cup. */
    if (clid_is_cup(clid) && index + 1 < country->allcups->len) {
        Cup *cup = g_ptr_array_index(country->allcups, index + 1);
        return &cup->c;
    }

    /* If we get here, we know the clid is from the last cup/league, so we
     * need to return the first cup/league. */

    /* Get the first cup. */
    if (clid_is_league(clid) || country->leagues->len == 0) {
        Cup *cup = g_ptr_array_index(country->allcups, 0);
        return &cup->c;
    }

    /* Get the first league. */
    league = g_ptr_array_index(country->leagues, 0);
    if (!count_inactive && !query_league_active(league))
        return country_get_next_competition(country, league->c.id, count_inactive);
    return &league->c;
}

Competition *
country_get_previous_competition(const Country *country, gint clid, gboolean count_inactive)
{
    League *league;
    gint index = league_allcups_get_index(country, clid);

    /* Get the previous league. */
    if (clid_is_league(clid) && index > 0) {
        league = g_ptr_array_index(country->leagues, index - 1);
        if (!count_inactive && !query_league_active(league))
            return country_get_previous_competition(country, league->c.id, count_inactive);
        return &league->c;
    }

    /* Get the previous cup. */
    if (clid_is_cup(clid) && index > 0) {
        Cup *cup = g_ptr_array_index(country->allcups, index - 1);
        return &cup->c;
    }

    /* If we get here, we know the clid is from the first cup/league, so we
     * need to return the last cup/league. */

    /* Get the last cup. */
    if (clid_is_league(clid) || country->leagues->len == 0) {
        Cup *cup = g_ptr_array_index(country->allcups, country->allcups->len - 1);
        return &cup->c;
    }

    /* Get the last league. */
    league = g_ptr_array_index(country->leagues, country->leagues->len - 1);
    if (!count_inactive && !query_league_active(league))
        return country_get_previous_competition(country, league->c.id, count_inactive);
    return &league->c;
}

Competition *
country_get_competition_id(const Country *country, int id)
{
    int i;

    for (i = 0; i < country->leagues->len; i++) {
        League *league = g_ptr_array_index(country->leagues, i);
        if (league->c.id == id)
            return &league->c;
    }

    for (i = 0; i < country->cups->len; i++) {
        Cup *cup = g_ptr_array_index(country->cups, i);
        if (cup->c.id == id)
            return &cup->c;
    }
    return NULL;
}
