"""
Yer istasyonu - FUSION: Termal tetik + AI onay
Mantik: termal 60C+ gorurse -> RGB fotoya bak -> AI onaylarsa ALARM.
Termal esigin altindaysa AI HIC calismaz (duvar/smoke false-positive burada elenir).

Donanimsiz test edilir: sahte termal matris + kendi koydugun test fotograflari.
DONANIM GELINCE degistirilecek yerler "### DONANIM ###" ile isaretli.
"""

from ultralytics import YOLO
import numpy as np
import cv2
from datetime import datetime

# --- AYARLAR ---
MODEL_YOLU = "best.pt"          # egitilmis model (ayni klasorde)
TERMAL_ESIK = 60.0              # °C, bunun ustu tetikler
AI_GUVEN_ESIGI = 0.5            # AI bu guvenin altini saymaz
KABUL_EDILEN_SINIFLAR = []      # [] = hepsi, ["fire"] = sadece ates (smoke'u kapatir)


# --- 1. TERMAL TETIK ---
def termal_tetik(matris):
    """MLX90640 sicaklik matrisinde esigi asan piksel var mi?"""
    max_sic = float(np.max(matris))
    return (max_sic >= TERMAL_ESIK), max_sic


# --- 2. AI ONAY ---
def ai_onay(model, foto, izinli_idx):
    """RGB fotoda yangin/duman var mi? (sinif, guven) listesi doner."""
    sonuc = model.predict(foto, conf=AI_GUVEN_ESIGI, classes=izinli_idx, verbose=False)[0]
    tespitler = [(model.names[int(k.cls)], round(float(k.conf), 2)) for k in sonuc.boxes]
    return (len(tespitler) > 0), tespitler


# --- 3. FUSION KARAR ---
def karar_ver(model, izinli_idx, termal_matris, foto_yolu, gps):
    tetik, max_sic = termal_tetik(termal_matris)
    print(f"Termal: max {max_sic:.1f}C -> tetik = {'EVET' if tetik else 'hayir'}")

    if not tetik:
        print("  -> Termal esigin altinda. AI calistirilmadi. ALARM YOK.\n")
        return False

    foto = cv2.imread(foto_yolu)                       ### DONANIM ###: Deneyap kameradan gelen foto
    if foto is None:
        print(f"  HATA: foto okunamadi -> {foto_yolu}\n")
        return False

    onay, tespitler = ai_onay(model, foto, izinli_idx)
    print(f"  AI tespit: {tespitler if tespitler else 'yok'}")

    if onay:
        print(f"  -> ALARM! konum={gps}  zaman={datetime.now():%H:%M:%S}\n")
        return True
    else:
        print("  -> AI dogrulamadi (muhtemel false-positive). ALARM YOK.\n")
        return False


# --- YARDIMCI: sahte termal matris (test icin) ---
def sahte_termal(sicak_nokta=False):
    m = np.random.normal(24, 1.5, (24, 32))            # ortam ~24C
    if sicak_nokta:
        m[10:14, 14:18] = np.random.normal(85, 3, (4, 4))  # yangin gibi sicak bolge
    return m


def main():
    model = YOLO(MODEL_YOLU)
    print("Model siniflari:", model.names, "\n")

    if KABUL_EDILEN_SINIFLAR:
        izinli_idx = [i for i, a in model.names.items()
                      if a.lower() in [s.lower() for s in KABUL_EDILEN_SINIFLAR]]
    else:
        izinli_idx = None

    gps = (39.925, 32.866)                             ### DONANIM ###: drondan gelen gercek GPS

    # === SENARYOLAR (donanimsiz test) ===
    # test_fire.jpg  = internetten bir yangin fotosu
    # test_normal.jpg = kendi odan/duvarin fotosu
    print("=== A: sicak termal + yangin fotosu  (ALARM beklenir) ===")
    karar_ver(model, izinli_idx, sahte_termal(True), "test_fire.jpg", gps)

    print("=== B: sicak termal + normal/duvar foto  (AI elemeli, ALARM YOK) ===")
    karar_ver(model, izinli_idx, sahte_termal(True), "test_normal.jpg", gps)

    print("=== C: soguk termal + yangin fotosu  (termal gecmez, AI hic calismaz) ===")
    karar_ver(model, izinli_idx, sahte_termal(False), "test_fire.jpg", gps)


if __name__ == "__main__":
    main()
