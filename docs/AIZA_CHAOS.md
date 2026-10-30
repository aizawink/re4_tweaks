# Aiza Chaos v2 — BRRRT, speed, dan pelontar musuh

Dibangun dari fork asli re4_tweaks. Semua efek mulai OFF. Source mod eksperimen
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
- **F7**: shockwave manual, melontarkan sampai 16 target ESL di sekitar Leon.
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

Target pelontar/kecepatan: ESL humanoid dengan ID 11,12,15,16,17,19,1A,1F
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
6. El Gigante dicoba terpisah setelah tes humanoid, bukan bersama event boss.

Log: cari `[Aiza Chaos]` pada `dinput8.log`. Jika efek tidak muncul, kirim log
itu dan screenshot HUD/pilihan preset. Tidak perlu mengubah ID/flag event acak.

Penggantian chainsaw dan mod cutscene tidak diklaim selesai oleh versi ini.
