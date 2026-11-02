/*
  sqlite_data_store_struct.h

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

#ifndef SQLITE_DATA_STORE_H
#define SQLITE_DATA_STORE_H

#include "bygfoot_struct.h"
#include "data_store_struct.h"

DataStore *
sqlite_data_store_create(void);

gboolean
sqlite_load_bygfoot(Bygfoot *bygfoot);
#endif
