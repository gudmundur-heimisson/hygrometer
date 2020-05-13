#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <gobject/gsignal.h>
#include <bluetooth.h>


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
  g_variant_unref(alias_variant);
  g_print(alias);
  g_print("\n");

   GVariant* manufacturer_data_dict = g_dbus_proxy_get_cached_property(
                                           device,  /* proxy */
                                           "ManufacturerData"  /* property name */
                                         );
   if (manufacturer_data_dict == NULL) {
    g_print("No manufacturer data\n");
    return;
   }
  g_print("Got dict\n");
  size_t manufacturer_data_dict_size = g_variant_n_children(manufacturer_data_dict);
  g_printf("Size of manufacturer data dict: %d\n", manufacturer_data_dict_size);
  GVariant* manufacturer_data_item = g_variant_get_child_value(manufacturer_data_dict, 0);
  uint16_t* manufacturer_id;
  GVariant* bytes_variant;
  g_variant_get(manufacturer_data_item, "{qv}", &manufacturer_id, &bytes_variant);
  g_printf("Manufacturer ID: %d\n", *manufacturer_id);
  g_print(g_variant_print(bytes_variant, TRUE));
  g_print("\n");
  GVariantIter bytes_iter;
  g_variant_iter_init(&bytes_iter, bytes_variant);
  GVariant* byte_variant;
  size_t index = 0;
  g_print("Manufacturer data: ");
  size_t data_size = g_variant_iter_n_children(&bytes_iter);
  uint8_t* data = (uint8_t*) calloc(data_size, sizeof(uint8_t));
  while (byte_variant = g_variant_iter_next_value(&bytes_iter)) {
    uint8_t* byte;
    g_variant_get(byte_variant, "y", &data[index++]);
  }
  for (size_t i = 0; i < data_size; ++i) {
    g_printf("%x ", data[i]);
  }
  g_print("\n");
  free(data);
  g_print("\n\n");
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
  g_variant_unref(start_discover_variant);

  GVariant* address_variant = g_dbus_proxy_get_cached_property(
                                    adapter,  /* proxy */
                                    "Address"  /* property name */
                          );
  const gchar* address = g_variant_get_string(address_variant,
                                              NULL /* length */);
  g_variant_unref(address_variant);
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
  g_variant_unref(stop_discover_variant);

  return 0;
}