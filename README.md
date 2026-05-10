# Tucil3_13524046  
Ice Sliding Puzzle Solver Menggunakan UCS, GBFS, dan A*

## Penjelasan Singkat Program

Program ini dibuat untuk menyelesaikan permainan **Ice Sliding Puzzle** menggunakan algoritma pathfinding. Pada permainan ini, aktor harus bergerak dari titik awal menuju titik tujuan pada papan es. Aktor hanya dapat bergerak secara horizontal atau vertikal, tetapi karena permukaan licin, aktor akan terus meluncur sampai menabrak rintangan.

Program membaca file input `.txt` yang berisi konfigurasi papan dan cost traversal setiap tile. Setelah input dibaca, pengguna dapat memilih algoritma pathfinding yang ingin digunakan, yaitu:

- Uniform Cost Search (UCS)
- Greedy Best First Search (GBFS)
- A*

Untuk algoritma GBFS dan A*, pengguna juga dapat memilih heuristic yang digunakan, yaitu H1, H2, atau H3.

Program juga memperhatikan aturan tambahan pada puzzle, yaitu:

- Aktor harus melewati angka secara berurutan, mulai dari 0, 1, 2, dan seterusnya.
- Aktor tidak boleh melewati lava.
- Aktor harus tepat berhenti di titik tujuan setelah seluruh angka wajib dilewati.
- Cost solusi dihitung berdasarkan jumlah cost seluruh tile yang dilewati selama sliding.

Program akan menampilkan informasi:

- Papan awal
- Solusi gerakan yang ditemukan
- Cost dari solusi
- Visualisasi papan untuk setiap step solusi
- Waktu eksekusi
- Banyaknya iterasi atau konfigurasi yang ditinjau
- Playback solusi
- Path file output solusi

---

## Requirement Program dan Instalasi

### Perangkat Lunak

- Compiler C, disarankan menggunakan GCC

### Sistem Operasi

- Windows dengan MinGW / MSYS2 / WSL
- Linux
- macOS

Program tidak membutuhkan library tambahan di luar library standar C.

### Instalasi Compiler Windows Menggunakan MSYS2

```bash
pacman -S mingw-w64-x86_64-gcc
```

---

## Cara Mengkompilasi Program

### Struktur Program

```txt
Tucil3_13524046/
├── src/
│   ├── main.c
│   ├── ice_solver.exe
│   ├── test1.txt
│   ├── test2.txt
│   ├── test3.txt
│   ├── test4.txt
│   └── test5.txt
├── test/
│   └── SemuaOutputSolusi
├── doc/
│   └── Laporan Tucil 3 13524046.pdf
└── README.md
```

Keterangan:

- Folder `src` berisi source code, executable program, dan file input `.txt`.
- Folder `test` berisi file output solusi dari data uji yang digunakan dalam laporan.
- Folder `doc` berisi laporan tugas kecil dalam bentuk PDF.

### Kompilasi Program

Masuk ke folder `src`:

```bash
cd src
```

Kemudian jalankan:

### Windows

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -o ice_solver.exe
```

### Linux / macOS

```bash
gcc -std=c11 -Wall -Wextra -O2 main.c -o ice_solver
```

---

## Cara Menjalankan dan Menggunakan Program

### Menjalankan Program

Dari folder `src`, jalankan program dengan command berikut.

### Windows

```bash
.\ice_solver.exe
```

### Linux / macOS

```bash
./ice_solver
```

---

## Penjelasan Input

File input harus berada di folder `src` dan memiliki ekstensi `.txt`.

Format input:

```txt
N M
<baris papan sebanyak N>
<cost tile sebanyak N baris>
```

Keterangan:

- `N` adalah banyak baris papan.
- `M` adalah banyak kolom papan.
- Bagian papan berisi karakter yang merepresentasikan tile.
- Bagian cost berisi biaya traversal untuk setiap tile.

Keterangan karakter pada papan:

- `*` = path yang dapat dilewati
- `X` = rintangan/batu
- `L` = lava
- `Z` = posisi awal aktor
- `O` = titik tujuan
- `0` sampai `9` = angka yang harus dilewati secara berurutan

Contoh input:

```txt
7 7
XXXXXXX
X0****X
X**X**X
X****OX
X1***LX
XZ**X*X
XXXXXXX
999 999 999 999 999 999 999
999 3 5 2 8 1 999
999 7 4 999 6 9 999
999 2 8 3 5 4 999
999 6 1 7 2 999 999
999 9 3 4 999 8 999
999 999 999 999 999 999 999
```

---

## Alur Penggunaan Program

Setelah program dijalankan, pengguna akan diminta memasukkan nama file input.

Contoh:

```txt
>> Masukan file input:
   test1.txt
```

Kemudian pengguna memilih algoritma pathfinding.

```txt
>> Algoritma apa yang anda pilih? (UCS/GBFS/A*)
   A*
```

Jika pengguna memilih GBFS atau A*, program akan meminta pilihan heuristic.

```txt
>> Heuristic apa yang anda pilih? (H1/H2/H3)
   H3
```

Setelah pencarian selesai, program akan menampilkan solusi dan visualisasi papan dari initial state sampai final state.

Contoh informasi output yang ditampilkan:

- Solusi Yang Ditemukan
- Cost dari Solusi
- Initial board
- Step gerakan solusi
- Waktu eksekusi
- Banyak iterasi yang dilakukan

---

## Playback Solusi

Setelah solusi ditemukan, program akan menanyakan apakah pengguna ingin melakukan playback.

```txt
>> Apakah Anda ingin melakukan playback? (Ya/Tidak):
   Ya
```

Kontrol playback:

- Panah kanan / `n` / `d` untuk maju ke step berikutnya
- Panah kiri / `p` / `a` untuk kembali ke step sebelumnya
- `ESC` / `j` untuk lompat ke step tertentu
- `q` untuk keluar dari playback

---

## Penjelasan Output

Program dapat menyimpan solusi ke file `.txt`.

Setelah pencarian selesai, program akan menanyakan apakah solusi ingin disimpan.

```txt
>> Apakah Anda ingin menyimpan solusi? (Ya/Tidak):
   Ya
```

Jika ingin menyimpan dengan nama default, masukkan:

```txt
-
```

Maka solusi akan disimpan ke:

```txt
../test/solusi.txt
```

Jika ingin menyimpan dengan nama tertentu, masukkan nama file output.

Contoh:

```txt
solusi_test1.txt
```

Maka solusi akan disimpan ke:

```txt
../test/solusi_test1.txt
```

File output berisi:

- Algoritma yang digunakan
- Heuristic yang digunakan
- Solusi gerakan
- Cost solusi
- Waktu eksekusi
- Banyak iterasi
- Visualisasi papan setiap step
- Trace konfigurasi yang ditinjau

---

## Algoritma yang Digunakan

### 1. Uniform Cost Search (UCS)

UCS memilih state berdasarkan total cost terkecil dari posisi awal. Algoritma ini tidak menggunakan heuristic. Pada program ini, UCS digunakan untuk mencari solusi dengan cost minimum selama semua cost traversal tidak negatif.

### 2. Greedy Best First Search (GBFS)

GBFS memilih state berdasarkan nilai heuristic. Algoritma ini cenderung lebih cepat menuju target karena memilih state yang diperkirakan paling dekat. Namun, GBFS tidak menjamin solusi dengan cost minimum.

### 3. A*

A* memilih state berdasarkan gabungan antara cost aktual dari posisi awal dan heuristic menuju target. Algoritma ini menyeimbangkan pencarian berbasis cost dan estimasi jarak, sehingga lebih terarah dibanding UCS dan lebih memperhatikan cost dibanding GBFS.

---

## Heuristic yang Digunakan

### H1

H1 menggunakan jarak Manhattan dari posisi aktor saat ini menuju target berikutnya. Target berikutnya dapat berupa angka yang harus dilewati atau titik tujuan jika semua angka sudah dilewati.

### H2

H2 menggunakan jarak Manhattan menuju target berikutnya, kemudian dikalikan dengan cost tile minimum pada papan.

### H3

H3 menggunakan estimasi jarak dari posisi saat ini menuju seluruh angka yang tersisa secara berurutan, lalu menuju titik tujuan. Nilai total jarak tersebut kemudian dikalikan dengan cost tile minimum.

---

## Author

Nama: Farrell

NIM: 13524046
