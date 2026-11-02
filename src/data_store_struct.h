/*
  data_store_struct.h

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

#ifndef DATA_STORE_STRUCT_H
#define DATA_STORE_STRUCT_H

#include "cup_struct.h"
#include "job_struct.h"
#include "league_struct.h"
#include "news_struct.h"
#include "option_struct.h"
#include "transfer_struct.h"

typedef struct data_store
{
    int (*set_name)(struct data_store *, const char *);
    int (*begin_transaction)(struct data_store *);
    int (*end_transaction)(struct data_store *);
    int (*write_options)(struct data_store *, const char *, const OptionList *,
                         int);
    int (*write_league)(struct data_store *, const League *);
    int (*write_cup)(struct data_store *, const Cup *);
    int (*write_team)(struct data_store *, const Team *);
    int (*write_player)(struct data_store *, const Player *, int);
    int (*write_user)(struct data_store *, const User *);
    int (*write_transfer)(struct data_store *, const Transfer *);
    int (*write_season_stat)(struct data_store *, const SeasonStat *);
    int (*write_job)(struct data_store *, const Job *);
    int (*write_newspaper)(struct data_store *, const NewsPaperArticle *);
    int (*write_bets)(struct data_store *, GArray *bets[2]);
    int (*write_country)(struct data_store *, const Country *);
    int (*write_bygfoot)(struct data_store *, const Bygfoot *);
    void (*destroy)(struct data_store *);
} DataStore;

#endif
