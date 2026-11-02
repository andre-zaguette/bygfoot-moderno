/*
  sqlite_data_store.c

  Bygfoot Football Manager -- a small and simple GTK2-based
  football management game.

  http://bygfoot.sourceforge.net

  Copyright (C) 2024 Tom Stellard (tom@stellard.net)

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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
  USA.
*/

#include "sqlite_data_store.h"
#include "data_store_struct.h"
#include "free.h"
#include "json_deserialize.h"
#include "json_serialize.h"
#include "option.h"
#include "player.h"
#include "team.h"
#include "user.h"
#include "variables.h"
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <sqlite3.h>
#include <stdio.h>

static gboolean
read_player_stats(DataStore *parent, Bygfoot *bygfoot, Player *player);

typedef struct
{
    DataStore parent;
    sqlite3 *db;

    sqlite3_stmt *begin_transaction_stmt;
    sqlite3_stmt *end_transaction_stmt;
    sqlite3_stmt *insert_option_stmt;
    sqlite3_stmt *insert_league_stmt;
    sqlite3_stmt *insert_cup_stmt;
    sqlite3_stmt *insert_team_stmt;
    sqlite3_stmt *insert_player_json_stmt;
    sqlite3_stmt *insert_player_stmt;
    sqlite3_stmt *insert_player_stat_stmt;
    sqlite3_stmt *insert_user_stmt;
    sqlite3_stmt *insert_transfer_stmt;
    sqlite3_stmt *insert_season_stat_stmt;
    sqlite3_stmt *insert_job_stmt;
    sqlite3_stmt *insert_newspaper_stmt;
    sqlite3_stmt *insert_bet_stmt;
    sqlite3_stmt *insert_country_stmt;
    sqlite3_stmt *insert_bygfoot_stmt;

    sqlite3_stmt *select_options_stmt;
    sqlite3_stmt *select_bygfoot_stmt;
    sqlite3_stmt *select_countries_stmt;
    sqlite3_stmt *select_country_sid_stmt;
    sqlite3_stmt *select_league_id_stmt;
    sqlite3_stmt *select_team_id_stmt;
    sqlite3_stmt *select_team_players_stmt;
    sqlite3_stmt *select_cup_id_stmt;
    sqlite3_stmt *select_player_id_stmt;
    sqlite3_stmt *select_player_id_json_stmt;
    sqlite3_stmt *select_player_stat_stmt;
    sqlite3_stmt *select_users_stmt;
    sqlite3_stmt *select_user_options_stmt;
    sqlite3_stmt *select_transfers_stmt;
    sqlite3_stmt *select_bets_stmt;
    sqlite3_stmt *select_jobs_stmt;
    sqlite3_stmt *select_newspaper_stmt;
    sqlite3_stmt *select_season_stats_stmt;

    sqlite3_stmt *create_player_stat_player_id_index_stmt;
    sqlite3_stmt *create_player_team_id_index_stmt;

} SqliteDataStore;

#define SQLITE_LOG_ERROR(db, rc, msg)                                          \
    printf("%s(): %s : (%d) %s\n", __FUNCTION__, msg, rc, sqlite3_errmsg(db));

static gboolean
begin_transaction(DataStore *parent)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    int rc = sqlite3_step(sql->begin_transaction_stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }
    rc = sqlite3_reset(sql->begin_transaction_stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    return TRUE;
}

static gboolean
end_transaction(DataStore *parent)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    int rc = sqlite3_step(sql->end_transaction_stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }
    rc = sqlite3_reset(sql->end_transaction_stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    return TRUE;
}

static gboolean
insert_json(SqliteDataStore *sql, sqlite3_stmt *stmt,
            void (*serialize_func)(const gpointer, const json_object *,
                                   void (*)(const char *, gpointer), gpointer),
            const json_object *fields, const gpointer obj)
{
    struct bygfoot_json_serialize_buffer json_buffer;
    gint rc;

    json_buffer.data = calloc(1024, 1);
    json_buffer.len = 0;
    json_buffer.alloc_size = 1024;
    serialize_func(obj, fields, bygfoot_json_serialize_write_buffer,
                   &json_buffer);

    rc = sqlite3_bind_text(stmt, 1, json_buffer.data, json_buffer.len, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        goto error;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

static int
write_options(DataStore *parent, const char *user, const OptionList *options,
              gint type)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_option_stmt;
    gint i;

    for (i = 0; i < options->list->len; i++) {
        gint rc;
        const Option *option = &g_array_index(options->list, Option, i);
        rc = sqlite3_bind_text(stmt, 1, user, -1, NULL);
        if (rc)
            sqlite3_errmsg(sql->db);
        rc = sqlite3_bind_text(stmt, 2, option->name, -1, NULL);
        if (rc)
            sqlite3_errmsg(sql->db);
        rc = sqlite3_bind_text(stmt, 3, option->string_value, -1, NULL);
        if (rc)
            sqlite3_errmsg(sql->db);
        rc = sqlite3_bind_int(stmt, 4, option->value);
        if (rc)
            sqlite3_errmsg(sql->db);
        rc = sqlite3_bind_int(stmt, 5, type);
        if (rc)
            sqlite3_errmsg(sql->db);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    return TRUE;
}

static int
write_league(DataStore *parent, const League *league)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_league_stmt;
    json_object *fields = json_object_new_object();
    json_object *team_fields = json_object_new_object();
    struct bygfoot_json_serialize_buffer json_buffer;
    gint rc;

    json_object_object_add_ex(team_fields, "id", NULL,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                  JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(fields, "teams", team_fields,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                  JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(fields, "*", json_object_new_boolean(TRUE),
                              JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                  JSON_C_OBJECT_KEY_IS_CONSTANT);

    json_buffer.data = calloc(1024, 1);
    json_buffer.len = 0;
    json_buffer.alloc_size = 1024;
    bygfoot_json_serialize_league(
        league, fields, bygfoot_json_serialize_write_buffer, &json_buffer);

    rc = sqlite3_bind_int(stmt, 1, league->c.id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");
        goto error;
    }

    rc = sqlite3_bind_text(stmt, 2, json_buffer.data, json_buffer.len, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        goto error;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

static gboolean
write_cup(DataStore *parent, const Cup *cup)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_cup_stmt;
    struct bygfoot_json_serialize_buffer json_buffer;
    gint rc;

    json_buffer.data = calloc(1024, 1);
    json_buffer.len = 0;
    json_buffer.alloc_size = 1024;
    bygfoot_json_serialize_cup(cup, NULL, bygfoot_json_serialize_write_buffer,
                               &json_buffer);

    rc = sqlite3_bind_int(stmt, 1, cup->c.id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");
        goto error;
    }

    rc = sqlite3_bind_text(stmt, 2, json_buffer.data, json_buffer.len, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        goto error;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

static gboolean
write_team(DataStore *parent, const Team *team)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_team_stmt;
    gint argno = 1;
    gint rc;
    gint i;
    gboolean ret = TRUE;

#define BIND_ARG(value, type)                                                  \
    rc = sqlite3_bind_##type(stmt, argno++, team->value);                      \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
    }
#define BIND_TEXT_ARG(value)                                                   \
    rc = sqlite3_bind_text(stmt, argno++, team->value, -1, SQLITE_STATIC);     \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
    }
    BIND_ARG(id, int);
    BIND_TEXT_ARG(name);
    BIND_TEXT_ARG(symbol);
    BIND_TEXT_ARG(names_file);
    BIND_TEXT_ARG(def_file);
    BIND_TEXT_ARG(strategy_sid);
    BIND_ARG(structure, int);
    BIND_ARG(style, int);
    BIND_ARG(boost, int);
    BIND_ARG(average_talent, double);
    BIND_ARG(luck, double);
    BIND_ARG(first_team.team->id, int);
    BIND_ARG(reserve_level, int);
    BIND_ARG(league->c.id, int);

#undef BIND_ARG
#undef BIND_TEXT_ARG

#define BIND_ARG(value, type)                                                  \
    rc = sqlite3_bind_##type(stmt, argno++, team->stadium.value);              \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
    }
#define BIND_TEXT_ARG(value)                                                   \
    rc = sqlite3_bind_text(stmt, argno++, team->stadium.value, -1,             \
                           SQLITE_STATIC);                                     \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
    }

    BIND_TEXT_ARG(name);
    BIND_ARG(capacity, int);
    BIND_ARG(average_attendance, int);
    BIND_ARG(possible_attendance, int);
    BIND_ARG(games, int);
    BIND_ARG(safety, double);
    BIND_ARG(ticket_price, double);

#undef BIND_TEXT_ARG
#undef BIND_ARG

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        ret = FALSE;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        ret = FALSE;
    }
    if (!ret)
        return ret;

    return ret;
}

static gboolean
write_player_json(DataStore *parent, const Player *player)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_player_json_stmt;
    struct bygfoot_json_serialize_buffer json_buffer;
    gint rc;

    json_buffer.data = calloc(1024, 1);
    json_buffer.len = 0;
    json_buffer.alloc_size = 1024;
    bygfoot_json_serialize_player(
        player, NULL, bygfoot_json_serialize_write_buffer, &json_buffer);

    rc = sqlite3_bind_int(stmt, 1, player->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");
        goto error;
    }
    rc = sqlite3_bind_text(stmt, 2, json_buffer.data, json_buffer.len, NULL);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        goto error;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        goto error;
    }

    return TRUE;

error:
    return FALSE;
}

static gboolean
write_player(DataStore *parent, const Player *player, gint roster_idx)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_player_stmt;
    GList *iter;
    gint rc, season_id;
    gboolean ret = TRUE;
    gint index = 1;

#define BIND_ARG(value, type)                                                  \
    rc = sqlite3_bind_##type(stmt, index++, player->value);                    \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
        goto done;                                                             \
    }

    BIND_ARG(id, int);
    BIND_ARG(team->id, int);

    rc = sqlite3_bind_text(stmt, index++, player->name, -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        ret = FALSE;
        goto done;
    }
    BIND_ARG(pos, int);
    BIND_ARG(cpos, int);
    BIND_ARG(health, int);
    BIND_ARG(recovery, int);
    BIND_ARG(value, int);
    BIND_ARG(wage, int);
    BIND_ARG(offers, int);
    BIND_ARG(streak, int);
    BIND_ARG(skill, double);
    BIND_ARG(cskill, double);
    BIND_ARG(talent, double);
    BIND_ARG(etal[0], double);
    BIND_ARG(etal[1], double);
    BIND_ARG(etal[2], double);
    BIND_ARG(etal[3], double);
    BIND_ARG(fitness, double);
    BIND_ARG(lsu, double);
    BIND_ARG(age, double);
    BIND_ARG(peak_age, double);
    BIND_ARG(peak_region, double);
    BIND_ARG(contract, double);
    BIND_ARG(streak_prob, double);
    BIND_ARG(streak_count, double);
    BIND_ARG(participation, int);
#undef BIND_ARG
    rc = sqlite3_bind_int(stmt, index++, roster_idx);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");
        ret = FALSE;
        goto done;
    }
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        if (sqlite3_extended_errcode(sql->db) == SQLITE_CONSTRAINT_PRIMARYKEY) {
            printf("Player id: %d name: %s\n", player->id, player->name);
        }
        ret = FALSE;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error reseting statement");
        ret = FALSE;
    }
    if (!ret)
        goto done;

#define BIND_ARG(value, type, index)                                           \
    rc = sqlite3_bind_##type(stmt, index, value);                              \
    if (rc != SQLITE_OK) {                                                     \
        SQLITE_LOG_ERROR(sql->db, rc, "error binding int argument");           \
        ret = FALSE;                                                           \
        goto done;                                                             \
    }
    stmt = sql->insert_player_stat_stmt;
    BIND_ARG(player->id, int, 1);
    for (iter = player->stats, season_id = 0; iter;
         iter = iter->next, ++season_id) {
        gint i;
        GArray *season_stats = iter->data;
        BIND_ARG(season_id, int, 2);

        /* Special case for when there have been no stats yet this season. */
        if (season_stats->len == 0) {
            for (i = 3; i < 10; i++) {
                BIND_ARG(0, int, i);
            }
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
                ret = FALSE;
            }
            rc = sqlite3_reset(stmt);
            if (rc != SQLITE_OK) {
                SQLITE_LOG_ERROR(sql->db, rc, "error reseting statement");
                ret = FALSE;
            }
            if (ret == FALSE)
                break;
            continue;
        }
        for (i = 0; i < season_stats->len; i++) {
            PlayerCompetitionStats *stats =
                &g_array_index(season_stats, PlayerCompetitionStats, i);
            BIND_ARG(i, int, 3);
            BIND_ARG(stats->competition->id, int, 4);
            BIND_ARG(stats->games, int, 5);
            BIND_ARG(stats->goals, int, 6);
            BIND_ARG(stats->shots, int, 7);
            BIND_ARG(stats->yellow, int, 8);
            BIND_ARG(stats->red, int, 9);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
                ret = FALSE;
            }
            rc = sqlite3_reset(stmt);
            if (rc != SQLITE_OK) {
                SQLITE_LOG_ERROR(sql->db, rc, "error reseting statement");
                ret = FALSE;
            }
            if (ret == FALSE)
                break;
        }
    }
#undef BIND_ARG
done:
    return ret;
}

static gboolean
write_user(DataStore *parent, const User *user)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_user_stmt;
    json_object *fields = json_object_new_object();

    write_options(parent, user->name, &user->options, OPTION_TYPE_USER);

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_user,
                       NULL, (const gpointer)user);
}

static gboolean
write_transfer(DataStore *parent, const Transfer *transfer)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_transfer_stmt;

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_transfer,
                       NULL, (const gpointer)transfer);
}

static gboolean
write_season_stat(DataStore *parent, const SeasonStat *stat)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_season_stat_stmt;

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_season_stat,
                       NULL, (const gpointer)stat);
}

static gboolean
write_job(DataStore *parent, const Job *job)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_job_stmt;

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_job,
                       NULL, (const gpointer)job);
}

static gboolean
write_newspaper(DataStore *parent, const NewsPaperArticle *newspaper)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_newspaper_stmt;

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer), gpointer))
                           bygfoot_json_serialize_newspaper_article,
                       NULL, (const gpointer)newspaper);
}

static gboolean
write_bets(DataStore *parent, GArray *bets[2])
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_bet_stmt;

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_bets,
                       NULL, (const gpointer)bets);
}

static gboolean
write_country(DataStore *parent, const Country *country)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_country_stmt;
    json_object *json_fields = json_object_new_object();
    json_object *json_id = json_object_new_object();
    gboolean ret;
    const char *fields[] = {
        "name",    "symbol", "sid", "rating", "reserve_promotion_rules",
        "allcups", NULL};

    const char *id_fields[] = {"leagues", "cups", NULL};
    const char **iter;
    for (iter = fields; *iter; ++iter) {
        const char *field = *iter;
        json_object_object_add_ex(json_fields, field, NULL,
                                  JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                      JSON_C_OBJECT_KEY_IS_CONSTANT);
    }

    json_object_object_add_ex(json_id, "id", json_object_new_boolean(TRUE),
                              JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                  JSON_C_OBJECT_KEY_IS_CONSTANT);
    for (iter = id_fields; *iter; ++iter) {
        const char *field = *iter;
        json_object_object_add_ex(json_fields, field, json_id,
                                  JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                      JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
    ret = insert_json(sql, stmt,
                      (void (*)(const gpointer, const json_object *,
                                void (*)(const char *, gpointer),
                                gpointer))bygfoot_json_serialize_country,
                      json_fields, (const gpointer)country);
    return ret;
}

static gboolean
write_bygfoot(DataStore *parent, const Bygfoot *bygfoot)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->insert_bygfoot_stmt;
    json_object *json_fields = json_object_new_object();
    json_object *country_fields = json_object_new_object();
    json_object *intl_cup_fields = json_object_new_object();
    const char *fields[] = {
        "season",   "week",     "week_round",  "current_interest",
        "cur_user", "counters", "random_seed", NULL,
    };
    const char **iter;
    for (iter = fields; *iter; ++iter) {
        const char *field = *iter;
        json_object_object_add_ex(json_fields, field, NULL,
                                  JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                      JSON_C_OBJECT_KEY_IS_CONSTANT);
    }
    json_object_object_add_ex(
        country_fields, "sid", json_object_new_boolean(TRUE),
        JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(json_fields, "country", country_fields,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW |
                                  JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(
        intl_cup_fields, "id", json_object_new_boolean(TRUE),
        JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(
        json_fields, "international_cups", intl_cup_fields,
        JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);

    return insert_json(sql, stmt,
                       (void (*)(const gpointer, const json_object *,
                                 void (*)(const char *, gpointer),
                                 gpointer))bygfoot_json_serialize_bygfoot,
                       json_fields, (const gpointer)bygfoot);
}

static void
destroy(DataStore *parent)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_close(sql->db);
}

static gboolean
create_file(DataStore *parent, const char *name)
{
    gint rc;
    const char **iter;
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    static const char *tables[] = {
        "CREATE TABLE option (user TEXT, name TEXT, string_value TEXT, "
        "int_value INTEGER, type INTEGER);",
        "CREATE TABLE league (id INTEGER PRIMARY KEY, json TEXT);",
        "CREATE TABLE cup (id INTEGER PRIMARY KEY, json TEXT);",
        "CREATE TABLE team (id INTEGER PRIMARY KEY,"
        "name TEXT,"
        "symbol TEXT,"
        "names_file TEXT,"
        "def_file TEXT,"
        "strategy_sid TEXT,"
        "structure INTEGER,"
        "style INTEGER,"
        "boost INTEGER,"
        "average_talent REAL,"
        "luck REAL,"
        "first_team_id INTEGER,"
        "reserve_level INTEGER,"
        "league_id INTEGER,"
        "stadium_name TEXT,"
        "stadium_capacity INTEGER,"
        "stadium_average_attendance INTEGER,"
        "stadium_possible_attendance INTEGER,"
        "stadium_games INTEGER,"
        "stadium_safety REAL,"
        "stadium_ticket_price REAL);",
        "CREATE TABLE player (id INTEGER PRIMARY KEY,"
        "team_id INTEGER,"
        "name TEXT,"
        "pos INTEGER,"
        "cpos INTEGER,"
        "health INTEGER,"
        "recovery INTEGER,"
        "value INTEGER,"
        "wage INTEGER,"
        "offers INTEGER,"
        "streak INTEGER,"
        "skill REAL,"
        "cskill REAL,"
        "talent REAL,"
        "etal0 REAL,"
        "etal1 REAL,"
        "etal2 REAL,"
        "etal3 REAL,"
        "fitness REAL,"
        "lsu REAL,"
        "age REAL,"
        "peak_age REAL,"
        "peak_region REAL,"
        "contract REAL,"
        "streak_prob REAL,"
        "streak_count REAL,"
        "participation INTEGER,"
        "roster_index INTEGER);",
        "CREATE TABLE player_stat (player_id INTEGER, "
        "season_id INTEGER,"
        "stat_id INTEGER,"
        "competition_id INTEGER,"
        "games INTEGER,"
        "goals INTEGER,"
        "shots INTEGER,"
        "yellows INTEGER,"
        "reds INTEGER);",
        "CREATE TABLE user (json TEXT);",
        "CREATE TABLE transfer (json TEXT);",
        "CREATE TABLE season_stat (json TEXT);",
        "CREATE TABLE job (json TEXT);",
        "CREATE TABLE newspaper (json TEXT);",
        "CREATE TABLE bet (json TEXT);",
        "CREATE TABLE country (json TEXT);",
        "CREATE TABLE bygfoot (json TEXT);",
        NULL};

    for (iter = tables; *iter; ++iter) {
        const char *table = *iter;
        sqlite3_stmt *stmt;
        rc = sqlite3_prepare_v3(sql->db, table, -1, 0, &stmt, NULL);
        if (rc != SQLITE_OK) {
            return FALSE;
        }
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
            return FALSE;
        }
        rc = sqlite3_finalize(stmt);
        if (rc != SQLITE_OK) {
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean
set_name(DataStore *parent, const char *name)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    gint rc;

    struct query_stmt
    {
        const char *query;
        sqlite3_stmt **stmt;
    } queries[] = {
        {"BEGIN TRANSACTION;", &sql->begin_transaction_stmt},
        {"END TRANSACTION;", &sql->end_transaction_stmt},
        {"INSERT INTO option VALUES(?,?,?,?,?);", &sql->insert_option_stmt},
        {"INSERT INTO league VALUES(?, ?);", &sql->insert_league_stmt},
        {"INSERT INTO cup VALUES(?, ?);", &sql->insert_cup_stmt},
        {"INSERT INTO team VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
         "?, ?, ?, ?, ?, ?);",
         &sql->insert_team_stmt},
        {"INSERT INTO player VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
         "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
         &sql->insert_player_stmt},
        {"INSERT INTO player_stat VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);",
         &sql->insert_player_stat_stmt},
        {"INSERT INTO user VALUES(?);", &sql->insert_user_stmt},
        {"INSERT INTO transfer VALUES(?);", &sql->insert_transfer_stmt},
        {"INSERT INTO season_stat VALUES(?);", &sql->insert_season_stat_stmt},
        {"INSERT INTO job VALUES(?);", &sql->insert_job_stmt},
        {"INSERT INTO newspaper VALUES(?);", &sql->insert_newspaper_stmt},
        {"INSERT INTO bet VALUES(?);", &sql->insert_bet_stmt},
        {"INSERT INTO country VALUES(?);", &sql->insert_country_stmt},
        {"INSERT INTO bygfoot VALUES(?);", &sql->insert_bygfoot_stmt},
        {"SELECT * from option where type = ?", &sql->select_options_stmt},
        {"SELECT json from country;", &sql->select_countries_stmt},
        {"SELECT json from country where json_extract(json,'$.sid') == ?;",
         &sql->select_country_sid_stmt},
        {"SELECT json from bygfoot;", &sql->select_bygfoot_stmt},
        {"SELECT json from league where id = ?;", &sql->select_league_id_stmt},
        {"SELECT * from team where id = ?;", &sql->select_team_id_stmt},
        {"SELECT * from player where team_id = ? order by roster_index ASC;",
         &sql->select_team_players_stmt},
        {"SELECT json from cup where id = ?;", &sql->select_cup_id_stmt},
        {"SELECT * from player where id = ?;", &sql->select_player_id_stmt},
        {"SELECT * from player_stat where player_id = ? ORDER BY season_id "
         "ASC, stat_id ASC;",
         &sql->select_player_stat_stmt},
        {"SELECT json from user;", &sql->select_users_stmt},
        {"SELECT * from option where user = ?;",
         &sql->select_user_options_stmt},
        {"SELECT json from transfer;", &sql->select_transfers_stmt},
        {"SELECT json from bet;", &sql->select_bets_stmt},
        {"SELECT json from job;", &sql->select_jobs_stmt},
        {"SELECT json from season_stat;", &sql->select_season_stats_stmt},
        {"SELECT json from newspaper;", &sql->select_newspaper_stmt},
        {"CREATE INDEX IF NOT EXISTS player_stat_player_id ON "
         "player_stat(player_id);",
         &sql->create_player_stat_player_id_index_stmt},
        {"CREATE INDEX IF NOT EXISTS player_team_id ON player(team_id);",
         &sql->create_player_team_id_index_stmt},
        {NULL, NULL}};
    struct query_stmt *iter;

    if (sql->db) {
        const char *current_name = sqlite3_db_filename(sql->db, NULL);
        fprintf(stderr, "Name %s current name: %s\n", name, current_name);
        if (name && !strcmp(name, current_name))
            return TRUE;
        for (iter = queries; iter->query; ++iter) {
            rc = sqlite3_finalize(*iter->stmt);
            if (rc != SQLITE_OK) {
                SQLITE_LOG_ERROR(sql->db, rc, iter->query);
            }
        }
        sqlite3_close(sql->db);
        sql->db = NULL;
    }

    if (!name)
        return TRUE;

    fprintf(stderr, "name %s\n", name);
    rc = sqlite3_open_v2(name, &sql->db, SQLITE_OPEN_READWRITE, NULL);
    fprintf(stderr, "Open Readwrite %d\n", rc);
    if (rc == SQLITE_NOTADB)
        goto fail;
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Create new file\n");
        /* File doesn't exist, create it. */
        rc = sqlite3_open_v2(name, &sql->db,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (rc != SQLITE_OK)
            goto fail;
        if (!create_file(parent, name))
            goto fail;
    }

    for (iter = queries; iter->query; ++iter) {
        rc = sqlite3_prepare_v3(sql->db, iter->query, -1, 0, iter->stmt, NULL);
        if (rc != SQLITE_OK) {
            SQLITE_LOG_ERROR(sql->db, rc, iter->query);
            goto fail;
        }
    }

    return TRUE;

fail:
    sqlite3_close(sql->db);
    return FALSE;
}

DataStore *
sqlite_data_store_create(void)
{
    gint rc, i;
    SqliteDataStore *sql = calloc(1, sizeof(SqliteDataStore));

    sql->parent.set_name = set_name;
    sql->parent.begin_transaction = begin_transaction;
    sql->parent.end_transaction = end_transaction;
    sql->parent.write_options = write_options;
    sql->parent.write_league = write_league;
    sql->parent.write_cup = write_cup;
    sql->parent.write_team = write_team;
    sql->parent.write_player = write_player;
    sql->parent.write_user = write_user;
    sql->parent.write_transfer = write_transfer;
    sql->parent.write_season_stat = write_season_stat;
    sql->parent.write_job = write_job;
    sql->parent.write_newspaper = write_newspaper;
    sql->parent.write_bets = write_bets;
    sql->parent.write_country = write_country;
    sql->parent.write_bygfoot = write_bygfoot;
    sql->parent.destroy = destroy;

    return &sql->parent;
}

static struct json_object *
deserialize_json(const char *json)
{
    struct json_object *obj;
    enum json_tokener_error json_error;

    obj = json_tokener_parse_verbose(json, &json_error);
    if (!obj) {
        fprintf(stderr, "Failed to parse json %s:\n",
                json_tokener_error_desc(json_error));
        return NULL;
    }
    return obj;
}

static struct json_object *
select_json(sqlite3_stmt *stmt)
{
    sqlite3 *db = sqlite3_db_handle(stmt);
    gint rc = sqlite3_step(stmt);
    const char *json;
    json_object *obj;
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(db, rc, "error stepping argument");
        return FALSE;
    }
    json = sqlite3_column_text(stmt, 0);
    obj = deserialize_json(json);
    return obj;
}

static gboolean
read_league(DataStore *parent, Bygfoot *bygfoot, League *league)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_league_id_stmt;
    json_object *obj, *teams_obj;
    const char *json;
    gboolean ret = TRUE;
    int rc;

    rc = sqlite3_bind_int(stmt, 1, league->c.id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    obj = select_json(stmt);
    if (!obj) {
        goto reset;
        ret = FALSE;
    }
    teams_obj = json_object_object_get(obj, "teams");
    if (teams_obj) {
        gint i, len;
        for (i = 0, len = json_object_array_length(teams_obj); i < len; ++i) {
            json_object *team_obj = json_object_array_get_idx(teams_obj, i);
            json_object_set_userdata(team_obj, league, NULL);
        }
    }
    if (!bygfoot_json_deserialize_league(bygfoot, obj, &league))
        ret = FALSE;
    json_object_put(obj);
reset:
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        ret = FALSE;
    }
    return ret;
}

static gboolean
read_leagues(DataStore *parent, Bygfoot *bygfoot, Country *country)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_league_id_stmt;
    gint i;

    for (i = 0; i < country->leagues->len; i++) {
        League *league = g_ptr_array_index(country->leagues, i);
        read_league(parent, bygfoot, league);
        league->country = country;
    }
    return TRUE;
}

static gboolean
read_cup(DataStore *parent, Bygfoot *bygfoot, Cup *cup)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_cup_id_stmt;
    json_object *obj;
    const char *json;
    int rc;
    gboolean ret = TRUE;

    rc = sqlite3_bind_int(stmt, 1, cup->c.id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    obj = select_json(stmt);
    if (!obj) {
        ret = FALSE;
        goto reset;
    }
    if (!bygfoot_json_deserialize_cup(bygfoot, obj, &cup))
        ret = FALSE;
    json_object_put(obj);
reset:
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }
}

static gboolean
read_cups(DataStore *parent, Bygfoot *bygfoot, Country *country)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_cup_id_stmt;
    gint i;

    for (i = 0; i < country->cups->len; i++) {
        Cup *cup = g_ptr_array_index(country->cups, i);
        read_cup(parent, bygfoot, cup);
    }
    return TRUE;
}

static gboolean
row_to_player(sqlite3_stmt *stmt, Player *player)
{
    gint opidx = 0;
#define GET_COLUMN_STRING(field)                                               \
    if (!player->field) {                                                      \
        gint bytes = sqlite3_column_bytes(stmt, opidx);                        \
        player->field = calloc(1, bytes + 1);                                  \
        strncpy(player->field, sqlite3_column_text(stmt, opidx++), bytes);     \
    }
#define GET_COLUMN(name, type)                                                 \
    player->name = sqlite3_column_##type(stmt, opidx++);

    GET_COLUMN(id, int);
    opidx++; /* team_id */
    GET_COLUMN_STRING(name);
    GET_COLUMN(pos, int);
    GET_COLUMN(cpos, int);
    GET_COLUMN(health, int);
    GET_COLUMN(recovery, int);
    GET_COLUMN(value, int);
    GET_COLUMN(wage, int);
    GET_COLUMN(offers, int);
    GET_COLUMN(streak, int);
    GET_COLUMN(skill, double);
    GET_COLUMN(cskill, double);
    GET_COLUMN(talent, double);
    GET_COLUMN(etal[0], double);
    GET_COLUMN(etal[1], double);
    GET_COLUMN(etal[2], double);
    GET_COLUMN(etal[3], double);
    GET_COLUMN(fitness, double);
    GET_COLUMN(lsu, double);
    GET_COLUMN(age, double);
    GET_COLUMN(peak_age, double);
    GET_COLUMN(peak_region, double);
    GET_COLUMN(contract, double);
    GET_COLUMN(streak_prob, double);
    GET_COLUMN(streak_count, double);
    GET_COLUMN(participation, int);

#undef GET_COLUMN_STRING
#undef GET_COLUMN

    return TRUE;
}

static gboolean
read_team_players(DataStore *parent, Bygfoot *bygfoot, Team *team)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_team_players_stmt;
    gboolean ret = TRUE;
    gint rc;

    rc = sqlite3_bind_int(stmt, 1, team->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Player *new_player;
        new_player = malloc(sizeof(Player));
        memset(new_player, 0, sizeof(Player));
        new_player->team = team;
        if (!row_to_player(stmt, new_player)) {
            ret = FALSE;
            break;
        }
        read_player_stats(parent, bygfoot, new_player);
        g_ptr_array_add(team->players, new_player);
    }

    if (!ret)
        return FALSE;

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }
    return TRUE;
}

static gboolean
read_team(DataStore *parent, Bygfoot *bygfoot, League *league, Team *team)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_team_id_stmt;
    json_object *obj;
    const char *json;
    int rc;
    gboolean ret = TRUE;
    gint opidx = 0;

    rc = sqlite3_bind_int(stmt, 1, team->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

#define GET_COLUMN_STRING(field)                                               \
    if (!team->field) {                                                        \
        if (sqlite3_column_type(stmt, opidx) == SQLITE_NULL) {                 \
            opidx++;                                                           \
        } else {                                                               \
            gint bytes = sqlite3_column_bytes(stmt, opidx);                    \
            team->field = calloc(1, bytes + 1);                                \
            strncpy(team->field, sqlite3_column_text(stmt, opidx++), bytes);   \
        }                                                                      \
    }

#define GET_COLUMN(field, type)                                                \
    team->field = sqlite3_column_##type(stmt, opidx++);

    GET_COLUMN(id, int);
    GET_COLUMN_STRING(name);
    GET_COLUMN_STRING(symbol);
    GET_COLUMN_STRING(names_file);
    GET_COLUMN_STRING(def_file);
    GET_COLUMN_STRING(strategy_sid);
    GET_COLUMN(structure, int);
    GET_COLUMN(style, int);
    GET_COLUMN(boost, int);
    GET_COLUMN(average_talent, double);
    GET_COLUMN(luck, double);
    GET_COLUMN(first_team.id, int);
    GET_COLUMN(reserve_level, int);
    opidx++; /* league id */
    GET_COLUMN_STRING(stadium.name);
    GET_COLUMN(stadium.capacity, int);
    GET_COLUMN(stadium.average_attendance, int);
    GET_COLUMN(stadium.possible_attendance, int);
    GET_COLUMN(stadium.games, int);
    GET_COLUMN(stadium.safety, double);
    GET_COLUMN(stadium.ticket_price, double);

#undef GET_COLUMN_STRING
#undef GET_COLUMN

    team->league = league;
    team->country = league->country;
    /* In most cases (non-reserve teams), a team's first team is itself,
     * so optimize for that case.
     */
    team->first_team.team = team->first_team.id == team->id
                                ? team
                                : team_of_id(team->first_team.id);
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }
    return ret;
}

static gboolean
read_teams(DataStore *parent, Bygfoot *bygfoot, League *league)
{
    gint i;
    for (i = 0; i < league->teams->len; i++) {
        Team *team = g_ptr_array_index(league->teams, i);
        read_team(parent, bygfoot, league, team);
    }
    return TRUE;
}

static gboolean
read_player_json(DataStore *parent, Bygfoot *bygfoot, Player *player,
                 Team *team)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_player_id_stmt;
    json_object *obj, *team_obj;
    const char *json;
    int rc;
    gboolean ret = TRUE;

    rc = sqlite3_bind_int(stmt, 1, player->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    obj = select_json(stmt);
    if (!obj) {
        ret = FALSE;
        goto reset;
    }
    team_obj = json_object_object_get(obj, "team");
    if (team_obj)
        json_object_set_userdata(team_obj, team, NULL);
    if (!bygfoot_json_deserialize_player(bygfoot, obj, &player))
        ret = FALSE;
    json_object_put(obj);
reset:
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }
    return ret;
}

static gboolean
read_player(DataStore *parent, Bygfoot *bygfoot, Player *player, Team *team)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_player_id_stmt;
    int rc, season_id = -1;
    int name_bytes;
    GArray *season_stats;
    gboolean ret = TRUE;

    rc = sqlite3_bind_int(stmt, 1, player->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    if (!row_to_player(stmt, player))
        return FALSE;

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }
    return TRUE;
}

static gboolean
read_player_stats(DataStore *parent, Bygfoot *bygfoot, Player *player)
{
    SqliteDataStore *sql = (SqliteDataStore *)parent;
    sqlite3_stmt *stmt = sql->select_player_stat_stmt;
    int rc, season_id = -1;
    GArray *season_stats;
    gboolean ret = TRUE;

    rc = sqlite3_bind_int(stmt, 1, player->id);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    player->stats = NULL;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        PlayerCompetitionStats stats;
        gint row_season_id = sqlite3_column_int(stmt, 1);
        gint competition_id = sqlite3_column_int(stmt, 3);
        if (season_id != row_season_id) {
            season_stats =
                g_array_new(FALSE, FALSE, sizeof(PlayerCompetitionStats));
            player->stats = g_list_append(player->stats, season_stats);
            season_id = row_season_id;
        }
        if (competition_id == 0)
            continue;
        stats.competition = bygfoot_get_competition_id(bygfoot, competition_id);
        stats.games = sqlite3_column_int(stmt, 4);
        stats.goals = sqlite3_column_int(stmt, 5);
        stats.shots = sqlite3_column_int(stmt, 6);
        stats.yellow = sqlite3_column_int(stmt, 7);
        stats.red = sqlite3_column_int(stmt, 8);
        g_array_append_val(season_stats, stats);
    }

    if (!player->stats)
        player->stats = g_list_append(
            player->stats,
            g_array_new(FALSE, FALSE, sizeof(PlayerCompetitionStats)));

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }

    return TRUE;
}

static gboolean
read_players(DataStore *parent, Bygfoot *bygfoot, Team *team)
{
    gint i;
    for (i = 0; i < team->players->len; i++) {
        Player *player = g_ptr_array_index(team->players, i);
        player->team = team;
        read_player(parent, bygfoot, player, team);
    }
    return TRUE;
}

gboolean static select_options(OptionList *options, sqlite3_stmt *stmt)
{
    gint rc;
    sqlite3 *db = sqlite3_db_handle(stmt);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *name = sqlite3_column_text(stmt, 1);
        const char *string_value = sqlite3_column_text(stmt, 2);
        gint int_value = sqlite3_column_int(stmt, 3);
        option_add(options, name, int_value, string_value);
    }
    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(db, rc, "errpr reseting statement");
        return FALSE;
    }
    return TRUE;
}

gboolean
sqlite_load_bygfoot(Bygfoot *bygfoot)
{
    SqliteDataStore *sql = (SqliteDataStore *)bygfoot->data_store;
    sqlite3_stmt *stmt;
    json_object *obj;
    const char *json;
    int rc;
    gint i;
    Country *country_ptr = &country;
    json_object *allcups;
    GPtrArray *allcups_list = g_ptr_array_new();

    stmt = sql->create_player_stat_player_id_index_stmt;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
        ;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    stmt = sql->create_player_team_id_index_stmt;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
        ;
    }
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    begin_transaction(bygfoot->data_store);
    /*set_name(bygfoot->data_store, name);*/

    /* Load global options. */
    stmt = sql->select_options_stmt;
    rc = sqlite3_bind_int(stmt, 1, OPTION_TYPE_OPTIONS);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error int argument");
        return FALSE;
    }
    if (!select_options(&options, stmt))
        return FALSE;

    /* Load global settings */
    rc = sqlite3_bind_int(stmt, 1, OPTION_TYPE_SETTINGS);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error int argument");
        return FALSE;
    }
    if (!select_options(&settings, sql->select_options_stmt))
        return FALSE;

    /* Load bygfoot */
    stmt = sql->select_bygfoot_stmt;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
        ;
    }
    json = sqlite3_column_text(stmt, 0);
    bygfoot_json_deserialize_bygfoot(json, bygfoot);
    g_rand_set_seed(rand_generator, bygfoot->random_seed);

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* Load the current country. */
    stmt = sql->select_country_sid_stmt;
    rc = sqlite3_bind_text(stmt, 1, country.sid, strlen(country.sid), NULL);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "error binding text argument");
        return FALSE;
    }
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ERROR || rc == SQLITE_MISUSE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
        ;
    }
    json = sqlite3_column_text(stmt, 0);
    free(country.sid);
    country.sid = NULL;
    obj = deserialize_json(json);
    if (!obj)
        return FALSE;

    /* We can't parse allcups until all the cups are parsed so we need
     * to store it and parse it later. */
    allcups = json_object_get(json_object_object_get(obj, "allcups"));
    json_object_object_del(obj, "allcups");

    if (!bygfoot_json_deserialize_country(bygfoot, obj, &country_ptr))
        return FALSE;
    json_object_put(obj);
    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* Load the countries. */
    if (country_list)
        g_ptr_array_free(country_list, TRUE);
    country_list = g_ptr_array_new();
    stmt = sql->select_countries_stmt;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Country *c = NULL;
        const char *json = sqlite3_column_text(stmt, 0);
        const char *sid;
        json_object *obj = deserialize_json(json);
        // printf("json %s\n", json);
        // printf("object: %s\n", json_object_to_json_string(obj));
        if (!obj)
            return FALSE;
        sid = json_object_get_string(json_object_object_get(obj, "sid"));
        if (!strcmp(sid, country.sid)) {
            json_object_put(obj);
            continue;
        }
        g_ptr_array_add(allcups_list, json_object_get(json_object_object_get(
                                          obj, "allcups")));
        // FIXME: Cuases a crash
        // json_object_object_del(obj, "allcups");
        if (!bygfoot_json_deserialize_country(bygfoot, obj, &c))
            return FALSE;
        json_object_put(obj);

        g_ptr_array_add(country_list, c);
    }

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* The order we read the different data structures matters.
     *
     * * We need to read the teams before the fixtures, because the fixtures
     *   contain team ptrs.
     * * We need to read the cups and leagues before the players, because
     *   their stats contain team/cup ptrs.
     */

    /* Load the leagues. */
    read_leagues(bygfoot->data_store, bygfoot, &country);

    /* Load the Teams. */
    for (i = 0; i < country.leagues->len; i++) {
        League *league = g_ptr_array_index(country.leagues, i);
        read_teams(bygfoot->data_store, bygfoot, league);
    }
    /* Load the cups. */
    read_cups(bygfoot->data_store, bygfoot, &country);

    for (i = 0; i < country_list->len; i++) {
        int j;
        Country *country = g_ptr_array_index(country_list, i);
        read_leagues(bygfoot->data_store, bygfoot, country);
        for (j = 0; j < country->leagues->len; j++) {
            League *league = g_ptr_array_index(country->leagues, j);
            read_teams(bygfoot->data_store, bygfoot, league);
        }
    }

    /* FIXME: There are still some international cups which are referenced
     * in the country definitions, so we need to first load all the teams
     * for all the countries and then load all the cups, because if a
     * country references an international cup then it could require teams
     * from another country.
     */
    for (i = 0; i < country_list->len; i++) {
        Country *country = g_ptr_array_index(country_list, i);
        read_cups(bygfoot->data_store, bygfoot, country);
    }

    for (i = 0; i < bygfoot->international_cups->len; i++) {
        Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        read_cup(bygfoot->data_store, bygfoot, cup);
    }

    /* Load the allcups. */
    if (!bygfoot_json_deserialize_allcups(bygfoot, allcups, &country.allcups))
        return FALSE;
    json_object_put(allcups);

    for (i = 0; i < country_list->len; i++) {
        json_object *a = g_ptr_array_index(allcups_list, i);
        Country *c = g_ptr_array_index(country_list, i);
        if (!bygfoot_json_deserialize_allcups(bygfoot, a, &c->allcups))
            return FALSE;
        json_object_put(a);
    }

    /* Load the players */
    for (i = 0; i < country.leagues->len; i++) {
        League *league = g_ptr_array_index(country.leagues, i);
        gint j;
        for (j = 0; j < league->teams->len; j++) {
            Team *team = g_ptr_array_index(league->teams, j);
            read_team_players(bygfoot->data_store, bygfoot, team);
        }
    }

    for (i = 0; i < country_list->len; i++) {
        Country *country = g_ptr_array_index(country_list, i);
        gint j;
        for (j = 0; j < country->leagues->len; j++) {
            League *league = g_ptr_array_index(country->leagues, j);
            gint k;
            for (k = 0; k < league->teams->len; k++) {
                Team *team = g_ptr_array_index(league->teams, k);
                read_team_players(bygfoot->data_store, bygfoot, team);
            }
        }
    }

    /* Load the users. */
    stmt = sql->select_users_stmt;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        g_array_set_size(users, users->len + 1);
        User *user = &g_array_index(users, User, users->len - 1);
        *user = user_new();
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        gint rc_opt;
        if (!obj)
            return FALSE;
        if (!bygfoot_json_deserialize_user(bygfoot, obj, &user))
            return FALSE;
        json_object_put(obj);

        /* Select user options. */
        rc_opt = sqlite3_bind_text(sql->select_user_options_stmt, 1, user->name,
                                   -1, NULL);
        if (rc_opt != SQLITE_OK)
            return FALSE;
        if (!select_options(&user->options, sql->select_user_options_stmt))
            return FALSE;
        rc_opt = sqlite3_reset(sql->select_user_options_stmt);
        if (rc_opt != SQLITE_OK) {
            SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
            return FALSE;
        }
    }

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* Load the transfers */
    transfer_list = g_array_new(FALSE, FALSE, sizeof(Transfer));
    stmt = sql->select_transfers_stmt;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Transfer transfer;
        Transfer *transfer_ptr = &transfer;
        memset(transfer_ptr, 0, sizeof(Transfer));
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        if (!obj)
            return FALSE;
        if (!bygfoot_json_deserialize_transfer(bygfoot, obj, &transfer_ptr))
            return FALSE;
        json_object_put(obj);
        g_array_append_val(transfer_list, transfer);
    }

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* Load the bets. */
    stmt = sql->select_bets_stmt;

    /* There is just one row with all the bets. */
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        return FALSE;
    {
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        if (!obj)
            return FALSE;
        if (!bygfoot_json_deserialize_bets(bygfoot, obj, bets))
            return FALSE;
        json_object_put(obj);
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
    }

    /* Load the jobs. */
    stmt = sql->select_jobs_stmt;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Job job;
        Job *job_ptr = &job;
        memset(job_ptr, 0, sizeof(Job));
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        if (!obj)
            return FALSE;
        if (!bygfoot_json_deserialize_job(bygfoot, obj, job_ptr))
            return FALSE;
        json_object_put(obj);
        g_array_append_val(jobs, job);
    }

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    /* Load the newspapers. */
    stmt = sql->select_newspaper_stmt;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        NewsPaperArticle article;
        NewsPaperArticle *article_ptr = &article;
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        memset(article_ptr, 0, sizeof(NewsPaperArticle));

        if (!obj)
            return FALSE;
        if (!bygfoot_json_deserialize_newspaper_article(bygfoot, obj,
                                                        article_ptr))
            return FALSE;
        json_object_put(obj);
        g_array_append_val(newspaper.articles, article);
    }

    /* Load the season stats. */
    stmt = sql->select_season_stats_stmt;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        SeasonStat stat;
        SeasonStat *stat_ptr = &stat;
        const char *json = sqlite3_column_text(stmt, 0);
        json_object *obj = deserialize_json(json);
        if (!obj)
            return FALSE;
        memset(stat_ptr, 0, sizeof(SeasonStat));
        if (!bygfoot_json_deserialize_season_stat(bygfoot, obj, stat_ptr))
            return FALSE;
        json_object_put(obj);
        g_array_append_val(season_stats, stat);
    }

    if (rc != SQLITE_DONE) {
        SQLITE_LOG_ERROR(sql->db, rc, "error stepping argument");
        return FALSE;
    }

    rc = sqlite3_reset(stmt);
    if (rc != SQLITE_OK) {
        SQLITE_LOG_ERROR(sql->db, rc, "errpr reseting statement");
        return FALSE;
        ;
    }

    end_transaction(bygfoot->data_store);
    return TRUE;
}
