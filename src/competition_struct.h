/*
   competition_struct.h

   Bygfoot Football Manager -- a small and simple GTK2-based
   football management game.

   http://bygfoot.sourceforge.net

   Copyright (C) 2023  Tom Stellard (tom@stellard.net)

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

#ifndef COMPETITION_STRUCT_H
#define COMPETITION_STRUCT_H

#include <glib.h>

/** Competition struct represents the common parts of the Cup and League
 * structs. */
typedef struct
{
    /** Numerical id. */
    gint id;

    gchar *name;

    /** Number of yellow cards that lead to a missed match.
	Default: 1000 (off). */
    gint yellow_red;

    /** A gchar pointer array of properties (like "national" or "inactive"). */
    GPtrArray *properties;

} Competition;

#endif
