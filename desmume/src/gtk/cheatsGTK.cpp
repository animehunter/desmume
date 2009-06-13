/* cheats.cpp - this file is part of DeSmuME
 *
 * Copyright (C) 2006-2009 DeSmuME Team
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "cheatsGTK.h"
#include "cheatSystem.h"
#include "main.h"

static  GtkWidget *win;

static void enabled_toggled(GtkCellRendererToggle * cell,
                            gchar * path_str, gpointer data);

enum {
    COLUMN_POS,
    COLUMN_ENABLED,
    COLUMN_SIZE,
    COLUMN_HI,
    COLUMN_LO,
    COLUMN_DESC,
    NUM_COL
};

enum {
    TYPE_TOGGLE,
    TYPE_STRING
};

static struct {
    const gchar *caption;
    gint type;
    gint column;
} columnTable[]={
    { "Position", TYPE_STRING, COLUMN_POS},
    { "Enabled", TYPE_TOGGLE, COLUMN_ENABLED},
    { "Size", TYPE_STRING, COLUMN_SIZE},
    { "Offset", TYPE_STRING, COLUMN_HI},
    { "Value", TYPE_STRING, COLUMN_LO},
    { "Description", TYPE_STRING, COLUMN_DESC}
};

static void cell_edited(GtkCellRendererText * cell,
                        const gchar * path_string,
                        const gchar * new_text, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
    GtkTreeIter iter;

    gint column =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));

    gtk_tree_model_get_iter(model, &iter, path);

    {
        CHEATS_LIST cheat;
        u32 ii;
        gtk_tree_model_get(model, &iter, COLUMN_POS, &ii, -1);
        cheatsGet(&cheat, ii);
        if (column == COLUMN_LO || column == COLUMN_HI
            || column == COLUMN_SIZE) {
            u32 v = atoi(new_text);
            switch (column) {
            case COLUMN_SIZE:
                cheatsUpdate(v, cheat.hi[0], cheat.lo[0],
                             cheat.description, cheat.enabled, ii);
                break;
            case COLUMN_HI:
                cheatsUpdate(cheat.size, v, cheat.lo[0], cheat.description,
                             cheat.enabled, ii);
                break;
            case COLUMN_LO:
                cheatsUpdate(cheat.size, cheat.hi[0], v, cheat.description,
                             cheat.enabled, ii);
                break;
            }
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                               atoi(new_text), -1);
        } else {
            cheatsUpdate(cheat.size, cheat.hi[0], cheat.lo[0],
                         g_strdup(new_text), cheat.enabled, ii);
            gtk_list_store_set(GTK_LIST_STORE(model), &iter, column,
                               g_strdup(new_text), -1);
        }

    }
}

static void cheat_list_add_columns(GtkTreeView * tree, GtkListStore *store)
{

    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));

    for (u32 ii = 0; ii < sizeof(columnTable) / sizeof(columnTable[0]); ii++) {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        const gchar *attrib;
        if (columnTable[ii].type == TYPE_TOGGLE) {
            renderer = gtk_cell_renderer_toggle_new();
            g_signal_connect(renderer, "toggled",
                             G_CALLBACK(enabled_toggled), model);
            attrib = "active";
        } else {
            renderer = gtk_cell_renderer_text_new();
            g_object_set (renderer, "editable", TRUE, NULL);
            g_signal_connect (renderer, "edited", G_CALLBACK (cell_edited), store);
            attrib = "text";
        }
        column = gtk_tree_view_column_new_with_attributes(columnTable[ii].caption,
                                   renderer, attrib, columnTable[ii].column, NULL);
        g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (columnTable[ii].column));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

}

static void cheatListEnd()
{
    cheatsSave();
    Launch();
}

static GtkListStore *cheat_list_populate()
{
    GtkListStore *store = gtk_list_store_new (6, G_TYPE_INT, G_TYPE_BOOLEAN, 
            G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);

    CHEATS_LIST cheat;
    u32 chsize = cheatsGetSize();
    for(u32 ii = 0; ii < chsize; ii++){
        GtkTreeIter iter;
        cheatsGet(&cheat, ii);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                COLUMN_POS, ii,
                COLUMN_ENABLED, cheat.enabled,
                COLUMN_SIZE, cheat.size+1,
                COLUMN_HI, cheat.hi[0],
                COLUMN_LO, cheat.lo[0],
                COLUMN_DESC, cheat.description,
                -1);
    }
    return store;
}

static void cheat_list_remove_cheat(GtkWidget * widget, gpointer data)
{
    GtkTreeView *tree = (GtkTreeView *) data;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
    GtkTreeModel *model = gtk_tree_view_get_model (tree);
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
        u32 ii;
        gtk_tree_model_get(model, &iter, COLUMN_POS, &ii, -1);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        cheatsRemove(ii);
    }
}

static void cheat_list_add_cheat(GtkWidget * widget, gpointer data)
{
#define NEW_DESC "New cheat"
    GtkListStore *store = (GtkListStore *) data;
    GtkTreeIter iter;
    cheatsAdd(1, 0, 0, g_strdup(NEW_DESC), FALSE);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       COLUMN_POS, 1,
                       COLUMN_ENABLED, FALSE,
                       COLUMN_SIZE, 1,
                       COLUMN_HI, 0,
                       COLUMN_LO, 0, COLUMN_DESC, NEW_DESC, -1);

#undef NEW_DESC
}

static GtkWidget *cheat_list_create_ui()
{
    GtkListStore *store = cheat_list_populate();
    GtkWidget *tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    GtkWidget *vbox = gtk_vbox_new(FALSE, 1);
    GtkWidget *hbbox = gtk_hbutton_box_new();
    GtkWidget *button;
  
    gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(tree));
    gtk_container_add(GTK_CONTAINER(vbox), GTK_WIDGET(hbbox));
    gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(vbox));

    button = gtk_button_new_with_label("Add cheat");
    g_signal_connect (button, "clicked", G_CALLBACK (cheat_list_add_cheat), store);
    gtk_container_add(GTK_CONTAINER(hbbox),button);

    button = gtk_button_new_with_label("Remove cheat");
    g_signal_connect (button, "clicked", G_CALLBACK (cheat_list_remove_cheat), tree);
    gtk_container_add(GTK_CONTAINER(hbbox),button);

    cheat_list_add_columns(GTK_TREE_VIEW(tree), store);
    
    /* Setup the selection handler */
    GtkTreeSelection *select;
    select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

    return tree;
}

void CheatList ()
{
    Pause();
    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win),"Cheat List");
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    g_signal_connect(G_OBJECT(win), "destroy", cheatListEnd, NULL);

    cheat_list_create_ui();

    gtk_widget_show_all(win);
}

static void
enabled_toggled(GtkCellRendererToggle * cell,
                gchar * path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *) data;
    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    gboolean enabled;

    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, COLUMN_ENABLED, &enabled, -1);

    enabled ^= 1;
    CHEATS_LIST cheat;
    u32 ii;
    gtk_tree_model_get(model, &iter, COLUMN_POS, &ii, -1);
    cheatsGet(&cheat, ii);

    cheatsUpdate(cheat.size, cheat.hi[0], cheat.lo[0], cheat.description,
                 enabled, ii);

    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_ENABLED, enabled, -1);

    gtk_tree_path_free(path);
}


void CheatSearch ()
{
    printf("Cheat searching feature is not hooked up\n");
}
