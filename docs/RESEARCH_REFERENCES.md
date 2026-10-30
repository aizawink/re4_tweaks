# Referensi implementasi Aiza Chaos v3

Referensi di bawah dipakai untuk membaca alur dan format. Implementasi Director
ditulis untuk fork UHD ini; alamat GameCube, signature baru yang belum diperiksa,
dan kode pemuatan modul dari platform lain tidak disalin menjadi hook PC.

| Sumber primer | Bukti yang dipakai | Pemakaian dalam fork ini |
|---|---|---|
| [nipkownix/re4_tweaks — ModExpansion.cpp](https://github.com/nipkownix/re4_tweaks/blob/master/dllmain/ModExpansion.cpp) | Upstream sudah me-resolve `EmSetEvent`, `EmSetFromList2`, dan hook inisialisasi list | Director memanggil pointer native yang sudah ada; tidak membuat alamat spawn baru |
| [SDK dan hook pada base fork](https://github.com/aizawink/re4_tweaks/tree/92c0208bd09c29c9640e13a2493b8e6bd6edd700/dllmain) | `EM_LIST`, `cEm`, `cManager`, room, GUID, HP int16, dan scheduler | Layout objek dan akses native berasal dari source UHD yang dibangun bersama mod |
| [adonis-singh/re4 — em_set.cpp](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/src/game/em_set.cpp) | Alur ESL vs event spawn, penggandaan posisi short × 10, indeks event 255, hasil null/error dan lifecycle list | Rancangan antrean, verifikasi koordinat pada native init, kepemilikan tambahan terpisah dari musuh ESL; ini referensi GameCube, bukan bukti ABI UHD |
| [adonis-singh/re4 — em15_set.cpp](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/src/em15/em15_set.cpp) dan [em16_set.cpp](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/src/em16/em16_set.cpp) | Type/variant dan weapon setup dapat mengubah penampilan; nama ID saja tidak cukup | Mode chainsaw memerlukan descriptor native yang cocok, termasuk type dan equipment. Tidak sekadar menulis ID musuh |
| [christianmateus/RE4_Enemy_Editor — Enemy.cs](https://github.com/christianmateus/RE4_Enemy_Editor/blob/master/Enemy.cs) | Entri serialized memisahkan subtype, animation, sight, equipment, weapon, HP dan posisi | Replacement mempertahankan byte sight serta routine/lifecycle; hanya penampilan dari template teramati yang dipinjam |
| [JADERLINK/Re4QuadNewAge](https://github.com/JADERLINK/Re4QuadNewAge) | Editor enemy/ESL menampilkan properti terpisah; berguna untuk cross-check format | Referensi pendukung, bukan loader model atau sumber alamat runtime |
| [emoose/re4-research — RE4_ESL.bt](https://github.com/emoose/re4-research/blob/master/file-formats/RE4_ESL.bt) | Dokumentasi format ESL | Cross-check struktur data terhadap SDK fork; game tetap menentukan resource yang tersedia |
| [adonis-singh/re4 — read.cpp](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/src/game/read.cpp) | Pemilihan/pemuatan resource musuh terpisah dari pembuatan actor | Alasan v3 membatasi template pada room aktif; belum ada loader lintas room yang terverifikasi untuk UHD |
| [adonis-singh/re4 — character.py](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/tools/motion/character.py) dan [simbol em10](https://github.com/adonis-singh/re4/blob/9b76ff003081d6ece8b838453fb8310743a972c7/config/G4BE08/modules/em10/symbols.txt) | Ada routine kick khusus; state player-action bukan bukti kick tunggal | F9 adalah blast manual. Auto Bowling diberi label inference eksperimental; belum mengklaim hook kick native UHD |

## Yang benar-benar dibuktikan pengujian

`tests/test_director_runtime.py` mengompilasi badan `Director.cpp` produksi
dengan pool dan native creator tiruan, memakai AddressSanitizer dan
UndefinedBehaviorSanitizer. Ia menguji koordinat, antrean, spawn gagal,
kepemilikan pointer+GUID, skip grab saat Clear, pemulihan descriptor tanpa
membalik lifecycle, penghitung replacement, wave, elite dan scene change.
`director_core_test.cpp` memeriksa seed, template key, rentang posisi, overflow
dan cone Bowling. Tes Chaos lama tetap dijalankan, termasuk F9 arah depan.

Build Windows Debug/Release memeriksa source, SDK, linkage dan pengemasan DLL.
Build dan harness tidak membuktikan model/AI/animasi dalam executable game
pengguna. Belum ada tes gameplay langsung di lingkungan ini.

## Batas implementasi yang disengaja

- Descriptor katalog bukan jaminan modul, weapon, efek dan animasi semuanya
  tersedia. Director mensyaratkan actor model yang cocok masih ada, lalu
  memeriksa hasil native creator. Bila gagal, antrean/wave berhenti dengan status.
- Tidak memuat boss/model apa pun di room mana pun. Heavy opt-in juga tetap
  memakai template native area aktif; beberapa perilaku tetap bergantung arena.
- Replacement hanya entri humanoid biasa yang belum diproses, sesudah katalog
  tersedia. Bukan perubahan semua spawn awal, semua event, atau musuh aktif.
- HP/ukuran visual yang sudah ditulis tidak otomatis pulih pada master OFF;
  reload room dengan master OFF untuk baseline. Collision/range serangan tidak
  dibangun ulang mengikuti ukuran model.
- Efek terbang adalah lintasan posisi buatan dengan titik kembali yang diketahui,
  bukan ragdoll fisika atau collision sweep baru. Target HUD adalah kandidat cone
  kamera, bukan raycast bidikan.

Lihat [AIZA_CHAOS.md](AIZA_CHAOS.md) untuk menu, kontrol dan urutan tes gameplay.
