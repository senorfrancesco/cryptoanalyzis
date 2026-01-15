#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <string>
#include <algorithm>
#include "zmak.h"

using namespace std;

// Настройки генерации
const int X_LIMIT = 64;       // Количество различных открытых текстов для каждой разности
const int NUM_THREADS = 16;    // Количество потоков

void worker(int tid, int start_X, int end_X) {
    string fname = "pairs_data_part_" + to_string(tid) + ".txt";
    ofstream fout(fname);
    if (!fout.is_open()) return;

    for (int valX = start_X; valX < end_X; ++valX) {
        // Формируем базовый блок X из текущего индекса
        // Для разнообразия можно использовать разные биты valX
        Block X;
        X.x[0] = (valX >> 4) & 0xF;
        X.x[1] = valX & 0xF;
        X.x[2] = (valX >> 8) & 0xF; // Добавляем еще битов, если X_LIMIT > 256
        X.x[3] = (valX >> 12) & 0xF;

        // Шифруем базовый текст
        Block base = X;
        encrypt(base);

        // Перебираем все возможные входные разности dX (16 бит = 65536 вариантов)
        for (int valDX = 0; valDX < 65536; ++valDX) {
            uint8_t dX0 = (valDX >> 12) & 0xF;
            uint8_t dX1 = (valDX >> 8) & 0xF;
            uint8_t dX2 = (valDX >> 4) & 0xF;
            uint8_t dX3 = valDX & 0xF;

            // Создаем модифицированный текст X' = X ^ dX
            Block Xp = X;
            Xp.x[0] ^= dX0; 
            Xp.x[1] ^= dX1; 
            Xp.x[2] ^= dX2; 
            Xp.x[3] ^= dX3;

            // Шифруем модифицированный текст
            Block mod = Xp;
            encrypt(mod);

            // Записываем данные в формате: 
            // X0 X1 X2 X3  dX0 dX1 dX2 dX3  Y0 Y1 Y2 Y3  Y0p Y1p Y2p Y3p
            fout << (int)X.x[0] << " " << (int)X.x[1] << " " << (int)X.x[2] << " " << (int)X.x[3] << " "
                 << (int)dX0 << " " << (int)dX1 << " " << (int)dX2 << " " << (int)dX3 << " "
                 << (int)base.x[0] << " " << (int)base.x[1] << " " << (int)base.x[2] << " " << (int)base.x[3] << " "
                 << (int)mod.x[0] << " " << (int)mod.x[1] << " " << (int)mod.x[2] << " " << (int)mod.x[3] << "\n";
        }
        
        if (tid == 0 && valX % 8 == 0) {
            cout << "Thread 0 progress: " << (valX - start_X) << "/" << (end_X - start_X) << " texts processed\n";
        }
    }
    fout.close();
}

int main() {
    cout << "Starting data generation (Variant 5)..." << endl;
    
    int perThread = (X_LIMIT + NUM_THREADS - 1) / NUM_THREADS;
    vector<thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        int start = t * perThread;
        int end = min(X_LIMIT, (t + 1) * perThread);
        if (start >= end) break;
        threads.emplace_back(worker, t, start, end);
    }

    for (auto& th : threads) th.join();

    // Сборка всех частей в один файл
    cout << "Merging files..." << endl;
    ofstream out("pairs_data.txt");
    for (int t = 0; t < (int)threads.size(); ++t) {
        string fname = "pairs_data_part_" + to_string(t) + ".txt";
        ifstream in(fname);
        out << in.rdbuf();
        in.close();
        remove(fname.c_str()); // Удаляем временный файл
    }
    out.close();

    cout << "Generation complete. Data saved to pairs_data.txt" << endl;
    return 0;
}