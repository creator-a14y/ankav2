"""
Yer Istasyonu - TCP alici + AI (zincirin sonundaki laptop)
Firmware'den (zincir_dugum.ino) gelen paketi alir, best.pt ile yangin onaylar.

Paket formati (firmware ile ayni):
    GPS:<enlem,boylam>\n
    LEN:<bayt_sayisi>\n
    <JPEG baytlari>

Foto geldiyse termal zaten tetiklemis demektir; burada AI son karari verir.
"""

import socket
import os
from datetime import datetime
import numpy as np
import cv2
from ultralytics import YOLO

# --- AYARLAR ---
MODEL_YOLU = "yolo26n.pt"
GUVEN_ESIGI = 0.25
KABUL_EDILEN_SINIFLAR = []          # [] = hepsi, ["fire"] = sadece ates
HOST = "0.0.0.0"                    # tum arayuzlerde dinle
PORT = 5000                        # firmware'deki PORT ile ayni
ARSIV_KLASORU = "alarmlar"


def paketi_oku(conn):
    """GPS + LEN basliklarini ve ardindan LEN kadar JPEG baytini oku."""
    veri = b""
    while veri.count(b"\n") < 2:                     # iki basligi tamamla
        parca = conn.recv(4096)
        if not parca:
            return None, None
        veri += parca

    gps_satiri, _, kalan = veri.partition(b"\n")     # 1. satir: GPS:...
    len_satiri, _, govde = kalan.partition(b"\n")     # 2. satir: LEN:...

    gps = gps_satiri.decode(errors="ignore").replace("GPS:", "").strip()
    try:
        boyut = int(len_satiri.decode(errors="ignore").replace("LEN:", "").strip())
    except ValueError:
        return None, None

    while len(govde) < boyut:                         # foto'nun tamamini bekle
        parca = conn.recv(4096)
        if not parca:
            break
        govde += parca

    return gps, govde[:boyut]


def main():
    model = YOLO(MODEL_YOLU)
    izinli_idx = ([i for i, a in model.names.items()
                   if a.lower() in [s.lower() for s in KABUL_EDILEN_SINIFLAR]]
                  if KABUL_EDILEN_SINIFLAR else None)
    os.makedirs(ARSIV_KLASORU, exist_ok=True)

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    print(f"Yer istasyonu dinliyor: {HOST}:{PORT}  (cikis: Ctrl+C)")

    while True:
        conn, addr = s.accept()
        try:
            gps, foto = paketi_oku(conn)
        finally:
            conn.close()

        if not foto:
            print(f"Bozuk/eksik paket ({addr[0]})")
            continue

        kare = cv2.imdecode(np.frombuffer(foto, np.uint8), cv2.IMREAD_COLOR)
        if kare is None:
            print(f"JPEG cozulemedi ({addr[0]})")
            continue

        sonuc = model.predict(kare, conf=GUVEN_ESIGI, classes=izinli_idx, verbose=False)[0]
        tespitler = [(model.names[int(k.cls)], round(float(k.conf), 2)) for k in sonuc.boxes]

        if tespitler:
            for k in sonuc.boxes:
                x1, y1, x2, y2 = map(int, k.xyxy[0])
                cv2.rectangle(kare, (x1, y1), (x2, y2), (0, 0, 255), 2)
            ad = datetime.now().strftime("yangin_%Y%m%d_%H%M%S.png")
            cv2.imwrite(os.path.join(ARSIV_KLASORU, ad), kare)
            print(f"ALARM  konum={gps}  tespit={tespitler}  -> {ARSIV_KLASORU}/{ad}")
        else:
            print(f"temiz  konum={gps}  (AI dogrulamadi, alarm yok)")


if __name__ == "__main__":
    main()
