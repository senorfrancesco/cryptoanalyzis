CXX = g++
CXXFLAGS = -O3 -pthread -std=c++17 -Wall

# Цели по умолчанию
all: generator analysis attack

# Компиляция генератора
generator: generator_of_data.cpp zmak.h
	$(CXX) $(CXXFLAGS) generator_of_data.cpp -o generator

# Компиляция анализатора
analysis: analysis_attack.cpp zmak.h
	$(CXX) $(CXXFLAGS) analysis_attack.cpp -o analysis

# Компиляция атаки
attack: attack_last_round.cpp zmak.h
	$(CXX) $(CXXFLAGS) attack_last_round.cpp -o attack

# Очистка
clean:
	rm -f generator analysis attack pairs_data.txt diff_round_5_top.txt diff_round_5_by_dX.txt last_round_key_guess.txt *.o

# Тестовый запуск (полный цикл)
run_test: all
	@echo "--- 1. Generating Data ---"
	./generator
	@echo "--- 2. Analyzing Data ---"
	./analysis
	@echo "--- 3. Running Attack ---"
	./attack
