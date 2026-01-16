#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <iomanip>
#include "cipher_engine.h"

using namespace std;

// Целевые характеристики (загружаются из файла)
int T_dX[4] = {0};
int T_dY[4] = {0};

void load_trail_targets() {
    ifstream in("trail_results.txt");
    if (!in.is_open()) {
        cerr << "Error: trail_results.txt not found.\n";
        exit(1);
    }
    // Читаем: in0 in1 in2 in3 out0 out1 out2 out3
    in >> T_dX[0] >> T_dX[1] >> T_dX[2] >> T_dX[3]
       >> T_dY[0] >> T_dY[1] >> T_dY[2] >> T_dY[3];
    in.close();
    
    cout << "Loaded Targets:\n";
    cout << "  dX: " << T_dX[0] << " " << T_dX[1] << " " << T_dX[2] << " " << T_dX[3] << endl;
    cout << "  dY: " << T_dY[0] << " " << T_dY[1] << " " << T_dY[2] << " " << T_dY[3] << endl;
}

struct PairData {
    Block Y;
    Block Yp;
};

// Загрузка данных (теперь без фильтрации, т.к. генератор уже отфильтровал)
vector<PairData> loadPairs(const string& filename) {
    ifstream fin(filename);
    vector<PairData> data;
    if (!fin.is_open()) return data;

    int vals[16];
    // Формат: X(4) dX(4) Y(4) Yp(4)
    while (fin >> vals[0]) {
        for (int i = 1; i < 16; ++i) fin >> vals[i];
        
        PairData p;
        p.Y.x[0] = vals[8]; p.Y.x[1] = vals[9]; p.Y.x[2] = vals[10]; p.Y.x[3] = vals[11];
        p.Yp.x[0] = vals[12]; p.Yp.x[1] = vals[13]; p.Yp.x[2] = vals[14]; p.Yp.x[3] = vals[15];
        data.push_back(p);
    }
    return data;
}

int main() {
    load_trail_targets();

    vector<PairData> data = loadPairs("pairs_data.txt");
    if (data.empty()) {
        cerr << "No pairs data found.\n";
        return 1;
    }
    cout << "Loaded " << data.size() << " pairs for attack.\n";

    vector<long long> key_scores(16, 0);

    // Атака на ключ 6-го раунда
    for (int k = 0; k < 16; ++k) {
        uint8_t key_guess = (uint8_t)k;
        
        for (const auto& p : data) {
            Block Z = p.Y;
            decryptOneRound(Z, key_guess);

            Block Zp = p.Yp;
            decryptOneRound(Zp, key_guess);

            // Проверка на совпадение с TARGET_dY
            bool match = true;
            for(int i=0; i<4; ++i) {
                int diff = Z.x[i] ^ Zp.x[i];
                if (diff != T_dY[i]) {
                    match = false;
                    break;
                }
            }
            if (match) key_scores[k]++;
        }
    }

    // Вывод
    ofstream fout("last_round_key_guess.txt");
    cout << "\n--- Attack Results ---\n";
    vector<pair<long long, int>> results;
    for(int k=0; k<16; ++k) results.push_back({key_scores[k], k});
    
    sort(results.begin(), results.end(), [](auto& a, auto& b) { return a.first > b.first; });

    for(const auto& res : results) {
        cout << "Key = " << res.second << " (0x" << hex << res.second << dec << ") | Hits: " << res.first << endl;
        fout << "Key=" << res.second << " Hits=" << res.first << "\n";
    }
    fout.close();

    return 0;
}