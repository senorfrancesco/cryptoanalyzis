#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <iomanip>
#include <fstream>
#include "cipher_engine.h"

using namespace std;

double DDT_PROB[16][16];

void load_ddt() {
    int ddt_int[16][16];
    FILE* f = fopen("ddt_table.bin", "rb");
    if (!f) {
        cerr << "Error: ddt_table.bin not found.\n";
        exit(1);
    }
    if (fread(ddt_int, sizeof(int), 16 * 16, f) != 16 * 16) {
        cerr << "Error reading ddt_table.bin\n";
        fclose(f);
        exit(1);
    }
    fclose(f);
    for(int i=0; i<16; ++i)
        for(int j=0; j<16; ++j)
            DDT_PROB[i][j] = (double)ddt_int[i][j] / 16.0;
}

double get_prob_F(int dx2, int dx3, int dout) {
    double total_prob = 0.0;
    for (int d_mid = 0; d_mid < 16; ++d_mid) {
        double p1 = DDT_PROB[dx3][d_mid];
        if (p1 == 0) continue;
        int second_in = dx2 ^ d_mid;
        double p2 = DDT_PROB[second_in][dout];
        if (p2 > 0) total_prob += p1 * p2;
    }
    return total_prob;
}

struct State {
    uint16_t initial_dx; 
    uint16_t current_dx; 
    double prob;
    // Для отладки можно хранить путь, но это дорого по памяти.
    // Ограничимся выводом результата.

    bool operator<(const State& other) const {
        return prob > other.prob;
    }
};

uint16_t pack(int x0, int x1, int x2, int x3) {
    return (x0 << 12) | (x1 << 8) | (x2 << 4) | x3;
}

void unpack(uint16_t val, int& x0, int& x1, int& x2, int& x3) {
    x0 = (val >> 12) & 0xF;
    x1 = (val >> 8) & 0xF;
    x2 = (val >> 4) & 0xF;
    x3 = val & 0xF;
}

int main() {
    load_ddt();
    const int BEAM_WIDTH = 20000;
    const int ROUNDS = 5;

    vector<State> current_states;
    for (int val = 1; val < 65536; ++val) {
        current_states.push_back({(uint16_t)val, (uint16_t)val, 1.0});
    }

    cout << "Starting Search. Initial states: " << current_states.size() << endl;

    ofstream debug_log("trail_debug.txt");
    debug_log << "--- Trail Search Log ---\\n";

    for (int r = 1; r <= ROUNDS; ++r) {
        map<pair<uint16_t, uint16_t>, double> next_map;
        
        int processed = 0;
        for (const auto& s : current_states) {
            int dx[4];
            unpack(s.current_dx, dx[0], dx[1], dx[2], dx[3]);

            for (int dF = 0; dF < 16; ++dF) {
                double p_trans = get_prob_F(dx[2], dx[3], dF);
                if (p_trans > 0) {
                    double new_p = s.prob * p_trans;
                    if (new_p < 0.00001) continue;

                    int nx0 = dx[1];
                    int nx1 = dx[2];
                    int nx2 = dx[3];
                    int nx3 = dx[0] ^ dF;
                    
                    uint16_t next_packed = pack(nx0, nx1, nx2, nx3);
                    next_map[{s.initial_dx, next_packed}] += new_p;
                }
            }
            if (r==1 && ++processed % 5000 == 0) cout << "." << flush;
        }
        if (r==1) cout << endl;

        vector<State> next_gen;
        next_gen.reserve(next_map.size());
        for (auto const& [key, prob] : next_map) {
            next_gen.push_back({key.first, key.second, prob});
        }
        
        sort(next_gen.begin(), next_gen.end());
        
        if (next_gen.size() > BEAM_WIDTH) {
            next_gen.resize(BEAM_WIDTH);
        }
        current_states = next_gen;

        cout << "Round " << r << " complete. Top prob: " << current_states[0].prob 
             << " (States: " << current_states.size() << ")\n";

        // Логирование топа для отладки
        debug_log << "--- Round " << r << " Top 5 ---\\n";
        for(size_t i=0; i<5 && i<current_states.size(); ++i) {
             int in[4], out[4];
             unpack(current_states[i].initial_dx, in[0], in[1], in[2], in[3]);
             unpack(current_states[i].current_dx, out[0], out[1], out[2], out[3]);
             debug_log << i+1 << ") In:("<<in[0]<<","<<in[1]<<","<<in[2]<<","<<in[3]<<")"
                       << " Out:("<<out[0]<<","<<out[1]<<","<<out[2]<<","<<out[3]<<")"
                       << " P=" << current_states[i].prob << "\n";
        }
    }
    debug_log.close();

    cout << "\n--- TOP ANALYTICAL DIFFERENTIALS (5 Rounds) ---\\n";
    cout << "Detailed log saved to trail_debug.txt\n";
    
    ofstream out("trail_results.txt");
    // Пишем данные лучшей траектории
    if (!current_states.empty()) {
        const auto& s = current_states[0];
        int in[4], out_d[4];
        unpack(s.initial_dx, in[0], in[1], in[2], in[3]);
        unpack(s.current_dx, out_d[0], out_d[1], out_d[2], out_d[3]);
        
        cout << "1) dX=(" << in[0]<<","<<in[1]<<","<<in[2]<<","<<in[3] << ")"
             << " -> dY=(" << out_d[0]<<","<<out_d[1]<<","<<out_d[2]<<","<<out_d[3] << ")"
             << " Prob=" << s.prob << "\n";
             
        out << in[0]<<" "<<in[1]<<" "<<in[2]<<" "<<in[3] << " "
            << out_d[0]<<" "<<out_d[1]<<" "<<out_d[2]<<" "<<out_d[3] << " "
            << s.prob << "\n"; // Добавили вероятность в конец файла
    }
    out.close();

    return 0;
}
