/*
   strategy_struct.h

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

#ifndef STRATEGY_STRUCT_H
#define STRATEGY_STRUCT_H

/** Lineup types for a CPU team (ie. which players
    are preferred when putting together the first 11). */
enum StratLineupType
{
    STRAT_LINEUP_BEST = 1,
    STRAT_LINEUP_WEAKEST,
    STRAT_LINEUP_FITTEST,
    STRAT_LINEUP_UNFITTEST,
    STRAT_LINEUP_END
};

/** Tokens that represent parts of a strategy condition. */
enum StratCondToken {
    STRAT_COND_NONE,
    STRAT_COND_EQ,
    STRAT_COND_NE,
    STRAT_COND_GT,
    STRAT_COND_GE,
    STRAT_COND_LT,
    STRAT_COND_LE,
    STRAT_COND_INT,
    STRAT_COND_VAR,
    STRAT_COND_AND,
    STRAT_COND_OR,
    STRAT_COND_OPEN_PAREN,
    STRAT_COND_CLOSE_PAREN,
    STRAT_CON_LAST
};

enum ReplacementToken {
    /* General */
    REP_TOKEN_LEAGUE_CUP_NAME, /* _LEAGUECUPNAME_ # ALSO FOR NEWS */
    REP_TOKEN_CUP_ROUND_NAME, /* _CUPROUNDNAME_ # ALSO FOR NEWS */
    REP_TOKEN_ATTENDANCE, /* 	_AT_ # ALSO FOR NEWS */
    REP_TOKEN_RESULT, /* 	_RE_ # ALSO FOR NEWS */
    REP_TOKEN_TIME, /* 	_TIME_ */
    REP_TOKEN_MINUTE, /* 	_MI_ */
    REP_TOKEN_MINUTE_REMAINING, /* _MR_ */
    REP_TOKEN_MINUTE_TOTAL, /* _MT_ */
    REP_TOKEN_YELLOW_LIMIT, /* _YELLOWLIMIT_ */
    REP_TOKEN_GOALS0, /* 	_GOALS0_ # ALSO FOR NEWS */
    REP_TOKEN_GOALS1, /* 	_GOALS1_ # ALSO FOR NEWS */
    REP_TOKEN_GOAL_DIFF, /* 	_GD_ # ALSO FOR NEWS */
    REP_TOKEN_POSSESSION, /* 	_POSS_ */
    REP_TOKEN_NO_POSSESSION, /* _NPOSS_ */
    /* Misc */
    REP_TOKEN_EXTRA, /* 	_EX_ */
    /* Team */
    REP_TOKEN_TEAM_HOME, /* 	_T0_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_AWAY, /* 	_T1_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_LOSING, /* _TL_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_WINNING, /* _TW_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_WINNINGN, /* _TWN_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_LOSINGN, /* _TLN_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM, /* 	_TT_ */
    REP_TOKEN_TEAM_LAYER0, /* _TLAYER0_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_LAYER1, /* _TLAYER1_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_LAYERDIFF, /* _TLAYERDIFF_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_AVSKILL0, /* _TAVSKILL0_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_AVSKILL1, /* _TAVSKILL1_ # ALSO FOR NEWS */
    REP_TOKEN_TEAM_AVSKILLDIFF, /* _TAVSKILLDIFF_ # ALSO FOR NEWS */
    /* Player */
    REP_TOKEN_PLAYER0, /* 	_P0_ */
    REP_TOKEN_PLAYER1, /* 	_P1_ */
    REP_TOKEN_PLAYER_GOALS0, /* _PLGOALS0_ */
    REP_TOKEN_PLAYER_GOALS_ALL0, /* _PLGOALSALL0_ */
    REP_TOKEN_PLAYER_GOALS1, /* _PLGOALS1_ */
    REP_TOKEN_PLAYER_GOALS_ALL1, /* _PLGOALSALL1_ */
    REP_TOKEN_PLAYER_YELLOWS, /* _PLYELLOWS_ */
    /* STAT TOKENS */
    REP_TOKEN_STAT_SHOTS0, /* _SHOTS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_SHOTS1, /* _SHOTS1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_SHOT_PER0, /* _SHOTPER0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_SHOT_PER1, /* _SHOTPER1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_POSS0, /* 	_POSS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_POSS1, /* 	_POSS1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_PEN0, /* 	_PEN0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_PEN1, /* 	_PEN1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_FOULS0, /* _FOULS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_FOULS1, /* _FOULS1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_YELLOWS0, /* _YELLOWS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_YELLOWS1, /* _YELLOWS1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_REDS0, /* 	_REDS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_REDS1, /* 	_REDS1_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_INJS0, /* 	_INJS0_ # ALSO FOR NEWS */
    REP_TOKEN_STAT_INJS1, /* 	_INJS1_ # ALSO FOR NEWS */
    /* PREMATCH */
    REP_TOKEN_OPPONENT_SKILL, /* _OPPSKILL_ */
    REP_TOKEN_AVSKILLDIFF, /* _AVSKILLDIFF_ */
    REP_TOKEN_HOMEADV, /* 	_HOMEADV_ */
    REP_TOKEN_CUP, /* 	_CUP_ */
    REP_TOKEN_GOALS_TO_WIN, /* _GOALSTOWIN_ */
    /* MATCH */
    REP_TOKEN_SUBS_LEFT, /* 	_SUBSLEFT_ */
    REP_TOKEN_NUM_DEF, /* 	_NUMDEF_ */
    REP_TOKEN_NUM_MID, /* 	_NUMMID_ */
    REP_TOKEN_NUM_ATT, /* 	_NUMATT_ */
    REP_TOKEN_FORM, /* 	_FORMATION_ */

    /* News Articles */
    REP_TOKEN_RESULT_REW, /* 		_REW_ */
    REP_TOKEN_RESULT_REL, /* 		_REL_ */
    REP_TOKEN_BOOL_CUP, /*                   _CUP_ */
    REP_TOKEN_BOOL_CUP_KNOCKOUT, /*          _CUPKO_ */
    REP_TOKEN_BOOL_CUP_HOME_AWAY, /*         _CUPHOMEAWAY_ */
    REP_TOKEN_CUP_STAGE, /*                  _CUPSTAGE_ */
    REP_TOKEN_BOOL_CUP_NEUTRAL, /*           _CUPNEUTRAL_ */
    REP_TOKEN_BOOL_CUP_NATIONAL, /*          _CUPNATIONAL_ */
    REP_TOKEN_BOOL_CUP_INTERNATIONAL, /*     _CUPINTERNATIONAL_ */
    REP_TOKEN_BOOL_CUP_PROMREL, /*           _CUPPROMREL_ */
    REP_TOKEN_BOOL_CUP_AUX, /*               _CUPAUX_ */
    REP_TOKEN_BOOL_CUP_PENALTIES, /*         _CUPPEN_ */
    REP_TOKEN_BOOL_CUP_EXTRA, /*             _CUPET_ */
    REP_TOKEN_BOOL_CUP_FIRST_LEG, /*         _CUPFIRSTLEG_ */
    REP_TOKEN_BOOL_CUP_SECOND_LEG, /*        _CUPSECONDLEG_ */
    REP_TOKEN_CUP_MATCH_WINNER, /*           _CUPMATCHWINNER_ */
    REP_TOKEN_CUP_MATCH_LOSER, /*            _CUPMATCHLOSER_ */
    REP_TOKEN_CUP_MATCH_WINNERN, /*          _CUPMATCHWINNERN_ */
    REP_TOKEN_CUP_MATCH_LOSERN, /*           _CUPMATCHLOSERN_ */
    REP_TOKEN_BOOL_MULTIPLE_SCORERS0, /*     _MULTIPLESCORERS0_ */
    REP_TOKEN_BOOL_MULTIPLE_SCORERS1, /*     _MULTIPLESCORERS1_ */
    REP_TOKEN_SCORERS0, /*                   _SCORERS0_ */
    REP_TOKEN_HIGHSCORER0, /*                _HIGHSCORER0_ */
    REP_TOKEN_HIGHSCORER_GOALS0, /*         _HIGHGOALS0_ */
    REP_TOKEN_SCORERS1, /*                   _SCORERS1_ */
    REP_TOKEN_HIGHSCORER1, /*                _HIGHSCORER1_ */
    REP_TOKEN_HIGHSCORER_GOALS1, /*          _HIGHGOALS1_ */
    REP_TOKEN_GOAL_DIFF_AGGREGATE, /* _GDAGG_ */
    REP_TOKEN_RANK0, /*                      _RANK0_ */
    REP_TOKEN_OLDRANK0, /*                   _OLDRANK0_ */
    REP_TOKEN_RANK1, /*                      _RANK1_ */
    REP_TOKEN_OLDRANK1, /*                   _OLDRANK1_ */
    REP_TOKEN_STREAK_WON0, /* 	_WON0_ */
    REP_TOKEN_STREAK_LOST0, /* 	_LOST0_ */
    REP_TOKEN_STREAK_WON1, /* 	_WON1_ */
    REP_TOKEN_STREAK_LOST1, /* 	_LOST1_ */
    REP_TOKEN_STREAK_LEAGUE_WON0, /* 	_LEAGUEWON0_ */
    REP_TOKEN_STREAK_LEAGUE_LOST0, /* _LEAGUELOST0_ */
    REP_TOKEN_STREAK_LEAGUE_WON1, /* 	_LEAGUEWON1_ */
    REP_TOKEN_STREAK_LEAGUE_LOST1, /* _LEAGUELOST1_ */
    REP_TOKEN_STREAK_UNBEATEN0, /* 	_UNBEATEN0_ */
    REP_TOKEN_STREAK_UNBEATEN1, /* 	_UNBEATEN1_ */
    REP_TOKEN_STREAK_LEAGUE_UNBEATEN0, /* _LEAGUEUNBEATEN0_ */
    REP_TOKEN_STREAK_LEAGUE_UNBEATEN1, /* _LEAGUEUNBEATEN1_ */
    REP_TOKEN_LAST
};

typedef struct
{
    gboolean initialized;
    gint value;
} StratTokenValue;

typedef struct
{
    enum StratCondToken token;
    const gchar *value;
    gint len;
} StratCondPart;

/** A struct describing the pre-match strategy settings 
    of a CPU team. */
typedef struct
{
    /** A condition describing when the strategy should be applied. */
    gchar *condition;
    /** A lexed version of the conditions for faster processing. */
    GArray *parsed_condition;
    /** Array of possible formations, sorted by preference. */
    GArray *formations;
    /** Boost, style values and lineup type. */
    gint boost, style, lineup;
    /** The fitness value below which a player gets substituted
	if there's a sub with better fitness. */
    gfloat min_fitness;
} StrategyPrematch;

typedef struct
{
    /** A condition describing when the action should be taken. */
    gchar *condition, *sub_condition;
    /** A lexed version of the conditions for faster processing. */
    GArray *parsed_condition, *parsed_sub_condition;
    /** New boost and style values. */
    gint boost, style;
    /** Substitution specifiers (position and property).
	Property is taken from #StratLineupType. */
    gint sub_in_pos, sub_in_prop,
	sub_out_pos, sub_out_prop;
    /** An id to prevent actions from being applied again and
	again during a match. */
    gint id;
} StrategyMatchAction;

/** A CPU strategy. */
typedef struct
{
    /** String id and description of the strategy. */
    gchar *sid, *desc;
    /** How often this strategy gets picked, relative
	to the other strategies. */
    gint priority;
    /** Array with prematch settings. */
    GArray *prematch;
    /** Array with match settings. */
    GArray *match_action;
} Strategy;

#endif
