# Implementasi Particle System pada Simulasi Kembang Api 3D Berbasis OpenGL

Project ini adalah simulasi kembang api 3D berbasis OpenGL (Core 3.3) yang menerapkan konsep **particle system**.

## Fitur Utama
- Simulasi 3D kembang api dengan roket naik lalu meledak.
- Sistem partikel dinamis (posisi, kecepatan, warna, umur, ukuran).
- Efek fisika sederhana:
  - gravitasi,
  - drag (redaman kecepatan),
  - fade-out berdasarkan umur partikel.
- Variasi pola ledakan (bola dan ring) untuk visual lebih natural.
- Kamera orbit otomatis untuk menegaskan kedalaman ruang 3D.
- Shader partikel berbentuk glow bulat (point-sprite).

## Struktur Project
- `CMakeLists.txt` : konfigurasi build (GLFW, GLAD, GLM via FetchContent).
- `src/main.cpp` : implementasi utama particle system dan render loop.
- `shaders/particle.vert` : vertex shader partikel.
- `shaders/particle.frag` : fragment shader partikel glow.

## Kebutuhan
- CMake >= 3.16
- Compiler C++17 (MSVC / MinGW / Clang)
- Koneksi internet saat build pertama (untuk unduh dependency via FetchContent)

## Cara Build (Windows, PowerShell)
```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Cara Menjalankan
Jika memakai Visual Studio generator:
```powershell

cd "C:\Users\PIXWAR\Semester 4\Grafika Komputer\UAS"
.\build\Release\fireworks.exe

## Kontrol
- `ESC` : keluar dari aplikasi.

## Konsep Particle System yang Dipakai
Setiap partikel memiliki atribut:
- posisi (`vec3`),
- kecepatan (`vec3`),
- warna + alpha (`vec4`),
- umur (`life`, `maxLife`),
- ukuran titik (`size`).

Alur simulasi:
1. Rocket di-spawn periodik dari tanah.
2. Rocket bergerak ke atas hingga tinggi tertentu.
3. Rocket meledak menjadi ratusan partikel.
4. Tiap frame, partikel di-update oleh gravitasi, drag, dan pengurangan umur.
5. Alpha dan ukuran partikel dipudarkan seiring sisa umur.
6. Partikel mati dinonaktifkan dan slot-nya dipakai ulang.

## Catatan Akademik
Project ini bisa dijadikan dasar laporan UAS untuk topik:
- implementasi particle system real-time,
- visual effects pada grafik komputer,
- pemodelan gerak berbasis fisika sederhana di OpenGL.