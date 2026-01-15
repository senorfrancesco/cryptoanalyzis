#include "cipher_engine.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

// Константы из linear_result_5_rounds.txt (Rank 1)
const uint16_t TARGET_MASK_IN = 0x4;
const uint16_t TARGET_MASK_OUT = 0x2140;

struct KeyScore {
    int key;
    double bias;
    int count; // Сколько раз уравнение выполнилось
};

bool compareKeyScores(const KeyScore& a, const KeyScore& b) {
    return a.bias > b.bias;
}

int main() {
    std::cout << "--- Linear Attack (Variant 5) ---" << std::endl;
    std::cout << "Target Mask IN:  0x" << std::hex << TARGET_MASK_IN << std::endl;
    std::cout << "Target Mask OUT: 0x" << TARGET_MASK_OUT << std::dec << std::endl;

    // 1. Загрузка данных
    std::vector<uint16_t> P_data;
    std::vector<uint16_t> C_data;
    
    std::ifstream infile("linear_data.txt");
    if (!infile.is_open()) {
        std::cerr << "Error opening linear_data.txt!" << std::endl;
        return 1;
    }

    uint16_t p_val, c_val;
    while (infile >> std::hex >> p_val >> c_val) {
        P_data.push_back(p_val);
        C_data.push_back(c_val);
    }
    infile.close();

    int N = P_data.size();
    std::cout << "Loaded " << N << " pairs." << std::endl;

    // 2. Атака (перебор ключа)
    std::vector<int> scores(16, 0); // Счетчики совпадений для каждого ключа (0..15)

    for (int i = 0; i < N; ++i) {
        uint16_t P = P_data[i];
        Block C_block = unpackBlock(C_data[i]);

        // Бит уравнения от открытого текста (константа для пары)
        uint8_t bit_P = parity(P & TARGET_MASK_IN);

        for (int k = 0; k < 16; ++k) {
            // Частичное дешифрование последнего раунда
            Block C_prev = C_block;
            decryptOneRound(C_prev, (uint8_t)k);
            
            uint16_t val_prev = packBlock(C_prev);

            // Бит уравнения от выхода 5-го раунда
            uint8_t bit_C5 = parity(val_prev & TARGET_MASK_OUT);

            // Если уравнение выполняется (bit_P ^ bit_C5 == 0)
            if ((bit_P ^ bit_C5) == 0) {
                scores[k]++;
            }
        }
    }

    // 3. Анализ результатов
    std::vector<KeyScore> results;
    for (int k = 0; k < 16; ++k) {
        double diff = std::abs((double)scores[k] - (double)N / 2.0);
        double bias = diff / (double)N;
        results.push_back({k, bias, scores[k]});
    }

    std::sort(results.begin(), results.end(), compareKeyScores);

    // 4. Вывод
    std::ofstream outfile("linear_key_guess.txt");
    std::cout << "\nTop Key Candidates:\n";
    
    for (int i = 0; i < 16; ++i) {
        const auto& res = results[i];
        std::cout << "Key = " << res.key << " (" << std::hex << res.key << std::dec 
                  << ") | Bias: " << std::fixed << std::setprecision(5) << res.bias 
                  << " | Matches: " << res.count << "/" << N << std::endl;
        
        outfile << "Key=" << res.key << " Bias=" << res.bias << " Matches=" << res.count << "\n";
    }
    outfile.close();
    
    std::cout << "\nResults saved to linear_key_guess.txt" << std::endl;

    return 0;
}
