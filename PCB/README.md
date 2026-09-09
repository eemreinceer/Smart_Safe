# PCB kaynakları

`akilli_guvenlik_kasasi/` altındaki KiCad proje, şema ve PCB dosyaları tasarım
kaynağıdır.

Repository'deki eski Gerber, drill, zip ve schematic PDF dosyaları güncel RFID +
MOSFET mimarisini temsil etmedikleri için kaldırılmıştır. PCB tasarımı
tamamlanıp ERC/DRC kontrolleri geçmeden bu repository'den üretim dosyası
gönderilmemelidir.

Yeni üretim paketi oluşturulurken en az şu dosyalar aynı KiCad revizyonundan
yeniden üretilmelidir:

- Copper, solder mask ve silkscreen Gerber katmanları
- Edge cuts
- PTH ve NPTH drill dosyaları
- Gerber job dosyası
- Assembly/BOM dokümanları
- Görsel kontrol için schematic PDF

KiCad'in kullanıcıya ve yerel pencere durumuna özel `*.kicad_prl` dosyaları
takip edilmez.
