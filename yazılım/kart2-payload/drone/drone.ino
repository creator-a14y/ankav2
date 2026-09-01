/*
  ZINCIR DUGUM FIRMWARE ISKELETI - Deneyap Kart V2 (ESP32-WROVER-E, PSRAM'li)

  Her dugum: kendi AP'sini kurar (arkadaki baglanir) + onundeki dugumun AP'sine
  STA baglanir (veriyi ileri tasir). Akis: tespit eden dron -> role(ler) -> yer ist.

  Bu surumde DOLDURULDU:
   - termalTetik()  -> Adafruit MLX90640 (I2C), DINAMIK esik: ortam ortalamasindan
                       fark + mutlak taban (guneste isinan yuzeyleri eler)
   - foto cekme     -> esp_camera (OV2640, JPEG)
  HALA TODO:
   - gpsOku()       -> GPS modulu baglaninca
   - KAMERA PIN HARITASI -> Deneyap kamera orneginden kopyalanacak (asagida)
   - FAILSAFE DOGRULAMASI: bu dosya uctan bagimsiz calisir (flight32 ayri kartta).
     Uçuş kartinin RC sinyali kesilince motorlari kestigi SAHADA test edilmeli -
     ilk ucustan once atlanmamasi gereken guvenlik adimi.

  !! DERLENIP TEST EDILMEDI !! Deneyap board paketi + kutuphaneler + donanim gerekir.
  Gerekli kutuphaneler (Arduino Library Manager):
    - "Adafruit MLX90640"  (+ bagimliliklarini kabul et)
    - esp_camera: Deneyap/ESP32 arduino core ile birlikte gelir (ayri kurulum yok)
*/

#include <WiFi.h>
#include "esp_camera.h"
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include "Deneyap_GPSveGLONASSkonumBelirleyici.h"   // Deneyap GPS (I2C), Library Manager'dan kur

// ==================== HER DUGUMDE DEGISEN AYARLAR ====================
const char* UPLINK_SSID = "ZINCIR_1";          // ONUMDEKI (GS'e yakin) dugumun AP adi
const char* UPLINK_PASS = "12345678";
const char* MY_AP_SSID  = "ZINCIR_2";          // BENIM AP adim (arkadaki buna baglanir)
const char* MY_AP_PASS  = "12345678";
IPAddress   MY_AP_IP(192, 168, 20, 1);         // HER DUGUMDE FARKLI subnet
// ====================================================================

const uint16_t PORT = 5000;

// --- DINAMIK TERMAL ESIK ---
// Guneste isinan cati/kaya/asfalt gibi yuzeyler tek basina 60C'yi asabilir ama
// CEVRESIYLE BIRLIKTE isinir (fark kucuk kalir). Gercek yangin/sicak nokta,
// cevresinden KESKIN sekilde farklidir. Iki sart AYNI ANDA saglanmali:
//   1) maks sicaklik, kare ORTALAMASINDAN en az TERMAL_FARK_ESIK derece yuksek
//   2) maks sicaklik, mutlak taban TERMAL_TABAN'i gecmis (cok soguk gunde
//      relatif fark tek basina yaniltmasin diye guvenlik payi)
const float TERMAL_TABAN     = 45.0;   // °C, bunun altinda hicbir zaman tetiklemez
const float TERMAL_FARK_ESIK = 20.0;   // °C, ortam ortalamasindan fark


// ============ KAMERA PIN HARITASI (Deneyap Kart 1A v2 - ESP32-S3, FPC soketi) ============
// V2 core kamera pinlerini CAMD2..CAMD9, CAMSD, CAMSC, CAMXC, CAMPC, CAMV, CAMH
// sabitleriyle KENDISI tanimlar -> otoriter kaynak. GPIO'lar yorumda.
// (Not: eski Deneyap Kart tablosu (D5/A2/DAC...) V2'ye UYMUYOR.)
#define CAM_PIN_D0     CAMD2   // GPIO41
#define CAM_PIN_D1     CAMD3   // GPIO2
#define CAM_PIN_D2     CAMD4   // GPIO1
#define CAM_PIN_D3     CAMD5   // GPIO42
#define CAM_PIN_D4     CAMD6   // GPIO40
#define CAM_PIN_D5     CAMD7   // GPIO38
#define CAM_PIN_D6     CAMD8   // GPIO17
#define CAM_PIN_D7     CAMD9   // GPIO15
#define CAM_PIN_SIOD   CAMSD   // GPIO4  (SCCB veri)
#define CAM_PIN_SIOC   CAMSC   // GPIO5  (SCCB clock)
#define CAM_PIN_XCLK   CAMXC   // GPIO16
#define CAM_PIN_PCLK   CAMPC   // GPIO39
#define CAM_PIN_VSYNC  CAMV    // GPIO6
#define CAM_PIN_HREF   CAMH    // GPIO7
#define CAM_PIN_PWDN   -1
#define CAM_PIN_RESET  -1
// ================================================================================

Adafruit_MLX90640 mlx;
GPS konum;                                        // Deneyap GPS. 'gps' ismi kutuphanede kullanildigi icin 'konum' dedik
float termalKare[32 * 24];                      // 768 piksel
WiFiServer server(PORT);


void kameraInit() {
  camera_config_t c;
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = CAM_PIN_D0;  c.pin_d1 = CAM_PIN_D1;  c.pin_d2 = CAM_PIN_D2;  c.pin_d3 = CAM_PIN_D3;
  c.pin_d4 = CAM_PIN_D4;  c.pin_d5 = CAM_PIN_D5;  c.pin_d6 = CAM_PIN_D6;  c.pin_d7 = CAM_PIN_D7;
  c.pin_xclk  = CAM_PIN_XCLK;
  c.pin_pclk  = CAM_PIN_PCLK;
  c.pin_vsync = CAM_PIN_VSYNC;
  c.pin_href  = CAM_PIN_HREF;
  c.pin_sccb_sda = CAM_PIN_SIOD;   // eski core'da isim: pin_sscb_sda (derleme hatasi olursa degistir)
  c.pin_sccb_scl = CAM_PIN_SIOC;   // eski core'da isim: pin_sscb_scl
  c.pin_pwdn  = CAM_PIN_PWDN;
  c.pin_reset = CAM_PIN_RESET;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;              // sikistirilmis -> gonderime uygun
  c.frame_size   = FRAMESIZE_VGA;               // 640x480, zincirde hizli tasinir
  c.jpeg_quality = 12;                          // 10-15 (kucuk sayi = daha kaliteli/buyuk)
  c.fb_count     = 2;                           // PSRAM var (WROVER)
  c.fb_location  = CAMERA_FB_IN_PSRAM;          // eski core'da yoksa bu satiri sil
  c.grab_mode    = CAMERA_GRAB_LATEST;          // eski core'da yoksa bu satiri sil

  if (esp_camera_init(&c) != ESP_OK)
    Serial.println("HATA: kamera baslatilamadi (pin haritasini kontrol et).");
}

// Kare 32 genislik x 24 yukseklik olarak duzlestirilmis (satir-satir, row-major)
const int TERMAL_GENISLIK = 32;
const int TERMAL_YUKSEKLIK = 24;

bool termalTetik() {
  if (mlx.getFrame(termalKare) != 0) return false;   // okuma hatasi

  float toplam = 0;
  for (int i = 0; i < 768; i++) toplam += termalKare[i];
  float ortalama = toplam / 768.0;

  // Tekil piksel yerine 3x3 blok ortalamasi al - tekil parazit/dead pixel
  // yanlis tetiklemeyi (spurious trigger) onler. En sicak 3x3 blok "maks" sayilir.
  float maksBlok = -1000;
  for (int y = 0; y <= TERMAL_YUKSEKLIK - 3; y++) {
    for (int x = 0; x <= TERMAL_GENISLIK - 3; x++) {
      float blokToplam = 0;
      for (int dy = 0; dy < 3; dy++)
        for (int dx = 0; dx < 3; dx++)
          blokToplam += termalKare[(y + dy) * TERMAL_GENISLIK + (x + dx)];
      float blokOrt = blokToplam / 9.0;
      if (blokOrt > maksBlok) maksBlok = blokOrt;
    }
  }
  float fark = maksBlok - ortalama;

  // iki sart AYNI ANDA: mutlak taban VE ortamdan keskin fark
  return (maksBlok >= TERMAL_TABAN) && (fark >= TERMAL_FARK_ESIK);
}

String gpsOku() {
  // Deneyap GPS'ten konum. GGA cumlesi konum icerir. Fix yoksa 0.0,0.0 doner.
  if (konum.readGPS(GGA)) {
    double lat = konum.readLocationLat();
    double lng = konum.readLocationLng();
    return String(lat, 6) + "," + String(lng, 6);
  }
  return "0.0,0.0";   // fix yok / okuma basarisiz (ic mekanda normal)
}

// Onumdeki dugume (gateway) paketi gonder
void ileriGonder(uint8_t* foto, size_t boyut, String gps) {
  WiFiClient c;
  if (c.connect(WiFi.gatewayIP(), PORT)) {
    c.print("GPS:"); c.print(gps);   c.print("\n");
    c.print("LEN:"); c.print(boyut); c.print("\n");
    if (foto && boyut) c.write(foto, boyut);
    c.stop();
    Serial.println("Ileri gonderildi.");
  } else {
    Serial.println("HATA: uplinke baglanilamadi.");
  }
}

// Arkadan gelen paketi aynen ileri aktar (blok bazli, byte-byte degil - overhead'i dusurur)
void relayle(WiFiClient& arka) {
  WiFiClient c;
  if (!c.connect(WiFi.gatewayIP(), PORT)) {
    Serial.println("HATA: relay icin uplinke baglanilamadi, paket dusuyor.");
    arka.stop();
    return;
  }
  uint8_t buf[256];
  while (arka.connected() || arka.available()) {
    size_t hazir = arka.available();
    if (hazir) {
      size_t okunan = arka.read(buf, min(hazir, sizeof(buf)));
      c.write(buf, okunan);
    } else if (!arka.connected()) {
      break;
    }
  }
  c.stop();
  arka.stop();
}

// Uplink baglantisini bloklamadan kontrol/yeniden dener (setup'ta sonsuz beklemeyi onler)
unsigned long sonUplinkDenemesi = 0;
const unsigned long UPLINK_DENEME_ARALIGI_MS = 3000;

void uplinkKontrolEt() {
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long simdi = millis();
  if (simdi - sonUplinkDenemesi < UPLINK_DENEME_ARALIGI_MS) return;  // henuz erken, tekrar deneme
  sonUplinkDenemesi = simdi;
  Serial.println("Uplink baglantisi yok, tekrar deneniyor...");
  WiFi.begin(UPLINK_SSID, UPLINK_PASS);   // non-blocking; sonucu bir sonraki kontrolde gorulur
}

void setup() {
  Serial.begin(115200);

  // Termal (Deneyap I2C konnektoru - varsayilan SDA/SCL)
  Wire.begin();
  Wire.setClock(400000);                              // 400kHz Fast Mode - termal okumayi hizlandirir
  if (!mlx.begin(0x33, &Wire)) Serial.println("HATA: MLX90640 bulunamadi.");
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_4_HZ);

  // GPS (ayni I2C hatti, adres 0x2F - termal 0x33 ile cakismaz)
  if (!konum.begin()) Serial.println("HATA: GPS bulunamadi.");

  // Kamera
  kameraInit();

  // Kamera warm-up: DMA tamponundaki olasi "bayat" ilk kareyi at (esp_camera_fb_get
  // sensor acilisinda/ilk karede eski/gecersiz veri dondurebilir)
  camera_fb_t* isinma = esp_camera_fb_get();
  if (isinma) esp_camera_fb_return(isinma);

  // WiFi zincir (AP + STA) - AP HER ZAMAN acilir, uplink olmasa bile arkadaki dugum baglanabilsin
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(MY_AP_IP, MY_AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(MY_AP_SSID, MY_AP_PASS);
  WiFi.begin(UPLINK_SSID, UPLINK_PASS);

  // Uplink'i EN FAZLA ~5sn bekle, sonra bloklamadan devam et - server.begin() her turlu calisir
  unsigned long baslangic = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - baslangic < 5000) {
    delay(200);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nUplink OK. Ileri adres: ");
    Serial.println(WiFi.gatewayIP());
  } else {
    Serial.println("\nUplink henuz yok, AP modunda devam ediliyor (loop icinde tekrar denenecek).");
  }

  server.begin();   // uplink olsun olmasin BASLAR - arkadaki dugumu asla bekletmez
}

void loop() {
  uplinkKontrolEt();   // uplink kopmussa bloklamadan periyodik yeniden dener

  // A) TESPIT: yangin gordu mu -> foto cek -> ileri gonder
  if (termalTetik()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      String gps = gpsOku();
      ileriGonder(fb->buf, fb->len, gps);
      esp_camera_fb_return(fb);                 // ONEMLI: buffer'i geri ver
    } else {
      Serial.println("HATA: foto alinamadi.");
    }
    delay(2000);                                // ayni yangin icin spam'i onle
  }

  // B) RELAY: arkadaki dugumden veri geldiyse ileri tasi
  WiFiClient arka = server.available();
  if (arka) relayle(arka);
}
