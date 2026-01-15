#include "cipher_engine.h"
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <thread>
#include <mutex>

// Количество текстов для поиска смещения (Bias)
// Чем больше, тем точнее, но дольше.
// Для 16-битного шифра 100,000 должно хватить для обнаружения сильных смещений.
const int NUM_SAMPLES = 100000;

struct LinearResult {
    uint16_t mask_in;
    uint16_t mask_out;
    double bias;
};

// Сортировка по убыванию смещения
bool compareResults(const LinearResult& a, const LinearResult& b) {
    return std::abs(a.bias) > std::abs(b.bias);
}

// Генератор масок с ограниченным весом Хэмминга
std::vector<uint16_t> generateMasks(int max_weight) {
    std::vector<uint16_t> masks;
    // Всегда добавляем 0 (хотя он не нужен для атаки, но пусть будет для полноты)
    // Начинаем с 1, так как маска 0 бесполезна (parity всегда 0)
    for (int i = 1; i < 65536; ++i) {
        if (__builtin_popcount(i) <= max_weight) {
            masks.push_back(i);
        }
    }
    return masks;
}

int main() {
    std::cout << "--- Linear Characteristic Search (5 Rounds) ---" << std::endl;

    // 1. Генерация данных (Known Plaintext)
    std::cout << "Generating " << NUM_SAMPLES << " samples..." << std::endl;
    std::vector<Block> plaintexts(NUM_SAMPLES);
    std::vector<Block> ciphertexts5(NUM_SAMPLES); // После 5 раундов

    std::mt19937 rng(12345); // Фиксированный seed
    std::uniform_int_distribution<uint16_t> dist(0, 65535);

    for (int i = 0; i < NUM_SAMPLES; ++i) {
        uint16_t val = dist(rng);
        plaintexts[i] = unpackBlock(val);
        
        Block temp = plaintexts[i];
        encryptRounds(temp, 5); // 5 раундов!
        ciphertexts5[i] = temp;
    }

    // Предварительно упакуем в uint16_t для скорости
    std::vector<uint16_t> P_packed(NUM_SAMPLES);
    std::vector<uint16_t> C_packed(NUM_SAMPLES);
    for(int i=0; i<NUM_SAMPLES; ++i) {
        P_packed[i] = packBlock(plaintexts[i]);
        C_packed[i] = packBlock(ciphertexts5[i]);
    }

    // 2. Генерация масок для перебора
    // Ограничим вес масок, чтобы не перебирать 4 миллиарда пар.
    // Вес <= 3 дает ~576 масок. 576 * 576 = 330,000 комбинаций. Это очень быстро.
    // Вес <= 4 дает ~2500 масок. 2500^2 = 6.25 млн. Тоже приемлемо.
    int max_w = 3; 
    std::cout << "Generating masks with Hamming Weight <= " << max_w << "..." << std::endl;
    std::vector<uint16_t> masks = generateMasks(max_w);
    std::cout << "Total masks to check: " << masks.size() << std::endl;
    std::cout << "Total pairs (In/Out): " << (long long)masks.size() * masks.size() << std::endl;

    // 3. Поиск (Brute-force)
    std::cout << "Starting search (using single thread for simplicity)..." << std::endl;
    
    std::vector<LinearResult> top_results;
    double min_bias_threshold = 0.02; // Порог для сохранения (2%)

    // Чтобы ускорить, можно распараллелить внешний цикл, но для 300к итераций это не критично.
    for (uint16_t m_in : masks) {
        for (uint16_t m_out : masks) {
            
            int count = 0;
            for (int i = 0; i < NUM_SAMPLES; ++i) {
                // Уравнение: parity(MaskIn & P) ^ parity(MaskOut & C) == 0
                uint8_t bit_in = parity(m_in & P_packed[i]);
                uint8_t bit_out = parity(m_out & C_packed[i]);
                
                if (bit_in == bit_out) {
                    count++;
                }
            }

            double bias = ((double)count / NUM_SAMPLES) - 0.5;
            
            if (std::abs(bias) > min_bias_threshold) {
                top_results.push_back({m_in, m_out, bias});
            }
        }
    }

    // 4. Сортировка и вывод
    std::sort(top_results.begin(), top_results.end(), compareResults);

    std::cout << "Found " << top_results.size() << " characteristics with |bias| > " << min_bias_threshold << std::endl;

    std::ofstream outfile("linear_result_5_rounds.txt");
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    outfile << "Top Linear Characteristics (5 rounds)\n";
    outfile << "Format: MaskIn(hex) -> MaskOut(hex) | Bias\n\n";

    for (int i = 0; i < std::min((int)top_results.size(), 50); ++i) {
        const auto& res = top_results[i];
        outfile << "MaskIn=0x" << std::hex << res.mask_in 
                << " -> MaskOut=0x" << res.mask_out 
                << " | Bias=" << std::dec << std::fixed << std::setprecision(6) << res.bias << "\n";
        
        std::cout << "Rank " << i+1 << ": In=" << std::hex << res.mask_in 
                  << " Out=" << res.mask_out << std::dec 
                  << " Bias=" << res.bias << std::endl;
    }
    
    outfile.close();
    std::cout << "Results saved to linear_result_5_rounds.txt" << std::endl;

    return 0;
}
