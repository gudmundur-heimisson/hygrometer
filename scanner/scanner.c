#include <glib.h>
#include <gio/gio.h>

GDBusConnection *con;

int main(void) {

  GError* error = NULL;
  GDBusProxy* adapter = g_dbus_proxy_new_for_bus_sync(
                        G_BUS_TYPE_SYSTEM,  /* bus type */
                        G_DBUS_PROXY_FLAGS_NONE,  /* flags */
                        NULL,  /* info */
                        "org.bluez",  /* name */
                        "/org/bluez/hci0",  /* object path */
                        "org.bluez.Adapter1",  /* interface name */
                        NULL,  /* cancellable */
                        &error  /* error */
                      );

  error = NULL;
  GVariant* start_discover_variant = g_dbus_proxy_call_sync(
                                      adapter,  /* proxy */
                                      "StartDiscovery",  /* method name */
                                      NULL,  /* parameters */
                                      G_DBUS_CALL_FLAGS_NONE,  /* flags */
                                      -1,  /* timeout_msec */
                                      NULL,  /* cancellable */
                                      &error /* error */
                                     );
  if (start_discover_variant == NULL) {
    g_print(error -> message);
    return 1;
  }

  GVariant* address_variant = g_dbus_proxy_get_cached_property(
                                    adapter  /*proxy */,
                                    "Address"  /* property name */
                          );
  const gchar* address = g_variant_get_string(address_variant,
                                              NULL /* length */);
  g_print(address);
  g_print("\n");


  error = NULL;
  GVariant* stop_discover_variant = g_dbus_proxy_call_sync(
                                     adapter,  /* proxy */
                                     "StopDiscovery",  /* method name */
                                     NULL,  /* parameters */
                                     G_DBUS_CALL_FLAGS_NONE,  /* flags */
                                     -1,  /* timeout_msec */
                                     NULL,  /* cancellable */
                                     &error  /* error */
                                    );
  if (stop_discover_variant == NULL) {
    g_print(error -> message);
    return 1;
  }

  return 0;
}