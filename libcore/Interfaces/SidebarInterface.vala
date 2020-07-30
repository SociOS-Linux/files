/***
    Copyright (c) 2020 elementary LLC <https://elementary.io>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License version 3, as published
    by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranties of
    MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
    PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.

    Authors : Jeremy Wootten <jeremy@elementaryos.org>
***/

namespace Marlin {
    [CCode (has_target = false)]
    public delegate void SidebarCallbackFunc (Gtk.Widget widget);

    public enum PlaceType {
        BUILT_IN,
        MOUNTED_VOLUME,
        BOOKMARK,
        BOOKMARKS_CATEGORY,
        PERSONAL_CATEGORY,
        STORAGE_CATEGORY,
        NETWORK_CATEGORY,
        PLUGIN_ITEM
    }
}

public interface Marlin.SidebarInterface : Gtk.Widget {
        public signal void request_update ();
        public signal bool request_focus ();
        public signal void sync_needed ();
        public signal void path_change_request (string uri, Marlin.OpenFlag flag);
        public signal void connect_server_request ();
        /**
         * Adds plugin item to Sidebar
         *
         * @param a {@link Marlin.SidebarPluginItem}
         *
         * @param {@link PlaceType} with the category it should be appended
         *
         * @return An ID for the item to use to update with update_plugin_item or null if add failed
         */
        public abstract int32 add_plugin_item (Marlin.SidebarPluginItem item, Marlin.PlaceType category);
        public abstract bool update_plugin_item (Marlin.SidebarPluginItem item, int32 item_id);
        public abstract void remove_plugin_item (int32 item_id);

}
