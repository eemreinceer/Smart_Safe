# RFID/NFC Compatibility Probe (standalone experiment)

SmartSafe production firmware'inden **bağımsız** bir deney projesi.
`FIRMWARE/Platform.IO` altındaki hiçbir dosya değiştirilmedi.

Tek amacı şu soruya deneysel cevap vermek:

> Elimizdeki MFRC522 okuyucu hangi kişisel kartları / NFC cihazlarını
> algılayabiliyor, hangilerinden SmartSafe için **güvenilir bir identifier**
> alabiliyoruz?

Firebase yok, kamera yok, solenoid yok, DFPlayer yok, authentication yok.

---

## Kapsam sınırı (bilinçli)

Firmware yalnızca **discovery / anti-collision / UID / kart tipi** seviyesinde
çalışır: REQA, WUPA, anti-collision, SAK, `PICC_GetType()`.

Yapılmayanlar (kodda hiç yok):

- karta veri yazma
- MIFARE sektör okuma / key deneme / brute-force
- APDU veya EMV sorgulama
- banka/kredi kartından hesap verisi çıkarma
- kimlik/ehliyet üzerindeki kişisel verilere erişme

T.C. kimlik, ehliyet ve banka/kredi kartlarında testin görebileceği tek şey
kartın RF'ye cevap verip vermediği ve (varsa) anti-collision UID'sidir.

---

## Donanım

Standart **ESP32 DevKit** (ESP32-WROOM-32) + **RC522**.
ESP32-CAM boot edemediği için bu deneyde kullanılmıyor.

### Pin tablosu (VSPI)

| RC522 | ESP32 DevKit | Not |
|---|---|---|
| SDA / SS / NSS | **GPIO5**  | VSPI CS |
| SCK            | **GPIO18** | VSPI CLK |
| MOSI           | **GPIO23** | VSPI MOSI |
| MISO           | **GPIO19** | VSPI MISO |
| RST            | **GPIO22** | boşta, strapping pini değil |
| 3.3V           | **3V3**    | **5V bağlama** |
| GND            | **GND**    | ortak toprak |
| IRQ            | bağlanmıyor | firmware polling yapıyor |

Pin seçimi notları:
- GPIO5 boot sırasında strapping pinidir ama harici pull-up/pull-down yoksa
  sorun çıkarmaz; RC522 CS'i yüksek empedanslıdır. Boot problemi yaşarsan
  SS'i GPIO4'e alıp `PIN_SS`'i güncelle.
- GPIO22 (eski I2C SCL) bu projede boştur.
- GPIO6–11 flash'a bağlıdır, **kullanma**.
- 12, 0, 2, 15 strapping pinleridir; bu projede kullanılmıyor.

### Besleme

RC522 **3.3 V** ile beslenir. ESP32'nin 3V3 çıkışı RC522 için yeterlidir, ancak
USB üzerinden zayıf besleme durumunda okuma menzili düşer. Kart algılanmıyorsa
ilk şüphelenilecek şey beslemedir.

---

## Build / flash / monitor

```bash
cd EXPERIMENTS/rfid_nfc_probe

pio run                      # derle
pio run -t upload            # ESP32 DevKit'e yükle
pio device monitor -b 115200 # seri monitör

# port belirtmek gerekirse:
pio run -t upload --upload-port /dev/ttyUSB0
pio device monitor -b 115200 -p /dev/ttyUSB0
```

Windows'ta port `COM3` gibi olur.

> **Not:** Bu container'da `api.registry.platformio.org` ve `dl.espressif.com`
> ağ politikası tarafından bloklu olduğu için espressif32 platformu ve MFRC522
> paketi indirilemedi; tam ESP32 derlemesi burada çalıştırılamadı.
> `src/main.cpp` bunun yerine gerçek MFRC522 1.4.11 başlıklarına karşı
> host derleyicisiyle `-Wall -Wextra` ile syntax/API kontrolünden temiz geçti.
> İlk `pio run` senin makinende paketleri indirecek.

---

## Seri komutlar

| Komut | Etki |
|---|---|
| `n<etiket>` | yeni test oturumu başlat, ör. `n izmirim kart` |
| `r` | stabilite raporu + A/B/C sınıfı |
| `v` | RC522 versiyon / self-test |
| `h` | yardım |

## Test prosedürü (her nesne için)

1. `n <nesne adı>` yaz → yeni oturum.
2. Nesneyi antene yaklaştır, **kaldır**, tekrar yaklaştır — toplam **5 kez**.
   (Kart antenin üstünde tutulurken log basılmaz; yeni ölçüm ancak kart
   kaldırılıp tekrar yaklaştırılınca sayılır.)
3. 5 okuma tamamlanınca firmware uyarır.
4. `r` yaz → rapor + sınıf.
5. Sonucu `RESULTS.md` tablosuna yaz.

---

## Örnek çıktı

```text
----------------------------------------
NEW RFID/NFC TARGET
Label         : rc522 beyaz kart
Measurement # 2
Card detected : YES
ATQA          : 0x0400
Wake command  : REQA (0x26)
UID           : 04:71:AB:39
UID length    : 4 bytes
SAK           : 0x08
PICC type     : MIFARE 1KB
vs previous   : SAME UID
Progress      : 2 / 5 reads
Remove target and present again for
stability testing.  ('r' = report)
----------------------------------------
```

---

## Üç ayrı özellik

Firmware bunları **ayrı ayrı** raporlar, çünkü aynı şey değildirler:

```text
DETECTED     -> RF alanına ISO14443A cevabı verdi (ATQA geldi)
UID READABLE -> anti-collision tamamlandı, UID alındı
UID STABLE   -> tekrar okumalarda UID değişmedi
```

`Detected: YES / UID readable: YES / UID stable: NO` tamamen mümkün bir
sonuçtur ve SmartSafe için **kullanılamaz** demektir.

## Sınıflandırma

| Sınıf | Anlamı |
|---|---|
| **A** | Algılanıyor + UID okunuyor + UID stabil → SmartSafe V1 için doğrudan uygun |
| **B** | RC522 görüyor ama UID authentication için uygun değil (rastgele UID, aktivasyon hatası, değişken UID) |
| **C** | RC522 + mevcut MFRC522 kütüphanesi ile algılanamıyor |

**C, "cihazda NFC yok" demek değildir.** Yalnızca şu demektir:

> MFRC522 + bu ISO14443A discovery firmware'i kombinasyonu bu cihazla
> iletişim kuramadı.

iPhone için beklenen sonuç büyük ihtimalle C'dir ve bu normaldir: iPhone
pasif bir ISO14443A kartı gibi davranmaz, sabit bir UID yayınlamaz.
Bunu "iPhone NFC çalışmıyor" diye kaydetme; şöyle kaydet:

> RC522 + mevcut ISO14443A/UID probe ile kullanılabilir credential elde edilemedi.

Android'de de bu aşamada pasif test yapılır. HCE uygulaması **yazılmayacak**;
o ayrı bir deney.

### Rastgele UID (önemli)

ISO/IEC 14443-3'e göre `0x08` ile başlayan 4 baytlık UID **rastgele ID**'dir ve
her aktivasyonda değişmesi beklenir. Temassız banka/kredi kartları ve bazı
kimlik belgeleri bunu kullanır. Firmware bunu tespit edip işaretler → sınıf B.

---

## Sonuçlar

Ölçüm sonuçları `RESULTS.md` dosyasında. Tablo **yalnızca fiziksel test
sonuçlarıyla** doldurulur, tahminle değil.
