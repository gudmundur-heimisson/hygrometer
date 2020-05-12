#include <glib.h>
#include <gio/gio.h>
#include <gobject/gsignal.h>
#include <bluetooth.h>

GDBusConnection *con;

void on_device_manager_object_added(GDBusObjectManager* device_manager,
                                    GDBusObject* object,
                                    gpointer user_data) {
  g_print("Object added\n");
  const gchar* object_path = g_dbus_object_get_object_path(object);
  GDBusInterface* device_interface = g_dbus_object_manager_get_interface(
                                       device_manager,  /* object manager */
                                       object_path,  /* object path */
                                       "org.bluez.Device1"  /* interface name */
                                      );
  if (device_interface == NULL) {
    // Not a device
    g_print("Not a device.\n");
    return;
  }
  g_print("Found device.\n");
  GError* error = NULL;
  GDBusProxy* device = g_dbus_proxy_new_for_bus_sync(
                         G_BUS_TYPE_SYSTEM,  /* bus type */
                         G_DBUS_PROXY_FLAGS_NONE,  /* flags */
                         NULL,  /* info */
                         "org.bluez",  /* bus name */
                         object_path,  /* object path */
                         "org.bluez.Device1",  /* interface name */
                         NULL,  /* cancellable */
                         &error);
  GVariant* alias_variant = g_dbus_proxy_get_cached_property(
                              device,  /* proxy */
                              "Alias"  /* property name */
                            );
  const gchar* alias = g_variant_get_string(alias_variant, NULL  /* length */);
  g_print(alias);
  g_print("\n");

  GVariant* manufacturer_data_dict = g_dbus_proxy_get_cached_property(
                                          device,  /* proxy */
                                          "ManufacturerData"  /* property name */
                                        );

  GVariantIter dict_iter;
  uint16_t* manufacturer_id;
  GVariant* bytes_variant;
  g_variant_iter_init(&dict_iter, manufacturer_data_dict);
  while(g_variant_iter_loop(&dict_iter, "{qv}", &manufacturer_id, &bytes_variant)) {
    printf("Manufacturer ID: %d\n", *manufacturer_id);
  }
 // g_variant_iter_free(dicts_array_iter);
}

int main(void) {

  GMainLoop* loop = g_main_loop_new(NULL, FALSE);

  GError* error = NULL;
  GDBusObjectManager* device_manager = g_dbus_object_manager_client_new_for_bus_sync(
                                         G_BUS_TYPE_SYSTEM,  /* bus type */
                                         G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,  /* flags */
                                         "org.bluez",  /* name */
                                         "/",  /* object path */
                                         NULL,  /* get proxy type function */
                                         NULL,  /* get proxy type user data */
                                         NULL,  /* get proxy type destroy notify */
                                         NULL,  /* cancellable */
                                         &error  /* error */
                                       );
  if (device_manager == NULL) {
    g_print(error->message);
    return 1;
  }

  gulong handler_id = g_signal_connect(
                       device_manager,  /* instance */
                       "object-added",  /* detailed signal */
                       G_CALLBACK(on_device_manager_object_added),  /* callback */
                       NULL  /* data */
                       );

  error = NULL;
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
    g_print(error->message);
    return 1;
  }

  GVariant* address_variant = g_dbus_proxy_get_cached_property(
                                    adapter,  /* proxy */
                                    "Address"  /* property name */
                          );
  const gchar* address = g_variant_get_string(address_variant,
                                              NULL /* length */);
  g_print(address);
  g_print("\n");

  g_main_loop_run(loop);

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