#if defined(ESP32)
  #include <WiFiMulti.h>
  WiFiMulti wifiMulti;
  #define DEVICE "ESP32"
#elif defined(ESP8266)
  #include <ESP8266WiFiMulti.h>
  ESP8266WiFiMulti wifiMulti;
  #define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// WiFi credentials
#define WIFI_SSID "IVI"
#define WIFI_PASSWORD "!v!-2@21-AI"

// InfluxDB credentials
#define INFLUXDB_URL "http://192.168.150.183:8086"
#define INFLUXDB_TOKEN "1ZH0oco8BFIsLzr9dJtiwVVfmwZOfGAageK9lbsfmIvDcYJz9K0nY1y0RnOHXSo7JrBOh8aN4pJojsv0r3l6xg=="
#define INFLUXDB_ORG "e73ea52dec75562a"
#define INFLUXDB_BUCKET "MPU6050"
#define TZ_INFO "UTC1"

// InfluxDB client
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("mpu6050_data");

// MPU6050 sensor
Adafruit_MPU6050 mpu;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println("\nConnected to WiFi");

    // Sync time for InfluxDB
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    // Check InfluxDB connection
    if (client.validateConnection()) {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(client.getServerUrl());
    } else {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(client.getLastErrorMessage());
    }

    // Add tags to the data point
    sensor.addTag("device", DEVICE);
    sensor.addTag("SSID", WiFi.SSID());

    // Initialize MPU6050
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 sensor!");
        while (1) delay(10);
    }
    Serial.println("MPU6050 initialized!");
}

void loop() {
    // Clear fields for new data
    sensor.clearFields();

    // Read data from MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Add sensor data to InfluxDB
    sensor.addField("acc_x", a.acceleration.x);
    sensor.addField("acc_y", a.acceleration.y);
    sensor.addField("acc_z", a.acceleration.z);
    sensor.addField("gyro_x", g.gyro.x);
    sensor.addField("gyro_y", g.gyro.y);
    sensor.addField("gyro_z", g.gyro.z);
    sensor.addField("temperature", temp.temperature);
    sensor.addField("rssi", WiFi.RSSI());

    // Print data before sending
    Serial.print("Writing to InfluxDB: ");
    Serial.println(sensor.toLineProtocol());

    // Check WiFi and reconnect if necessary
    if (wifiMulti.run() != WL_CONNECTED) {
        Serial.println("WiFi connection lost!");
    }

    // Write data to InfluxDB
    if (!client.writePoint(sensor)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
    }

    Serial.println("Data sent. Waiting 1 second...");
    delay(1000);
}
