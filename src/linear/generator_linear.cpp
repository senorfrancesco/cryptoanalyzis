#include "cipher_engine.h"
#include <vector>
#include <random>
#include <iostream>
#include <fstream>
#include <algorithm>

// Количество пар для атаки
// Смещения 0.125 (как мы нашли) очень сильное.
// Теоретически N ~ 1/(bias^2) = 64. 
// Возьмем 50,000 с огромным запасом.
const int NUM_PAIRS = 50000;

int main() {
    std::cout << "--- Linear Attack Data Generator (Variant 5) ---" << std::endl;
    std::cout << "Generating " << NUM_PAIRS << " Known Plaintext-Ciphertext pairs..." << std::endl;

    std::vector<uint16_t> P_data(NUM_PAIRS);
    std::vector<uint16_t> C_data(NUM_PAIRS);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint16_t> dist(0, 65535);

    // Можно распараллелить, но 50к это мгновенно.
    for (int i = 0; i < NUM_PAIRS; ++i) {
        uint16_t val = dist(rng);
        Block b = unpackBlock(val);
        
        // Сохраняем открытый текст
        P_data[i] = val;

        // Шифруем полными 6 раундами
        encrypt(b);

        // Сохраняем шифротекст
        C_data[i] = packBlock(b);
    }

    // Сохранение в файл
    // Формат: Plaintext(hex) Ciphertext(hex)
    std::ofstream outfile("linear_data.txt");
    if (!outfile.is_open()) {
        std::cerr << "Error opening output file!" << std::endl;
        return 1;
    }

    for (int i = 0; i < NUM_PAIRS; ++i) {
        outfile << std::hex << P_data[i] << " " << C_data[i] << "\n";
    }

    outfile.close();
    std::cout << "Data saved to linear_data.txt" << std::endl;

    return 0;
}
