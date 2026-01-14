#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iomanip>
#include "zmak.h"

using namespace std;

// --- КОНФИГУРАЦИЯ АТАКИ ---
// ВНИМАНИЕ: Эти значения нужно обновить после запуска ./analysis
// Возьмите лучший dX -> dY из файла diff_round_5_top.txt
const uint8_t T_dX[4] = { 9, 4, 12, 0 }; 
const uint8_t T_dY[4] = { 0, 0, 0, 2 }; 

struct PairData {
    Block Y;   // Ciphertext 1
    Block Yp;  // Ciphertext 2
    Block dX;  // Input difference (for filtering)
};

// Функция загрузки и фильтрации данных
vector<PairData> loadFilteredPairs(const string& filename) {
    ifstream fin(filename);
    vector<PairData> data;
    if (!fin.is_open()) return data;

    int vals[16];
    // Формат файла: X(4) dX(4) Y(4) Yp(4)
    // Индексы:
    // X: 0-3
    // dX: 4-7
    // Y: 8-11
    // Yp: 12-15

    while (fin >> vals[0]) {
        for (int i = 1; i < 16; ++i) fin >> vals[i];

        // Проверяем, совпадает ли dX с целевым
        if (vals[4] == T_dX[0] && vals[5] == T_dX[1] &&
            vals[6] == T_dX[2] && vals[7] == T_dX[3]) 
        {
            PairData p;
            p.dX.x[0] = vals[4]; p.dX.x[1] = vals[5]; p.dX.x[2] = vals[6]; p.dX.x[3] = vals[7];
            
            p.Y.x[0] = vals[8]; p.Y.x[1] = vals[9]; p.Y.x[2] = vals[10]; p.Y.x[3] = vals[11];
            
            p.Yp.x[0] = vals[12]; p.Yp.x[1] = vals[13]; p.Yp.x[2] = vals[14]; p.Yp.x[3] = vals[15];
            
            data.push_back(p);
        }
    }
    return data;
}

int main() {
    cout << "--- Key Recovery Attack (Variant 5) ---" << endl;
    cout << "Target dX: " << (int)T_dX[0] << " " << (int)T_dX[1] << " " << (int)T_dX[2] << " " << (int)T_dX[3] << endl;
    cout << "Target dY: " << (int)T_dY[0] << " " << (int)T_dY[1] << " " << (int)T_dY[2] << " " << (int)T_dY[3] << endl;

    if (T_dX[0] == 0 && T_dX[1] == 0 && T_dX[2] == 0 && T_dX[3] == 0) {
        cerr << "WARNING: Target dX is all zeros. Please update T_dX and T_dY in attack_last_round.cpp based on analysis results!" << endl;
    }

    // 1. Загрузка данных
    vector<PairData> data = loadFilteredPairs("pairs_data.txt");
    if (data.empty()) {
        cerr << "No pairs found with the specified input difference dX.\n";
        return 1;
    }
    cout << "Loaded and filtered " << data.size() << " pairs." << endl;

    // 2. Перебор ключа последнего (6-го) раунда
    // Ключ 4-битный: 0..15
    vector<long long> key_scores(16, 0);

    for (int k = 0; k < 16; ++k) {
        uint8_t key_guess = (uint8_t)k;
        
        for (const auto& p : data) {
            // Дешифруем Y на 1 раунд назад
            Block Z = p.Y;
            decryptOneRound(Z, key_guess);

            // Дешифруем Yp на 1 раунд назад
            Block Zp = p.Yp;
            decryptOneRound(Zp, key_guess);

            // Считаем разность после 5 раундов
            bool match = true;
            for(int i=0; i<4; ++i) {
                if ((Z.x[i] ^ Zp.x[i]) != T_dY[i]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                key_scores[k]++;
            }
        }
    }

    // 3. Вывод результатов
    ofstream fout("last_round_key_guess.txt");
    cout << "\nTop Key Candidates:\n";
    fout << "Key Candidates (sorted by score):\n";

    vector<pair<long long, int>> results;
    for(int k=0; k<16; ++k) results.push_back({key_scores[k], k});
    
    // Сортировка по убыванию очков
    sort(results.begin(), results.end(), [](auto& a, auto& b) { return a.first > b.first; });

    for(const auto& res : results) {
        cout << "Key = " << res.second << " (" << hex << res.second << dec << ") | Hits: " << res.first << endl;
        fout << "Key=" << res.second << " Hits=" << res.first << "\n";
    }
    fout.close();
        cout << "Results saved to last_round_key_guess.txt" << endl;
        return 0;
    }
    