/*-
 * Copyright 2020 elementary LLC <https://elementary.io>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1335 USA.
 *
 * Authored by: Corentin Noël <corentin@elementary.io>
 */

[DBus (name = "org.freedesktop.impl.portal.Request")]
public class Files.FileChooserDialog : Gtk.FileChooserDialog {
    private GLib.DBusConnection connection;
    private uint registration_id;

    public FileChooserDialog (GLib.DBusConnection connection, GLib.ObjectPath handle, string app_id, string parent_window, string title, GLib.HashTable<string, GLib.Variant> options) {
        this.connection = connection;
        try {
            registration_id = connection.register_object<FileChooserDialog> (handle, this);
        } catch (Error e) {
            critical (e.message);
        }
    }

    construct {
        add_button (_("Cancel"), Gtk.ResponseType.CANCEL);
        set_default_response (Gtk.ResponseType.OK);
        response.connect (() => {
            destroy ();
        });

        destroy.connect (() => {
            if (registration_id != 0) {
                connection.unregister_object (registration_id);
                registration_id = 0;
            }
        });
    }

    [DBus (name = "Close")]
    public void on_close () throws GLib.DBusError, GLib.IOError {
        response (Gtk.ResponseType.DELETE_EVENT);
    }
}
