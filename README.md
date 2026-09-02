# ANKA-v2 Zincir Düğüm ve Yangın Algılama İHA Projesi

ANKA-v2; otonom yangın algılama, termal görüntü işleme, GPS konum tespiti ve düğümler arası kablosuz (WiFi Mesh/Relay) veri aktarımı gerçekleştirmek üzere tasarlanmış modüler bir İHA ve haberleşme altyapısı projesidir.

Proje; arama-tarama ve yangın tespit görevlerini icra eden tespit düğümü ile bu verileri yer kontrol istasyonuna aktaran röle (zincir) düğümlerinden oluşur.

---

## Sistem Mimarisi ve Çalışma Prensibi

Sistem, uçuş kontrolü ve faydalı yük (payload) işlemlerini birbirinden ayıran bağımsız bir donanım mimarisine sahiptir:

* **Uçuş Kartı (Kart 1):** Uçuş kontrolcüsü olarak `flight32` kullanılmaktadır. Payloaddan (Kart 2) tamamen bağımsız çalışır. RC sinyali kesildiğinde motorları kesen Failsafe doğrulaması uçuş öncesinde sahada test edilir.
* **Payload Kartı (Kart 2):** Deneyap Kart v2 (ESP32-S3) üzerindedir. Termal kamera, kamera ve GPS modülünü yönetir.
* **Termal Tetikleme (Dinamik Eşik):** Adafruit MLX90640 termal kamerası ortam sıcaklığını taranan kare üzerinden okur. Maksimum sıcaklık mutlak tabanı ($45.0^{\circ}C$) ve ortam ortalamasından fark eşiğini ($20.0^{\circ}C$) aynı anda aştığında sistem yangın tespiti yapar.
* **Görsel ve Konum Verisi Toplama:** Termal tetikleme sağlandığında OV2640 kameradan JPEG fotoğraf çekilir ve Deneyap GPS/GLONASS modülünden anlık koordinatlar (GGA cümlesinden enlem ve boylam) okunur.
* **Zincir Aktarımı (Relay/Mesh):** Elde edilen veri paketi, zincirdeki bir sonraki düğümün Erişim Noktasına (AP) iletilir. Düğümler arası iletişim nöbetleşe yapıdadır; arka düğümden gelen veri forward edilir (relaylenir).

---

## Donanım Bileşenleri ve Maliyet Özeti (BOM)

Yüklenen `anka_maliyet.xlsx` dosyasındaki verilere göre 1 dron için tahmini bütçe tablosu aşağıdadır (Eylül 2026 piyasa verilerine göre):

| # | Kategori | Parça / Ürün | Kaynak / Referans | Adet | Birim (TL) | Toplam (TL) |
|---|---|---|---|:---:|:---:|:---:|
| **1** | Uçuş Kartı | Deneyap Kart v2 (ESP32-S3) | magaza.deneyapkart.org | 1 | 727 TL | 727 TL |
| **2** | Motor | 2205 2300KV Fırçasız Motor (CW+CCW set) | promodelhobby.com | 4 | 670 TL | 2.680 TL |
| **3** | ESC | 60A BLHeli_S 4-in-1 ESC (DShot, 30.5×30.5mm) | AliExpress TR | 1 | 1.446 TL | 1.446 TL |
| **4** | Pervane | 5045 Pervane seti (CW+CCW, 2 çift) | f1depo.com | 1 | 150 TL | 150 TL |
| **5** | Batarya | 4S 1500mAh 95C LiPo Batarya | robotzade.com | 1 | 3.216 TL | 3.216 TL |
| **6** | BEC | 5V 3A UBEC (LiPo→5V) | robotzade.com / robotsepeti.com | 1 | 280 TL | 280 TL |
| **7** | IMU | MPU9250 IMU Modülü (I2C, 9 eksen) | amazon.com.tr | 1 | 400 TL | 400 TL |
| **8** | Kumanda / Alıcı | Flysky FS-i6X Kumanda + FS-iA6B Alıcı seti | promodelhobby.com | 1 | 4.279 TL | 4.279 TL |
| **9** | Şasi | 5" FPV Quadcopter Çerçeve (210-250mm, karbon) | Piyasa tahmini | 1 | 800 TL | 800 TL |
| **10** | Güç | XT60 LiPo Konnektörü (dişi+erkek çift) | Genel piyasa | 1 | 75 TL | 75 TL |
| **11** | Bağlantı | Kablo, vida, standoff, ısıyla küçülen boru | Genel piyasa | 1 | 150 TL | 150 TL |
| **12** | Payload Kartı | Deneyap Kart v2 (ESP32-S3) | magaza.deneyapkart.org | 1 | 727 TL | 727 TL |
| **13** | Kamera | Deneyap Kamera (OV2640, FPC) | robolinkmarket.com | 1 | 715 TL | 715 TL |
| **14** | Termal Kamera | MLX90640 Termal Kamera (32×24, I2C) | direnc.net | 1 | 3.303 TL | 3.303 TL |
| **15** | GPS | Deneyap GPS/GLONASS Modülü (Quectel L86) | magaza.deneyapkart.org | 1 | 900 TL | 900 TL |
| **16** | Bağlantı | I2C Bağlantı Kablosu (10cm, 2 adet) | Modül ile gelebilir | 2 | 30 TL | 60 TL |
| **17** | Programlama | USB Kablo (Type-C, programlama) | Genel piyasa | 2 | 100 TL | 200 TL |
| **18** | Şarj Aleti | LiPo Şarj Aleti (dengeli şarj, 4S uyumlu) | Genel piyasa | 1 | 700 TL | 700 TL |
| **TOPLAM** | | | | | | **20.808 TL** |

---

## Projenin Amacı

ANKA-v2 projesinin temel amacı; ormanlık, dağlık veya geniş karasal alanlarda meydana gelebilecek yangınları **otonom olarak erken aşamada tespit etmek**, taranan bölgedeki anlık termal/görsel verileri ve GPS koordinatlarını **GSM (hücresel ağ) veya internet altyapısına ihtiyaç duymadan**, dronlar arası kurulan kablosuz zincir/röle (Mesh/Relay) haberleşme ağı üzerinden **Yer Kontrol İstasyonuna kesintisiz ve hızlı bir şekilde aktarmaktır**.
