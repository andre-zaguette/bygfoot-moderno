#include <glib.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <stdio.h>

#include "cup_struct.h"
#include "fixture.h"
#include "free.h"
#include "json_deserialize.h"
#include "league.h"
#include "league_struct.h"
#include "live_game_struct.h"
#include "misc.h"
#include "team.h"
#include "team_struct.h"
#include "user.h"
#include "variables.h"

#include "json_serialize.h" /* For debugging */

typedef gboolean (*deserialize_func_type)(Bygfoot *, json_object *, gpointer *);

#define DESERIALIZE_FIELD_CUSTOM(parent, field, func)                          \
    {                                                                          \
        json_object *json_obj = json_object_object_get(parent, field);         \
        if (json_obj)                                                          \
            func;                                                              \
    }

#define DESERIALIZE_FIELD(parent, object, field, func)                         \
    {                                                                          \
        json_object *json_obj = json_object_object_get(parent, #field);        \
        if (json_obj) {                                                        \
            func(bygfoot, json_obj, &(object->field));                         \
        }                                                                      \
    }

#define DESERIALIZE_GARRAY_FUNC_DEF(func, item_type, item_deserialize_func)    \
    GArray *func(const json_object *array_obj)                                 \
    {                                                                          \
        GArray *array;                                                         \
        gint i, len;                                                           \
        if (!json_object_is_type(array_obj, json_type_array)) {                \
            fprintf(stderr, #item_type ": invalid json\n");                    \
            return NULL;                                                       \
        }                                                                      \
        len = json_object_array_length(array_obj);                             \
        array = g_array_sized_new(FALSE, FALSE, sizeof(item_type), len);       \
        for (i = 0; i < array->len; i++) {                                     \
            json_object *obj = json_object_array_get_idx(array_obj, i);        \
            item_type *item = item_deserialize_func(obj);                      \
            g_array_append_val(array, *item);                                  \
            g_free(item);                                                      \
        }                                                                      \
        return array;                                                          \
    }

struct ptr_update_userdata
{
    Bygfoot *bygfoot;
    gint id;
    gpointer ptr;
};

static gboolean
deserialize_boolean(Bygfoot *bygfoot, json_object *obj, gboolean *out)
{
    if (!json_object_is_type(obj, json_type_boolean))
        return FALSE;

    *out = json_object_get_boolean(obj);
    return TRUE;
}

static gboolean
deserialize_string(Bygfoot *bygfoot, json_object *obj, char **string)
{
    gint len;

    if (!json_object_is_type(obj, json_type_string))
        return FALSE;

    if (*string)
        free(*string);

    len = json_object_get_string_len(obj);
    *string = malloc(len + 1);
    strncpy(*string, json_object_get_string(obj), len + 1);

    return TRUE;
}

static gboolean
deserialize_string_ptr_array(Bygfoot *bygfoot, json_object *obj,
                             GPtrArray **array_ptr)
{
    gint i, len;
    GPtrArray *array;

    if (!json_object_is_type(obj, json_type_array))
        return FALSE;

    len = json_object_array_length(obj);
    if (*array_ptr)
        g_ptr_array_free(*array_ptr, TRUE);
    *array_ptr = g_ptr_array_sized_new(len);
    array = *array_ptr;
    for (i = 0; i < len; i++) {
        char *string = NULL;
        json_object *string_obj = json_object_array_get_idx(obj, i);
        deserialize_string(bygfoot, string_obj, &string);
        g_ptr_array_add(array, string);
    }
    return TRUE;
}

static gboolean
deserialize_int64(Bygfoot *bygfoot, json_object *obj, gint *value)
{
    if (!json_object_is_type(obj, json_type_int))
        return FALSE;

    *value = json_object_get_int64(obj);
    return TRUE;
}

static gboolean
deserialize_int64_array_wrapper(Bygfoot *bygfoot, json_object *obj,
                                gint **value_ptr)
{
    gint *value = *value_ptr;
    return deserialize_int64(bygfoot, obj, value);
}

static gboolean
deserialize_int_array(Bygfoot *bygfoot, json_object *obj, gint *array, gint len)
{
    gint i;
    if (!json_object_is_type(obj, json_type_array))
        return FALSE;

    for (i = 0; i < len; i++) {
        deserialize_int64(bygfoot, json_object_array_get_idx(obj, i),
                          &array[i]);
    }
    return TRUE;
}

static gboolean
deserialize_double(Bygfoot *bygfoot, json_object *obj, float *value)
{
    if (!json_object_is_type(obj, json_type_double) &&
        !json_object_is_type(obj, json_type_int))
        return FALSE;
    *value = json_object_get_double(obj);
    return TRUE;
}

static gpointer
array_index(gpointer array_generic, gint index)
{
    GArray *array = (GArray *)array_generic;
    return array->data + (index * g_array_get_element_size(array));
}

static gpointer
array_new(gint element_size, gint len)
{
    return g_array_sized_new(FALSE, TRUE, element_size, len);
}

static gint
array_len(gpointer array_generic)
{
    GArray *array = (GArray *)array_generic;

    return array->len;
}

static gpointer
array_append(gpointer array_generic, gpointer value)
{
    GArray *array = (GArray *)array_generic;
    if (!value)
        value = calloc(1, g_array_get_element_size(array));
    g_array_append_vals(array, value, 1);
    return array_index(array, array->len - 1);
}

static gpointer
array_remove(gpointer array_generic, gint index)
{
    GArray *array = (GArray *)array_generic;

    gpointer value = array_index(array, index);
    g_array_remove_index_fast(array, index);
    return value;
}

static gpointer
array_insert(gpointer array_generic, gint index, gpointer val)
{
    GArray *array = (GArray *)array_generic;
    gpointer temp;
    gint element_size = g_array_get_element_size(array);

    /* If val is a pointer to an item in the array, it will be
     * invalidated by the insert, so we need to copy the value
     * first.
     */
    temp = calloc(1, element_size);
    if (val) {
        memcpy(temp, val, element_size);
    }
    g_array_insert_vals(array, index, temp, 1);
    free(temp);
    return array_index(array, index);
}

static gpointer
array_insert_empty(gpointer array_generic, gint index)
{
    GArray *array = (GArray *)array_generic;
    gint element_size = g_array_get_element_size(array);
    gpointer empty = malloc(element_size);
    memset(empty, 0, element_size);
    g_array_insert_vals(array, index, empty, 1);
    return array_index(array, index);
}

static void
array_swap_values(gpointer array_generic, gint a_idx, gint b_idx)
{
    GArray *array = (GArray *)array_generic;
    gint element_size = g_array_get_element_size(array);
    gpointer a = array_index(array, a_idx);
    gpointer b = array_index(array, b_idx);
    gpointer temp = malloc(element_size);

    memcpy(temp, a, element_size);
    memcpy(a, b, element_size);
    memcpy(b, temp, element_size);
    free(temp);
}

static void
array_move_value(gint dest_idx, gint src_idx)
{
    /* Copy value to new spot. */
}

static gpointer
ptr_array_index(gpointer array_generic, gint index)
{
    GPtrArray *array = (GPtrArray *)array_generic;
    return g_ptr_array_index(array, index);
}

static gpointer
ptr_array_new(gint element_size, gint len)
{
    return g_ptr_array_sized_new(len);
}

static gint
ptr_array_len(gpointer array_generic)
{
    GPtrArray *array = (GPtrArray *)array_generic;

    return array->len;
}

static gpointer
ptr_array_append(gpointer array_generic, gpointer value)
{
    GPtrArray *array = (GPtrArray *)array_generic;

    g_ptr_array_add(array, value);
    return ptr_array_index(array, array->len - 1);
}

static gpointer
ptr_array_remove(gpointer array_generic, gint index)
{
    GPtrArray *array = (GPtrArray *)array_generic;

    return g_ptr_array_remove_index_fast(array, index);
}

static gpointer
ptr_array_insert(gpointer array_generic, gint index, gpointer val)
{
    GPtrArray *array = (GPtrArray *)array_generic;
    misc_g_ptr_array_insert(array, index, val);
    return ptr_array_index(array, index);
}

static void
ptr_array_swap_values(gpointer array_generic, gint a_idx, gint b_idx)
{
    GPtrArray *array = (GPtrArray *)array_generic;
    gpointer a = g_ptr_array_index(array, a_idx);
    gpointer b = g_ptr_array_index(array, b_idx);
    array->pdata[a_idx] = b;
    array->pdata[b_idx] = a;
}

static gint
find_item(gpointer array_generic, gint start_idx, json_object *item,
          gpointer (*array_get_index_func)(gpointer, gint),
          gint (*array_len_func)(gpointer),
          gboolean (*compare_func)(json_object *, gpointer))
{
    gint i, len;

    for (i = start_idx, len = array_len_func(array_generic); i < len; i++) {
        gpointer p = array_get_index_func(array_generic, i);
        if (compare_func(item, p))
            return i;
    }
    return -1;
}

static gboolean
deserialize_array_generic(
    Bygfoot *bygfoot, json_object *array_obj, gpointer *array,
    gint element_size, gpointer (*array_index_func)(gpointer, gint),
    gpointer (*array_new_func)(gint, gint), gint (*array_len_func)(gpointer),
    gpointer (*array_append_func)(gpointer, gpointer),
    gpointer (*array_insert_func)(gpointer, gint, gpointer),
    gpointer (*array_remove_func)(gpointer, gint),
    gpointer (*array_swap_values_func)(gpointer, gint, gint),
    gboolean (*compare_func)(json_object *, gpointer),
    gboolean (*deserialize_func)(Bygfoot *, json_object *, gpointer *))
{
    gint i, len;

    len = json_object_array_length(array_obj);
    if (!*array) {
        *array = array_new_func(element_size, len);
    }

    for (i = 0; i < len; i++) {
        json_object *element_obj = json_object_array_get_idx(array_obj, i);
        gpointer element = NULL;

        /* Check if the item is alrady in the array. */
        gint index = -1;
        if (compare_func) {
            index = find_item(*array, i, element_obj, array_index_func,
                              array_len_func, compare_func);
        }

        /* The element is not in the array */
        if (index < 0) {
            if (i == array_len_func(*array))
                element = array_append_func(*array, NULL);
            else
                element = array_insert_func(*array, i, NULL);
            if (!deserialize_func(bygfoot, element_obj, &element))
                return FALSE;
            continue;
        }

        /* The element is already in the array. */
        element = array_index_func(*array, index);
        if (!deserialize_func(bygfoot, element_obj, &element))
            return FALSE;

        /* The element needs to be moved to a new spot*/
        if (i != index)
            array_swap_values_func(*array, i, index);
    }

    /* Remove items from list that aren't in array_obj. */
    while (len < array_len_func(*array)) {
        array_remove_func(*array, array_len_func(*array) - 1);
    }
    return TRUE;
}

static gboolean
deserialize_ptr_array(Bygfoot *bygfoot, json_object *array_obj,
                      GPtrArray **array_ptr,
                      gboolean (*compare_func)(json_object *, gpointer),
                      gboolean (*deserialize_func)(Bygfoot *, json_object *,
                                                   gpointer *))
{
    gint i, len;
    GPtrArray *array;

    len = json_object_array_length(array_obj);

    if (!*array_ptr) {
        *array_ptr = g_ptr_array_sized_new(len);
    }
    array = *array_ptr;

    for (i = 0; i < len; i++) {
        json_object *element_obj = json_object_array_get_idx(array_obj, i);
        gpointer element = NULL;

        /* Check if the item is already in the array. */
        gint index = -1;
        if (compare_func)
            index = find_item(array, i, element_obj, ptr_array_index,
                              ptr_array_len, compare_func);

        if (index >= 0) {
            /* The element is already in the array. */
            element = g_ptr_array_index(array, index);
            if (i != index) {
                /* The element is in the array, but in the wrong spot. Remove
                 * it from its old spot.  We will put it in the new spot
                 * later. */
                g_ptr_array_remove_index_fast(array, index);
            }
        }

        if (!deserialize_func(bygfoot, element_obj, &element))
            return FALSE;

        /* Insert the element to the correct spot in the array. */
        if (i != index) {
            g_ptr_array_insert(array, i, element);
        }
    }

    while (len < array->len) {
        g_ptr_array_remove_index_fast(array, array->len - 1);
    }
    return TRUE;
}

static gboolean
deserialize_array(Bygfoot *bygfoot, json_object *array_obj, GArray **array_ptr,
                  gint element_size,
                  gboolean (*compare_func)(json_object *, gpointer),
                  gboolean (*deserialize_func)(Bygfoot *, json_object *,
                                               gpointer *))
{
    gint i, len;
    GArray *array;

    len = json_object_array_length(array_obj);
    if (!*array_ptr) {
        *array_ptr = g_array_sized_new(FALSE, TRUE, element_size, len);
    }
    array = *array_ptr;

    for (i = 0; i < len; i++) {
        json_object *element_obj = json_object_array_get_idx(array_obj, i);
        gpointer element = NULL;
        gint index = -1;

        /* Check if the item is already in the array. */
        if (compare_func) {
            gint index = find_item(array, i, element_obj, array_index,
                                   array_len, compare_func);
        }

        if (index < 0) {
            /* The element is not in the array.
             * Reserve empty slot at the end of the array for the new
               element. */
            g_array_set_size(array, array->len + 1);
            /* FIXME: Not all arrays are created to automatically clear new
             * members, so we need to clear it. */
            memset(array_index(array, array->len - 1), 0, element_size);
            if (i != array->len - 1) {
                /* We are inserting the new value into the middle, so copy the
                 * old value to the empty slot at the end of the array. */
                gpointer dst = array_index(array, array->len - 1);
                gpointer src = array_index(array, i);
                memcpy(dst, src, element_size);

                /* Clear out the spot we will be inserting the new value. */
                memset(src, 0, element_size);
            }
        } else if (i != index) {
            /* The element is in the array, but needs to be moved to a new
             * spot*/
            array_swap_values(array, i, index);
        }

        element = array->data + (element_size * i);
        if (!deserialize_func(bygfoot, element_obj, (void *)&element))
            return FALSE;
    }
    while (len < array->len) {
        g_array_remove_index_fast(array, array->len - 1);
    }
    return TRUE;
}

static gboolean
deserialize_competition_ptr(Bygfoot *bygfoot, json_object *comp_obj,
                            Competition **comp)
{
    gint id = json_object_get_int64(json_object_object_get(comp_obj, "clid"));
    *comp = bygfoot_get_competition_id(bygfoot, id);
    if (!*comp)
        fprintf(stderr, "Could not find comp for id: %d\n", id);
    return TRUE;
}

static void
fixtures_set_competition(GArray *fixtures, Competition *competition)
{
    gint i;
    for (i = 0; fixtures && i < fixtures->len; i++) {
        Fixture *fixture = &g_array_index(fixtures, Fixture, i);
        fixture->competition = competition;
    }
}

static void
tables_set_competition(GArray *tables, Competition *competition)
{
    gint i;
    for (i = 0; tables && i < tables->len; i++) {
        Table *table = &g_array_index(tables, Table, i);
        table->competition = competition;
    }
}

static gboolean
country_compare_func(json_object *a, gpointer b)
{
    Country *country = (Country *)b;
    const gchar *sid = json_object_get_string(json_object_object_get(a, "sid"));

    return !strcmp(sid, country->sid);
}

gboolean
bygfoot_json_deserialize_countries(Bygfoot *bygfoot, json_object *countries_obj,
                                   GPtrArray **countries)
{

    return deserialize_ptr_array(
        bygfoot, countries_obj, countries, country_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_country);
}

static gboolean
user_compare_func(json_object *a, gpointer b)
{
    User *user = (User *)b;
    const char *name =
        json_object_get_string(json_object_object_get(a, "name"));
    return !strcmp(name, user->name);
}

gboolean
bygfoot_json_deserialize_users(Bygfoot *bygfoot, json_object *users_obj,
                               GArray **users_ptr)
{
    return deserialize_array(
        bygfoot, users_obj, users_ptr, sizeof(User), user_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_user);
}

gboolean
bygfoot_json_deserialize_user_histories(Bygfoot *bygfoot,
                                        json_object *histories_obj,
                                        GArray **histories_ptr)
{
    return deserialize_array(
        bygfoot, histories_obj, histories_ptr, sizeof(UserHistory), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_user_history);
}

gboolean
bygfoot_json_deserialize_user_history(Bygfoot *bygfoot,
                                      json_object *history_obj,
                                      UserHistory **history_ptr)
{
    UserHistory *history = *history_ptr;
    json_object *string_obj;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(history_obj, history, field, func);

    DESERIALIZE(season, deserialize_int64);
    DESERIALIZE(week, deserialize_int64);
    DESERIALIZE(type, deserialize_int64);
    DESERIALIZE(team_name, deserialize_string);
    string_obj = json_object_object_get(history_obj, "string");
    if (string_obj) {
        gint i;
        if (!json_object_is_type(string_obj, json_type_array))
            return FALSE;
        for (i = 0; i < 3; i++) {
            json_object *obj = json_object_array_get_idx(string_obj, i);
            deserialize_string(bygfoot, obj, &history->string[i]);
        }
    }
#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_game_units(Bygfoot *bygfoot, json_object *units_obj,
                                    GArray **units)
{
    return deserialize_array(
        bygfoot, units_obj, units, sizeof(LiveGameUnit), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_game_unit);
}

gboolean
bygfoot_json_deserialize_live_game_event(Bygfoot *bygfoot,
                                         json_object *event_obj,
                                         LiveGameEvent *event)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(event_obj, event, field, func);

    DESERIALIZE(type, deserialize_int64);
    DESERIALIZE(verbosity, deserialize_int64);
    DESERIALIZE(team, deserialize_int64);
    event->player =
        json_object_get_userdata(json_object_object_get(event_obj, "player"));
    event->player2 =
        json_object_get_userdata(json_object_object_get(event_obj, "player2"));
    DESERIALIZE(commentary, deserialize_string);
    DESERIALIZE(commentary_id, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_game_unit(Bygfoot *bygfoot, json_object *unit_obj,
                                   LiveGameUnit **unit_ptr)
{
    LiveGameUnit *unit = *unit_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(unit_obj, unit, field, func);

    DESERIALIZE(possession, deserialize_int64);
    DESERIALIZE(area, deserialize_int64);
    DESERIALIZE(minute, deserialize_int64);
    DESERIALIZE(time, deserialize_int64);

    DESERIALIZE_FIELD_CUSTOM(
        unit_obj, "result",
        deserialize_int_array(bygfoot,
                              json_object_object_get(unit_obj, "result"),
                              (int *)&unit->result, 2));
    DESERIALIZE(event, bygfoot_json_deserialize_live_game_event);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_live_game_stats(Bygfoot *bygfoot,
                                         json_object *stats_obj,
                                         LiveGameStats *stats)
{
    json_object *values_obj, *players_obj;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stats_obj, stats, field, func);

    DESERIALIZE(possession, deserialize_double);

    static const struct key_index
    {
        const gchar *key;
        gint index;
    } value_fields[] = {
        {"goals_regular", LIVE_GAME_STAT_VALUE_GOALS_REGULAR},
        {"shots", LIVE_GAME_STAT_VALUE_SHOTS},
        {"shot_percentage", LIVE_GAME_STAT_VALUE_SHOT_PERCENTAGE},
        {"possession", LIVE_GAME_STAT_VALUE_POSSESSION},
        {"penalties", LIVE_GAME_STAT_VALUE_PENALTIES},
        {"fouls", LIVE_GAME_STAT_VALUE_FOULS},
        {"cards", LIVE_GAME_STAT_VALUE_CARDS},
        {"reds", LIVE_GAME_STAT_VALUE_REDS},
        {"injuries", LIVE_GAME_STAT_VALUE_INJURIES},
        {NULL, LIVE_GAME_STAT_VALUE_END}};

    values_obj = json_object_object_get(stats_obj, "values");
    if (values_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *team_values_obj =
                json_object_array_get_idx(values_obj, i);
            const struct key_index *iter;
            for (iter = value_fields; iter->key; iter++) {
                deserialize_int64(
                    bygfoot, json_object_object_get(team_values_obj, iter->key),
                    (int *)&stats->values[i][iter->index]);
            }
        }
    }

    players_obj = json_object_object_get(stats_obj, "players");
    if (players_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *array_obj = json_object_array_get_idx(players_obj, i);
            gint j;
            for (j = 0; j < LIVE_GAME_STAT_ARRAY_END; j++) {
                json_object *obj = json_object_array_get_idx(array_obj, j);
                deserialize_string_ptr_array(bygfoot, obj,
                                             &stats->players[i][j]);
            }
        }
    }
#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_live_game_team_state(Bygfoot *bygfoot,
                                              json_object *team_state_obj,
                                              LiveGameTeamState *team_state)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(team_state_obj, team_state, field, func);

    DESERIALIZE(structure, deserialize_int64);
    DESERIALIZE(style, deserialize_int64);
    DESERIALIZE(boost, deserialize_boolean);
    DESERIALIZE_FIELD_CUSTOM(
        team_state_obj, "player_ids",
        deserialize_int_array(
            bygfoot, json_object_object_get(team_state_obj, "player_ids"),
            (int *)&team_state->player_ids, 1));

#undef DESERIALIZE
    return TRUE;
}

static Player *
find_player(GPtrArray *teams, gint id)
{
    gint i, j;
    if (!id)
        return NULL;
    for (i = 0; i < teams->len; i++) {
        Team *team = g_ptr_array_index(teams, i);
        for (j = 0; j < team->players->len; j++) {
            Player *player = g_ptr_array_index(team->players, j);
            if (player->id == id)
                return player;
        }
    }
    return NULL;
}

gboolean
bygfoot_json_deserialize_live_game(Bygfoot *bygfoot, json_object *live_game_obj,
                                   LiveGame *live_game)
{
    json_object *team_names_obj, *team_values_obj, *team_state_obj,
        *action_ids_obj;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(live_game_obj, live_game, field, func);

    DESERIALIZE(fix, bygfoot_json_deserialize_fixture_ptr);
    DESERIALIZE(fix_id, deserialize_int64);
    team_names_obj = json_object_object_get(live_game_obj, "team_names");
    if (team_names_obj) {
        gint i;
        GPtrArray *teams = g_ptr_array_new();
        json_object *units_obj;
        gint units_obj_len;
        for (i = 0; i < 2; i++) {
            json_object *obj = json_object_array_get_idx(team_names_obj, i);
            deserialize_string(bygfoot, obj, &live_game->team_names[i]);
            g_ptr_array_add(teams,
                            bygfoot_get_team_name(live_game->team_names[i]));
        }

        /* Find the players for each event. */
        units_obj = json_object_object_get(live_game_obj, "units");
        for (i = 0, units_obj_len = json_object_array_length(units_obj);
             i < units_obj_len; i++) {
            gint j;
            json_object *unit_obj = json_object_array_get_idx(units_obj, i);
            json_object *event_obj = json_object_object_get(unit_obj, "event");
            json_object *player_objs[2];
            player_objs[0] = json_object_object_get(event_obj, "player");
            player_objs[1] = json_object_object_get(event_obj, "player2");
            for (j = 0; j < 2; j++) {
                json_object *player_obj = player_objs[j];
                json_object *player_id_obj;
                gint id;
                if (!player_obj)
                    continue;
                player_id_obj = json_object_object_get(player_obj, "id");
                id = json_object_get_int64(player_id_obj);
                json_object_set_userdata(player_obj, find_player(teams, id),
                                         NULL);
            }
        }
        g_ptr_array_free(teams, TRUE);
    }
    DESERIALIZE(attendance, deserialize_int64);
    DESERIALIZE_FIELD_CUSTOM(
        live_game_obj, "subs_left",
        deserialize_int_array(
            bygfoot, json_object_object_get(live_game_obj, "subs_left"),
            (int *)&live_game->subs_left, 2));
    DESERIALIZE(started_game, deserialize_int64);
    DESERIALIZE(stadium_event, deserialize_int64);
    team_values_obj = json_object_object_get(live_game_obj, "team_values");
    if (team_values_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *team_value_obj =
                json_object_array_get_idx(team_values_obj, i);
            gint j;
            for (j = 0; j < GAME_TEAM_VALUE_END; j++) {
                json_object *obj = json_object_array_get_idx(team_value_obj, j);
                deserialize_double(bygfoot, obj, &live_game->team_values[i][j]);
            }
        }
    }
    DESERIALIZE(home_advantage, deserialize_double);
    DESERIALIZE(units, bygfoot_json_deserialize_game_units);
    DESERIALIZE(stats, bygfoot_json_deserialize_live_game_stats);
    team_state_obj = json_object_object_get(live_game_obj, "team_state");
    if (team_state_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *obj = json_object_array_get_idx(team_state_obj, i);
            bygfoot_json_deserialize_live_game_team_state(
                bygfoot, team_state_obj, &live_game->team_state[i]);
        }
    }

    action_ids_obj = json_object_object_get(live_game_obj, "action_ids");
    if (action_ids_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *obj = json_object_array_get_idx(action_ids_obj, i);
            deserialize_array(
                bygfoot, obj, &live_game->action_ids[i], sizeof(gint), NULL,
                (deserialize_func_type)deserialize_int64_array_wrapper);
        }
    }
#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_user_sponsor(Bygfoot *bygfoot,
                                      json_object *sponsor_obj,
                                      UserSponsor *sponsor)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(sponsor_obj, sponsor, field, func);

    json_object *name_obj = json_object_object_get(sponsor_obj, "name");
    if (name_obj) {
        char *name = NULL;
        deserialize_string(bygfoot, name_obj, &name);
        sponsor->name = g_string_new(name);
        free(name);
    }
    DESERIALIZE(benefit, deserialize_int64);
    DESERIALIZE(contract, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_youth_academy(Bygfoot *bygfoot,
                                       json_object *academy_obj,
                                       YouthAcademy *academy)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(academy_obj, academy, field, func);

    DESERIALIZE(tm, bygfoot_json_deserialize_team_ptr);
    DESERIALIZE(pos_pref, deserialize_int64);
    DESERIALIZE(coach, deserialize_int64);
    DESERIALIZE(percentage, deserialize_int64);
    DESERIALIZE(av_coach, deserialize_double);
    DESERIALIZE(av_percentage, deserialize_double);
    DESERIALIZE(counter_youth, deserialize_double);
    DESERIALIZE(players, bygfoot_json_deserialize_players);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_user_bets(Bygfoot *bygfoot, json_object *bets_obj,
                                   GArray *bets_ptr[2])
{
    gint i;

    for (i = 0; i < 2; i++) {
        GArray *bet_users = bets[i];
        json_object *obj = json_object_array_get_idx(bets_obj, i);
        deserialize_array(
            bygfoot, obj, &bet_users, sizeof(BetUser), NULL,
            (deserialize_func_type)bygfoot_json_deserialize_bet_user);
    }
    return TRUE;
}

gboolean
bygfoot_json_deserialize_bet_user(Bygfoot *bygfoot, json_object *bet_obj,
                                  BetUser **bet_user_ptr)
{
    BetUser *bet_user = *bet_user_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(bet_obj, bet_user, field, func);

    DESERIALIZE(fix_id, deserialize_int64);
    DESERIALIZE(outcome, deserialize_int64);
    DESERIALIZE(wager, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_user(Bygfoot *bygfoot, json_object *user_obj,
                              User **user_ptr)
{
    User *user;
    json_object *money_in_obj, *money_out_obj;

    if (!*user_ptr) {
        *user_ptr = calloc(1, sizeof(User));
        **user_ptr = user_new();
    }

    user = *user_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(user_obj, user, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(tm, bygfoot_json_deserialize_team_ptr);
    /* options not serialized */
    /* events not serialized */
    DESERIALIZE(history, bygfoot_json_deserialize_user_histories);
    DESERIALIZE_FIELD_CUSTOM(
        user_obj, "counters",
        deserialize_int_array(bygfoot,
                              json_object_object_get(user_obj, "counters"),
                              user->counters, COUNT_USER_END));
    DESERIALIZE(money, deserialize_int64);
    DESERIALIZE(debt, deserialize_int64);
    money_in_obj = json_object_object_get(user_obj, "money_in");
    if (money_in_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *money_in_week_obj =
                json_object_array_get_idx(money_in_obj, i);
            deserialize_int_array(bygfoot, money_in_week_obj,
                                  (int *)user->money_in[i], MON_IN_END);
        }
    }
    money_out_obj = json_object_object_get(user_obj, "money_out");
    if (money_out_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *money_out_week_obj =
                json_object_array_get_idx(money_out_obj, i);
            deserialize_int_array(bygfoot, money_out_week_obj,
                                  (int *)user->money_out[i], MON_OUT_END);
        }
    }
    DESERIALIZE(debt_interest, deserialize_double);
    DESERIALIZE(alr_start_week, deserialize_int64);
    DESERIALIZE(alr_weekly_installment, deserialize_int64);
    DESERIALIZE(scout, deserialize_int64);
    DESERIALIZE(physio, deserialize_int64);
    DESERIALIZE(live_game, bygfoot_json_deserialize_live_game);
    DESERIALIZE(sponsor, bygfoot_json_deserialize_user_sponsor);
    DESERIALIZE(youth_academy, bygfoot_json_deserialize_youth_academy);
    DESERIALIZE(mmatches_file, deserialize_string);
    DESERIALIZE_FIELD_CUSTOM(
        user_obj, "bets",
        bygfoot_json_deserialize_user_bets(
            bygfoot, json_object_object_get(user_obj, "bets"), user->bets));
    DESERIALIZE_FIELD_CUSTOM(
        user_obj, "default_team",
        deserialize_array(
            bygfoot, json_object_object_get(user_obj, "default_team"),
            &user->default_team, sizeof(gint), NULL,
            (deserialize_func_type)deserialize_int64_array_wrapper));
    DESERIALIZE(default_structure, deserialize_int64);
    DESERIALIZE(default_style, deserialize_int64);
    DESERIALIZE(default_boost, deserialize_int64);

#undef DESERIALIZE

    return TRUE;
}

static gboolean
league_compare_func(json_object *a, gpointer b)
{
    League *league = (League *)b;
    gint id = json_object_get_int64(json_object_object_get(a, "id"));
    return id == league->c.id;
}

gboolean
bygfoot_json_deserialize_leagues(Bygfoot *bygfoot, json_object *leagues_obj,
                                 GPtrArray **leagues_ptr)
{
    return deserialize_ptr_array(
        bygfoot, leagues_obj, leagues_ptr, league_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_league);
}

static gboolean
fixture_json_compare_func(json_object *a, gpointer b)
{
    Fixture *fixture = (Fixture *)b;
    gint id = json_object_get_int64(json_object_object_get(a, "id"));
    return id == fixture->id;
}

gboolean
bygfoot_json_deserialize_fixtures(Bygfoot *bygfoot, json_object *fixtures_obj,
                                  GArray **fixtures_ptr)
{
    return deserialize_array(
        bygfoot, fixtures_obj, fixtures_ptr, sizeof(Fixture),
        fixture_json_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_fixture);
}

gboolean
bygfoot_json_deserialize_fixture_ptr(Bygfoot *bygfoot, json_object *fixture_obj,
                                     Fixture **fixture_ptr)
{
    gint id = json_object_get_int64(json_object_object_get(fixture_obj, "id"));
    *fixture_ptr = fixture_from_id(id, TRUE);
    return TRUE;
}

gboolean
bygfoot_json_deserialize_fixture(Bygfoot *bygfoot, json_object *fixture_obj,
                                 Fixture **fixture_ptr)
{
    Fixture *fixture;
    json_object *result_obj;

    if (!*fixture_ptr) {
        *fixture_ptr = calloc(1, sizeof(Fixture));
    }
    fixture = *fixture_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(fixture_obj, fixture, field, func);

    DESERIALIZE(id, deserialize_int64);
    DESERIALIZE(round, deserialize_int64);
    DESERIALIZE(replay_number, deserialize_int64);
    DESERIALIZE(week_number, deserialize_int64);
    DESERIALIZE(week_round_number, deserialize_int64);
    DESERIALIZE(teams, bygfoot_deserialize_fixture_teams);
    DESERIALIZE(home_advantage, deserialize_boolean);
    DESERIALIZE(second_leg, deserialize_boolean);
    DESERIALIZE(decisive, deserialize_boolean);
    DESERIALIZE(attendance, deserialize_int64);

    result_obj = json_object_object_get(fixture_obj, "result");
    if (result_obj) {
        gint i;
        static const char *fields[] = {"goals_regulation", "goals_extra_time",
                                       "goals_penalty_shootout", NULL};
        for (i = 0; i < 2; i++) {
            json_object *team_obj = json_object_array_get_idx(result_obj, i);
            const char **iter;
            for (iter = fields; *iter; ++iter) {
                const char *field = *iter;
                gint index = (iter - fields);
                fixture->result[i][index] = json_object_get_int64(
                    json_object_object_get(team_obj, field));
            }
        }
    }
    DESERIALIZE_FIELD_CUSTOM(
        fixture_obj, "live_game",
        bygfoot_json_deserialize_live_game(
            bygfoot, json_object_object_get(fixture_obj, "live_game"),
            fixture->live_game));

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_deserialize_fixture_teams(Bygfoot *bygfoot, json_object *teams_obj,
                                  Team *(*teams)[2])
{
    gint i;
    for (i = 0; i < 2; i++) {
        json_object *team_obj = json_object_array_get_idx(teams_obj, i);
        if (!bygfoot_json_deserialize_team_ptr(bygfoot, team_obj, &(*teams)[i]))
            return FALSE;
    }
    return TRUE;
}

static gboolean
cup_compare_func(json_object *a, gpointer b)
{
    Cup *cup = (Cup *)b;
    gint id = json_object_get_int64(json_object_object_get(a, "id"));
    return id = cup->c.id;
}

gboolean
bygfoot_json_deserialize_cups(Bygfoot *bygfoot, json_object *cups_obj,
                              GPtrArray **cups_ptr)
{
    return deserialize_ptr_array(
        bygfoot, cups_obj, cups_ptr, cup_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_cup);
}

static Cup *
find_cup(const GArray *cups, gint id)
{
    gint i;
    for (i = 0; i < cups->len; i++) {
        Cup *cup = &g_array_index(cups, Cup, i);
        if (cup->c.id == id) {
            return cup;
        }
    }
    return NULL;
}

gboolean
bygfoot_json_deserialize_choose_teams(Bygfoot *bygfoot, json_object *choose_obj,
                                      GArray **choose_teams)
{
    return deserialize_array(
        bygfoot, choose_obj, choose_teams, sizeof(CupChooseTeam), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_choose_team);
}

gboolean
bygfoot_json_deserialize_choose_team(Bygfoot *bygfoot,
                                     json_object *choose_list_obj,
                                     CupChooseTeam **choose_team_ptr)
{

    CupChooseTeam *choose_team = *choose_team_ptr;
    gint i, len;

    for (i = 0, len = json_object_array_length(choose_list_obj); i < len; ++i) {
        json_object *choose_obj = json_object_array_get_idx(choose_list_obj, i);

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(choose_obj, choose_team, field, func);

        DESERIALIZE(sid, deserialize_string);
        DESERIALIZE(number_of_teams, deserialize_int64);
        DESERIALIZE(from_table, deserialize_int64);
        DESERIALIZE(start_idx, deserialize_int64);
        DESERIALIZE(end_idx, deserialize_int64);
        DESERIALIZE(losers_round, deserialize_int64);
        DESERIALIZE(randomly, deserialize_boolean);
        DESERIALIZE(generate, deserialize_boolean);
        DESERIALIZE(skip_group_check, deserialize_boolean);
        DESERIALIZE(preload, deserialize_boolean);
        DESERIALIZE(optional, deserialize_boolean);

        if (i + 1 < len) {
            choose_team->next = calloc(1, sizeof(CupChooseTeam));
            choose_team = choose_team->next;
        }
    }
#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_cup_ptr(Bygfoot *bygfoot, json_object *cup_obj,
                                 Cup **cup_ptr)
{
    gint id = json_object_get_int64(json_object_object_get(cup_obj, "id"));
    *cup_ptr = (Cup *)bygfoot_get_competition_id(bygfoot, id);
    return TRUE;
}

gboolean
bygfoot_json_deserialize_allcups(Bygfoot *bygfoot, json_object *allcups_obj,
                                 GPtrArray **allcups_ptr)
{
    return deserialize_ptr_array(
        bygfoot, allcups_obj, allcups_ptr, NULL,
        (deserialize_func_type)bygfoot_json_deserialize_cup_ptr);
}

gboolean
bygfoot_json_deserialize_waits(Bygfoot *bygfoot, json_object *waits_obj,
                               GArray **waits)
{
    return deserialize_array(
        bygfoot, waits_obj, waits, sizeof(CupRoundWait), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_cup_round_wait);
}

gboolean
bygfoot_json_deserialize_cup_round_wait(Bygfoot *bygfoot, json_object *wait_obj,
                                        CupRoundWait **wait_ptr)
{
    CupRoundWait *wait = *wait_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(wait_obj, wait, field, func);

    DESERIALIZE(cup_sid, deserialize_string);
    DESERIALIZE(cup_round, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_cup_rounds(Bygfoot *bygfoot, json_object *rounds_obj,
                                    GArray **rounds)
{
    return deserialize_array(
        bygfoot, rounds_obj, rounds, sizeof(CupRound), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_cup_round);
}

gboolean
bygfoot_json_deserialize_cup_round(Bygfoot *bygfoot, json_object *round_obj,
                                   CupRound **round_ptr)
{
    CupRound *round = *round_ptr;
    json_object *two_match_weeks_obj;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(round_obj, round, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(home_away, deserialize_boolean);
    DESERIALIZE(replay, deserialize_int64);
    DESERIALIZE(neutral, deserialize_boolean);
    DESERIALIZE(randomise_teams, deserialize_boolean);
    DESERIALIZE(round_robin_number_of_groups, deserialize_int64);
    DESERIALIZE(round_robin_number_of_advance, deserialize_int64);
    DESERIALIZE(round_robin_number_of_best_advance, deserialize_int64);
    DESERIALIZE(round_robins, deserialize_int64);
    DESERIALIZE_FIELD_CUSTOM(
        round_obj, "rr_breaks",
        deserialize_array(
            bygfoot, json_object_object_get(round_obj, "rr_breaks"),
            &round->rr_breaks, sizeof(gint), NULL,
            (deserialize_func_type)deserialize_int64_array_wrapper));
    DESERIALIZE(new_teams, deserialize_int64);
    DESERIALIZE(byes, deserialize_int64);
    DESERIALIZE(delay, deserialize_int64);

    two_match_weeks_obj = json_object_object_get(round_obj, "two_match_weeks");
    if (two_match_weeks_obj) {
        gint i;
        for (i = 0; i < 2; i++) {
            json_object *array_obj =
                json_object_array_get_idx(two_match_weeks_obj, i);
            deserialize_array(
                bygfoot, array_obj, &round->two_match_weeks[i], sizeof(gint),
                NULL, (deserialize_func_type)deserialize_int64_array_wrapper);
        }
    }

    DESERIALIZE(two_match_week, deserialize_boolean);
    DESERIALIZE(team_ptrs, bygfoot_json_deserialize_team_ptrs);
    DESERIALIZE(choose_teams, bygfoot_json_deserialize_choose_teams);
    DESERIALIZE(tables, bygfoot_json_deserialize_tables);
    DESERIALIZE(waits, bygfoot_json_deserialize_waits);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_week_breaks(Bygfoot *bygfoot,
                                     json_object *week_breaks_obj,
                                     GArray **week_breaks)
{
    deserialize_array(
        bygfoot, week_breaks_obj, week_breaks, sizeof(WeekBreak), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_week_break);
    gint i;
    GArray *w = *week_breaks;
    for (i = 0; i < w->len; i++) {
        WeekBreak *wb = &g_array_index(w, WeekBreak, i);
    }
    return TRUE;
}

gboolean
bygfoot_json_deserialize_week_break(Bygfoot *bygfoot,
                                    json_object *week_break_obj,
                                    WeekBreak **week_break_ptr)
{
    WeekBreak *week_break = *week_break_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(week_break_obj, week_break, field, func);

    DESERIALIZE(week_number, deserialize_int64);
    DESERIALIZE(length, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_cup(Bygfoot *bygfoot, json_object *cup_obj,
                             Cup **cup_ptr)
{
    gint i;
    Cup *cup;
    json_object *history_obj;

    if (!*cup_ptr) {
        *cup_ptr = calloc(1, sizeof(Cup));
        **cup_ptr = cup_new(0, bygfoot);
    }

    cup = *cup_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(cup_obj, cup, field, func);

    DESERIALIZE_FIELD_CUSTOM(
        cup_obj, "name",
        deserialize_string(bygfoot, json_object_object_get(cup_obj, "name"),
                           &cup->c.name));
    DESERIALIZE(short_name, deserialize_string);
    DESERIALIZE(symbol, deserialize_string);
    DESERIALIZE(sid, deserialize_string);
    DESERIALIZE_FIELD_CUSTOM(
        cup_obj, "id",
        deserialize_int64(bygfoot, json_object_object_get(cup_obj, "id"),
                          &cup->c.id));
    DESERIALIZE(group, deserialize_int64);
    DESERIALIZE(last_week, deserialize_int64);
    DESERIALIZE(week_gap, deserialize_int64);
    DESERIALIZE(add_week, deserialize_int64);
    DESERIALIZE_FIELD_CUSTOM(
        cup_obj, "yellow_red",
        deserialize_int64(bygfoot,
                          json_object_object_get(cup_obj, "yellow_red"),
                          &cup->c.yellow_red));
    DESERIALIZE(talent_diff, deserialize_double);
    DESERIALIZE(next_fixture_update_week, deserialize_int64);
    DESERIALIZE(next_fixture_update_week_round, deserialize_int64);
    DESERIALIZE_FIELD_CUSTOM(cup_obj, "properties",
                             deserialize_string_ptr_array(
                                 bygfoot,
                                 json_object_object_get(cup_obj, "properties"),
                                 &cup->c.properties));
    DESERIALIZE(rounds, bygfoot_json_deserialize_cup_rounds);
    DESERIALIZE(bye, bygfoot_json_deserialize_team_ptrs);
    DESERIALIZE(fixtures, bygfoot_json_deserialize_fixtures);
    DESERIALIZE(week_breaks, bygfoot_json_deserialize_week_breaks);
    DESERIALIZE(skip_weeks_with, deserialize_string_ptr_array);
    history_obj = json_object_object_get(cup_obj, "history");
    if (history_obj) {
        gint i, len;
        /* Clear the object */
        if (cup->history) {
            for (i = 0; i < cup->history->len; i++) {
                GPtrArray *array = g_ptr_array_index(cup->history, i);
                g_ptr_array_free(array, TRUE);
            }
            free_g_ptr_array(&cup->history);
        }
        cup->history = g_ptr_array_new();
        for (i = 0, len = json_object_array_length(history_obj); i < len; i++) {
            json_object *obj = json_object_array_get_idx(history_obj, i);
            GPtrArray *array = NULL;
            bygfoot_json_deserialize_team_ptrs(bygfoot, obj, &array);
            g_ptr_array_add(cup->history, array);
        }
    }

#undef DESERIALIZE

    /* Update the competition pointer for the fixtures. */
    fixtures_set_competition(cup->fixtures, &cup->c);

    /* Update the competition pointer for the tables in all the rounds. */
    for (i = 0; i < cup->rounds->len; i++) {
        CupRound *round = &g_array_index(cup->rounds, CupRound, i);
        tables_set_competition(round->tables, &cup->c);
    }
    return TRUE;
}

static gboolean
team_json_compare_func(json_object *a, gpointer b)
{
    Team *team = (Team *)b;
    gint id = json_object_get_int64(json_object_object_get(a, "id"));
    return id == team->id;
}

gboolean
bygfoot_json_deserialize_teams(Bygfoot *bygfoot, json_object *teams_obj,
                               GPtrArray **teams_ptr)
{
    return deserialize_ptr_array(
        bygfoot, teams_obj, teams_ptr, team_json_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_team);
}

gboolean
bygfoot_json_deserialize_stadium(Bygfoot *bygfoot, json_object *stadium_obj,
                                 Stadium *stadium)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stadium_obj, stadium, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(capacity, deserialize_int64);
    DESERIALIZE(average_attendance, deserialize_int64);
    DESERIALIZE(possible_attendance, deserialize_int64);
    DESERIALIZE(games, deserialize_int64);
    DESERIALIZE(safety, deserialize_double);
    DESERIALIZE(ticket_price, deserialize_double);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_team(Bygfoot *bygfoot, json_object *team_obj,
                              Team **team_ptr)
{
    Team *team;
    gint i;

    if (!json_object_is_type(team_obj, json_type_object)) {
        fprintf(stderr, "team: invalid json %s\n",
                json_object_to_json_string(team_obj));
        return FALSE;
    }

    if (!*team_ptr) {
        League *league = json_object_get_userdata(team_obj);
        *team_ptr = calloc(1, sizeof(Team));
        **team_ptr = team_new(FALSE, league->country);
        (*team_ptr)->league = league;
    }

    team = *team_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(team_obj, team, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(symbol, deserialize_string);
    DESERIALIZE(names_file, deserialize_string);
    DESERIALIZE(def_file, deserialize_string);
    DESERIALIZE(strategy_sid, deserialize_string);
    DESERIALIZE(id, deserialize_int64);
    DESERIALIZE(structure, deserialize_int64);
    DESERIALIZE(style, deserialize_int64);
    DESERIALIZE(boost, deserialize_int64);
    DESERIALIZE(average_talent, deserialize_double);
    DESERIALIZE(luck, deserialize_double);
    DESERIALIZE(stadium, bygfoot_json_deserialize_stadium);
    DESERIALIZE(players, bygfoot_json_deserialize_players);
    {
        json_object *first_team_obj =
            json_object_object_get(team_obj, "first_team");
        if (first_team_obj) {
            json_object *first_team_id_obj =
                json_object_object_get(first_team_obj, "id");
            deserialize_int64(bygfoot, first_team_obj, &team->first_team.id);
        }
    }
    DESERIALIZE(reserve_level, deserialize_int64);

#undef DESERIALIZE

    /* Update the team pointer for all players. */
    for (i = 0; i < team->players->len; i++) {
        Player *player = g_ptr_array_index(team->players, i);
        player->team = team;
    }

    return TRUE;
}

gboolean
bygfoot_json_deserialize_team_ptrs(Bygfoot *bygfoot, json_object *teams_obj,
                                   GPtrArray **teams_ptr)
{
    return deserialize_ptr_array(
        bygfoot, teams_obj, teams_ptr, team_json_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_team_ptr);
}

static void
update_team_ptr(json_object *obj, void *userdata)
{
    struct ptr_update_userdata *update = (struct ptr_update_userdata *)userdata;
    Team **ptr = (Team **)update->ptr;

    *ptr = team_of_id(update->id);
    g_free(update);
}

gboolean
bygfoot_json_deserialize_team_ptr(Bygfoot *bygfoot, json_object *team_obj,
                                  Team **team_ptr)
{

    Team *team = json_object_get_userdata(team_obj);
    if (team) {
        *team_ptr = team;
        return TRUE;
    }

    gint id = json_object_get_int64(json_object_object_get(team_obj, "id"));
    *team_ptr = team_of_id(id);
    return TRUE;
}

static gboolean
player_json_compare_func(json_object *a, gpointer b)
{
    Player *player = (Player *)b;
    gint id = json_object_get_int64(json_object_object_get(a, "id"));
    return id == player->id;
}

gboolean
bygfoot_json_deserialize_players(Bygfoot *bygfoot, json_object *players_obj,
                                 GPtrArray **players_ptr)
{
    return deserialize_ptr_array(
        bygfoot, players_obj, players_ptr, player_json_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_player);

    /*
        gint i, len;

        len = json_object_array_length(players_obj);
        if (!*players_ptr) {
            *players_ptr = g_array_sized_new(FALSE, TRUE, sizeof(Player), len);
        }

        for (i = 0; i < len; i++) {
            json_object *player_obj = json_object_array_get_idx(players_obj, i);


        }
    */
}

gboolean
bygfoot_json_deserialize_player(Bygfoot *bygfoot, json_object *player_obj,
                                Player **player_ptr)
{
    Player *player;
    json_object_iter iter;
    json_object *etal_obj;

    if (!*player_ptr) {
        *player_ptr = calloc(1, sizeof(Player));
    }

    player = *player_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(player_obj, player, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(pos, deserialize_int64);
    DESERIALIZE(cpos, deserialize_int64);
    DESERIALIZE(health, deserialize_int64);
    DESERIALIZE(recovery, deserialize_int64);
    DESERIALIZE(id, deserialize_int64);
    DESERIALIZE(value, deserialize_int64);
    DESERIALIZE(wage, deserialize_int64);
    DESERIALIZE(offers, deserialize_int64);
    DESERIALIZE(streak, deserialize_int64);
    DESERIALIZE(skill, deserialize_double);
    DESERIALIZE(cskill, deserialize_double);
    DESERIALIZE(talent, deserialize_double);
    etal_obj = json_object_object_get(player_obj, "etal");
    if (etal_obj) {
        gint i, len;
        for (i = 0; i < QUALITY_END; i++) {
            json_object *obj = json_object_array_get_idx(etal_obj, i);
            deserialize_double(bygfoot, obj, &player->etal[i]);
        }
    }
    DESERIALIZE(fitness, deserialize_double);
    DESERIALIZE(lsu, deserialize_double);
    DESERIALIZE(age, deserialize_double);
    DESERIALIZE(peak_age, deserialize_double);
    DESERIALIZE(peak_region, deserialize_double);
    DESERIALIZE(contract, deserialize_double);
    DESERIALIZE(streak_prob, deserialize_double);
    DESERIALIZE(streak_count, deserialize_double);
    DESERIALIZE(participation, deserialize_boolean);
    DESERIALIZE(stats, bygfoot_json_deserialize_player_competition_stats_list);
    DESERIALIZE(team, bygfoot_json_deserialize_team_ptr);

#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_player_competition_stats_list(Bygfoot *bygfoot,
                                                       json_object *list_obj,
                                                       GList **list_ptr)
{
    GList *list = *list_ptr;
    gint i, len;
    for (i = 0, len = json_object_array_length(list_obj); i < len; i++) {
        GArray *season_stats =
            g_array_new(FALSE, FALSE, sizeof(PlayerCompetitionStats));
        bygfoot_json_deserialize_player_competition_stats_array(
            bygfoot, json_object_array_get_idx(list_obj, i), &season_stats);
        list = g_list_append(list, season_stats);
    }
    /* We need to assign list back to list_ptr in case the original list
     * was NULL.  This should be a no-op if the list wasn't null. */
    *list_ptr = list;
    return TRUE;
}

gboolean
bygfoot_json_deserialize_player_competition_stats_array(Bygfoot *bygfoot,
                                                        json_object *array_obj,
                                                        GArray **array_ptr)
{
    return deserialize_array(
        bygfoot, array_obj, array_ptr, sizeof(PlayerCompetitionStats), NULL,
        (deserialize_func_type)
            bygfoot_json_deserialize_player_competition_stats);
}

gboolean
bygfoot_json_deserialize_player_competition_stats(
    Bygfoot *bygfoot, json_object *stats_obj,
    PlayerCompetitionStats **stats_ptr)
{
    PlayerCompetitionStats *stats = *stats_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stats_obj, stats, field, func);

    DESERIALIZE_FIELD_CUSTOM(
        stats_obj, "clid",
        deserialize_competition_ptr(bygfoot, stats_obj, &stats->competition));
    DESERIALIZE(games, deserialize_int64);
    DESERIALIZE(goals, deserialize_int64);
    DESERIALIZE(shots, deserialize_int64);
    DESERIALIZE(yellow, deserialize_int64);
    DESERIALIZE(red, deserialize_int64);

#undef DESERIALIZE

    return TRUE;
}

static gboolean
table_compare_func(json_object *a, gpointer b)
{
    return TRUE;
}

gboolean
bygfoot_json_deserialize_tables(Bygfoot *bygfoot, json_object *tables_obj,
                                GArray **tables_ptr)
{
    return deserialize_array(
        bygfoot, tables_obj, tables_ptr, sizeof(Table), table_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_table);
}

gboolean
bygfoot_json_deserialize_table(Bygfoot *bygfoot, json_object *table_obj,
                               Table **table_ptr)
{
    Table *table;

    if (!*table_ptr)
        *table_ptr = calloc(1, sizeof(Table));

    table = *table_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(table_obj, table, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(competition, deserialize_competition_ptr);
    DESERIALIZE(round, deserialize_int64);
    DESERIALIZE(elements, bygfoot_json_deserialize_table_elements);

#undef DESERIALIZE

    return TRUE;
}

static gboolean
table_element_compare_func(json_object *a, gpointer b)
{
    TableElement *element = (TableElement *)b;
    json_object *team_obj = json_object_object_get(a, "team");
    gint team_id =
        json_object_get_int64(json_object_object_get(team_obj, "id"));

    return element->team->id == team_id;
}

gboolean
bygfoot_json_deserialize_table_elements(Bygfoot *bygfoot,
                                        json_object *elements_obj,
                                        GArray **elements_ptr)
{
    return deserialize_array(
        bygfoot, elements_obj, elements_ptr, sizeof(TableElement),
        table_element_compare_func,
        (deserialize_func_type)bygfoot_json_deserialize_table_element);
}

gboolean
bygfoot_json_deserialize_table_element(Bygfoot *bygfoot,
                                       json_object *element_obj,
                                       TableElement **element_ptr)
{
    gint i;
    TableElement *element;
    struct fields
    {
        enum TableElementValues v;
        const char *name;
    };

    struct fields element_fields[] = {
        {TABLE_PLAYED, "played"}, {TABLE_WON, "won"}, {TABLE_DRAW, "draw"},
        {TABLE_LOST, "lost"},     {TABLE_GF, "gf"},   {TABLE_GA, "ga"},
        {TABLE_GD, "gd"},         {TABLE_PTS, "pts"},
    };

    if (!*element_ptr) {
        *element_ptr = calloc(1, sizeof(TableElement));
    }
    element = *element_ptr;

#define DESERIALIZE(field, func)                                               \
    DESERIALIZE_FIELD(element_obj, element, field, func);

    DESERIALIZE(team, bygfoot_json_deserialize_team_ptr);
    DESERIALIZE(old_rank, deserialize_int64);

    for (i = 0; i < TABLE_END; i++) {
        const struct fields *field = &element_fields[i];
        json_object *field_obj =
            json_object_object_get(element_obj, field->name);
        deserialize_int64(bygfoot, field_obj, element->values + field->v);
    }

#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_prom_games(Bygfoot *bygfoot,
                                    json_object *prom_games_obj,
                                    PromGames **prom_games_ptr)
{
    PromGames *prom_games = *prom_games_ptr;
#define DESERIALIZE(field, func)                                               \
    DESERIALIZE_FIELD(prom_games_obj, prom_games, field, func);

    DESERIALIZE_FIELD_CUSTOM(
        prom_games_obj, "ranks",
        deserialize_int_array(bygfoot,
                              json_object_object_get(prom_games_obj, "ranks"),
                              prom_games->ranks, 2));
    DESERIALIZE(dest_sid, deserialize_string);
    DESERIALIZE(loser_sid, deserialize_string);
    DESERIALIZE(number_of_advance, deserialize_int64);
    DESERIALIZE(cup_sid, deserialize_string);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_prom_rel_prom_games(Bygfoot *bygfoot,
                                             json_object *prom_games_obj,
                                             GArray **prom_games_ptr)
{
    return deserialize_array(
        bygfoot, prom_games_obj, prom_games_ptr, sizeof(PromGames), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_prom_games);
}

gboolean
bygfoot_json_deserialize_prom_rel_element(Bygfoot *bygfoot,
                                          json_object *element_obj,
                                          PromRelElement **element_ptr)
{
    PromRelElement *element = *element_ptr;
#define DESERIALIZE(field, func)                                               \
    DESERIALIZE_FIELD(element_obj, element, field, func);

    DESERIALIZE_FIELD_CUSTOM(
        element_obj, "ranks",
        deserialize_int_array(bygfoot,
                              json_object_object_get(element_obj, "ranks"),
                              element->ranks, 2));
    DESERIALIZE(from_table, deserialize_int64);
    DESERIALIZE(dest_sid, deserialize_string);
    DESERIALIZE(type, deserialize_int64);
    DESERIALIZE(num_teams, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_prom_rel_elements(Bygfoot *bygfoot,
                                           json_object *elements_obj,
                                           GArray **elements_ptr)
{
    return deserialize_array(
        bygfoot, elements_obj, elements_ptr, sizeof(PromRelElement), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_prom_rel_element);
}

gboolean
bygfoot_json_deserialize_prom_rel(Bygfoot *bygfoot, json_object *prom_rel_obj,
                                  PromRel *prom_rel)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(prom_rel_obj, prom_rel, field, func);

    DESERIALIZE(elements, bygfoot_json_deserialize_prom_rel_elements);
    DESERIALIZE(prom_games, bygfoot_json_deserialize_prom_rel_prom_games);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_joined_leagues(Bygfoot *bygfoot,
                                        json_object *leagues_obj,
                                        GArray **leagues_ptr)
{
    return deserialize_array(
        bygfoot, leagues_obj, leagues_ptr, sizeof(JoinedLeague), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_joined_league);
}

gboolean
bygfoot_json_deserialize_joined_league(Bygfoot *bygfoot,
                                       json_object *league_obj,
                                       JoinedLeague **league_ptr)
{
    JoinedLeague *league = *league_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(league_obj, league, field, func);

    DESERIALIZE(sid, deserialize_string);
    DESERIALIZE(rr, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_new_tables(Bygfoot *bygfoot, json_object *tables_obj,
                                    GArray **tables_ptr)
{
    return deserialize_array(
        bygfoot, tables_obj, tables_ptr, sizeof(NewTable), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_new_table);
}

gboolean
bygfoot_json_deserialize_new_table(Bygfoot *bygfoot, json_object *table_obj,
                                   NewTable **table_ptr)
{
    NewTable *table = *table_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(table_obj, table, field, func);

    DESERIALIZE(add_week, deserialize_int64);
    DESERIALIZE(name, deserialize_string);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_stats(Bygfoot *bygfoot, json_object *stats_obj,
                               GArray **stats_ptr)
{
    return deserialize_array(
        bygfoot, stats_obj, stats_ptr, sizeof(Stat), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_stat);
}

gboolean
bygfoot_json_deserialize_stat(Bygfoot *bygfoot, json_object *stat_obj,
                              Stat **stat_ptr)
{
    Stat *stat = *stat_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stat_obj, stat, field, func);

    DESERIALIZE(team_name, deserialize_string);
    DESERIALIZE(value1, deserialize_int64);
    DESERIALIZE(value2, deserialize_int64);
    DESERIALIZE(value3, deserialize_int64);
    DESERIALIZE(value_string, deserialize_string);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_league_stat(Bygfoot *bygfoot, json_object *stat_obj,
                                     LeagueStat **stat_ptr)
{
    LeagueStat *stat = *stat_ptr;
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stat_obj, stat, field, func);

    DESERIALIZE(league_symbol, deserialize_string);
    DESERIALIZE(league_name, deserialize_string);
    DESERIALIZE(teams_off, bygfoot_json_deserialize_stats);
    DESERIALIZE(teams_def, bygfoot_json_deserialize_stats);
    DESERIALIZE(player_scorers, bygfoot_json_deserialize_stats);
    DESERIALIZE(player_goalies, bygfoot_json_deserialize_stats);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_league(Bygfoot *bygfoot, json_object *league_obj,
                                League **league_ptr)
{

    League *league;
    json_object *field_obj;
    json_object *two_match_weeks_obj;
    json_object *rr_breaks_obj;
    LeagueStat *stats;

    if (!*league_ptr) {
        *league_ptr = calloc(1, sizeof(League));
        **league_ptr = league_new(FALSE, NULL);
    }

    league = *league_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(league_obj, league, field, func);

    DESERIALIZE_FIELD_CUSTOM(
        league_obj, "name",
        deserialize_string(bygfoot, json_object_object_get(league_obj, "name"),
                           &league->c.name));
    DESERIALIZE(short_name, deserialize_string);
    DESERIALIZE(sid, deserialize_string);
    DESERIALIZE(symbol, deserialize_string);
    DESERIALIZE(names_file, deserialize_string);
    DESERIALIZE(prom_rel, bygfoot_json_deserialize_prom_rel);
    field_obj = json_object_object_get(league_obj, "id");
    if (field_obj)
        deserialize_int64(bygfoot, field_obj, &league->c.id);
    DESERIALIZE(layer, deserialize_int64);
    DESERIALIZE(first_week, deserialize_int64);
    DESERIALIZE(week_gap, deserialize_int64);

    two_match_weeks_obj = json_object_object_get(league_obj, "two_match_weeks");
    if (two_match_weeks_obj) {
        gint i;
        if (!json_object_is_type(two_match_weeks_obj, json_type_array))
            return FALSE;
        for (i = 0; i < 2; i++) {
            json_object *array_obj =
                json_object_array_get_idx(two_match_weeks_obj, i);
            deserialize_array(
                bygfoot, array_obj, &league->two_match_weeks[i], sizeof(gint),
                NULL, (deserialize_func_type)deserialize_int64_array_wrapper);
        }
    }

    DESERIALIZE(round_robins, deserialize_int64);
    rr_breaks_obj = json_object_object_get(league_obj, "rr_breaks");
    if (rr_breaks_obj) {
        deserialize_array(
            bygfoot, rr_breaks_obj, &league->rr_breaks, sizeof(gint), NULL,
            (deserialize_func_type)deserialize_int64_array_wrapper);
    }

    field_obj = json_object_object_get(league_obj, "yellow_red");
    if (field_obj)
        deserialize_int64(bygfoot, field_obj, &league->c.yellow_red);
    DESERIALIZE(average_talent, deserialize_double);
    DESERIALIZE_FIELD_CUSTOM(league_obj, "teams",
                             bygfoot_json_deserialize_teams(
                                 bygfoot,
                                 json_object_object_get(league_obj, "teams"),
                                 &league->teams));
    DESERIALIZE(joined_leagues, bygfoot_json_deserialize_joined_leagues);
    DESERIALIZE(tables, bygfoot_json_deserialize_tables);
    DESERIALIZE(new_tables, bygfoot_json_deserialize_new_tables);
    DESERIALIZE(fixtures, bygfoot_json_deserialize_fixtures);
    DESERIALIZE_FIELD_CUSTOM(
        league_obj, "properties",
        deserialize_string_ptr_array(
            bygfoot, json_object_object_get(league_obj, "properties"),
            &league->c.properties));
    DESERIALIZE(week_breaks, bygfoot_json_deserialize_week_breaks);
    stats = &league->stats;
    DESERIALIZE_FIELD_CUSTOM(
        league_obj, "stats",
        bygfoot_json_deserialize_league_stat(
            bygfoot, json_object_object_get(league_obj, "stats"), &stats));

#undef DESERIALIZE

    /* Update the competition pointer for the fixtures. */
    fixtures_set_competition(league->fixtures, &league->c);

    /* Update the competition pointer for the tables. */
    tables_set_competition(league->tables, &league->c);

    return TRUE;
}

gboolean
bygfoot_json_deserialize_transfers(Bygfoot *bygfoot, json_object *transfers_obj,
                                   GArray **transfers)
{
    return deserialize_array(
        bygfoot, transfers_obj, transfers, sizeof(Transfer), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_transfer);
}

gboolean
bygfoot_json_deserialize_transfer_offers(Bygfoot *bygfoot,
                                         json_object *offers_obj,
                                         GArray **offers)
{
    return deserialize_array(
        bygfoot, offers_obj, offers, sizeof(TransferOffer), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_transfer_offer);
}

gboolean
bygfoot_json_deserialize_transfer_offer(Bygfoot *bygfoot,
                                        json_object *offer_obj,
                                        TransferOffer **offer_ptr)
{
    TransferOffer *offer = *offer_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(offer_obj, offer, field, func);

    DESERIALIZE(tm, bygfoot_json_deserialize_team_ptr);
    DESERIALIZE(fee, deserialize_int64);
    DESERIALIZE(wage, deserialize_int64);
    DESERIALIZE(status, deserialize_int64);

#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_transfer(Bygfoot *bygfoot, json_object *transfer_obj,
                                  Transfer **transfer_ptr)
{
    Transfer *transfer = *transfer_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(transfer_obj, transfer, field, func);

    DESERIALIZE(tm, bygfoot_json_deserialize_team_ptr);
    DESERIALIZE(id, deserialize_int64);
    DESERIALIZE(time, deserialize_int64);
    DESERIALIZE_FIELD_CUSTOM(
        transfer_obj, "fee",
        deserialize_int_array(bygfoot,
                              json_object_object_get(transfer_obj, "fee"),
                              transfer->fee, QUALITY_END));
    DESERIALIZE_FIELD_CUSTOM(
        transfer_obj, "wage",
        deserialize_int_array(bygfoot,
                              json_object_object_get(transfer_obj, "wage"),
                              transfer->wage, QUALITY_END));
    DESERIALIZE(offers, bygfoot_json_deserialize_transfer_offers);

#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_bets(Bygfoot *bygfoot, json_object *bet_obj,
                              GArray **bets)
{
    gint i;

    for (i = 0; i < 2; i++) {
        GArray *bet_matches = bets[i];
        json_object *obj = json_object_array_get_idx(bet_obj, i);
        deserialize_array(
            bygfoot, obj, &bet_matches, sizeof(BetMatch), NULL,
            (deserialize_func_type)bygfoot_json_deserialize_bet_match);
    }
    return TRUE;
}

gboolean
bygfoot_json_deserialize_bet_match(Bygfoot *bygfoot, json_object *bet_match_obj,
                                   BetMatch **bet_match_ptr)
{
    BetMatch *bet_match = *bet_match_ptr;
    json_object *odds_obj;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(bet_match_obj, bet_match, field, func);

    DESERIALIZE(fix_id, deserialize_int64);
    odds_obj = json_object_object_get(bet_match_obj, "odds");
    if (odds_obj) {
        gint i;
        for (i = 0; i < 3; i++) {
            json_object *obj = json_object_array_get_idx(odds_obj, i);
            deserialize_double(bygfoot, obj, &bet_match->odds[i]);
        }
    }
#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_job(Bygfoot *bygfoot, json_object *job_obj, Job *job)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(job_obj, job, field, func);

    DESERIALIZE(type, deserialize_int64);
    DESERIALIZE(time, deserialize_int64);
    DESERIALIZE(country_file, deserialize_string);
    DESERIALIZE(country_name, deserialize_string);
    DESERIALIZE(league_name, deserialize_string);
    DESERIALIZE(league_layer, deserialize_int64);
    DESERIALIZE(country_rating, deserialize_int64);
    DESERIALIZE(talent_percent, deserialize_int64);
    DESERIALIZE(team_id, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_champ_stats(Bygfoot *bygfoot, json_object *stats_obj,
                                     GArray **stats)
{
    return deserialize_array(
        bygfoot, stats_obj, stats, sizeof(ChampStat), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_champ_stat);
}

gboolean
bygfoot_json_deserialize_champ_stat(Bygfoot *bygfoot, json_object *stat_obj,
                                    ChampStat **stat_ptr)
{
    ChampStat *stat = *stat_ptr;

#define DESERIALIZE(field, func) DESERIALIZE_FIELD(stat_obj, stat, field, func);

    DESERIALIZE(team_name, deserialize_string);
    DESERIALIZE(cl_name, deserialize_string);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_league_stats(Bygfoot *bygfoot, json_object *stats_obj,
                                      GArray **stats)
{
    return deserialize_array(
        bygfoot, stats_obj, stats, sizeof(LeagueStat), NULL,
        (deserialize_func_type)bygfoot_json_deserialize_league_stat);
}

gboolean
bygfoot_json_deserialize_season_stat(Bygfoot *bygfoot, json_object *stat_obj,
                                     SeasonStat *stat)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(stat_obj, stat, field, func);

    DESERIALIZE(season_number, deserialize_int64);
    DESERIALIZE(league_champs, bygfoot_json_deserialize_champ_stats);
    DESERIALIZE(cup_champs, bygfoot_json_deserialize_champ_stats);
    DESERIALIZE(league_stats, bygfoot_json_deserialize_league_stats);

#undef DESERIALIZE
    return TRUE;
}

gboolean
bygfoot_json_deserialize_bygfoot(const gchar *json, Bygfoot *bygfoot)
{
    struct json_object *bygfoot_obj;
    enum json_tokener_error json_error;
    const char *sid;
    Country *country_ptr = &country;

    bygfoot_obj = json_tokener_parse_verbose(json, &json_error);
    if (!bygfoot_obj) {
        fprintf(stderr, "Failed to parse json %s:\n",
                json_tokener_error_desc(json_error));
        return FALSE;
    }

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    func(bygfoot, json_object_object_get(bygfoot_obj, #field), &field);

    DESERIALIZE(season, deserialize_int64);
    DESERIALIZE(week, deserialize_int64);
    DESERIALIZE(week_round, deserialize_int64);
    /*DESERIALIZE(season_stats, bygfoot_json_deserialize_season_stats);*/
    DESERIALIZE(current_interest, deserialize_double);
    DESERIALIZE(cur_user, deserialize_int64);
#undef DESERIALIZE
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(bygfoot_obj, bygfoot, field, func);
    DESERIALIZE(international_cups, bygfoot_json_deserialize_cups);

    bygfoot_json_deserialize_country(
        bygfoot, json_object_object_get(bygfoot_obj, "country"), &country_ptr);
    DESERIALIZE_FIELD_CUSTOM(
        bygfoot_obj, "counters",
        deserialize_int_array(bygfoot,
                              json_object_object_get(bygfoot_obj, "counters"),
                              counters, COUNT_END));
    DESERIALIZE(random_seed, deserialize_int64);

#undef DESERIALIZE
    json_object_put(bygfoot_obj);

    return TRUE;
}

gboolean
bygfoot_json_deserialize_country(Bygfoot *bygfoot, json_object *country_obj,
                                 Country **country_ptr)
{
    gboolean ret = FALSE;
    enum json_tokener_error json_error;

    Country *country;
    if (!*country_ptr) {
        *country_ptr = calloc(1, sizeof(Country));
    }

    country = *country_ptr;

#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(country_obj, country, field, func);

    DESERIALIZE(name, deserialize_string);
    DESERIALIZE(symbol, deserialize_string);
    DESERIALIZE(sid, deserialize_string);
    DESERIALIZE(rating, deserialize_int64);
    deserialize_int64(
        bygfoot, json_object_object_get(country_obj, "reserve_promotion_rules"),
        (int *)&country->reserve_promotion_rules);
    DESERIALIZE(leagues, bygfoot_json_deserialize_leagues);
    DESERIALIZE(cups, bygfoot_json_deserialize_cups);
    DESERIALIZE(allcups, bygfoot_json_deserialize_allcups);
    country->bygfoot = bygfoot;

#undef DESERIALIZE

    return TRUE;
}

gboolean
bygfoot_json_deserialize_newspaper_article(Bygfoot *bygfoot,
                                           json_object *article_obj,
                                           NewsPaperArticle *article)
{
#define DESERIALIZE(field, func)                                               \
    ;                                                                          \
    DESERIALIZE_FIELD(article_obj, article, field, func);
    gint comp_id;

    DESERIALIZE(week_number, deserialize_int64);
    DESERIALIZE(week_round_number, deserialize_int64);
    DESERIALIZE(title_id, deserialize_int64);
    DESERIALIZE(subtitle_id, deserialize_int64);
    DESERIALIZE(cup_round, deserialize_int64);
    deserialize_int64(
        bygfoot, json_object_object_get(article_obj, "competition"), &comp_id);
    article->competition = bygfoot_get_competition_id(bygfoot, comp_id);
    DESERIALIZE(title_small, deserialize_string);
    DESERIALIZE(title, deserialize_string);
    DESERIALIZE(subtitle, deserialize_string);
    DESERIALIZE(id, deserialize_int64);
    DESERIALIZE(user_idx, deserialize_int64);

#undef DESERIALIZE
    return TRUE;
}
