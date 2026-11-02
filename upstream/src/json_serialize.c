/** \file json_serialize.c
 *
 *
 * This contains functions for converting Bygfoot objects to json.  The general
 * strategy is to only serialize values that are unchanged during a Bygfoot
 * game.
 *
 * Object members that can change as the result of game play should be
 * queried with functions directly.
 *
 * Parent: Full, Child: Full : Fields == NULL, recursive = TRUE
 * Parent: Full, Child: ID: Fields == NULL, recursive = FALSE
 * Parent: ID: Fields == IDs
 * Child field in filter: IDs
 * Child field in filter, recursive = TRUE: FULL
 *
 */

#include <json-c/json_object.h>
#include <glib.h>
#include "variables.h"
#include "json_serialize.h"
#include "league_struct.h"

static void write_json(const char *str)
{
    printf("%s", str);
}

static void write_json_field_key(const char *key,
                                 void (*write_func)(const char *, gpointer),
                                 gpointer userdata)
{
    write_func("\"", userdata);
    write_func(key, userdata);
    write_func("\":", userdata);
}

static void temp_write_json_field_key(const char *key)
{
    printf("\"%s\":", key);
}

struct len_userdata {
    gint len;
    gpointer write_userdata;
};

#define SERIALIZE_BEGIN_OBJECT(write_func, userdata) \
   { \
     gboolean __have_field = FALSE; \
     write_func("{", userdata); \

#define SERIALIZE_END_OBJECT(write_func, userdata) \
   write_func("}", userdata); \
   }

#define SERIALIZE_BEGIN_ARRAY(write_func, userdata) \
   write_func("[", userdata);

#define SERIALIZE_END_ARRAY(write_func, userdata) \
   write_func("]", userdata);

#define STREAM_OBJ_FIELD_CUSTOM(field, serialize_stmt, field_list) \
    { \
    json_object *child_fields = NULL; \
    if (should_serialize_field(field, field_list, &child_fields)) { \
        if (__have_field) { \
            write_func(",", userdata); \
        } \
	write_json_field_key(field, write_func, userdata); \
        serialize_stmt; \
        __have_field = TRUE; \
    } \
    }

#define STREAM_SIZED_ARRAY(field, val, serialize_func, field_list, write_func, userdata, array_len) \
    STREAM_OBJ_FIELD_CUSTOM(field, {serialize_func(val, child_fields, write_func, userdata, array_len);}, field_list);

#define STREAM_VALUE(field, val, serialize_func, field_list, write_func, userdata) \
    { \
    json_object *child_fields = NULL; \
    if (should_serialize_field(field, field_list, &child_fields)) { \
        if (__have_field) { \
            write_func(",", userdata); \
        } \
        write_json_field_key(field, write_func, userdata); \
        serialize_func(val, child_fields, write_func, userdata); \
        __have_field = TRUE; \
    } \
    }

#define STREAM_OBJ_FIELD(object, field, serialize_func, field_list, write_func, userdata) \
        STREAM_VALUE(#field, object->field, serialize_func, field_list, write_func, userdata)

#define SERIALIZE_OBJ_LAST_FIELD ;

#define SERIALIZE_OBJECT_FIELD_FILTER(json_object, object, field, \
                                      serialize_func, field_list) \
    if (!field_list || g_hash_table_contains(field_list, #field)) { \
      json_object_object_add(json_object, #field, serialize_func(object->field)); \
    }

#define SERIALIZE_OBJECT_FIELD_STRUCT(json_object, object, field, \
                                      serialize_func, field_list) \
    if (!field_list || g_hash_table_contains(field_list, #field)) { \
      json_object_object_add(json_object, #field, serialize_func(object->field, field_list)); \
    }

#define SERIALIZE_OBJECT_FIELD(json_object, object, field, serialize_func) \
  SERIALIZE_OBJECT_FIELD_FILTER(json_object, object, field, serialize_func, NULL)

#define SERIALIZE_WITH_CUSTOM_USERDATA(userdata, length, serialize_stmt) \
{ \
    struct len_userdata other_userdata; \
    other_userdata.len = length; \
    other_userdata.write_userdata = userdata; \
    userdata = &other_userdata; \
    serialize_stmt; \
    userdata = other_userdata.write_userdata; \
}


#define SERIALIZE_GARRAY_FUNC_DEF(func, item_type, item_serialize_func) \
    void \
    func(const GArray *garray, const json_object *fields, void (*write_func)(const char*, gpointer), gpointer userdata) \
    { \
        gint i; \
        SERIALIZE_BEGIN_ARRAY(write_func, userdata); \
        for (i = 0; i < garray->len; i++) { \
            if (i) { \
                write_func(",", userdata); \
            } \
            const item_type *item = &g_array_index(garray, item_type, i); \
            item_serialize_func(item, fields, write_func, userdata); \
        } \
        SERIALIZE_END_ARRAY(write_func, userdata); \
    }

static gboolean should_serialize_field(const char *field,
                       const json_object *fields,
                       json_object **out_child_fields)
{
    gboolean default_include = FALSE;
    json_object *default_obj;

    if (!fields)
        return TRUE;

    /* Check for the * key. */
    if (json_object_object_get_ex(fields, "*", &default_obj) &&
        json_object_is_type(default_obj, json_type_boolean)) {

        default_include = json_object_get_boolean(default_obj);
    }

    if (!json_object_object_get_ex(fields, field, out_child_fields))
        return default_include;

    if (json_object_is_type(*out_child_fields, json_type_boolean) &&
        !json_object_get_boolean(*out_child_fields))
        return FALSE;

    return TRUE;
}


void
serialize_boolean(gboolean val,
                  const json_object *fields,
                  void (*write_func)(const char *, gpointer),
                  gpointer userdata)
{
    if (val)
        write_func("true", userdata);
    else
        write_func("false", userdata);
}

void
serialize_string(const gchar * string,
                 const json_object *fields,
                 void (*write_func)(const char *, gpointer),
                 gpointer userdata)
{
    json_object *obj;
    if (string)
        obj = json_object_new_string(string);
    else
        obj = NULL; 
    write_func(json_object_to_json_string(obj), userdata);
    json_object_put(obj);
}

static void
serialize_string_len(const gchar * string, gint len,
                 const json_object *fields,
                 void (*write_func)(const char *, gpointer),
                 gpointer userdata)
{
    json_object *obj;
    if (string)
        obj = json_object_new_string_len(string, len);
    else
        obj = NULL;

    write_func(json_object_to_json_string(obj), userdata);
    json_object_put(obj);
}

void
serialize_gchar_ptr_array(GPtrArray* ptr_array,
                          const json_object *fields,
                          void (*write_func)(const char *, gpointer),
                          gpointer userdata)
{
    int i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < ptr_array->len; i++) {
        if (i)
            write_func(",", userdata);
        const gchar *string = g_ptr_array_index(ptr_array, i);
        serialize_string(string, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

static json_object *
serialize_gchar_array(gchar *const *array,
                      const json_object *fields,
                      void (*write_func)(const char *, gpointer),
                      gpointer userdata,
                      int len)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < len; i++) {
        if (i)
            write_func(",", userdata);
        serialize_string(array[i], fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
serialize_double(double val, const json_object *fields,
              void (*write_func)(const char *, gpointer),
              gpointer userdata)
{
    char string[32];
    snprintf(string, 32, "%.17g", val);
    write_func(string, userdata);
}

void
serialize_int(gint val, const json_object *fields,
              void (*write_func)(const char *, gpointer),
              gpointer userdata)
{
    char string[16];
    snprintf(string, 16, "%d", val);
    write_func(string, userdata);
}

void
serialize_int_array(const gint *array,
                    const json_object *fields,
                    void (*write_func)(const char *, gpointer),
                    gpointer userdata,
                    gint len)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < len; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_int(array[i], fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
serialize_float_array(const float *array,
                      const json_object *fields,
                      void (*write_func)(const char *, gpointer),
                      gpointer userdata,
                      int len)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < len; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_double(array[i], fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
serialize_int_garray(GArray *array,
                     const json_object *fields,
                     void (*write_func)(const char *, gpointer),
                     gpointer userdata)
{
    serialize_int_array((const gint*)array->data,
                        fields, write_func, userdata, array->len);
}

static GHashTable*
fields_to_hash_table(gchar **fields)
{
    gchar *field;
    GHashTable *hash_table;

    if (!fields)
        return NULL;
    hash_table = g_hash_table_new(g_str_hash, g_str_equal);
    for (field = *fields; field; field = *fields++) {
        g_hash_table_add(hash_table, field);
    }
    return hash_table;
}

void
bygfoot_json_serialize_write_stdout(const char *s, gpointer userdata)
{
    printf("%s", s);
}

void
bygfoot_json_serialize_write_buffer(const char *s, gpointer userdata)
{
    #define MAX_LEN 1024
    struct bygfoot_json_serialize_buffer *buffer = userdata;

    gint len = strnlen(s, MAX_LEN);

    if (len + buffer->len > buffer->alloc_size) {
        buffer->data = realloc(buffer->data, buffer->alloc_size + MAX_LEN);
        buffer->alloc_size += MAX_LEN;
    }

    strncpy(buffer->data + buffer->len, s, len);
    buffer->len += len;
}

void
bygfoot_json_serialize_write_file(const char *s, gpointer userdata)
{
    FILE *file = userdata;

    fprintf(file, "%s", s);
}

void
bygfoot_json_serialize_country_list(GPtrArray *country_list,
                                    const json_object *fields,
                                    void (*write_func)(const char *, gpointer),
                                    gpointer userdata)
{
    serialize_gchar_ptr_array(country_list, fields, write_func, userdata);
}

#define SERIALIZE_OBJECT_FIELD_JSON_FILTER(json_object, object, field, \
                                           serialize_func, field_list) \
    json_object *child_fields = NULL; \
    if (!field_list || json_object_object_get_ex(field_list, #field, &child_fields)) \
       json_object_object_add_ex(json_object, #field, serialize_func(object->field, child_fields), JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
        

void
bygfoot_json_serialize_bygfoot(const Bygfoot *bygfoot,
                               const json_object *fields,
                               void (*write_func)(const char *, gpointer),
                               gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_VALUE(#field, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_VALUE("country", (&country), bygfoot_json_serialize_country,
                 fields, write_func, userdata);
    SERIALIZE(country_list, bygfoot_json_serialize_countries);
    STREAM_OBJ_FIELD_CUSTOM("counters", serialize_int_array(counters, fields, write_func, userdata, COUNT_END), fields);
    SERIALIZE(users, bygfoot_json_serialize_users);
    SERIALIZE(season, serialize_int);
    SERIALIZE(week, serialize_int);
    SERIALIZE(week_round, serialize_int);
    SERIALIZE(transfer_list, bygfoot_json_serialize_transfers);
    SERIALIZE(season_stats, bygfoot_json_serialize_season_stats);
    SERIALIZE(bets, bygfoot_json_serialize_bets);
    SERIALIZE(current_interest, serialize_double);
    SERIALIZE(jobs, bygfoot_json_serialize_jobs);
    SERIALIZE(cur_user, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("newspaper",
                            bygfoot_json_serialize_newspaper(&newspaper, fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("random_seed",
	                    serialize_int(bygfoot->random_seed, fields, write_func, userdata), fields);
    SERIALIZE_OBJ_LAST_FIELD;
    STREAM_VALUE("international_cups", bygfoot->international_cups,
                 bygfoot_json_serialize_cups, fields, write_func, userdata);
    SERIALIZE_END_OBJECT(write_func, userdata);

    #undef SERIALIZE
}

void
bygfoot_json_serialize_users(const GArray *users,
                             const json_object *fields,
                             void (*write_func)(const char*, gpointer),
                             gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);

    for (i = 0 ; i < users->len; i++) {
        const User *user = &g_array_index(users, User, i);
        bygfoot_json_serialize_user(user, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_user(const User *user,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(user, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata); 
    SERIALIZE(name, serialize_string);
    SERIALIZE(tm, bygfoot_json_serialize_team_ptr);
    /* options: I don't think we need to save these, because they appear to
     * be loaded from a file. */
    /* events: I don't think we need to save these, because they appear to
     * be temporary */
    SERIALIZE(history, bygfoot_json_serialize_user_histories);
    STREAM_OBJ_FIELD_CUSTOM("counters", serialize_int_array(user->counters, fields, write_func, userdata, COUNT_USER_END), fields);
    SERIALIZE(money, serialize_int);
    SERIALIZE(debt, serialize_int);
    SERIALIZE(money_in, bygfoot_json_serialize_user_money_in);
    SERIALIZE(money_out, bygfoot_json_serialize_user_money_out);
    SERIALIZE(debt_interest, serialize_double);
    SERIALIZE(alr_start_week, serialize_int);
    SERIALIZE(alr_weekly_installment, serialize_int);
    SERIALIZE(scout, serialize_int);
    SERIALIZE(physio, serialize_int);
    if (user->live_game.fix)
        SERIALIZE(live_game, bygfoot_json_serialize_live_game);
    SERIALIZE(sponsor, bygfoot_json_serialize_user_sponsor);
    SERIALIZE(youth_academy, bygfoot_json_serialize_youth_academy);
    SERIALIZE(mmatches_file, serialize_string);
    SERIALIZE(bets, bygfoot_json_serialize_user_bets);
    SERIALIZE(default_team, serialize_int_garray);
    SERIALIZE(default_structure, serialize_int);
    SERIALIZE(default_style, serialize_int);
    SERIALIZE(default_style, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(default_boost, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_user_bets(GArray * const *bets,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        int j;
        const GArray *bets_user = bets[i];
        if (i) {
            write_func(",", userdata);
        }
        SERIALIZE_BEGIN_ARRAY(write_func, userdata);
        for (j = 0; j < bets_user->len; j++) {
            const BetUser *bet_user = &g_array_index(bets_user, BetUser, j);
            if (j) {
                write_func(",", userdata);
            }
            bygfoot_json_serialize_bet_user(bet_user, fields, write_func, userdata);
        }
        SERIALIZE_END_ARRAY(write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_user_sponsor(UserSponsor sponsor,
                                    const json_object *fields,
                                    void (*write_func)(const char*, gpointer),
                                    gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&sponsor), field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata); 
    STREAM_OBJ_FIELD_CUSTOM("name", serialize_string_len(sponsor.name->str,
                                                         sponsor.name->len,
                                                         fields,
							 write_func,
							 userdata), fields);
    SERIALIZE(benefit, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(contract, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata); 
    #undef SERIALIZE
}

void
bygfoot_json_serialize_user_histories(const GArray *histories,
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < histories->len; i++) {
        const UserHistory *user_history = &g_array_index(histories, UserHistory, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_user_history(user_history, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_user_history(const UserHistory *history,
                                    const json_object *fields,
                                    void (*write_func)(const char*, gpointer),
                                    gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(history, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(season, serialize_int);
    SERIALIZE(week, serialize_int);
    SERIALIZE(type, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(team_name, serialize_string);
    STREAM_OBJ_FIELD_CUSTOM("string", serialize_gchar_array(history->string, fields, write_func, userdata, 3), fields);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_user_money_in(const gint (*money_in)[5],
                                     const json_object *fields,
                                     void (*write_func)(const char*, gpointer),
                                     gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_int_array(money_in[i], fields, write_func, userdata, MON_IN_END);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_user_money_out(const gint (*money_out)[13],
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    gint i;
    struct len_userdata other_userdata;
    other_userdata.len = MON_OUT_END;
    other_userdata.write_userdata = userdata;
    
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_int_array(money_out[i], fields, write_func, userdata, MON_OUT_END);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_bet_user(const BetUser *bet_user,
                                const json_object *fields,
                                void (*write_func)(const char*, gpointer),
                                gpointer userdata)
{

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(bet_user, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(fix_id, serialize_int);
    SERIALIZE(outcome, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(wager, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_bets(GArray **bets,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        gint j;
        if (i) {
            write_func(",", userdata);
        }
        GArray *bet_match_array = bets[i];
        SERIALIZE_BEGIN_ARRAY(write_func, userdata);
        for (j = 0; j < bet_match_array->len; j++) {
            if (j) {
                write_func(",", userdata);
            }
            const BetMatch *bet_match = &g_array_index(bet_match_array, BetMatch, j);
	    bygfoot_json_serialize_bet_match(bet_match, fields, write_func,
	                                     userdata);
        }
	SERIALIZE_END_ARRAY(write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_bet_match(const BetMatch *bet_match,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(bet_match, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(fix_id, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    STREAM_OBJ_FIELD_CUSTOM("odds", serialize_float_array(bet_match->odds, fields, write_func, userdata, 3), fields);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_countries(const GPtrArray *countries,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < countries->len; i++) {
        const Country *country = g_ptr_array_index(countries, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_country(country, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_country(const Country *country,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(country, field, serialize_func, fields, write_func, userdata)
    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(symbol, serialize_string);
    SERIALIZE(sid, serialize_string);
    SERIALIZE(rating, serialize_int);
    SERIALIZE(reserve_promotion_rules, serialize_int);
    SERIALIZE(leagues, bygfoot_json_serialize_leagues);
    SERIALIZE(cups, bygfoot_json_serialize_cups);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(allcups, bygfoot_json_serialize_cup_ptrs);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_leagues(const GPtrArray *leagues,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    gint i;
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < leagues->len; i++) {
        const League *league = g_ptr_array_index(leagues, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_league(league, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_league(const League *league,
                              const json_object *fields,
                              void (*write_func)(const char*, gpointer),
                              gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(league, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_OBJ_FIELD_CUSTOM("name", serialize_string(league->c.name, fields, write_func, userdata), fields);
    SERIALIZE(short_name, serialize_string);
    SERIALIZE(sid, serialize_string);
    SERIALIZE(symbol, serialize_string);
    SERIALIZE(names_file, serialize_string);
    SERIALIZE(prom_rel, bygfoot_json_serialize_prom_rel);
    STREAM_OBJ_FIELD_CUSTOM("id", serialize_int(league->c.id, fields, write_func, userdata), fields);
    SERIALIZE(layer, serialize_int);
    SERIALIZE(first_week, serialize_int);
    SERIALIZE(week_gap, serialize_int);
    SERIALIZE(two_match_weeks, bygfoot_json_serialize_two_match_weeks);
    SERIALIZE(round_robins, serialize_int);
    SERIALIZE(rr_breaks, serialize_int_garray);
    STREAM_OBJ_FIELD_CUSTOM("yellow_red", serialize_int(league->c.yellow_red, fields, write_func, userdata), fields);
    SERIALIZE(average_talent, serialize_double);
    STREAM_OBJ_FIELD_CUSTOM("teams", bygfoot_json_serialize_teams(league->teams, child_fields, write_func, userdata), fields);
    SERIALIZE(joined_leagues, bygfoot_json_serialize_joined_leagues);
    SERIALIZE(tables, bygfoot_json_serialize_tables);
    SERIALIZE(new_tables, bygfoot_json_serialize_new_tables);
    SERIALIZE(fixtures, bygfoot_json_serialize_fixtures);
    STREAM_OBJ_FIELD_CUSTOM("properties", serialize_gchar_ptr_array(league->c.properties, child_fields, write_func, userdata), fields);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(week_breaks, bygfoot_json_serialize_week_breaks);
    STREAM_OBJ_FIELD_CUSTOM("stats", bygfoot_json_serialize_league_stat(&league->stats, fields, write_func, userdata), fields);
    SERIALIZE(skip_weeks_with, serialize_gchar_ptr_array);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_prom_rel(PromRel prom_rel,
                                const json_object *fields,
                                void (*write_func)(const char*, gpointer),
                                gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&prom_rel), field, serialize_func, fields, write_func, userdata) 

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(elements, bygfoot_json_serialize_prom_rel_elements);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(prom_games, bygfoot_json_serialize_prom_games_array);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_prom_rel_elements,
                          PromRelElement,
                          bygfoot_json_serialize_prom_rel_element);

void
bygfoot_json_serialize_prom_rel_element(const PromRelElement* element,
                                        const json_object *fields,
                                        void (*write_func)(const char*, gpointer),
                                        gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(element, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_OBJ_FIELD_CUSTOM("ranks", serialize_int_array(element->ranks, fields, write_func, userdata, 2), fields);
    SERIALIZE(from_table, serialize_int);
    SERIALIZE(dest_sid, serialize_string);
    SERIALIZE(type, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(num_teams, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_prom_games_array,
                          PromGames,
                          bygfoot_json_serialize_prom_games);

void
bygfoot_json_serialize_prom_games(const PromGames *prom_games,
                                  const json_object *fields,
                                  void (*write_func)(const char*, gpointer),
                                  gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(prom_games, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_OBJ_FIELD_CUSTOM("ranks", serialize_int_array(prom_games->ranks, fields, write_func, userdata, 2), fields);
    SERIALIZE(dest_sid, serialize_string);
    SERIALIZE(loser_sid, serialize_string);
    SERIALIZE(number_of_advance, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(cup_sid, serialize_string);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_two_match_weeks(GArray * const *two_match_weeks,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_int_garray(two_match_weeks[i], fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_joined_leagues,
                          JoinedLeague,
                          bygfoot_json_serialize_joined_league);

void
bygfoot_json_serialize_joined_league(const JoinedLeague *league,
                                     const json_object *fields,
                                     void (*write_func)(const char*, gpointer),
                                     gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(league, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(sid, serialize_string);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(rr, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_new_tables,
                          NewTable,
                          bygfoot_json_serialize_new_table);

void
bygfoot_json_serialize_new_table(const NewTable *table,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(table, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(add_week, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(name, serialize_string);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_week_breaks,
                          WeekBreak,
                          bygfoot_json_serialize_week_break);

void
bygfoot_json_serialize_week_break(const WeekBreak *week_break,
                                  const json_object *fields,
                                  void (*write_func)(const char*, gpointer),
                                  gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(week_break, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(week_number, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(length, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_stadium(Stadium stadium,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&stadium), field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(capacity, serialize_int);
    SERIALIZE(average_attendance, serialize_int);
    SERIALIZE(possible_attendance, serialize_int);
    SERIALIZE(games, serialize_int);
    SERIALIZE(safety, serialize_double);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(ticket_price, serialize_double);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_teams(const GPtrArray *teams,
                             const json_object *fields,
                             void (*write_func)(const char*, gpointer),
                             gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < teams->len; i++) {
        Team *team = g_ptr_array_index(teams, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_team(team, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_team(const Team *team,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(team, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(symbol, serialize_string);
    SERIALIZE(names_file, serialize_string);
    SERIALIZE(def_file, serialize_string);
    SERIALIZE(strategy_sid, serialize_string);
    SERIALIZE(id, serialize_int);
    SERIALIZE(structure, serialize_int);
    SERIALIZE(style, serialize_int);
    SERIALIZE(boost, serialize_int);
    /* Skip average_talent.  This is a temporary value only used for generating
     * players.  It is not saved in the save game files. */
    SERIALIZE(luck, serialize_double);
    SERIALIZE(stadium, bygfoot_json_serialize_stadium);
    SERIALIZE(players, bygfoot_json_serialize_players);
    STREAM_OBJ_FIELD_CUSTOM("first_team", bygfoot_json_serialize_team_ptr(team->first_team.team, fields, write_func, userdata), fields);

    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(reserve_level, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_team_ptrs(GPtrArray *team_ptrs,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    gint i;

#if 0
    /* FIXME: SHOULD WE DO this? */
    if (!team_ptrs) {
        write_func("null", userdata);
        return;
    }
#endif
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; team_ptrs && i < team_ptrs->len; i++) {
        const Team *team = g_ptr_array_index(team_ptrs, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_team_ptr(team, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}


void
bygfoot_json_serialize_team_ptr(const Team *team,
                                const json_object *fields,
                                void (*write_func)(const char*, gpointer),
                                gpointer userdata)
{
    json_object *ptr_fields = json_object_new_object();
    json_object_object_add_ex(ptr_fields, "id", NULL,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(ptr_fields, "name", NULL,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
    bygfoot_json_serialize_team(team, ptr_fields, write_func, userdata);
    json_object_put(ptr_fields);
}

void
bygfoot_json_serialize_youth_academy(const YouthAcademy youth_academy,
                                     const json_object *fields,
                                     void (*write_func)(const char*, gpointer),
                                     gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&youth_academy), field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(tm, bygfoot_json_serialize_team_ptr);
    SERIALIZE(pos_pref, serialize_int);
    SERIALIZE(coach, serialize_int);
    SERIALIZE(percentage, serialize_int);
    SERIALIZE(av_coach, serialize_double);
    SERIALIZE(av_percentage, serialize_double);
    SERIALIZE(counter_youth, serialize_double);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(players, bygfoot_json_serialize_players);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_players(const GPtrArray *players,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < players->len; i++) {
        const Player *player = g_ptr_array_index(players, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_player(player, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_player(const Player *player,
                              const json_object *fields,
                              void (*write_func)(const char*, gpointer),
                              gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(player, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(pos, serialize_int);
    SERIALIZE(cpos, serialize_int);
    SERIALIZE(health, serialize_int);
    SERIALIZE(recovery, serialize_int);
    SERIALIZE(id, serialize_int);
    SERIALIZE(value, serialize_int);
    SERIALIZE(wage, serialize_int);
    SERIALIZE(offers, serialize_int);
    SERIALIZE(streak, serialize_int);
    SERIALIZE(skill, serialize_double);
    SERIALIZE(cskill, serialize_double);
    SERIALIZE(talent, serialize_double);
    STREAM_OBJ_FIELD_CUSTOM("etal", serialize_float_array(player->etal, fields, write_func, userdata, QUALITY_END), fields);
    SERIALIZE(fitness, serialize_double);
    SERIALIZE(lsu, serialize_double);
    SERIALIZE(age, serialize_double);
    SERIALIZE(peak_age, serialize_double);
    SERIALIZE(peak_region, serialize_double);
    SERIALIZE(contract, serialize_double);
    SERIALIZE(streak_prob, serialize_double);
    SERIALIZE(streak_count, serialize_double);
    SERIALIZE(participation, serialize_boolean);
    SERIALIZE(stats, bygfoot_json_serialize_player_competition_stats_list);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(team, bygfoot_json_serialize_team_ptr);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_player_ptr(const Player *player,
                                  const json_object *fields,
                                  void (*write_func)(const char*, gpointer),
                                  gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(player, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(id, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

struct serialize_list_cb {
    GList *list;
    gint i;
    gint len;
    const json_object *fields;
    void (*write_func)(const char*, gpointer);
    gpointer userdata;
};

static void
serialize_stats_list_cb(gpointer array, gpointer userdata)
{
    GArray *stats_array = (GArray*)array;
    struct serialize_list_cb *cb = (struct serialize_list_cb*)userdata;

    bygfoot_json_serialize_player_competition_stats_array(array, cb->fields, cb->write_func, cb->userdata);
    if (++cb->i != cb->len)
        cb->write_func(",", cb->userdata);
}

void
bygfoot_json_serialize_player_competition_stats_list(GList *stats_list,
                                         const json_object *fields,
                                         void (*write_func)(const char*, gpointer),
                                         gpointer userdata)
{
    struct serialize_list_cb cb;
    cb.list = stats_list;
    cb.i = 0;
    cb.len = g_list_length(stats_list);
    cb.fields = fields;
    cb.write_func = write_func;
    cb.userdata = userdata;
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    g_list_foreach(stats_list, serialize_stats_list_cb, &cb);
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_player_competition_stats_array(const GArray *stats_array,
                                         const json_object *fields,
                                         void (*write_func)(const char*, gpointer),
                                         gpointer userdata)
{
    gint i;
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < stats_array->len; i++) {
        const PlayerCompetitionStats *stats = &g_array_index(stats_array,PlayerCompetitionStats, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_player_competition_stats(stats, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_player_competition_stats(const PlayerCompetitionStats *stats,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(stats, field, serialize_func, fields, write_func, userdata);
    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_OBJ_FIELD_CUSTOM("clid", serialize_int(stats->competition->id, fields, write_func, userdata), fields);
    SERIALIZE(games, serialize_int);
    SERIALIZE(goals, serialize_int);
    SERIALIZE(shots, serialize_int);
    SERIALIZE(yellow, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(red, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_cup_round(const CupRound *round,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(round, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(home_away, serialize_boolean);
    SERIALIZE(replay, serialize_int);
    SERIALIZE(neutral, serialize_boolean);
    SERIALIZE(randomise_teams, serialize_boolean);
    SERIALIZE(round_robin_number_of_groups, serialize_int);
    SERIALIZE(round_robin_number_of_advance, serialize_int);
    SERIALIZE(round_robin_number_of_best_advance, serialize_int);
    SERIALIZE(round_robins, serialize_int);
    SERIALIZE(rr_breaks, serialize_int_garray);
    SERIALIZE(new_teams, serialize_int);
    SERIALIZE(byes, serialize_int);
    SERIALIZE(delay, serialize_int);
    SERIALIZE(two_match_weeks, bygfoot_json_serialize_two_match_weeks);
    SERIALIZE(team_ptrs, bygfoot_json_serialize_team_ptrs);
    SERIALIZE(choose_teams, bygfoot_json_serialize_cup_choose_teams);
    SERIALIZE(tables, bygfoot_json_serialize_tables);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(waits, bygfoot_json_serialize_cup_round_waits);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_cup_rounds(const GArray *rounds,
                                  const json_object *fields,
                                  void (*write_func)(const char*, gpointer),
                                  gpointer userdata)
{
    gint i;
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < rounds->len; i++) {
        const CupRound *round = &g_array_index(rounds, CupRound, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_cup_round(round, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_cups(const GPtrArray *cups,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{
    gint i;
    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < cups->len; i++) {
        const Cup *cup = g_ptr_array_index(cups, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_cup(cup, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_cup(const Cup *cup,
                           const json_object *fields,
                           void (*write_func)(const char*, gpointer),
                           gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(cup, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    STREAM_OBJ_FIELD_CUSTOM("name", serialize_string(cup->c.name, fields, write_func, userdata), fields);
    SERIALIZE(short_name, serialize_string);
    SERIALIZE(symbol, serialize_string);
    SERIALIZE(sid, serialize_string);
    STREAM_OBJ_FIELD_CUSTOM("id", serialize_int(cup->c.id, fields, write_func, userdata), fields);
    SERIALIZE(group, serialize_int);
    SERIALIZE(last_week, serialize_int);
    SERIALIZE(week_gap, serialize_int);
    SERIALIZE(add_week, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("yellow_red", serialize_int(cup->c.yellow_red, fields, write_func, userdata), fields);
    SERIALIZE(talent_diff, serialize_double);
    SERIALIZE(next_fixture_update_week, serialize_int);
    SERIALIZE(next_fixture_update_week_round, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("properties", serialize_gchar_ptr_array(cup->c.properties, fields, write_func, userdata), fields);
    SERIALIZE(rounds, bygfoot_json_serialize_cup_rounds);
    SERIALIZE(bye, bygfoot_json_serialize_team_ptrs);
    SERIALIZE(fixtures, bygfoot_json_serialize_fixtures);
    SERIALIZE(week_breaks, bygfoot_json_serialize_week_breaks);
    SERIALIZE(skip_weeks_with, serialize_gchar_ptr_array);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(history, bygfoot_json_serialize_cup_history);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_cup_ptrs(GPtrArray *cups,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    gint i;
    json_object *ptr_fields = json_object_new_object();
    json_object_object_add_ex(ptr_fields, "id", NULL,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);
    json_object_object_add_ex(ptr_fields, "sid", NULL,
                              JSON_C_OBJECT_ADD_KEY_IS_NEW | JSON_C_OBJECT_KEY_IS_CONSTANT);

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < cups->len; i++) {
        const Cup *cup = g_ptr_array_index(cups, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_cup(cup, ptr_fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
    json_object_put(ptr_fields);
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_cup_choose_teams,
                          CupChooseTeam,
                          bygfoot_json_serialize_cup_choose_team);

void
bygfoot_json_serialize_cup_choose_team(const CupChooseTeam *choose_team,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    const CupChooseTeam *iter;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (iter = choose_team; iter; iter = iter->next) {
        #define SERIALIZE(field, serialize_func) \
                STREAM_OBJ_FIELD(iter, field, serialize_func, fields, write_func, userdata);

        SERIALIZE_BEGIN_OBJECT(write_func, userdata);
        SERIALIZE(sid, serialize_string);
        SERIALIZE(number_of_teams, serialize_int);
        SERIALIZE(from_table, serialize_int);
        SERIALIZE(start_idx, serialize_int);
        SERIALIZE(end_idx, serialize_int);
        SERIALIZE(losers_round, serialize_int);
        SERIALIZE(randomly, serialize_boolean);
        SERIALIZE(generate, serialize_boolean);
        SERIALIZE(skip_group_check, serialize_boolean);
        SERIALIZE(preload, serialize_boolean);
        SERIALIZE_OBJ_LAST_FIELD;
        SERIALIZE(optional, serialize_boolean);
        SERIALIZE_END_OBJECT(write_func, userdata);
        #undef SERIALIZE

        if (iter->next)
            write_func(",", userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_cup_round_waits,
                          CupRoundWait,
                          bygfoot_json_serialize_cup_round_wait);

void
bygfoot_json_serialize_cup_round_wait(const CupRoundWait *wait,
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(wait, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(cup_sid, serialize_string);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(cup_round, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_cup_history(const GPtrArray *history,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < history->len; i++) {
        GPtrArray *teams = g_ptr_array_index(history, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_team_ptrs(teams, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_transfers(const GArray *transfers,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < transfers->len; i++) {
        const Transfer *transfer = &g_array_index(transfers, Transfer, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_transfer(transfer, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_transfer(const Transfer *transfer,
                                const json_object *fields,
                                void (*write_func)(const char*, gpointer),
                                gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(transfer, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(tm, bygfoot_json_serialize_team_ptr);
    SERIALIZE(id, serialize_int);
    SERIALIZE(time, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("fee", serialize_int_array(transfer->fee, fields, write_func, userdata, QUALITY_END), fields);
    STREAM_OBJ_FIELD_CUSTOM("wage", serialize_int_array(transfer->wage, fields, write_func, userdata, QUALITY_END), fields);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(offers, bygfoot_json_serialize_transfer_offers);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_transfer_offers(const GArray *offers,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < offers->len; i++) {
        const TransferOffer *offer = &g_array_index(offers, TransferOffer, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_transfer_offer(offer, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_transfer_offer(const TransferOffer *offer,
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(offer, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(tm, bygfoot_json_serialize_team_ptr);
    SERIALIZE(fee, serialize_int);
    SERIALIZE(wage, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(status, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_fixture_ptr(const Fixture *fixture,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(fixture, field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(id, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game(LiveGame live_game,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    struct len_userdata other_userdata;

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&live_game), field, serialize_func, fields, write_func, userdata)

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    if (live_game.fix)
      SERIALIZE(fix, bygfoot_json_serialize_fixture_ptr);
    SERIALIZE(fix_id, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("team_names", serialize_gchar_array(live_game.team_names, fields, write_func, userdata, 2), fields);
    STREAM_OBJ_FIELD_CUSTOM("subs_left", serialize_int_array(live_game.subs_left, fields, write_func, userdata, 2), fields);
    SERIALIZE(attendance, serialize_int);
    SERIALIZE(started_game, serialize_int);
    SERIALIZE(stadium_event, serialize_int);
    SERIALIZE(team_values, bygfoot_json_serialize_live_game_team_values);
    SERIALIZE(home_advantage, serialize_double);
    SERIALIZE(units, bygfoot_json_serialize_live_game_units);
    SERIALIZE(stats, bygfoot_json_serialize_live_game_stats);
    SERIALIZE(team_state, bygfoot_json_serialize_live_game_team_state_array);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(action_ids, bygfoot_json_serialize_live_game_action_ids);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game_team_values(const gfloat (*team_values)[4],
                                             const json_object *fields,
                                             void (*write_func)(const char*, gpointer),
                                             gpointer userdata)
{
    gint i;
    struct len_userdata other_userdata;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        const float *values = team_values[i];
        if (i) {
            write_func(",", userdata);
        }
        serialize_float_array(values, fields, write_func, userdata, GAME_TEAM_VALUE_END);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_live_game_units(const GArray *units,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < units->len; i++) {
        const LiveGameUnit *unit = &g_array_index(units, LiveGameUnit, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_live_game_unit(unit, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_live_game_unit(const LiveGameUnit *unit,
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    struct len_userdata result_userdata;

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(unit, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(possession, serialize_int);
    SERIALIZE(area, serialize_int);
    SERIALIZE(minute, serialize_int);
    SERIALIZE(time, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("result", serialize_int_array(unit->result, fields, write_func, userdata, 2), fields);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(event, bygfoot_json_serialize_live_game_event);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game_event(LiveGameEvent event,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&event), field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(type, serialize_int);
    SERIALIZE(verbosity, serialize_int);
    SERIALIZE(team, serialize_int);
    if (event.player) {
        SERIALIZE(player, bygfoot_json_serialize_player_ptr);
    }
    if (event.player2) {
        SERIALIZE(player2, bygfoot_json_serialize_player_ptr);
    }
    SERIALIZE(commentary, serialize_string);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(commentary_id, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game_stats(LiveGameStats stats,
                                       const json_object *fields,
                                       void (*write_func)(const char*, gpointer),
                                       gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD((&stats), field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(possession, serialize_double);
    SERIALIZE(values, bygfoot_json_serialize_live_game_stats_values);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(players, bygfoot_json_serialize_live_game_stats_players);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game_stats_values(gint (*values)[9],
                                              const json_object *fields,
                                              void (*write_func)(const char*, gpointer),
                                              gpointer userdata)
{
    static const struct key_index {
        const gchar *key;
        gint index;
    } value_fields[] = {
        { "goals_regular", LIVE_GAME_STAT_VALUE_GOALS_REGULAR },
        { "shots", LIVE_GAME_STAT_VALUE_SHOTS },
        { "shot_percentage", LIVE_GAME_STAT_VALUE_SHOT_PERCENTAGE },
        { "possession", LIVE_GAME_STAT_VALUE_POSSESSION },
        { "penalties", LIVE_GAME_STAT_VALUE_PENALTIES },
        { "fouls", LIVE_GAME_STAT_VALUE_FOULS },
        { "cards", LIVE_GAME_STAT_VALUE_CARDS },
        { "reds", LIVE_GAME_STAT_VALUE_REDS } ,
        { "injuries", LIVE_GAME_STAT_VALUE_INJURIES },
        { NULL, LIVE_GAME_STAT_VALUE_END }
    };

    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        const struct key_index *iter;
        if (i) {
            write_func(",", userdata);
        }
        SERIALIZE_BEGIN_OBJECT(write_func, userdata);
        for (iter = value_fields; iter->key; iter++) {
            STREAM_OBJ_FIELD_CUSTOM(iter->key, serialize_int(values[i][iter->index], fields, write_func, userdata), fields);
        }
        SERIALIZE_END_OBJECT(write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_live_game_stats_players(GPtrArray* (*players)[5],
                                               const json_object *fields,
                                               void (*write_func)(const char*, gpointer),
                                               gpointer userdata)
{

    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i ++) {
        gint j;
        if (i) {
            write_func(",", userdata);
        }
        SERIALIZE_BEGIN_ARRAY(write_func, userdata);
        for (j = 0; j < LIVE_GAME_STAT_ARRAY_END; j++) {
            GPtrArray *player_stats = players[i][j];
            if (j) {
                write_func(",", userdata);
            }
            serialize_gchar_ptr_array(player_stats, fields, write_func, userdata);
        }
        SERIALIZE_END_ARRAY(write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_live_game_team_state_array(LiveGameTeamState *team_states,
                                                  const json_object *fields,
                                                  void (*write_func)(const char*, gpointer),
                                                  gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        const LiveGameTeamState *team_state = team_states + i;
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_live_game_team_state(team_state, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_live_game_team_state(const LiveGameTeamState *team_state,
                                            const json_object *fields,
                                            void (*write_func)(const char*, gpointer),
                                            gpointer userdata)
{
    struct len_userdata player_ids_userdata;

    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(team_state, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(structure, serialize_int);
    SERIALIZE(style, serialize_int);
    SERIALIZE(boost, serialize_boolean);
    SERIALIZE_OBJ_LAST_FIELD;
    STREAM_OBJ_FIELD_CUSTOM("player_ids", serialize_int_array(team_state->player_ids, fields, write_func, userdata, 11), fields);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_live_game_action_ids(GArray **action_ids,
                                            const json_object *fields,
                                            void (*write_func)(const char*, gpointer),
                                            gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        if (i) {
            write_func(",", userdata);
        }
        serialize_int_garray(action_ids[i], fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}
    
void
bygfoot_json_serialize_season_stats(const GArray *stats,
                                    const json_object *fields,
                                    void (*write_func)(const char*, gpointer),
                                    gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < stats->len; i++) {
        SeasonStat *stat = &g_array_index(stats, SeasonStat, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_season_stat(stat, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_season_stat(const SeasonStat *stat,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(stat, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(season_number, serialize_int);
    SERIALIZE(league_champs, bygfoot_json_serialize_champ_stats);
    SERIALIZE(cup_champs, bygfoot_json_serialize_champ_stats);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(league_stats, bygfoot_json_serialize_league_stats);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_champ_stats(const GArray *stats,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0 ; i < stats->len; i++) {
        const ChampStat *stat = &g_array_index(stats, ChampStat, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_champ_stat(stat, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_champ_stat(const ChampStat *stat,
                                  const json_object *fields,
                                  void (*write_func)(const char*, gpointer),
                                  gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(stat, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(team_name, serialize_string);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(cl_name, serialize_string);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}
    
void
bygfoot_json_serialize_league_stats(const GArray *stats,
                                    const json_object *fields,
                                    void (*write_func)(const char*, gpointer),
                                    gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < stats->len; i++) {
        const LeagueStat *stat = &g_array_index(stats, LeagueStat, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_league_stat(stat, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_league_stat(const LeagueStat *stat,
                                   const json_object *fields,
                                   void (*write_func)(const char*, gpointer),
                                   gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(stat, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(league_symbol, serialize_string);
    SERIALIZE(league_name, serialize_string);
    SERIALIZE(teams_off, bygfoot_json_serialize_stats);
    SERIALIZE(teams_def, bygfoot_json_serialize_stats);
    SERIALIZE(player_scorers, bygfoot_json_serialize_stats);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(player_goalies, bygfoot_json_serialize_stats);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_stats(const GArray *stats,
                             const json_object *fields,
                             void (*write_func)(const char*, gpointer),
                             gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < stats->len; i++) {
        const Stat *stat = &g_array_index(stats, Stat, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_stat(stat, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_stat(const Stat *stat,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(stat, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(team_name, serialize_string);
    SERIALIZE(value1, serialize_int);
    SERIALIZE(value2, serialize_int);
    SERIALIZE(value3, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(value_string, serialize_string);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}
    
void
bygfoot_json_serialize_jobs(const GArray *jobs,
                            const json_object *fields,
                            void (*write_func)(const char*, gpointer),
                            gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < jobs->len; i++) {
        const Job *job = &g_array_index(jobs, Job, i);
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_job(job, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_job(const Job *job,
                           const json_object *fields,
                           void (*write_func)(const char*, gpointer),
                           gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(job, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(type, serialize_int);
    SERIALIZE(time, serialize_int);
    SERIALIZE(country_file, serialize_string);
    SERIALIZE(country_name, serialize_string);
    SERIALIZE(league_name, serialize_string);
    SERIALIZE(league_layer, serialize_int);
    SERIALIZE(country_rating, serialize_int);
    SERIALIZE(talent_percent, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(team_id, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_tables,
                          Table,
                          bygfoot_json_serialize_table);

void
bygfoot_json_serialize_table(const Table *table,
                             const json_object *fields,
                             void (*write_func)(const char*, gpointer),
                             gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(table, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(name, serialize_string);
    SERIALIZE(round, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(elements, bygfoot_json_serialize_table_elements);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_table_elements,
                          TableElement,
                          bygfoot_json_serialize_table_element);

void
bygfoot_json_serialize_table_element(const TableElement *element,
                                     const json_object *fields,
                                     void (*write_func)(const char*, gpointer),
                                     gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(element, field, serialize_func, fields, write_func, userdata);
    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(team, bygfoot_json_serialize_team_ptr);
    SERIALIZE(old_rank, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("played", serialize_int(element->values[TABLE_PLAYED], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("won", serialize_int(element->values[TABLE_WON], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("draw", serialize_int(element->values[TABLE_DRAW], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("lost", serialize_int(element->values[TABLE_LOST], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("gf", serialize_int(element->values[TABLE_GF], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("ga", serialize_int(element->values[TABLE_GA], fields, write_func, userdata), fields);
    STREAM_OBJ_FIELD_CUSTOM("gd", serialize_int(element->values[TABLE_GD], fields, write_func, userdata), fields);
    SERIALIZE_OBJ_LAST_FIELD;
    STREAM_OBJ_FIELD_CUSTOM("pts", serialize_int(element->values[TABLE_PTS], fields, write_func, userdata), fields);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_fixtures,
                          Fixture,
                          bygfoot_json_serialize_fixture);

void
bygfoot_json_serialize_fixture(const Fixture *fixture,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(fixture, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(id, serialize_int);
    SERIALIZE(round, serialize_int);
    SERIALIZE(replay_number, serialize_int);
    SERIALIZE(week_number, serialize_int);
    SERIALIZE(week_round_number, serialize_int);
    SERIALIZE(teams, bygfoot_json_serialize_fixture_teams);
    SERIALIZE(result, bygfoot_json_serialize_fixture_result);
    SERIALIZE(home_advantage, serialize_boolean);
    SERIALIZE(second_leg, serialize_boolean);
    SERIALIZE(decisive, serialize_boolean);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(attendance, serialize_int);
    /* live_game not serialized, because it is temporary */
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

void
bygfoot_json_serialize_fixture_teams(Team * const *teams,
                                     const json_object *fields,
                                     void (*write_func)(const char*, gpointer),
                                     gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        const Team *team = teams[i];
        if (i) {
            write_func(",", userdata);
        }
        bygfoot_json_serialize_team_ptr(team, fields, write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_fixture_result(const gint (*result)[3],
                                      const json_object *fields,
                                      void (*write_func)(const char*, gpointer),
                                      gpointer userdata)
{
    gint i;

    SERIALIZE_BEGIN_ARRAY(write_func, userdata);
    for (i = 0; i < 2; i++) {
        static const struct key_index {
            const gchar *key;
            gint index;
        } result_fields[] = {
            { "goals_regulation", 0 },
            { "goals_extra_time", 1 },
            { "goals_penalty_shootout", 2 },
            { NULL, 0 }
        };
        const struct key_index *iter;
        if (i) {
            write_func(",", userdata);
        }
        SERIALIZE_BEGIN_OBJECT(write_func, userdata);
        for (iter = result_fields; iter->key; iter++) {
            gint value = result[i][iter->index];
            STREAM_OBJ_FIELD_CUSTOM(iter->key, serialize_int(value, fields, write_func, userdata), fields);
            if (!(iter + 1)->key) {
                SERIALIZE_OBJ_LAST_FIELD;
            }
        }
        SERIALIZE_END_OBJECT(write_func, userdata);
    }
    SERIALIZE_END_ARRAY(write_func, userdata);
}

void
bygfoot_json_serialize_newspaper(const NewsPaper *newspaper,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(newspaper, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(articles, bygfoot_json_serialize_articles);
    SERIALIZE_OBJ_LAST_FIELD;
    /* live_game not serialized, because it is temporary */
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}

SERIALIZE_GARRAY_FUNC_DEF(bygfoot_json_serialize_articles,
                          NewsPaperArticle,
                          bygfoot_json_serialize_newspaper_article);

void
bygfoot_json_serialize_newspaper_article(const NewsPaperArticle *article,
                                         const json_object *fields,
                                         void (*write_func)(const char*, gpointer),
                                         gpointer userdata)
{
    #define SERIALIZE(field, serialize_func) \
            STREAM_OBJ_FIELD(article, field, serialize_func, fields, write_func, userdata);

    SERIALIZE_BEGIN_OBJECT(write_func, userdata);
    SERIALIZE(week_number, serialize_int);
    SERIALIZE(week_round_number, serialize_int);
    SERIALIZE(title_id, serialize_int);
    SERIALIZE(subtitle_id, serialize_int);
    SERIALIZE(cup_round, serialize_int);
    STREAM_OBJ_FIELD_CUSTOM("competition", serialize_int(article->competition->id, fields, write_func, userdata), fields);
    SERIALIZE(title_small, serialize_string);
    SERIALIZE(title, serialize_string);
    SERIALIZE(subtitle, serialize_string);
    SERIALIZE(id, serialize_int);
    SERIALIZE_OBJ_LAST_FIELD;
    SERIALIZE(user_idx, serialize_int);
    SERIALIZE_END_OBJECT(write_func, userdata);
    #undef SERIALIZE
}
