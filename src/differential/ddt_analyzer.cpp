#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include "cipher_engine.h"

using namespace std;

// Структура для хранения перехода S-блока
struct SBoxTransition {
    int d_in;
    int d_out;
    int count;
    double probability;
};

int main() {
    int ddt[16][16] = {0};

    // 1. Построение DDT
    for (int d_in = 0; d_in < 16; ++d_in) {
        for (int x = 0; x < 16; ++x) {
            int y1 = G(x);
            int y2 = G(x ^ d_in);
            int d_out = y1 ^ y2;
            ddt[d_in][d_out]++;
        }
    }

    // 2. Красивый вывод в файл
    ofstream out("ddt_pretty.txt");
    out << "Difference Distribution Table (DDT) for ZMAK S-Box\n";
    out << "Rows: Input Difference (d_in)\n";
    out << "Cols: Output Difference (d_out)\n\n";
    
    out << "   |";
    for(int i=0; i<16; ++i) out << " " << hex << uppercase << setw(1) << i << " ";
    out << "\n---+ ";
    for(int i=0; i<16; ++i) out << "---";
    out << "\n";

    for (int i = 0; i < 16; ++i) {
        out << hex << uppercase << setw(1) << i << "  |";
        for (int j = 0; j < 16; ++j) {
            if (ddt[i][j] == 0) out << " . ";
            else out << dec << setw(2) << ddt[i][j] << " ";
        }
        out << "\n";
    }
    out << "\nNotes: Values represent the number of pairs (out of 16).\n";
    out.close();

    cout << "Pretty DDT saved to 'ddt_pretty.txt'\n";

    // 3. Сохранение бинарного файла для Trail Search
    FILE* f = fopen("ddt_table.bin", "wb");
    if (f) {
        fwrite(ddt, sizeof(int), 16 * 16, f);
        fclose(f);
    }
    
    return 0;
}