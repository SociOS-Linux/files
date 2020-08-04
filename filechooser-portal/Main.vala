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

private static bool opt_replace = false;
private static bool show_version = false;

private static GLib.MainLoop loop;

private const GLib.OptionEntry[] ENTRIES = {
    { "replace", 'r', 0, OptionArg.NONE, ref opt_replace, "Replace a running instance", null },
    { "version", 0, 0, OptionArg.NONE, ref show_version, "Show program version.", null },
    { null }
};

[DBus (name = "org.freedesktop.impl.portal.FileChooser")]
public class Files.FileChooser : GLib.Object {
    private GLib.DBusConnection connection;

    public FileChooser (GLib.DBusConnection connection) {
        this.connection = connection;
    }

    public void open_file (GLib.ObjectPath handle, string app_id, string parent_window, string title, GLib.HashTable<string, GLib.Variant> options, out uint response, out GLib.HashTable<string, GLib.Variant> results) throws GLib.DBusError, GLib.IOError {
        results = new GLib.HashTable<string, GLib.Variant> (str_hash, str_equal);
        var dialog = new Files.FileChooserDialog (connection, handle, app_id, parent_window, title, options);
        dialog.action = Gtk.FileChooserAction.OPEN;

        unowned GLib.Variant? multiple_variant = options["multiple"];
        bool multiple = false;
        if (multiple_variant != null && multiple_variant.is_of_type (GLib.VariantType.BOOLEAN))
            multiple = multiple_variant.get_boolean ();

        dialog.select_multiple = multiple;
        unowned GLib.Variant? accept_label = options["accept_label"];
        if (accept_label != null && accept_label.is_of_type (GLib.VariantType.STRING)) {
            dialog.add_button (accept_label.get_string (), Gtk.ResponseType.OK);
        } else {
            dialog.add_button (multiple ? _("Open") : _("Select"), Gtk.ResponseType.OK);
        }

        dialog.show_all ();
        var dialog_loop = new GLib.MainLoop (null, false);
        Gtk.ResponseType dialog_response = Gtk.ResponseType.DELETE_EVENT;
        dialog.response.connect ((id) => {
            dialog_response = (Gtk.ResponseType) id;
            dialog_loop.quit ();
        });

        dialog_loop.run ();
        switch (dialog_response) {
            case Gtk.ResponseType.OK:
                response = 0;
                // Properly return
                // handle->filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER(widget));
                // handle->uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (widget));
                break;
            case Gtk.ResponseType.CANCEL:
                response = 1;
                break;
            case Gtk.ResponseType.DELETE_EVENT:
            default:
                response = 2;
                break;
        }
    }

    public void save_file (GLib.ObjectPath handle, string app_id, string parent_window, string title, GLib.HashTable<string, GLib.Variant> options, out uint response, out GLib.HashTable<string, GLib.Variant> results) throws GLib.DBusError, GLib.IOError {
        results = new GLib.HashTable<string, GLib.Variant> (str_hash, str_equal);
        var dialog = new Files.FileChooserDialog (connection, handle, app_id, parent_window, title, options);
        dialog.action = Gtk.FileChooserAction.SAVE;

        unowned GLib.Variant? accept_label = options["accept_label"];
        if (accept_label != null && accept_label.is_of_type (GLib.VariantType.STRING)) {
            dialog.add_button (accept_label.get_string (), Gtk.ResponseType.OK);
        } else {
            dialog.add_button (_("Save"), Gtk.ResponseType.OK);
        }

        dialog.show_all ();
        var dialog_loop = new GLib.MainLoop (null, false);
        Gtk.ResponseType dialog_response = Gtk.ResponseType.DELETE_EVENT;
        dialog.response.connect ((id) => {
            dialog_response = (Gtk.ResponseType) id;
            dialog_loop.quit ();
        });

        dialog_loop.run ();
        switch (dialog_response) {
            case Gtk.ResponseType.OK:
                response = 0;
                // Properly return
                // handle->filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER(widget));
                // handle->uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (widget));
                break;
            case Gtk.ResponseType.CANCEL:
                response = 1;
                break;
            case Gtk.ResponseType.DELETE_EVENT:
            default:
                response = 2;
                break;
        }
    }

    public void save_files (GLib.ObjectPath handle, string app_id, string parent_window, string title, GLib.HashTable<string, GLib.Variant> options, out uint response, out GLib.HashTable<string, GLib.Variant> results) throws GLib.DBusError, GLib.IOError {
        results = new GLib.HashTable<string, GLib.Variant> (str_hash, str_equal);
        var dialog = new Files.FileChooserDialog (connection, handle, app_id, parent_window, title, options);
        dialog.action = Gtk.FileChooserAction.SELECT_FOLDER;

        unowned GLib.Variant? accept_label = options["accept_label"];
        if (accept_label != null && accept_label.is_of_type (GLib.VariantType.STRING)) {
            dialog.add_button (accept_label.get_string (), Gtk.ResponseType.OK);
        } else {
            dialog.add_button (_("Save"), Gtk.ResponseType.OK);
        }

        dialog.show_all ();
        var dialog_loop = new GLib.MainLoop (null, false);
        Gtk.ResponseType dialog_response = Gtk.ResponseType.DELETE_EVENT;
        dialog.response.connect ((id) => {
            dialog_response = (Gtk.ResponseType) id;
            dialog_loop.quit ();
        });

        dialog_loop.run ();
        switch (dialog_response) {
            case Gtk.ResponseType.OK:
                response = 0;
                // Properly return
                // handle->filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER(widget));
                // handle->uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (widget));
                break;
            case Gtk.ResponseType.CANCEL:
                response = 1;
                break;
            case Gtk.ResponseType.DELETE_EVENT:
            default:
                response = 2;
                break;
        }
    }
}

private void on_bus_acquired (GLib.DBusConnection connection, string name) {
    try {
        connection.register_object ("/org/freedesktop/portal/desktop", new Files.FileChooser (connection));
    } catch (GLib.Error e) {
        critical ("Unable to register the object: %s", e.message);
    }
}

public int main (string[] args) {
    GLib.Intl.setlocale (GLib.LocaleCategory.ALL, "");
    GLib.Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
    GLib.Intl.textdomain (Config.GETTEXT_PACKAGE);

    /* Avoid pointless and confusing recursion */
    GLib.Environment.unset_variable ("GTK_USE_PORTAL");

    Gtk.init (ref args);

    var context = new GLib.OptionContext ("- FileChooser portal");
    context.add_main_entries (ENTRIES, null);
    try {
        context.parse (ref args);
    } catch (Error e) {
        printerr ("%s: %s", Environment.get_application_name (), e.message);
        printerr ("\n");
        printerr ("Try \"%s --help\" for more information.", GLib.Environment.get_prgname ());
        printerr ("\n");
        return 1;
    }

    if (show_version) {
      print ("0.0 \n");
      return 0;
    }

    loop = new GLib.MainLoop (null, false);

    try {
        var session_bus = GLib.Bus.get_sync (GLib.BusType.SESSION);
        var owner_id = GLib.Bus.own_name (
            GLib.BusType.SESSION,
            "org.freedesktop.impl.portal.desktop.elementary.files",
            GLib.BusNameOwnerFlags.ALLOW_REPLACEMENT | (opt_replace ? GLib.BusNameOwnerFlags.REPLACE : 0),
            on_bus_acquired,
            () => { debug ("org.freedesktop.impl.portal.desktop.elementary.files acquired"); },
            () => { loop.quit (); }
        );
        loop.run ();
        GLib.Bus.unown_name (owner_id);
    } catch (Error e) {
        printerr ("No session bus: %s\n", e.message);
        return 2;
    }

    return 0;

}
