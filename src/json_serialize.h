#ifndef JSON_SERIALIZE_H
#define JSON_SERIALIZE_H

#include <json-c/json_object.h>
#include "bygfoot.h"
#include "bet_struct.h"
#include "cup_struct.h"
#include "league_struct.h"
#include "job_struct.h"
#include "news_struct.h"
#include "transfer_struct.h"
#include "json_compat.h"

struct bygfoot_json_serialize_buffer {
    char *data;
    gint len;
    gint alloc_size;
};

void
bygfoot_json_serialize_write_stdout(const char *s, gpointer userdata);

void
bygfoot_json_serialize_write_buffer(const char *s, gpointer userdata);

void
bygfoot_json_serialize_write_file(const char *s, gpointer userdata);

void
bygfoot_json_serialize_bygfoot(const Bygfoot *bygfoot,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_country_list(GPtrArray *country_list,
                                    const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_countries(const GPtrArray *countries,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_country(const Country *country,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/** @name League */
/* @{ */
void
bygfoot_json_serialize_leagues(const GPtrArray *leagues,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_league(const League *league,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_prom_rel(PromRel prom_rel,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_prom_rel_elements(const GArray *garry,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_prom_rel_element(const PromRelElement *element,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_prom_games_array(const GArray *garry,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_prom_games(const PromGames *prom_games,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_two_match_weeks(GArray * const *two_match_weeks,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_joined_leagues(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_joined_league(const JoinedLeague *league,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_new_tables(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_new_table(const NewTable *table,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_week_breaks(const GArray *garry,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_week_break(const WeekBreak *week_break,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name User */
/* @{ */
void
bygfoot_json_serialize_users(const GArray *users,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user(const User *user,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_bets(GArray * const *bets,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_sponsor(UserSponsor sponsor,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_histories(const GArray *histories,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_history(const UserHistory *history,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_money_in(const gint (*money_in)[5],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_user_money_out(const gint (*money_out)[13],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Bet */
/* @{ */
void
bygfoot_json_serialize_bet_user(const BetUser *bet_user,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_bets(GArray **bets,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_bet_match(const BetMatch *bet_match,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

/* @} */

/** @name Team */
/* @{ */
void
bygfoot_json_serialize_teams(const GPtrArray *teams,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_team_ptrs(GPtrArray *team_ptrs,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
void
bygfoot_json_serialize_team_ptr(const Team *team,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_team(const Team *team,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_youth_academy(YouthAcademy youth_academy,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */


/** @name Player */
/* @{ */
void
bygfoot_json_serialize_players(const GPtrArray *player,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_player(const Player *player,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_player_ptr(const Player *player,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
void
bygfoot_json_serialize_player_competition_stats_list(GList *stats_list,
                                         const json_object *fields,
                                         void (*write_func)(const char*, gpointer),
                                         gpointer userdata);

void
bygfoot_json_serialize_player_competition_stats_array(const GArray *stats_array,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_player_competition_stats(const PlayerCompetitionStats *games_goals,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

/* @} */

/** @name Cup */
/* @{ */
void
bygfoot_json_serialize_cups(const GPtrArray *garry,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup(const Cup *cup,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_ptrs(GPtrArray *cups,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_ptr(const Cup *cup,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_choose_teams(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_choose_team(const CupChooseTeam *choose_team,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_round_waits(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_round_wait(const CupRoundWait *wait,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_cup_history(const GPtrArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

/* @} */

/** @name Transfers */
/* @{ */

void
bygfoot_json_serialize_transfers(const GArray *transfers,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_transfer(const Transfer *transfer,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_transfer_offers(const GArray *offers,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_transfer_offer(const TransferOffer *offer,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Fixtures */
/* @{ */
void
bygfoot_json_serialize_fixture_ptr(const Fixture *fixture,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Live Game */
/* @{ */
void
bygfoot_json_serialize_live_game(LiveGame live_game,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_team_values(const gfloat (*team_values)[4],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_units(const GArray *units,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_unit(const LiveGameUnit *unit,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_event(LiveGameEvent event,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_stats(LiveGameStats stats,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_stats_values(gint (*values)[9],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_stats_players(GPtrArray * (*players)[5],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_team_state_array(LiveGameTeamState *team_states,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_team_state(const LiveGameTeamState *team_state,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_live_game_action_ids(GArray **action_ids,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Stats */
/* @{ */
void
bygfoot_json_serialize_season_stats(const GArray *stats,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_season_stat(const SeasonStat *stat,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_champ_stats(const GArray *stats,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_champ_stat(const ChampStat *stat,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_league_stats(const GArray *stats,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_league_stat(const LeagueStat *stat,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_stats(const GArray *stats,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_stat(const Stat *stat,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Job */
/* @{ */
void
bygfoot_json_serialize_jobs(const GArray *jobs,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_job(const Job *job,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Table */
/* @{ */
void
bygfoot_json_serialize_tables(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_table(const Table *table,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_table_elements(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_table_element(const TableElement *element,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name Fixture */
/* @{ */
void
bygfoot_json_serialize_fixtures(const GArray *garray,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_fixture(const Fixture *fixture,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_fixture_teams(Team * const *teams,
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);

void
bygfoot_json_serialize_fixture_result(const gint (*result)[3],
                               const json_object *fields,
                               void (*write_func)(const char*, gpointer),
                               gpointer userdata);
/* @} */

/** @name newspaper */
/* @{ */

void
bygfoot_json_serialize_newspaper(const NewsPaper *newspaper,
                                 const json_object *fields,
                                 void (*write_func)(const char*, gpointer),
                                 gpointer userdata);
void
bygfoot_json_serialize_articles(const GArray *garry,
                                const json_object *fields,
                                void (*write_func)(const char*, gpointer),
                                gpointer userdata);
void
bygfoot_json_serialize_newspaper_article(const NewsPaperArticle *article,
                                         const json_object *fields,
                                         void (*write_func)(const char*, gpointer),
                                         gpointer userdata);
/* @} */
#endif
