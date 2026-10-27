# ANKA-v2: Chain Node and Fire Detection UAV Project



\[!\[Türkçe](https://shields.io)](https://github.com)



ANKA-v2 is a modular UAV and communication infrastructure project designed for autonomous fire detection, thermal image processing, GPS position fixing, and inter-node wireless data transmission (Wi-Fi Mesh/Relay).

The system consists of **detection nodes** executing search-and-scan and fire detection tasks, and **relay (chain) nodes** that forward this data to the ground control station.

\---

## 📌 Project Objective

The primary objective of the ANKA-v2 project is to autonomously detect fires at an early stage in forested, mountainous, or vast rural areas. It transmits real-time thermal/visual data and GPS coordinates from the scanned region seamlessly and rapidly to the Ground Control Station via an inter-drone wireless chain/relay (Mesh/Relay) network, without relying on cellular (GSM) networks or internet infrastructure.

\---

## 🛠️ System Architecture and Operating Principle

The system features an independent hardware architecture that separates flight control operations from payload processing:

* **Flight Controller (Board 1):** `flight32` is used as the flight controller and operates completely independently of the payload (Board 2). Failsafe verification, which cuts motor power upon RC signal loss, is tested in the field prior to flight.
* **Payload Board (Board 2):** Built on the Deneyap Card v2 (ESP32-S3), managing the thermal camera, optical camera, and GPS module.
* **Thermal Triggering (Dynamic Threshold):** The Adafruit MLX90640 thermal camera reads the ambient temperature across the scanned frame. The system registers a fire detection event when the maximum temperature simultaneously exceeds the absolute baseline ($45.0^\\circ\\text{C}$) and the differential threshold from the ambient average ($20.0^\\circ\\text{C}$).
* **Visual and Location Data Collection:** Upon thermal triggering, a JPEG image is captured using the OV2640 camera, and instantaneous coordinates (latitude and longitude from the NMEA GGA sentence) are retrieved from the Deneyap GPS/GLONASS module.
* **Chain Transmission (Relay/Mesh):** The collected data packet is forwarded to the Access Point (AP) of the next node in the chain. Inter-node communication operates in a relay manner, where data received from a preceding node is forwarded down the line.

\---

## 📊 Hardware Components and Bill of Materials (BOM)

Based on the data from `anka\_maliyet.xlsx`, the estimated budget table for 1 drone is as follows (based on September 2026 market figures):

|#|Category|Part / Product|Source / Reference|Qty|Unit Price (TL)|Total (TL)|
|-|-|-|-|-|-|-|
|1|Flight Controller|Deneyap Card v2 (ESP32-S3)|magaza.deneyapkart.org|1|727 TL|727 TL|
|2|Motor|2205 2300KV Brushless Motor (CW+CCW set)|promodelhobby.com|4|670 TL|2,680 TL|
|3|ESC|60A BLHeli\_S 4-in-1 ESC (DShot, 30.5×30.5mm)|AliExpress TR|1|1,446 TL|1,446 TL|
|4|Propeller|5045 Propeller Set (CW+CCW, 2 pairs)|f1depo.com|1|150 TL|150 TL|
|5|Battery|4S 1500mAh 95C LiPo Battery|robotzade.com|1|3,216 TL|3,216 TL|
|6|BEC|5V 3A UBEC (LiPo→5V)|robotzade.com / robotsepeti.com|1|280 TL|280 TL|
|7|IMU|MPU9250 IMU Module (I2C, 9-axis)|amazon.com.tr|1|400 TL|400 TL|
|8|Transmitter / Receiver|Flysky FS-i6X Transmitter + FS-iA6B Receiver Set|promodelhobby.com|1|4,279 TL|4,279 TL|
|9|Frame|5" FPV Quadcopter Frame (210-250mm, Carbon)|Market estimate|1|800 TL|800 TL|
|10|Power|XT60 LiPo Connector (Female+Male pair)|General market|1|75 TL|75 TL|
|11|Wiring \& Hardware|Cables, screws, standoffs, heat shrink tubing|General market|1|150 TL|150 TL|
|12|Payload Controller|Deneyap Card v2 (ESP32-S3)|magaza.deneyapkart.org|1|727 TL|727 TL|
|13|Optical Camera|Deneyap Camera (OV2640, FPC)|robolinkmarket.com|1|715 TL|715 TL|
|14|Thermal Camera|MLX90640 Thermal Camera (32×24, I2C)|direnc.net|1|3,303 TL|3,303 TL|
|15|GPS|Deneyap GPS/GLONASS Module (Quectel L86)|magaza.deneyapkart.org|1|900 TL|900 TL|
|16|Wiring|I2C Connection Cable (10cm, 2 pcs)|May come with module|2|30 TL|60 TL|
|17|Programming|USB Cable (Type-C, programming)|General market|2|100 TL|200 TL|
|18|Charger|LiPo Battery Charger (Balance charger, 4S compatible)|General market|1|700 TL|700 TL|
|||**TOTAL**||||**20,808 TL**|



