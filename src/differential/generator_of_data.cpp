#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <string>
#include <algorithm>
#include <cmath>
#include "cipher_engine.h"

using namespace std;

const int NUM_THREADS = 16;

int TARGET_dX[4] = {0};
double TARGET_PROB = 0.0;
int PAIRS_COUNT = 0;

void load_target_dx() {
    ifstream in("trail_results.txt");
    if (!in.is_open()) {
        cerr << "Error: trail_results.txt not found.\n";
        exit(1);
    }
    // Читаем: dx0 dx1 dx2 dx3 dy0 dy1 dy2 dy3 PROB
    // dy нам тут не нужны, но надо их прочитать, чтобы добраться до prob
    int garbage;
    in >> TARGET_dX[0] >> TARGET_dX[1] >> TARGET_dX[2] >> TARGET_dX[3]
       >> garbage >> garbage >> garbage >> garbage
       >> TARGET_PROB;
    in.close();
    
    // Расчет количества пар: N = ceil(4.0 / P)
    // Если P очень маленькая, ограничиваем разумным числом (например 10 млн)
    long long needed = (long long)ceil(8.0 / TARGET_PROB); // Взял запас 8/P для надежности
    if (needed > 10000000) needed = 10000000;
    if (needed < 1000) needed = 1000; // Минимум
    
    PAIRS_COUNT = (int)needed;

    cout << "Loaded Target dX: " << TARGET_dX[0] << " " << TARGET_dX[1] 
         << " " << TARGET_dX[2] << " " << TARGET_dX[3] << endl;
    cout << "Theoretical Prob: " << TARGET_PROB << endl;
    cout << "Generating " << PAIRS_COUNT << " pairs (Target ~ 8/P)...\n";
}

void worker(int tid, int count) {
    string fname = "pairs_data_part_" + to_string(tid) + ".txt";
    ofstream fout(fname);
    uint32_t seed = 12345 + tid * 999;
    
    for (int i = 0; i < count; ++i) {
        seed = seed * 1664525 + 1013904223;
        int valX = seed & 0xFFFF;

        Block X;
        X.x[0] = (valX >> 12) & 0xF;
        X.x[1] = (valX >> 8) & 0xF;
        X.x[2] = (valX >> 4) & 0xF;
        X.x[3] = valX & 0xF;

        Block base = X;
        encrypt(base);

        Block Xp = X;
        Xp.x[0] ^= TARGET_dX[0];
        Xp.x[1] ^= TARGET_dX[1];
        Xp.x[2] ^= TARGET_dX[2];
        Xp.x[3] ^= TARGET_dX[3];

        Block mod = Xp;
        encrypt(mod);

        fout << (int)X.x[0] << " " << (int)X.x[1] << " " << (int)X.x[2] << " " << (int)X.x[3] << " "
             << (int)TARGET_dX[0] << " " << (int)TARGET_dX[1] << " " << (int)TARGET_dX[2] << " " << (int)TARGET_dX[3] << " "
             << (int)base.x[0] << " " << (int)base.x[1] << " " << (int)base.x[2] << " " << (int)base.x[3] << " "
             << (int)mod.x[0] << " " << (int)mod.x[1] << " " << (int)mod.x[2] << " " << (int)mod.x[3] << "\n";
    }
    fout.close();
}

int main() {
    load_target_dx();
    
    vector<thread> threads;
    int perThread = PAIRS_COUNT / NUM_THREADS;
    if (perThread == 0) perThread = 1;

    for (int t = 0; t < NUM_THREADS; ++t) {
        // Коррекция для последнего потока (остаток)
        int my_count = perThread;
        if (t == NUM_THREADS - 1) my_count = PAIRS_COUNT - (NUM_THREADS - 1) * perThread;
        if (my_count <= 0) break;
        
        threads.emplace_back(worker, t, my_count);
    }

    for (auto& th : threads) th.join();

    ofstream out("pairs_data.txt");
    for (int t = 0; t < NUM_THREADS; ++t) {
        string fname = "pairs_data_part_" + to_string(t) + ".txt";
        ifstream in(fname);
        if(in) {
            out << in.rdbuf();
            in.close();
            remove(fname.c_str());
        }
    }
    out.close();
    cout << "Done.\n";

    return 0;
}