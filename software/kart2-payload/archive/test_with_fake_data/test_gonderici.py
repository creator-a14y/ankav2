"""
Donanimsiz test: sahte dron gibi davranip yer_istasyonu_tcp.py'ye bir foto yollar.
Once yer_istasyonu_tcp.py'yi calistir, sonra bunu ayri terminalde calistir.
test_fire.jpg'yi ayni klasore koy (bir yangin fotosu).
"""
import socket

HOST = "127.0.0.1"      # yer istasyonu ayni bilgisayarda; agdaysa onun IP'si
PORT = 5000
FOTO = "test_fire.jpg"
GPS = "39.925,32.866"

with open(FOTO, "rb") as f:
    foto = f.read()

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.sendall(f"GPS:{GPS}\n".encode())
s.sendall(f"LEN:{len(foto)}\n".encode())
s.sendall(foto)
s.close()
print(f"Gonderildi: {len(foto)} bayt, konum {GPS}")
