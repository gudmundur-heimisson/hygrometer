#include <stdlib.h>
#include <stdint.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <gobject/gsignal.h>

const char BLUEZ_DEVICE_INTERFACE[] = "org.bluez.Device1";
const char BLUEZ_BUS[] = "org.bluez";
const char BLUEZ_ADAPTER_INTERFACE[] = "org.bluez.Adapter1";
const char BLUEZ_ADAPTER_PATH[] = "/org/bluez/hci0";

// Used to get float values from the raw bytes of the manufacturer data
union float_bytes {
  float value;
  uint8_t bytes[sizeof(float)];
};
// Used to get unsigned long values from the raw bytes of the manufacturer data
union ulong_bytes {
  unsigned long value;
  uint8_t bytes[sizeof(unsigned long)];
};

/*
 * The manufacturer data property of the bluez device interface is in a weird
 * data structure, this function unpacks it into a byte array.
 *
 * Caller is responsible for freeing the output data.
 */
void get_manufacturer_data(GVariant* manufacturer_data_dict,  /* in */
                           uint16_t* manufacturer_id,  /* out */
                           uint8_t** data,  /* out */
                           size_t* data_len  /* out */) {
  // For some reason the manufacturer data is a dictionary of manufacturer
  // ID to manufacturer data, even though there can only ever be one entry.
  // First, get the only entry of the dictionary.
  GVariant* man_data = g_variant_get_child_value(manufacturer_data_dict, 0);
  // Unpack the manufacturer ID and variant containing the data
  GVariant* bytes_variant;
  g_variant_get(man_data, "{qv}", manufacturer_id, &bytes_variant);
  g_variant_unref(man_data);
  // Unpack the variant containing the data into a byte array
  GVariantIter bytes_iter;
  g_variant_iter_init(&bytes_iter, bytes_variant);
  GVariant* byte_variant;
  *data_len = g_variant_iter_n_children(&bytes_iter);
  *data = (uint8_t*) malloc(*data_len * sizeof(uint8_t));
  size_t index = 0;
  while (byte_variant = g_variant_iter_next_value(&bytes_iter)) {
    g_variant_get(byte_variant, "y", &((*data)[index++]));
  }
  // g_variant_iter_next_value takes care of unreferencing iterator and
  // bytes variant.
}

/**
 * Parses the raw bytes of the manufacturer data into the actual
 * data values and output them.
 */
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
  g_fprintf(stderr, "millis: %d ", millis.value);
  g_fprintf(stderr, "temp: %.2f ", temp.value);
  g_fprintf(stderr, "humidity: %.2f ", humidity.value);
  g_fprintf(stderr, "battery: %.2f \n", battery.value);
}

/**
 * Convenience function for parsing and displaying manufacturer data from
 * the hygrometer.
 */
void process_manufacturer_data(GVariant* manufacturer_data_variant) {
  uint16_t manufacturer_id;
  uint8_t* data;
  size_t data_len;
  get_manufacturer_data(manufacturer_data_variant,
                        &manufacturer_id,
                        &data,
                        &data_len);
  process_hygro_data(data, data_len);
  free(data);
}

/**
 * Callback for changes to the hygrometer devices.
 */
void on_device_properties_changed(GDBusProxy* device,
                                  GVariant* changed_properties,
                                  GStrv invalidated_properties,
                                  gpointer user_data) {
  // Iterate through the properties to get the manufacturer data
  GVariantIter* props_iter;
  g_variant_get(changed_properties, "a{sv}", &props_iter);
  char* prop_name;
  GVariant* prop_value;
  while (g_variant_iter_loop(props_iter, "{sv}", &prop_name, &prop_value)) {
    if (g_str_equal(prop_name, "ManufacturerData")) {
      // Process the manufacturer data
      process_manufacturer_data(prop_value);
    }
  }
  // g_variant_iter_loop frees prop_name and prop_value
  g_variant_iter_free(props_iter);
}

/**
 * Callback for new device discovery.
 *
 * Receives a GDBusObject, checks to see if it is a particular device,
 * then processes the manufacturer data and attaches a callback to it.
 *
 */
void on_device_manager_object_added(GDBusObjectManager* device_manager,
                                    GDBusObject* object,
                                    gpointer user_data) {
  // Check to see that it's a device and not some other interface
  const gchar* object_path = g_dbus_object_get_object_path(object);
  GDBusInterface* device_interface = g_dbus_object_manager_get_interface(
                                       device_manager,  /* object manager */
                                       object_path,  /* object path */
                                       BLUEZ_DEVICE_INTERFACE  /* interface name */
                                     );
  if (device_interface == NULL) {
    // Not a device, do nothing
    return;
  }
  g_object_unref(device_interface);

  // It's a device, get a proxy for it
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
  if (device == NULL) {
    // This should never happen because we checked that it's a device earlier
    // but not a fatal error if it does somehow
    return;
  }

  // Get the device name to see if we want to process it
  GVariant* alias_variant = g_dbus_proxy_get_cached_property(
                              device,  /* proxy */
                              "Alias"  /* property name */
                            );
  const gchar* alias = g_variant_get_string(alias_variant, NULL  /* length */);
  // Only want to process this one device
  if (g_str_equal("Gummi's Hygrometer", alias)) {
    // Get and process the manufacturer data for the sensor reading
    GVariant* manufacturer_data_dict = g_dbus_proxy_get_cached_property(
                                         device,  /* proxy */
                                         "ManufacturerData"  /* property name */
                                       );
    process_manufacturer_data(manufacturer_data_dict);
    g_variant_unref(manufacturer_data_dict);
    // Attach a callback to the device properties so that if the manufacturer
    // data changes we can reprocess it
    g_signal_connect(
      device, 
      "g-properties-changed",
      G_CALLBACK(on_device_properties_changed),
      NULL
    );
  }
  g_variant_unref(alias_variant);
}

int main(void) {
  GMainLoop* loop = g_main_loop_new(NULL, FALSE);

  // Get the default bluetooth adapter to do device scanning
  GError* error = NULL;
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
  if (adapter == NULL) {
    g_fprintf(stderr, error -> message);
    return 1;
  }

  // Get the bluez bus to listen for new devices being discovered
  error = NULL;
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
    g_fprintf(stderr, error->message);
    return 1;
  }

  // Check to see if device already exists before starting discovery
  GList* devices = g_dbus_object_manager_get_objects(device_manager);
  // GList is a linked list
  for (GList* d = devices; d != NULL; d = d->next) {
    GDBusObject* device = (GDBusObject*) d->data;
    on_device_manager_object_added(device_manager, device, NULL);
  }

  // Add a listener for new device discovery
  g_signal_connect(
    device_manager,  /* instance */
    "object-added",  /* detailed signal */
    G_CALLBACK(on_device_manager_object_added),  /* callback */
    NULL  /* data */
  );

  // Start device discovery
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
    g_fprintf(stderr, error->message);
    return 1;
  }
  g_variant_unref(start_discover_variant);

  // Run main loop forever, don't need to call stop discovery
  g_main_loop_run(loop);

  return 0;
}