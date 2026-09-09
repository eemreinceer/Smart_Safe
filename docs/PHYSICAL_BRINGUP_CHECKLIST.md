# SmartSafe Physical Bring-Up Checklist

Bu kontrol listesi tamamlanmadan `esp32cam-production` firmware'i fiziksel sisteme
yüklenmemeli ve solenoid beslemesi uygulanmamalıdır. Firmware build başarısı,
elektriksel güvenlik veya mekanik kilit geri bildirimi kanıtı değildir.

## 1. Enerji vermeden önce — NO-GO kapısı

- [ ] Güncel şema, PCB ve Gerber aynı revizyonu temsil ediyor.
- [ ] ESP32-CAM, RC522, DFPlayer ve MOSFET pinleri gerçek kablolamayla eşleşiyor.
- [ ] GPIO2, GPIO12 ve GPIO15 strapping-pin etkileri harici devreyle birlikte kontrol edildi.
- [ ] RC522 beslemesi ve tüm SPI sinyalleri 3.3 V seviyesinde.
- [ ] Solenoid akımı ESP32/logic regülatöründen geçmiyor.
- [ ] Logic ve actuator GND ilişkisi şemaya uygun; yük akımı logic ground yolundan geçmiyor.
- [ ] MOSFET, 3.3 V gate geriliminde ölçülen solenoid akımına uygun.
- [ ] Flyback diyot yönü ve akım kapasitesi doğrulandı.
- [ ] Besleme polaritesi, sigorta ve erişilebilir power disconnect mevcut.
- [ ] 12 V–GND, 5 V–GND ve 3.3 V–GND arasında kısa devre yok.
- [ ] Açıkta iletken, gevşek konnektör veya yetersiz strain relief yok.

Bu bölümde tek bir madde bile belirsizse durum **NO-GO**'dur.

## 2. Kademeli ilk enerji verme

- [ ] Akım limitli bench supply kullanıldı; limit beklenen idle akıma yakın başlatıldı.
- [ ] Önce yalnızca power source/regülatör, yüksüz test edildi.
- [ ] Yalnızca ESP32-CAM enerjilendirildi ve kararlı boot doğrulandı.
- [ ] RC522 tek başına eklendi ve tanımlı/tanımsız kart okuması doğrulandı.
- [ ] Kamera eklendi; capture sırasında brownout/reset olmadığı doğrulandı.
- [ ] DFPlayer eklendi; boot strapping ve UART seviyesi kontrol edildi.
- [ ] Solenoid beslemesi kapalıyken MOSFET gate komutu ölçüldü.
- [ ] Solenoid mekanik olarak yüksüz ve kısa süreli olarak ayrı test edildi.
- [ ] En son tüm sistem, akım ve voltage sag izlenerek birleştirildi.

## 3. Firmware ve security kabulü

- [ ] `include/secrets.h` Git tarafından izlenmiyor.
- [ ] En az `AUTHORIZED_UID_1` gerçek karta ait ve UID formatı doğrulandı.
- [ ] `pio run -e esp32cam-production` başarılı.
- [ ] Tanımlı kart kilidi bir kez ve 3 saniye açıyor.
- [ ] Tanımsız kart kilidi açmıyor ve audit event oluşturuyor.
- [ ] Üç hatalı deneme lockdown ve alarm oluşturuyor.
- [ ] Lockdown sonunda RFID taraması kontrollü olarak geri geliyor.
- [ ] Cloud/web panelinden kilit açma yolu bulunmuyor.
- [ ] Wi-Fi ve Firebase kapalıyken tanımlı RFID kart yerel olarak çalışıyor.
- [ ] Wi-Fi geri geldiğinde offline log ve NTP davranışı doğrulanıyor.
- [ ] Reset/brownout sonrası actuator varsayılan olarak kilitli kalıyor.
- [ ] Firebase device hesabında yalnızca `device_id: safe_001` claim'i var.
- [ ] Dashboard hesabı ile device hesabı birbirinden ayrı.

## 4. Ölçüm kaydı

| Aşama | Vin | 5 V | 3.3 V | Idle akım | Peak akım | En düşük gerilim | Regülatör/MOSFET sıcaklığı | Sonuç |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| Regülatör yüksüz | | | | | | | | |
| ESP32-CAM boot | | | | | | | | |
| RC522 okuma | | | | | | | | |
| Kamera capture | | | | | | | | |
| DFPlayer alarm | | | | | | | | |
| Solenoid tek başına | | | | | | | | |
| Entegre sistem | | | | | | | | |

Kısa transient, PWM/gate dalga şekli ve inrush için multimetre tek başına yeterli
değildir; oscilloscope veya logic analyzer kaydı alınmalıdır.

## Kabul durumu

- **SAFE TO PROCEED:** Tüm maddeler tamamlandı, değerler limit içinde ve test kanıtı kaydedildi.
- **BLOCKED:** Kısa devre, ters polarite, boot instability, brownout, aşırı akım/ısı veya fail-open davranış var.
- **UNKNOWN:** Ölçülmemiş her elektriksel veya mekanik davranış.

Mevcut repository durumu, PCB ve fiziksel ölçümler tamamlanana kadar
**software-verified / physical NO-GO** olarak kabul edilir.
