#ifndef CIPHER_ENGINE_H
#define CIPHER_ENGINE_H

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>

// --- КОНСТАНТЫ ВАРИАНТА 5 ---

// Количество раундов
const int NUM_ROUNDS = 6;

// S-Box (G): {13, 6, 0, 10, 15, 7, 14, 11, 9, 1, 5, 3, 4, 12, 8, 2}
const uint8_t SBOX[16] = {
    13, 6, 0, 10, 15, 7, 14, 11, 9, 1, 5, 3, 4, 12, 8, 2
};

// Раундовые ключи (6 штук). 
// Расписание: k1=t1, k2=t2, k3=t3, k4=t4, k5=t1, k6=t2
const uint8_t ROUND_KEYS[6] = {
    0x1, 0x3, 0x5, 0x7, 0x1, 0x3
};

// --- СТРУКТУРЫ ДАННЫХ ---

// Блок данных: 4 ниббла (по 4 бита)
// x[0], x[1], x[2], x[3]
struct Block {
    uint8_t x[4];

    // Оператор равенства для удобства сравнения
    bool operator==(const Block& other) const {
        for(int i=0; i<4; ++i) if(x[i] != other.x[i]) return false;
        return true;
    }
};

// --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ---

// Безопасное чтение S-Box (на случай выхода за границы, хотя тип uint8_t)
inline uint8_t G(uint8_t val) {
    return SBOX[val & 0xF];
}

// Печать блока (для отладки)
inline void printBlock(const Block& b) {
    std::cout << "[" 
              << (int)b.x[0] << " " << (int)b.x[1] << " " 
              << (int)b.x[2] << " " << (int)b.x[3] << "]";
}

// --- УТИЛИТЫ ДЛЯ ЛИНЕЙНОГО АНАЛИЗА ---

// Упаковка Block в uint16_t: x0 (старшие) .. x3 (младшие)
// Порядок: [x0][x1][x2][x3] -> 0x0123
inline uint16_t packBlock(const Block& b) {
    return (uint16_t)((b.x[0] << 12) | (b.x[1] << 8) | (b.x[2] << 4) | b.x[3]);
}

// Распаковка uint16_t в Block
inline Block unpackBlock(uint16_t val) {
    Block b;
    b.x[0] = (val >> 12) & 0xF;
    b.x[1] = (val >> 8) & 0xF;
    b.x[2] = (val >> 4) & 0xF;
    b.x[3] = val & 0xF;
    return b;
}

// Функция четности (Parity): 1 если число единиц нечетное, 0 если четное.
// Аналог скалярного произведения векторов.
inline uint8_t parity(uint16_t val) {
    return __builtin_parity(val); 
    // Для компиляторов без builtin: 
    // val ^= val >> 8; val ^= val >> 4; val ^= val >> 2; val ^= val >> 1; return val & 1;
}

// --- ЯДРО ШИФРА (CORE LOGIC) ---

// Функция раунда F(X2, X3, k) = G(X2 ^ G(k ^ X3))
inline uint8_t F(uint8_t x2, uint8_t x3, uint8_t k) {
    uint8_t inner = G(k ^ x3);
    return G(x2 ^ inner);
}

// Полное шифрование (6 раундов)
// Логика сдвига: temp=X0^F(...); X0=X1; X1=X2; X2=X3; X3=temp;
inline void encrypt(Block& b) {
    for (int r = 0; r < NUM_ROUNDS; ++r) {
        uint8_t k = ROUND_KEYS[r];
        
        // Вычисляем новое значение
        uint8_t temp = b.x[0] ^ F(b.x[2], b.x[3], k);
        
        // Сдвиг
        b.x[0] = b.x[1];
        b.x[1] = b.x[2];
        b.x[2] = b.x[3];
        b.x[3] = temp;
    }
}

// Шифрование на N раундов (для анализатора, где нужно 5 раундов)
inline void encryptRounds(Block& b, int rounds) {
    for (int r = 0; r < rounds && r < NUM_ROUNDS; ++r) {
        uint8_t k = ROUND_KEYS[r];
        uint8_t temp = b.x[0] ^ F(b.x[2], b.x[3], k);
        b.x[0] = b.x[1];
        b.x[1] = b.x[2];
        b.x[2] = b.x[3];
        b.x[3] = temp;
    }
}

// Обратный шаг одного раунда (для атаки)
// Вход: состояние ПОСЛЕ раунда (Y0, Y1, Y2, Y3)
// Выход: состояние ДО раунда
// Логика обратного сдвига:
// Old_X3 = Current_X2 (Y2)
// Old_X2 = Current_X1 (Y1)
// Old_X1 = Current_X0 (Y0)
// Old_X0 = Current_X3 (Y3) ^ F(Old_X2, Old_X3, k)
//        = Current_X3 ^ F(Current_X1, Current_X2, k)
inline void decryptOneRound(Block& b, uint8_t k) {
    uint8_t old_x0 = b.x[3] ^ F(b.x[1], b.x[2], k);
    
    b.x[3] = b.x[2];
    b.x[2] = b.x[1];
    b.x[1] = b.x[0];
    b.x[0] = old_x0;
}

#endif // CIPHER_ENGINE_H
