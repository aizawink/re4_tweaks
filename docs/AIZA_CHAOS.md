# Aiza Chaos v3 — spawn, encounter, dan efek absurd

Dibangun dari fork asli re4_tweaks. Semua efek yang mengubah gameplay mulai OFF. Source mod eksperimen
lama tidak ditimpa atau dicampur secara otomatis.

## Mulai cepat

1. Tutup RE4, backup `Bin32/dinput8.dll`, `dinput8.ini`, dan folder `re4_tweaks`
   ke folder di luar game. Jangan menimpa satu-satunya backup mod lamamu.
2. Buka GitHub Actions pada branch `feat/aiza-chaos-v1`. Pilih CI dengan commit
   terbaru yang lulus, lalu unduh artifact **release**, bukan debug/source ZIP.
3. Ekstrak isi artifact ke `Bin32` (DLL, INI, dan folder `re4_tweaks`). Jika
   memakai INI lama, opsi yang belum ada akan menggunakan default OFF.
4. Buka F1, aktifkan Trainer bila belum aktif. Masuk Trainer → Overrides,
   cari **Aiza Chaos**, lalu klik salah satu preset.
5. Masuk ulang desa untuk HP/ukuran baru. Tes pertama pada checkpoint yang sama.

## Kontrol dan preset

- **F6**: master ON/OFF ketika jendela game aktif.
- **F7**: shockwave manual, melontarkan sampai 16 target yang didukung di sekitar Leon.
- **F9**: Bowling blast ke depan; aktifkan checkbox Bowling atau salah satu preset.
- **HUD**: status master, multiplier yang aktif, jumlah musuh terbang.
- **BRRRT Content**: lari 5x, tembakan 12x, animasi musuh 2x, HP spawn 5x,
  ammo tak habis, ukuran desa acak, dan pelontar Boomerang.
- **Space Program**: kombinasi yang sama, dengan Rocket setinggi 18000 unit,
  durasi 5 detik.
- **Tornado Party**: kombinasi yang sama dengan orbit udara di sekitar posisi
  awal setiap musuh.

Preset mengaktifkan master secara eksplisit. Setiap fitur punya checkbox dan
slider sendiri. Klik Save pada konfigurasi agar pilihan tersimpan.

## Fitur dan batasnya

| Fitur | Cara kerja / rentang |
|---|---|
| Fast run | Tahan Run + arah, tanpa aim/pisau; multiplier animasi native 1–15x |
| BRRRT | Tahan Aim + Fire pada senjata api yang didukung; percepat animasi 1–30x, kosongkan shot timer, tambahkan bit fire-trigger native selama update |
| Infinite ammo | Gunakan flag debug ammo native selama update Chaos; pilihan infinite ammo asli pengguna dipertahankan |
| Enemy speed | Multiplier animasi 0.2–10x pada target humanoid ESL yang didukung; tidak menulis AI baru |
| Thick HP | Pengali 0.1–15x pada spawn baru; HP positif dibatasi 1..32767 |
| Launch shots | HP turun selama native update, damage aktif cocok dengan nomor senjata yang dipakai, serta input tembakan masih baru |
| Rocket | Terbang vertikal lalu kembali ke titik peluncuran |
| Boomerang | Terlempar menjauh lalu kembali ke titik peluncuran |
| Tornado | Mengorbit titik peluncuran sambil melayang, lalu kembali |
| F7 shockwave | Radius 200–6000 unit game; tidak menambahkan damage atau menghentikan event |
| Random village size | Ukuran visual 0.5–2x pada spawn ID 0x15 di desa room 0x0101; collision dan jangkauan serangan tetap native |

Pelontar punya pengaturan tinggi 200–30000 unit, durasi 0.5–8 detik, sebaran
0–4000 unit dan cooldown 0.2–5 detik per musuh. Unit game bukan meter.
Musuh tidak diluncurkan lagi saat masih terbang. Animasi native tetap berjalan;
ini bukan ragdoll fisik baru. Gerak buatan tidak melakukan sweep collision
sepanjang lintasan, sehingga musuh bisa melewati geometri saat terbang.
Ketiga gaya kembali ke titik peluncuran yang diketahui; tidak menebak tinggi
lantai pada koordinat lain. F6 OFF juga mengembalikan penerbangan aktif.

Target pelontar: humanoid ESL atau tambahan Director dengan ID 11,12,15,16,17,19,1A,1F
(hex). ID alias Merchant 13,14,18,1E tidak digunakan. El Gigante (2B) tersedia
sebagai opsi eksperimental, default OFF. Boss lain tidak disertakan. Entri
script sementara tanpa indeks ESL tidak dipaksa menjadi target.

Deteksi tembakan tidak mengklaim tahu sumber peluru secara sempurna: ia
mencocokkan penurunan HP, damage aktif, nomor senjata dan input terbaru.
Serangan dengan jenis senjata yang sama dalam waktu berdekatan masih bisa
ambigu. Tembakan fatal hanya bisa melontarkan model jika native belum
menghapusnya. Tidak menghidupkan kembali musuh yang sudah dihapus.

Kecepatan menggunakan animasi asli. Nilai ekstrem bisa melewati event animasi
native; multiplier bukan jaminan angka peluru/detik tertentu. Coba preset
terlebih dahulu. Grenade, egg, knife, harpoon dan rocket launcher tidak
mendapat injeksi BRRRT. Pemakaian Matilda tetap mengikuti opsi burst aslinya.

## Director: spawn, penggantian, dan gelombang

Buka F1 → Trainer → Overrides → **Aiza Director - Enemies & Encounters**.
Aktifkan Chaos master. Tombol hanya mengantrekan perintah: tutup menu agar
native gameplay berjalan dan perintah diproses. Status/log membedakan antrean,
keberhasilan native creator, dan kegagalan. Jangan menganggap tombol terklik
berarti model sudah muncul.

1. Masuk area berisi musuh. Katalog mempelajari descriptor musuh native yang
   sudah terinisialisasi. Pilih entri bertanda tersedia. Model yang belum
   pernah teramati di room ini tidak akan dibuat hanya berdasarkan ID.
2. Berdiri di lantai kosong, klik **Mark Leon's current position**, tutup menu
   agar posisi ditandai, lalu menjauh. Buka lagi untuk memilih **Spawn 1/3/5**.
3. Atur HP absolut 100–32767, ukuran visual 0.5–3x, dan animasi 0.2–5x.
   Pembuatan bertahap setiap 250 ms. Maksimum 24 objek tambahan yang masih
   dilacak, termasuk corpse yang belum dibuang native. Gunakan Clear bila penuh.
4. **Clear only additional enemies** hanya memilih objek tambahan yang
   pointer dan GUID-nya masih cocok. Musuh bawaan, termasuk penggantian ESL,
   tidak termasuk. Objek yang sedang terikat grab/action dilewati; lepas aksi
   lalu Clear lagi. Clear menghentikan antrean dan gelombang.

Koordinat ESL harus lebih dahulu cocok dengan posisi inisialisasi native
(`short * 10`). Bila indikator belum verified, masuk ulang room. Posisi ditandai
manual karena tidak ada raycast lantai baru. Tidak ada pilihan spawn acak di
udara atau di belakang geometri. Titik, katalog dan ownership dibuang saat
pindah room/manager/player; titik tidak disimpan dalam INI.

**Replace future normal spawns** punya tiga mode: template humanoid terpilih,
random dari template tersedia dengan seed, atau chainsaw saat template native
yang kompatibel tersedia. Tidak mengubah musuh yang sudah hidup. Hanya entri
ESL biasa yang belum SET/DIE, HP positif, dan bukan entri extended-scale yang
diproses; routine/event, lokasi, room, indeks, sight byte serta lifecycle asli
dipertahankan. Descriptor penampilan dipulihkan sesudah native update tanpa
membalik bit SET/DIE yang ditulis game. Penghitung **Observed replacements**
bertambah bila objek baru benar-benar cocok dengan descriptor pengganti.

Ini bukan pengganti seluruh musuh di semua chapter: katalog harus ditemukan
dulu, dan spawn awal saat room init tidak dipaksa diubah. Mode chainsaw
menunggu jika model native chainsaw belum tersedia. Menu tidak memuat modul
musuh lintas area, dan model witness tidak menjamin semua resource/animasi
sudah cocok. Gagal membuat objek menghentikan antrean, bukan menulis flag
weapon/behavior acak.

**Wave arena** memakai titik dan template terpilih. Jumlah awal 1–20, tambahan
0–10 per wave (tetap maksimum 20 per wave), 1–50 wave, jeda 0–20 detik, dan
pertumbuhan HP 0–2x per wave (faktor maksimum 15x, HP tetap int16 positif).
Hanya musuh tambahan kelompok wave berjalan yang dihitung; musuh native
boleh masih hidup. Wave berikut dimulai setelah kelompok itu mati dan jeda
berakhir. Pool penuh atau witness hilang menghentikan wave dengan alasan.
Corpse dapat memenuhi batas 24 sebelum dibuang native: Clear lalu mulai lagi.
Tidak ada jaminan wave tak terbatas atau resource tambahan otomatis dimuat.

**Random elite enemies** memberi peluang 0–100% per identitas actor yang
teramati: HP 1–10x, ukuran 1–4x, animasi 1–5x. Berlaku sekali per GUID,
termasuk musuh yang sudah hidup ketika toggle dinyalakan. Seed yang sama,
room, indeks ESL/urutan spawn dan katalog yang sama menghasilkan pilihan yang
sama. Mengubah seed tidak mengacak ulang actor yang telah diproses. HP/ukuran
yang sudah diterapkan bertahan; speed mengikuti toggle. Kombinasi beberapa
pengali tetap dibatasi sebelum ditulis.

**Bowling blast** F9 melontarkan target humanoid dalam cone 120° di depan
Leon, radius 200–2500 unit, memakai Boomerang. **Auto bowling** eksperimental
menginfer serangan melee dari penurunan HP, state player-action dan posisi;
ini belum hook kick native yang persis. Bowling tidak menambah damage, dan
memakai batas/cooldown pelontar yang sama. Collision masih native.

**Timed random events** berganti tiap 5–120 detik gameplay: Rocket Party,
Tornado Party, fast enemies (3x), slow enemies (0.35x), atau BRRRT (20x).
Rocket/Tornado mengubah reaksi hit, bukan otomatis mengangkat seluruh room;
F7 tetap bisa dipakai. Event menjeda dengan gameplay dan tidak menimpa slider
tersimpan. Master OFF menghentikannya.

**Target HUD** menampilkan kandidat pada cone kamera: nama, ID/type, HP,
GUID dan status tambahan. Ini bukan raycast bidikan: kandidat bisa tertutup
tembok atau berbeda dari target peluru. Informasi ini membantu memeriksa
hasil mod, bukan bukti serangan mengenai model tersebut.

Katalog heavy eksperimental default OFF. Hanya descriptor native teramati
yang dapat dipakai, dan pola script/resource heavy tetap bisa bergantung pada
arena asli. Whitelist pelontar tetap terpisah; memilih heavy di katalog tidak
membuat semua boss bisa Bowling/terbang. Lihat referensi kode dan batas bukti
di [RESEARCH_REFERENCES.md](RESEARCH_REFERENCES.md).

## OFF, pause, dan perpindahan area

- Master OFF menghentikan runtime Chaos. Snapshot kecepatan dikembalikan
  setelah setiap update; perubahan kecepatan yang sengaja ditulis native
  selama update tetap dipertahankan.
- HP dan ukuran yang sudah diterapkan pada spawn tidak dibalik saat F6 OFF;
  muat ulang room dengan master OFF untuk spawn normal.
- Pause/inventory, cutscene, death demo, boat, ladder, dan room jump membatalkan
  penerbangan dan menonaktifkan percepatan sementara. Efek cutscene belum dibuat.
- Saat area/manager/player berubah, state dibuang tanpa mengakses pointer
  musuh lama. Hook spawn yang sudah ada memberi notifikasi saat slot dipakai ulang.
- Opsi Player Speed Override dan HP Multiplier bawaan Trainer tetap independen.
  Matikan Player Speed Override saat membandingkan hasil slider Chaos.
- Parameter speed/scale ESL yang sengaja ditulis pembuat mod tetap diutamakan.

## Build sendiri di Windows

```powershell
git clone --recurse-submodules --branch feat/aiza-chaos-v1 https://github.com/aizawink/re4_tweaks.git "$env:USERPROFILE\Downloads\re4_tweaks_chaos"
Set-Location "$env:USERPROFILE\Downloads\re4_tweaks_chaos"
.\scripts\Build-AizaChaos.ps1
```

Jika sudah pernah clone branch tersebut:

```powershell
Set-Location "$env:USERPROFILE\Downloads\re4_tweaks_chaos"
git pull --ff-only
git submodule update --init --recursive
.\scripts\Build-AizaChaos.ps1
```

Syarat Git, Visual Studio C++ Build Tools, Windows SDK. Upstream memakai v145;
script mendeteksi v145 atau memakai v143 bila VS2022 tersedia. CI menguji v145;
kompatibilitas v143 harus diperiksa pada mesin yang memakainya.

Script menonaktifkan post-build copy ke game. Paket DLL/konfigurasi masuk ke
`artifacts/aiza-chaos-...`. Tidak memasang hasil build gagal ke game.

Rollback: tutup game lalu pulihkan DLL, INI dan folder mod yang dibackup.

## Verifikasi dan uji gameplay

Tes otomatis memeriksa:

- Seluruh 65536 nilai HP int16, multiplier ekstrem/nonfinite, batas skala,
  whitelist gun/target.
- Rocket/Boomerang/Tornado pada simulasi 30/60/144 FPS, ketinggian/sebaran,
  kembali ke origin, cancellation, parameter invalid.
- Harness yang menjalankan kode produksi: deteksi hit vs damage flag yang
  menetap, F7, batas 16 musuh, F6 foreground/edge, rate restore, flag ammo,
  pause, perpindahan area, slot reuse dan despawn. AddressSanitizer dan
  UndefinedBehaviorSanitizer aktif; leak detection dimatikan pada harness
  karena batasan ptrace executor.
- Director produksi dengan pool tiruan: validasi pointer/alignment, koordinat,
  spawn antrean, kegagalan native, kepemilikan GUID, clear/grab, penggantian
  beserta lifecycle, wave/HP growth, model witness, elite sekali per GUID,
  event pause dan request kedaluwarsa.
- Seed, identitas template, batas koordinat/int overflow wave dan cone Bowling.
- Bit fire sintetis dicabut setelah scheduler sambil mempertahankan bit lain
  dan input Fire asli pengguna.

Harness memakai objek native tiruan. Ia tidak membuktikan layout memori,
animasi, dan perilaku senjata dalam game. Windows Debug/Release diperiksa oleh
CI pada commit terbaru; tes gameplay tetap perlu dilakukan di PC pemain.

Urutan tes gameplay:

1. Preset BRRRT Content, masuk ulang desa; tahan Run dan periksa kecepatan.
2. Aim + Fire handgun; cek peluru, laju tembak dan musuh yang masih punya HP.
3. F7; cek sampai 16 musuh terangkat dan kembali ke titik awal.
4. Ubah gaya ke Rocket lalu Tornado; coba F6 OFF di tengah penerbangan.
5. Masuk inventory/keluar area saat efek berjalan; cek tidak ada flight lama
   diterapkan ke musuh baru. Matikan master dan muat ulang untuk baseline.
6. Dengan master ON, pilih template tersedia, tandai titik, tutup F1 agar
   perintah diproses, menjauh lalu Spawn 1. Periksa status sukses dan Added
   alive. Uji HP/ukuran/speed lalu Clear; pastikan musuh bawaan tetap ada.
7. Coba replacement pada spawn normal berikutnya; periksa penghitung observed.
   Coba mode chainsaw hanya jika katalog menunjukkan template tersedia.
8. Wave 2 kelompok, awal 1 lalu +1. Bunuh kelompok tambahan, periksa jeda dan
   HP kelompok kedua. Coba Stop, master OFF, dan pindah area ketika antrean ada.
9. Nyalakan elite, Bowling F9 ke depan, lalu timed events. Coba masing-masing
   sebelum menggabungkan semua efek. Uji Auto bowling terpisah.
10. Heavy dicoba terpisah setelah tes humanoid, bukan bersama event boss.

Log: cari `[Aiza Chaos]` dan `[Aiza Director]` pada `dinput8.log`. Jika efek tidak muncul, kirim log
itu dan screenshot HUD/pilihan preset. Tidak perlu mengubah ID/flag event acak.

Penggantian musuh v3 terbatas pada spawn normal mendatang dan template area aktif.
Pemuatan model musuh lintas area serta mod cutscene belum diimplementasikan.
