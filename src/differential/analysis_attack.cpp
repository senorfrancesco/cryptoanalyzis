#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <map>
#include <algorithm>
#include <iomanip>
#include <atomic>
#include "zmak.h"

using namespace std;

const int NUM_THREADS = 16;
const int ANALYSIS_ROUNDS = 5; // We look for characteristics after 5 rounds

struct PairData {
    Block X;
    Block dX;
    // Y and Yp are not strictly needed for the analysis phase if we re-encrypt, 
    // but the generator provides them. We only need X and dX to simulate 5 rounds.
};

// Helper to pack 4 nibbles into a 16-bit integer for map keys
uint16_t pack(const Block& b) {
    return (b.x[0] << 12) | (b.x[1] << 8) | (b.x[2] << 4) | b.x[3];
}

// Helper to unpack for printing
Block unpack(uint16_t val) {
    Block b;
    b.x[0] = (val >> 12) & 0xF;
    b.x[1] = (val >> 8) & 0xF;
    b.x[2] = (val >> 4) & 0xF;
    b.x[3] = val & 0xF;
    return b;
}

// --- Loading Data ---
vector<PairData> loadPairs(const string& filename) {
    ifstream fin(filename);
    vector<PairData> data;
    if (!fin.is_open()) return data;

    int vals[16];
    while (fin >> vals[0]) {
        for (int i = 1; i < 16; ++i) fin >> vals[i];
        
        PairData p;
        // X: 0-3
        p.X.x[0] = vals[0]; p.X.x[1] = vals[1]; p.X.x[2] = vals[2]; p.X.x[3] = vals[3];
        // dX: 4-7
        p.dX.x[0] = vals[4]; p.dX.x[1] = vals[5]; p.dX.x[2] = vals[6]; p.dX.x[3] = vals[7];
        
        // We ignore Y and Yp (indexes 8-15) for the analysis simulation phase
        // because we need to encrypt X and X^dX for exactly 5 rounds, 
        // whereas the file contains full 6-round encryption.

        data.push_back(p);
    }
    return data;
}

// --- Analysis ---
// Map: InputDiff (16bit) -> OutputDiff (16bit) -> Count
typedef map<uint16_t, map<uint16_t, long long>> DiffMap;

void diffWorker(const vector<PairData>& data, int start, int end,
                DiffMap& localFreq, map<uint16_t, long long>& localTotal,
                atomic<int>& processed, int total_n) 
{
    for (int i = start; i < end; ++i) {
        const PairData& p = data[i];

        // 1. Encrypt X for 5 rounds
        Block out1 = p.X;
        encryptRounds(out1, ANALYSIS_ROUNDS);

        // 2. Encrypt X' = X ^ dX for 5 rounds
        Block out2 = p.X;
        out2.x[0] ^= p.dX.x[0];
        out2.x[1] ^= p.dX.x[1];
        out2.x[2] ^= p.dX.x[2];
        out2.x[3] ^= p.dX.x[3];
        encryptRounds(out2, ANALYSIS_ROUNDS);

        // 3. Calculate Output Difference dY
        Block dY;
        dY.x[0] = out1.x[0] ^ out2.x[0];
        dY.x[1] = out1.x[1] ^ out2.x[1];
        dY.x[2] = out1.x[2] ^ out2.x[2];
        dY.x[3] = out1.x[3] ^ out2.x[3];

        uint16_t dX_packed = pack(p.dX);
        uint16_t dY_packed = pack(dY);

        localFreq[dX_packed][dY_packed]++;
        localTotal[dX_packed]++;

        // Progress update
        int done = ++processed;
        if (done % 500000 == 0 && start == 0) { // approximate check
             // Printing from worker logic is messy, handled loosely here or better in main
        }
    }
}

int main() {
    cout << "Loading data..." << endl;
    vector<PairData> data = loadPairs("pairs_data.txt");
    if (data.empty()) {
        cerr << "No data loaded from pairs_data.txt. Run generator first.\n";
        return 1;
    }

    int n = (int)data.size();
    cout << "Loaded " << n << " pairs." << endl;

    int perThread = (n + NUM_THREADS - 1) / NUM_THREADS;
    atomic<int> processed(0);

    vector<DiffMap> localFreq(NUM_THREADS);
    vector<map<uint16_t, long long>> localTotal(NUM_THREADS);
    vector<thread> threads;

    cout << "Starting analysis on " << NUM_THREADS << " threads..." << endl;

    for (int t = 0; t < NUM_THREADS; ++t) {
        int s = t * perThread;
        int e = min(n, (t + 1) * perThread);
        if (s >= e) break;
        threads.emplace_back(diffWorker, ref(data), s, e, 
                             ref(localFreq[t]), ref(localTotal[t]), 
                             ref(processed), n);
    }

    // Monitor progress
    while(processed < n) {
        int d = processed.load();
        double pct = 100.0 * d / n;
        cout << "\rProgress: " << fixed << setprecision(1) << pct << "%" << flush;
        this_thread::sleep_for(chrono::seconds(1));
    }
    cout << endl;

    for (auto& th : threads) th.join();

    cout << "Merging results..." << endl;

    // Merge results
    DiffMap globalFreq;
    map<uint16_t, long long> globalTotal;
    for (int t = 0; t < NUM_THREADS; ++t) {
        for (auto& kv : localTotal[t]) globalTotal[kv.first] += kv.second;
        for (auto& dx : localFreq[t]) {
            auto& gdx = globalFreq[dx.first];
            for (auto& dy : dx.second) gdx[dy.first] += dy.second;
        }
    }

    cout << "Writing top differentials..." << endl;

    // --- Output Global Top ---
    ofstream fout("diff_round_5_top.txt");
    vector< tuple<double, long long, uint16_t, uint16_t> > all_diffs; // prob, count, dX, dY

    for (auto& dx_pair : globalFreq) {
        uint16_t dX_val = dx_pair.first;
        long long total = globalTotal[dX_val];
        if (total == 0) continue;

        for (auto& dy_pair : dx_pair.second) {
            uint16_t dY_val = dy_pair.first;
            long long count = dy_pair.second;
            double p = (double)count / total;
            all_diffs.emplace_back(p, count, dX_val, dY_val);
        }
    }

    // Sort by probability descending
    sort(all_diffs.begin(), all_diffs.end(),
        [](auto& a, auto& b) { return get<0>(a) > get<0>(b); });

    fout << fixed << setprecision(6);
    fout << "Top Differentials after " << ANALYSIS_ROUNDS << " rounds (Variant 5):\n\n";
    
    for (int i = 0; i < 200 && i < (int)all_diffs.size(); ++i) {
        auto& e = all_diffs[i];
        Block dX = unpack(get<2>(e));
        Block dY = unpack(get<3>(e));
        
        fout << i + 1 << ") dX=(" 
             << (int)dX.x[0] << "," << (int)dX.x[1] << "," << (int)dX.x[2] << "," << (int)dX.x[3] << ") "
             << "-> dY=(" 
             << (int)dY.x[0] << "," << (int)dY.x[1] << "," << (int)dY.x[2] << "," << (int)dY.x[3] << ") "
             << " count=" << get<1>(e)
             << " p=" << get<0>(e) << "\n";
    }
    fout.close();
    cout << "Saved to diff_round_5_top.txt" << endl;

    return 0;
}