#ifndef JSON_DESERIALIZE_H
#define JSON_DESERIALIZE_H

#include <json-c/json_object.h>

#include "bet_struct.h"
#include "cup_struct.h"
#include "league_struct.h"
#include "transfer_struct.h"

GPtrArray *
bygfoot_json_deserialize_country_list(Bygfoot *bygfoot,
                                      const gchar *country_list_json);

gboolean
bygfoot_json_deserialize_transfer_offer(Bygfoot *bygfoot,
                                        json_object *offer_obj,
                                        TransferOffer **offer_ptr);
gboolean
bygfoot_json_deserialize_transfer(Bygfoot *bygfoot, json_object *transfer_obj,
                                  Transfer **transfer_ptr);
gboolean
bygfoot_json_deserialize_bets(Bygfoot *bygfoot, json_object *bet_obj,
                              GArray **bets);
gboolean
bygfoot_json_deserialize_bet_match(Bygfoot *bygfoot, json_object *bet_match_obj,
                                   BetMatch **bet_match_ptr);
gboolean
bygfoot_json_deserialize_job(Bygfoot *bygfoot, json_object *job_obj, Job *job);
gboolean
bygfoot_json_deserialize_champ_stats(Bygfoot *bygfoot, json_object *stats_obj,
                                     GArray **stats);
gboolean
bygfoot_json_deserialize_champ_stat(Bygfoot *bygfoot, json_object *stat_obj,
                                    ChampStat **stat_ptr);
gboolean
bygfoot_json_deserialize_season_stat(Bygfoot *bygfoot, json_object *stat_obj,
                                     SeasonStat *stat);
gboolean
bygfoot_json_deserialize_bygfoot(const gchar *json, Bygfoot *bygfoot);

gboolean
bygfoot_json_deserialize_countries(Bygfoot *bygfoot, json_object *countries_obj,
                                   GPtrArray **countries);

gboolean
bygfoot_json_deserialize_country_string(Bygfoot *bygfoot,
                                        const gchar *country_json,
                                        Country **country);
gboolean
bygfoot_json_deserialize_allcups(Bygfoot *bygfoot, json_object *allcups_obj,
                                 GPtrArray **allcups);
gboolean
bygfoot_json_deserialize_cup_ptr(Bygfoot *bygfoot, json_object *cup_obj,
                                 Cup **cup_ptr);

gboolean
bygfoot_json_deserialize_country(Bygfoot *bygfoot, json_object *country_obj,
                                 Country **country);

gboolean
bygfoot_json_deserialize_users(Bygfoot *bygfoot, json_object *users_obj,
                               GArray **users_ptr);
gboolean
bygfoot_json_deserialize_user_histories(Bygfoot *bygfoot,
                                        json_object *histories_obj,
                                        GArray **histories_ptr);
gboolean
bygfoot_json_deserialize_user_history(Bygfoot *bygfoot,
                                      json_object *history_obj,
                                      UserHistory **history_ptr);
gboolean
bygfoot_json_deserialize_live_game_event(Bygfoot *bygfoot,
                                         json_object *event_obj,
                                         LiveGameEvent *event);
gboolean
bygfoot_json_deserialize_game_units(Bygfoot *bygfoot, json_object *units_obj,
                                    GArray **units);
gboolean
bygfoot_json_deserialize_game_unit(Bygfoot *bygfoot, json_object *unit_obj,
                                   LiveGameUnit **unit_ptr);
gboolean
bygfoot_json_deserialize_live_game(Bygfoot *bygfoot, json_object *live_game_obj,
                                   LiveGame *live_game_ptr);
gboolean
bygfoot_json_deserialize_user_sponsor(Bygfoot *bygfoot,
                                      json_object *sponsor_obj,
                                      UserSponsor *sponsor);
gboolean
bygfoot_json_deserialize_bet_user(Bygfoot *bygfoot, json_object *bet_obj,
                                  BetUser **bet_user_ptr);
gboolean
bygfoot_json_deserialize_user(Bygfoot *bygfoot, json_object *user_obj,
                              User **user_ptr);
gboolean
bygfoot_json_deserialize_prom_games(Bygfoot *bygfoot,
                                    json_object *prom_games_obj,
                                    PromGames **prom_games);
gboolean
bygfoot_json_deserialize_prom_rel_prom_games(Bygfoot *bygfoot,
                                             json_object *prom_games_obj,
                                             GArray **prom_games_ptr);
gboolean
bygfoot_json_deserialize_prom_rel_elements(Bygfoot *bygfoot,
                                           json_object *elements_obj,
                                           GArray **elments_ptr);
gboolean
bygfoot_json_deserialize_prom_rel(Bygfoot *bygfoot, json_object *prom_rel_obj,
                                  PromRel *prom_rel);
gboolean
bygfoot_json_deserialize_leagues(Bygfoot *bygfoot, json_object *leagues_obj,
                                 GPtrArray **leagues);

gboolean
bygfoot_json_deserialize_joined_leagues(Bygfoot *bygfoot,
                                        json_object *leagues_obj,
                                        GArray **leagues_ptr);
gboolean
bygfoot_json_deserialize_joined_league(Bygfoot *bygfoot,
                                       json_object *league_obj,
                                       JoinedLeague **league_ptr);
gboolean
bygfoot_json_deserialize_new_tables(Bygfoot *bygfoot, json_object *tables_obj,
                                    GArray **tables_ptr);
gboolean
bygfoot_json_deserialize_new_table(Bygfoot *bygfoot, json_object *table_obj,
                                   NewTable **table_ptr);
gboolean
bygfoot_json_deserialize_stats(Bygfoot *bygfoot, json_object *stats_obj,
                               GArray **stats_ptr);
gboolean
bygfoot_json_deserialize_stat(Bygfoot *bygfoot, json_object *stat_obj,
                              Stat **stat_ptr);
gboolean
bygfoot_json_deserialize_league_stat(Bygfoot *bygfoot, json_object *stats_obj,
                                     LeagueStat **stat_ptr);
gboolean
bygfoot_json_deserialize_league(Bygfoot *bygfoot, json_object *league_obj,
                                League **league_ptr);

gboolean
bygfoot_json_deserialize_fixtures(Bygfoot *bygfoot, json_object *fixtures_obj,
                                  GArray **fixtures_ptr);

gboolean
bygfoot_json_deserialize_fixture_ptr(Bygfoot *bygfoot, json_object *fixture_obj,
                                     Fixture **fixture_ptr);
gboolean
bygfoot_json_deserialize_fixture(Bygfoot *bygfoot, json_object *fixture_obj,
                                 Fixture **fixture_ptr);

gboolean
bygfoot_deserialize_fixture_teams(Bygfoot *bygfoot, json_object *teams_obj,
                                  Team *(*teams)[2]);
gboolean
bygfoot_json_deserialize_tables(Bygfoot *bygfoot, json_object *tables_obj,
                                GArray **tables_ptr);

gboolean
bygfoot_json_deserialize_table(Bygfoot *bygfoot, json_object *table_obj,
                               Table **table_ptr);

gboolean
bygfoot_json_deserialize_table_elements(Bygfoot *bygfoot,
                                        json_object *elements_obj,
                                        GArray **elements_ptr);

gboolean
bygfoot_json_deserialize_table_element(Bygfoot *bygfoot,
                                       json_object *element_obj,
                                       TableElement **element_ptr);

gboolean
bygfoot_json_deserialize_teams(Bygfoot *bygfoot, json_object *teams_obj,
                               GPtrArray **teams_ptr);

gboolean
bygfoot_json_deserialize_team(Bygfoot *bygfoot, json_object *team_obj,
                              Team **team_ptr);

gboolean
bygfoot_json_deserialize_team_ptrs(Bygfoot *bygfoot, json_object *teams_obj,
                                   GPtrArray **teams_ptr);

gboolean
bygfoot_json_deserialize_team_ptr(Bygfoot *bygfoot, json_object *team_obj,
                                  Team **team_ptr);

gboolean
bygfoot_json_deserialize_cups(Bygfoot *bygfoot, json_object *cups_obj,
                              GPtrArray **cups_ptr);

gboolean
bygfoot_json_deserialize_choose_teams(Bygfoot *bygfoot, json_object *choose_obj,
                                      GArray **choose_teams);
gboolean
bygfoot_json_deserialize_choose_team(Bygfoot *bygfoot,
                                     json_object *choose_list_obj,
                                     CupChooseTeam **choose_team);
gboolean
bygfoot_json_deserialize_waits(Bygfoot *bygfoot, json_object *waits_obj,
                               GArray **waits);
gboolean
bygfoot_json_deserialize_cup_round_wait(Bygfoot *bygfoot, json_object *wait_obj,
                                        CupRoundWait **wait_ptr);
gboolean
bygfoot_json_deserialize_week_breaks(Bygfoot *bygfoot,
                                     json_object *week_breaks_obj,
                                     GArray **week_breaks);
gboolean
bygfoot_json_deserialize_week_break(Bygfoot *bygfoot,
                                    json_object *week_break_obj,
                                    WeekBreak **week_break);
gboolean
bygfoot_json_deserialize_cup_rounds(Bygfoot *bygfoot, json_object *rounds_obj,
                                    GArray **rounds);
gboolean
bygfoot_json_deserialize_cup_round(Bygfoot *bygfoot, json_object *round_obj,
                                   CupRound **round);
gboolean
bygfoot_json_deserialize_cup(Bygfoot *bygfoot, json_object *cup_obj,
                             Cup **cup_ptr);

gboolean
bygfoot_json_deserialize_players(Bygfoot *bygfoot, json_object *players_obj,
                                 GPtrArray **players_ptr);

gboolean
bygfoot_json_deserialize_player(Bygfoot *bygfoot, json_object *player_obj,
                                Player **player_ptr);

gboolean
bygfoot_json_deserialize_player_competition_stats_array(Bygfoot *bygfoot,
                                                        json_object *array_obj,
                                                        GArray **array_ptr);
gboolean
bygfoot_json_deserialize_player_competition_stats_list(Bygfoot *bygfoot,
                                                       json_object *list_obj,
                                                       GList **list);

gboolean
bygfoot_json_deserialize_player_competition_stats(
    Bygfoot *bygfoot, json_object *stats_obj, PlayerCompetitionStats **stats);
gboolean
bygfoot_json_deserialize_newspaper_article(Bygfoot *bygfoot,
                                           json_object *article_obj,
                                           NewsPaperArticle *article);

#endif
