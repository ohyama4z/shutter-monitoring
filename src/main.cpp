#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "env.h"

// ピン割当
const unsigned int LED_PIN = 13;
const unsigned int WAVE_ECHO_PIN = 14;
const unsigned int WAVE_TRIG_PIN = 12;

// 超音波センサからシャッターまでの距離(cm)
const unsigned int DISTANCE_SHUTTER = 10;

// UDPサーバとして設定するポート
const unsigned int UDP_PORT = 4210;

WiFiUDP Udp;
char incomingPacket[255]; // buffer for incoming packets

// シャッターの開閉状態を超音波センサで取得する
bool isOpened()
{
  digitalWrite(WAVE_TRIG_PIN, LOW);
  delayMicroseconds(1);
  digitalWrite(WAVE_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(WAVE_TRIG_PIN, LOW);
  double duration = pulseIn(WAVE_ECHO_PIN, HIGH);

  // 距離を計算する(cm)
  double distance = duration * 0.0001 * 340 / 2;

  // 距離が一定以上なら開いていると判断する
  return distance > DISTANCE_SHUTTER;
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(WAVE_ECHO_PIN, INPUT);
  pinMode(WAVE_TRIG_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();

  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  Udp.begin(UDP_PORT);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), UDP_PORT);
  Serial.println();
}

void loop()
{
  // シャッターが空いていたらLEDを点灯
  if (isOpened())
  {
    digitalWrite(LED_PIN, HIGH);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

  // UDP通信
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // UDPパケットを受信する
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);

    // シャッターの開閉状態を返す
    if (strcmp(incomingPacket, "shutter") == 0)
    {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      if (isOpened())
      {
        Udp.write("OPEN");
      }
      else
      {
        Udp.write("CLOSE");
      }
    }
    else
    {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("ERROR");
    }
    Udp.endPacket();
  }
}