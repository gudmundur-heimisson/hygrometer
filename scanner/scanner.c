#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <gobject/gsignal.h>

const char BLUEZ_DEVICE_INTERFACE[] = "org.bluez.Device1";
const char BLUEZ_BUS[] = "org.bluez";
const char BLUEZ_ADAPTER_INTERFACE[] = "org.bluez.Adapter1";
const char BLUEZ_ADAPTER_PATH[] = "/org/bluez/hci0";

union float_bytes {
  float value;
  uint8_t bytes[sizeof(float)];
};

union ulong_bytes {
  unsigned long value;
  uint8_t bytes[sizeof(unsigned long)];
};

void get_manufacturer_data(GVariant* manufacturer_data_dict,  /* in */
                           uint16_t* manufacturer_id,  /* out */
                           uint8_t** data,  /* out */
                           size_t* data_len  /* out */) {
  GVariant* man_data = g_variant_get_child_value(manufacturer_data_dict, 0);
  GVariant* bytes_variant;
  g_variant_get(man_data, "{qv}", manufacturer_id, &bytes_variant);
  GVariantIter bytes_iter;
  g_variant_iter_init(&bytes_iter, bytes_variant);
  GVariant* byte_variant;
  *data_len = g_variant_iter_n_children(&bytes_iter);
  *data = (uint8_t*) malloc(*data_len * sizeof(uint8_t));
  size_t index = 0;
  while (byte_variant = g_variant_iter_next_value(&bytes_iter)) {
    g_variant_get(byte_variant, "y", &((*data)[index++]));
  }
}

void process_hygro_data(uint8_t* data, size_t data_len) {
  union ulong_bytes millis;
  union float_bytes temp;
  union float_bytes humidity;
  union float_bytes battery;
  for (int i = 0; i < 4; ++i) {
    millis.bytes[i] = data[i];
    temp.bytes[i] = data[4 + i];
    humidity.bytes[i] = data[8 + i];
    battery.bytes[i] = data[12 + i];
  }
  g_printf("millis: %d ", millis.value);
  g_printf("temp: %.2f ", temp.value);
  g_printf("humidity: %.2f ", humidity.value);
  g_printf("battery: %.2f \n", battery.value);
}

void process_manufacturer_data(GVariant* manufacturer_data_variant) {
  uint16_t manufacturer_id;
  uint8_t* data;
  size_t data_len;
  get_manufacturer_data(manufacturer_data_variant,
                        &manufacturer_id,
                        &data,
                        &data_len);
  process_hygro_data(data, data_len);
}

void on_device_properties_changed(GDBusProxy* device,
                                  GVariant* changed_properties,
                                  GStrv invalidated_properties,
                                  gpointer user_data) {
  GVariantIter* props_iter;
  g_variant_get(changed_properties, "a{sv}", &props_iter);
  char* prop_name;
  GVariant* prop_value;
  while (g_variant_iter_loop(props_iter, "{sv}", &prop_name, &prop_value)) {
    if (g_str_equal(prop_name, "ManufacturerData")) {
      process_manufacturer_data(prop_value);
    }
  }
  g_variant_iter_free(props_iter);
}

void on_device_manager_object_added(GDBusObjectManager* device_manager,
                                    GDBusObject* object,
                                    gpointer user_data) {
  const gchar* object_path = g_dbus_object_get_object_path(object);
  GDBusInterface* device_interface = g_dbus_object_manager_get_interface(
                                       device_manager,  /* object manager */
                                       object_path,  /* object path */
                                       BLUEZ_DEVICE_INTERFACE  /* interface name */
                                     );
  if (device_interface == NULL) {
    return;
  }
  GError* error = NULL;
  GDBusProxy* device = g_dbus_proxy_new_for_bus_sync(
                         G_BUS_TYPE_SYSTEM,  /* bus type */
                         G_DBUS_PROXY_FLAGS_NONE,  /* flags */
                         NULL,  /* info */
                         BLUEZ_BUS,  /* bus name */
                         object_path,  /* object path */
                         BLUEZ_DEVICE_INTERFACE,  /* interface name */
                         NULL,  /* cancellable */
                         &error
                       );
  GVariant* alias_variant = g_dbus_proxy_get_cached_property(
                              device,  /* proxy */
                              "Alias"  /* property name */
                            );
  const gchar* alias = g_variant_get_string(alias_variant, NULL  /* length */);
  if (strncmp("Gummi", alias, strlen("Gummi")) == 0) {
    GVariant* manufacturer_data_dict = g_dbus_proxy_get_cached_property(
                                         device,  /* proxy */
                                         "ManufacturerData"  /* property name */
                                       );
    process_manufacturer_data(manufacturer_data_dict);
    g_signal_connect(
      device, 
      "g-properties-changed",
      G_CALLBACK(on_device_properties_changed),
      NULL
    );
    g_variant_unref(manufacturer_data_dict);
  }
}

int main(void) {
  GMainLoop* loop = g_main_loop_new(NULL, FALSE);

  GError* error = NULL;
  GDBusObjectManager* device_manager = g_dbus_object_manager_client_new_for_bus_sync(
                                         G_BUS_TYPE_SYSTEM,  /* bus type */
                                         G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,  /* flags */
                                         BLUEZ_BUS,  /* name */
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


  error = NULL;
  GDBusProxy* adapter = g_dbus_proxy_new_for_bus_sync(
                          G_BUS_TYPE_SYSTEM,  /* bus type */
                          G_DBUS_PROXY_FLAGS_NONE,  /* flags */
                          NULL,  /* info */
                          BLUEZ_BUS,  /* name */
                          BLUEZ_ADAPTER_PATH,  /* object path */
                          BLUEZ_ADAPTER_INTERFACE,  /* interface name */
                          NULL,  /* cancellable */
                          &error  /* error */
                        );

  // Check to see if device already exists
  GList* devices = g_dbus_object_manager_get_objects(device_manager);
  for (GList* d = devices; d != NULL; d = d->next) {
    GDBusObject* device = (GDBusObject*) d->data;
    on_device_manager_object_added(device_manager, device, NULL);
  }

  g_signal_connect(
    device_manager,  /* instance */
    "object-added",  /* detailed signal */
    G_CALLBACK(on_device_manager_object_added),  /* callback */
    NULL  /* data */
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