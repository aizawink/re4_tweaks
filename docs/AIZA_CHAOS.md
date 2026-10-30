# Aiza Chaos v1 — dasar baru

Branch ini dimulai dari fork asli re4_tweaks, bukan source mod lama.
BRRRT, Launch Shot, chainsaw replacement, dan eksperimen cutscene lama belum disertakan.

## Fitur pertama

- **Random Ganado size (visual)**: ukuran visual acak untuk Ganado ID `0x15`
  yang dibuat dari ESL di desa awal, room `0x0101`.
- Checkbox ON/OFF, status, minimum dan maksimum ukuran di Trainer → Overrides.
  Cari `Aiza Chaos` pada pencarian opsi bila sulit ditemukan.
- Default OFF. Rentang awal 0.75x–1.35x; batas slider/INI 0.5x–2.0x.
- Pengali HP bawaan tetap tersedia; ketiga hook HP membatasi hasil positif
  pada 1..32767 sebelum konversi ke int16. HP mati/sentinel tidak dihidupkan.

Ukuran dipilih hanya pada inisialisasi objek. Aktifkan **sebelum masuk desa**.
Jika sudah berada di desa, aktifkan lalu muat ulang checkpoint/room.
OFF berlaku untuk spawn berikutnya. Musuh yang sudah berubah tidak dikembalikan
secara langsung; matikan opsi lalu muat ulang room untuk ukuran normal.

Fitur ini mengubah ukuran model dan target skala internal Ganado. Collision,
jarak serangan, AI, jenis senjata, dan jumlah musuh tetap native. Visual mungkin
belum selaras dengan hitbox pada ukuran ekstrem. Mulai dari rentang default.
Entri ESL dengan flag speed/scale eksplisit tetap mengikuti mod pembuatnya.
Tidak mengubah file ESL/save dan tidak menyalin flag event dari musuh lain.
ID ini bukan identifikasi chainsaw; cakupan baru diperluas setelah tes game.

## Build di Windows

Clone ke folder baru agar project dan DLL lama tetap bisa dipulihkan:

```powershell
git clone --recurse-submodules --branch feat/aiza-chaos-v1 https://github.com/aizawink/re4_tweaks.git "$env:USERPROFILE\Downloads\re4_tweaks_chaos"
Set-Location "$env:USERPROFILE\Downloads\re4_tweaks_chaos"
.\scripts\Build-AizaChaos.ps1
```

Syarat: Git, Visual Studio C++ Build Tools, Windows SDK. Upstream memakai v145;
script mendeteksi v145 atau memakai v143 bila tools VS2022 tersedia. Build
VS2022 masih perlu dibuktikan pada mesin yang menggunakannya; jika gagal,
kirim error pertama. Tidak mengganti toolset permanen di project.

Script menonaktifkan post-build copy ke game. DLL dan konfigurasi lengkap
masuk ke folder `artifacts/aiza-chaos-...`. Jangan menginstal hasil build gagal.

## Pasang dan rollback

1. Tutup RE4. Backup `Bin32/dinput8.dll`, `dinput8.ini`, dan folder `re4_tweaks`
   di luar folder game. Jangan menghapus source atau backup mod lama.
2. Salin hasil build lengkap ke Bin32. Jika memakai konfigurasi lama, jangan
   timpa INI begitu saja; opsi baru akan memakai default OFF saat belum ada.
3. Buka F1, aktifkan Trainer bila masih nonaktif; buka Trainer → Overrides.
4. Aktifkan Random Ganado size (visual), masuk ulang desa awal.
5. Periksa beberapa Ganado, lalu coba OFF + muat ulang room.
6. Coba HP multiplier 5x pada spawn baru; 15x harus tetap menghasilkan HP
   positif dengan batas 32767. Jangan memakai HP dari sesi sebelum patch.

Rollback: tutup game lalu pulihkan DLL, INI, dan folder mod dari backup.

## Bukti dan batas verifikasi

Jalur kode: `ModExpansion.cpp::SetEmListParamExtensions` sudah dipakai upstream
untuk parameter ukuran/kecepatan ESL. Patch memakai ketiga pemanggil yang sudah
ada (`EmSetFromList`, `EmSetFromList2`, `EmSetEvent`), bukan hook alamat baru.
Hook hanya memproses entri selaras dalam array ESL dan objek Ganado yang cocok.

- Tes native portabel: seluruh nilai int16 HP, hasil overflow, pecahan,
  nilai nonfinite, batas skala, dan pemilihan room/ID.
- Harness menjalankan fungsi scale produksi dengan objek tiruan: OFF,
  bukan room target, entri event/stack, inisialisasi gagal, tidak menerapkan
  ukuran dua kali, dan prioritas parameter ESL eksplisit.
- Harness tidak membuktikan layout memori game atau animasi native.
- Build DLL Windows dan tes gameplay harus diverifikasi terpisah. Lihat
  hasil GitHub Actions CI untuk Debug/Release sebelum menginstal artifact.

Untuk percobaan gameplay, mulai dari checkpoint desa yang sama. Jika tidak
terlihat efek, periksa log `dinput8.log` untuk `[Aiza Chaos] Village size`;
jangan mengubah ID atau flag event secara acak.
