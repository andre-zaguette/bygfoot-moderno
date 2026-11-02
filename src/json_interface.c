
#include <json-c/json.h>
#include <json-c/json_tokener.h>
#include <json-c/json_object.h>
#include "json_interface.h"
#include "json_serialize.h"
#include "user.h"
#include "league_struct.h"
#include "load_save.h"
#include "misc.h"
#include "team.h"
#include "option.h"
#include "start_end.h"

static int bygfoot_json_do_commands(Bygfoot *bygfoot, const json_object *commands);
static int bygfoot_json_do_add_user(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_load_bygfoot(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_save_bygfoot(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_dump_bygfoot(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_add_country(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_add_user(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_start_bygfoot(Bygfoot *bygfoot, const json_object *args);
static void bygfoot_json_call_simulate_games(Bygfoot *bygfoot,
                                                      const json_object *args);
static void bygfoot_json_call_get_tables(Bygfoot *bygfoot,
                                                        const json_object *args);
static void bygfoot_json_call_get_players(Bygfoot *bygfoot,
                                                         const json_object *args);
static void bygfoot_json_call_get_fixtures(Bygfoot *bygfoot,
                                                          const json_object *args);
void bygfoot_json_call_get_cups(Bygfoot *bygfoot, const json_object *args);
void bygfoot_json_call_get_leagues(Bygfoot *bygfoot, const json_object *args);
void bygfoot_json_response_error(const char *command,
                                               const char *error);
void bygfoot_json_fixture_to_json(const Fixture *fixture);
void bygfoot_json_player_to_json(const Player *player);
void bygfoot_json_user_to_json(const User *user);
void bygfoot_json_table_to_json(const Table *table);

struct json_field {
    const gchar *name;
    enum json_type type;
};

static gboolean
bygfoot_json_validate_arg_types(const struct json_object *args,
                                const struct json_field *fields)
{
    const struct json_field *field;

    for (field = fields; field->name; field++ ) {
        struct json_object *value;
        if (!json_object_object_get_ex(args, field->name, &value))
            continue;
        if (!json_object_is_type(value, field->type))
            return FALSE;
    }
    return TRUE;
}

int bygfoot_json_main(Bygfoot *bygfoot, const CommandLineArgs *cl_args)
{
    gchar *contents;
    GError *error;
    struct json_object *json;
    enum json_tokener_error json_error;

    /* Disable option to skip rounds without user games, so the simulate_games
     * command works correctly. */
    opt_set_int("int_opt_skip", 0);

    if (!g_file_get_contents(cl_args->json_filename, &contents, NULL, &error)) {
        misc_print_error(&error, FALSE);
        return 1;
    }

    json = json_tokener_parse_verbose(contents, &json_error);
    if (!json) {
        fprintf(stderr, "Failed to parse json %s:\n", cl_args->json_filename);
        fprintf(stderr, "%s\n",
                json_tokener_error_desc(json_error));
        return 1;
    }

    /* Handle configuration options */
    /* TODO: Get option from json */
    //bygfoot_set_save_dir(bygfoot, NULL);

    json_object_object_foreach(json, key, val) {
        if (!strcmp("commands", key))
            return bygfoot_json_do_commands(bygfoot, val);
    }
    fprintf(stderr, "commands key is not present");
    return 1;
}

static int bygfoot_json_do_commands(Bygfoot *bygfoot, const json_object *commands)
{
    size_t i, num_commands;

    static const struct json_func {
        const gchar *command;
        void (*func)(Bygfoot *, const json_object *);
    } json_funcs[] = {
        { "load_bygfoot", bygfoot_json_call_load_bygfoot },
        { "save_bygfoot", bygfoot_json_call_save_bygfoot },
        { "dump_bygfoot", bygfoot_json_call_dump_bygfoot },
        { "add_country", bygfoot_json_call_add_country },
        { "add_user", bygfoot_json_call_add_user },
        { "start_bygfoot", bygfoot_json_call_start_bygfoot },
        { "simulate_games", bygfoot_json_call_simulate_games },
        { "get_tables", bygfoot_json_call_get_tables },
        { "get_players", bygfoot_json_call_get_players },
        { "get_fixtures", bygfoot_json_call_get_fixtures },
        { "cups", bygfoot_json_call_get_cups },
	{ "leagues", bygfoot_json_call_get_leagues },
        { NULL, NULL}
    };

    if (!json_object_is_type(commands, json_type_array)) {
        fprintf(stderr, "json commands should be in an array\n");
        return 1;
    }

    num_commands = json_object_array_length(commands);
    for (i = 0; i < num_commands; i++) {
        const json_object *command = json_object_array_get_idx(commands, i);
        struct json_object *response = NULL;
        // TODO: CHECK COMMAND SIZE

        json_object_object_foreach(command, key, val) {
            const struct json_func *json_func;
            for (json_func = json_funcs; json_func->command; json_func++) {
                if (!strcmp(json_func->command, key)) {
                    json_func->func(bygfoot, val);
                    break;
                }
            }
        }
    }
    return 0;
}

static const gchar *
bygfoot_json_validate_bygfoot_id(const json_object *args, json_object **error)
{
    const gchar *id;
    /* Get the id field */
    json_object *id_obj = json_object_object_get(args, "id");

    if (id_obj) {
        if(!json_object_is_type(id_obj, json_type_string)) {
            bygfoot_json_response_error("", "field 'id' must be a string");
            return NULL;
        }
        return json_object_get_string(id_obj);
    }

    bygfoot_json_response_error("", "field 'id' is missing or NULL");
    return NULL;
}

static void 
bygfoot_json_call_load_bygfoot(Bygfoot *bygfoot, const json_object *args)
{
    const gchar *id = NULL;
    struct json_object *filename_obj;
    const gchar *filename;
    static const struct json_field fields [] = {
        { "filename", json_type_string }
    };

    if (!bygfoot_json_validate_arg_types(args, fields)) {
        bygfoot_json_response_error("load_bygfoot",
                                           "wrong type for argument");
        return;
    }

    if (!json_object_object_get_ex(args, "filename", &filename_obj)) {
        bygfoot_json_response_error("load_bygfoot",
                                          "filename argument is required");
        return;
    }

    filename = json_object_get_string(filename_obj);
    if (!g_file_test(filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
        bygfoot_json_response_error("load_bygfoot", "file not found");
        return;
     }

    load_save_load_game(bygfoot, filename, FALSE);
}

static void 
bygfoot_json_call_save_bygfoot(Bygfoot *bygfoot, const json_object *args)
{
    const gchar *id = NULL;
    struct json_object *filename_obj;
    const gchar *filename;
    static const struct json_field fields [] = {
        { "filename", json_type_string }
    };

    if (!bygfoot_json_validate_arg_types(args, fields))
        return bygfoot_json_response_error("save_bygfoot",
                                           "wrong type for argument");

    if (!json_object_object_get_ex(args, "filename", &filename_obj))
        return bygfoot_json_response_error("save_bygfoot",
                                          "filename argument is required");

    filename = json_object_get_string(filename_obj);

    load_save_save_game(bygfoot, filename);
}

void
bygfoot_json_call_dump_bygfoot(Bygfoot *bygfoot, const json_object *args)
{
    bygfoot_json_serialize_bygfoot(bygfoot, NULL,
                                   bygfoot_json_serialize_write_stdout, NULL);
}

static void
bygfoot_json_call_add_country(Bygfoot *bygfoot, const json_object *args)
{
    gchar const *country_name = NULL;
    static const struct json_field fields [] = {
        { "name", json_type_string },
        { NULL, json_type_null }
    };
    const struct json_field *field;

    for (field = fields; field->name; field++ ) {
        struct json_object *value = json_object_object_get(args, field->name);
        if (!value)
            continue;
        if (!json_object_is_type(value, field->type))
            continue;

        if (!strcmp(field->name, "name"))
            country_name = json_object_get_string(value);
    }

    if (!country_name)
        return bygfoot_json_response_error("add_country", "field 'country' is required");

    bygfoot_load_country(bygfoot, country_name);
    return;
}

static void bygfoot_json_call_add_user(Bygfoot *bygfoot, const json_object *args)
{
    const char *command = "add_user";
    const char *username = NULL;
    const char *country_name = NULL;
    const char *team_name = NULL;
    struct json_object *response;
    Country *country;
    Team *tm;
    User *user;
    json_object_object_foreach(args, key, val) {
        if (!strcmp("username", key)) {
            if (!json_object_is_type(val, json_type_string)) {
                return bygfoot_json_response_error(command, "username must be of type string");
            }
            username = json_object_get_string(val);
        }
        if (!strcmp("country", key)) {
            if (!json_object_is_type(val, json_type_string)) {
                return bygfoot_json_response_error(command, "country must be of type string");
            }
            country_name = json_object_get_string(val);
        }
        if (!strcmp("team", key)) {
            if (!json_object_is_type(val, json_type_string)) {
                return bygfoot_json_response_error(command, "team must be of type string");
            }
            team_name = json_object_get_string(val);
        }
    }

    if (!username) {
        return bygfoot_json_response_error(command, "field 'username' is required");
    }
    if (!country_name) {
        return bygfoot_json_response_error(command, "field 'country' is required");
    }
    if (!team_name) {
        return bygfoot_json_response_error(command, "field 'team' is required");
    }

    country = bygfoot_load_country(bygfoot, country_name);
    tm = team_of_sid(team_name, country);
    user = bygfoot_add_user(bygfoot, username, tm);

    return;
}

static void
bygfoot_json_call_start_bygfoot(Bygfoot *bygfoot,
                                                  const json_object *args)
{
    unsigned i;
    bygfoot_start_game(bygfoot);

    /* FIXME: We should really be doing this when the user is added, but
     * the user options get loaded during the bygfoot_start_bygfoot() call and w
     * can't add options before that.
     */
    for (i = 0; i < users->len; i++) {
        /* We need to set this option to avoid activating the GUI.
         * FIXME: More work is needed to separate the logic from the GUI */
        option_set_int("int_opt_user_show_live_game", &usr(i).options, 0);
    }
}

static void simulate_weeks(Bygfoot *bygfoot, gint weeks)
{
    gint num_weeks = 0;
    gint current_week = week;
    while (num_weeks < weeks) {
        do {
            end_week_round(bygfoot);
        } while (current_week == week);
        current_week = week;
        num_weeks++;
    }
}

static void bygfoot_json_call_simulate_games(Bygfoot *bygfoot,
                                                      const json_object *args)
{
    int32_t rounds = 0;
    int32_t weeks = 0;
    int32_t seasons = 0;
    unsigned i;
    json_object_object_foreach(args, key, val) {
        if (!strcmp("rounds", key)) {
            rounds = json_object_get_int(val);
            for (i = 0; i < rounds; i++) {
                end_week_round(bygfoot);
            }
        } else if (!strcmp("weeks", key)) {
            weeks = json_object_get_int(val);
            simulate_weeks(bygfoot, weeks);
        } else if (!strcmp("seasons", key)) {
            gint num_seasons = 0;
            gint start_week = week;
            gint current_season = season;
            seasons = json_object_get_int(val);
            while (num_seasons < seasons) {
                do {
                    end_week_round(bygfoot);
                } while (current_season == season);
                current_season = season;
                num_seasons++;
            }
            simulate_weeks(bygfoot, start_week);
        }
    }
}

static void
bygfoot_json_call_get_fixtures(Bygfoot *bygfoot, const json_object *args)
{
    struct json_object *fixtures_obj = json_object_new_array();
    struct json_object *response, *data;
    int i;
    for (i = 0; i < country.leagues->len; i++) {
        const League *league = g_ptr_array_index(country.leagues, i);
        int j;
        for (j = 0; j < league->fixtures->len; j++) {
            const Fixture *fixture = &g_array_index(league->fixtures, Fixture, j);
            bygfoot_json_serialize_fixture(fixture, NULL, bygfoot_json_serialize_write_stdout, NULL);
        }
    }
    for (i = 0; i < country.cups->len; i++) {
        const Cup *cup = g_ptr_array_index(country.cups, i);
        int j;
        for (j = 0; j < cup->fixtures->len; j++) {
            const Fixture *fixture = &g_array_index(cup->fixtures, Fixture, j);
	    bygfoot_json_serialize_fixture(fixture, NULL, bygfoot_json_serialize_write_stdout, NULL);
        }
    }
    for (i = 0; i < bygfoot->international_cups->len; i++) {
        const Cup *cup = g_ptr_array_index(bygfoot->international_cups, i);
        int j;
        for (j = 0; j < cup->fixtures->len; j++) {
            const Fixture *fixture = &g_array_index(cup->fixtures, Fixture, j);
            bygfoot_json_serialize_fixture(fixture, NULL, bygfoot_json_serialize_write_stdout, NULL);
        }
    }
}

static void
bygfoot_json_call_get_tables(Bygfoot *bygfoot, const json_object *args)
{
    gchar const *country_name = NULL;
    static const struct json_field fields [] = {
        { NULL, json_type_null }
    };
    const struct json_field *field;
    int i;
    struct json_object *response, *data, *tables;

    for (field = fields; field->name; field++ ) {
        struct json_object *value = json_object_object_get(args, field->name);
        if (!value)
            continue;
        if (!json_object_is_type(value, field->type))
            continue;
    }

    tables = json_object_new_array();
    //GArray *leagues, *cups;
    for (i = 0; i < country.leagues->len; i++) {
        const League *league = g_ptr_array_index(country.leagues, i);
        int j;
        for (j = 0; j < league->tables->len; j++) {
            const Table *table = &g_array_index(league->tables, Table, j);
            bygfoot_json_serialize_table(table, NULL, bygfoot_json_serialize_write_stdout, NULL);
        }
    }
    return;
}

static void
bygfoot_json_call_get_players(Bygfoot *bygfoot, const json_object *args)
{
    struct json_object *players_obj = json_object_new_array();
    struct json_object *response, *data;
    int i;
    for (i = 0; i < country.leagues->len; i++) {
        const League *league = g_ptr_array_index(country.leagues, i);
        int j;
        for (j = 0; j < league->teams->len; j++) {
            const Team *team = g_ptr_array_index(league->teams, j);
            int k;
            for (k = 0; k < team->players->len; k++) {
                const Player *player = g_ptr_array_index(team->players, k);
                bygfoot_json_serialize_player(player, NULL, bygfoot_json_serialize_write_stdout, NULL);
            }

        }
    }
    return;
}

void
bygfoot_json_call_get_cups(Bygfoot *bygfoot, const json_object *args)
{
    bygfoot_json_serialize_cups(country.cups, NULL,
                                bygfoot_json_serialize_write_stdout, NULL);
}

void
bygfoot_json_call_get_leagues(Bygfoot *bygfoot, const json_object *args)
{
    bygfoot_json_serialize_leagues(country.leagues, NULL,
                                   bygfoot_json_serialize_write_stdout,
                                   NULL);
}

static void 
bygfoot_json_live_game_stats_to_json(const LiveGameStats *stats, gint team_index)
{

}

void bygfoot_json_response_error(const char *command, const char *error)
{
    printf("'error' : '%s'\n", error);
}

static void bygfoot_json_error_to_console(const char *command, const char *error)
{
    bygfoot_json_response_error(command, error);
}
