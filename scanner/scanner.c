#include <glib.h>
#include <gio/gio.h>

GDBusConnection *con;

int main(void) {
  GMainLoop *loop;
  int ret_code;

  con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
  if (con == NULL) {
    g_print("Not able to get connection to system bus\n");
    return 1;
  }
  return 0;
}