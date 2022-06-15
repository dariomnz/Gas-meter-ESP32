#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define INFLUXDB_URL "https://europe-west1-1.gcp.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "ON6_k3_hjHJqZf5fPXFB0hAuBwEfpN7K-7RfaF_1zLAZXIn3Vfpa6RCZtJGKva3iFil3TkNTSOgy9gtGGXR6RQ=="
#define INFLUXDB_ORG "100405982@alumnos.uc3m.es"
#define INFLUXDB_BUCKET "Gas-metrics"
#define DEVICE "ESP32-001"
#define TZ_INFO "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("gas_metrics");

void send_data(const std::string &data){
  sensor.addTag("device", DEVICE);

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  sensor.clearFields();

  float number = std::stof(data); 
  sensor.addField("measurement", number, 3);

  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
