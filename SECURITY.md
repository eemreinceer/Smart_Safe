# Security Policy

## Supported scope

Bu repository bir prototype'tır. `main` branch'in son sürümü için security fix kabul edilir; eski firmware binary'leri desteklenmez.

## Vulnerability reporting

Public issue açıp credential, exploit veya kişisel veri yayınlamayın. GitHub repository sayfasındaki **Security > Report a vulnerability** private reporting kanalını kullanın.

## Secret handling

- `secrets.h`, `.env`, service-account key ve token'lar commit edilmez.
- Firebase device ve dashboard hesapları ayrı olmalıdır.
- Sızan credential yalnızca Git'ten silinmez; derhal revoke/rotate edilir.
- Production key'leri firmware image içinde extraction'a karşı mutlak gizli kabul edilmez. Device authorization minimum yetkiyle ve server-side rules ile sınırlanır.

## Security assumptions

- RC522 UID allowlist, klonlamaya dayanıklı authentication değildir.
- `is_locked`, fiziksel kilit sensörü eklenene kadar commanded state'tir.
- TLS certificate verification ve authenticated OTA zorunludur.
- Firebase rules deploy edilmeden remote unlock etkinleştirilmemelidir.
