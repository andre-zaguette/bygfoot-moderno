#ifndef BYGFOOT_STRUCT_H
#define BYGFOOT_STRUCT_H

#include <glib.h>
#include "gui_struct.h"

struct data_store;

enum BygfootFrontend {
    BYGFOOT_FRONTEND_GTK,
    BYGFOOT_FRONTEND_CONSOLE,
};

/** This struct holds all of the global state for a bygfoot game.  The goal
 * is for ths struct to eventually replace all the global variables.
*/
typedef struct
{
    /** Which kind of frontend is being use. See #enum BygfootFrontend. */
    enum BygfootFrontend frontend;

    /* GUI state. */
    GUI *gui;

    /** @name Frontend functions */
    /* @{ */
    void (*show_progress)(gfloat, const gchar *, gint);
    gdouble (*get_progress_bar_fraction)(void);
    /* @} */

    /** @name Global state */
    /* @{ */
    guint32 random_seed;
    GPtrArray *international_cups;
    struct live_game *current_live_game;
    /* @} */

    struct data_store *data_store;
} Bygfoot;


#endif
