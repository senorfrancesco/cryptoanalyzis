CXX = g++
CXXFLAGS = -O3 -pthread -std=c++17 -Wall -Iinclude

# Исходники и цели
SRC_DIFF = src/differential
SRC_LIN = src/linear

# Основные цели
all: differential linear

# Differential Tools
ddt_gen: $(SRC_DIFF)/ddt_analyzer.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_DIFF)/ddt_analyzer.cpp -o ddt_gen

trail_search: $(SRC_DIFF)/trail_search.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_DIFF)/trail_search.cpp -o trail_search

generator: $(SRC_DIFF)/generator_of_data.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_DIFF)/generator_of_data.cpp -o generator

analysis: $(SRC_DIFF)/analysis_attack.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_DIFF)/analysis_attack.cpp -o analysis

attack: $(SRC_DIFF)/attack_last_round.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_DIFF)/attack_last_round.cpp -o attack

differential: ddt_gen trail_search generator analysis attack

# Linear Tools
linear_search: $(SRC_LIN)/linear_search.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_LIN)/linear_search.cpp -o linear_search

generator_linear: $(SRC_LIN)/generator_linear.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_LIN)/generator_linear.cpp -o generator_linear

attack_linear: $(SRC_LIN)/attack_linear.cpp include/cipher_engine.h
	$(CXX) $(CXXFLAGS) $(SRC_LIN)/attack_linear.cpp -o attack_linear

linear: linear_search generator_linear attack_linear

# --- Automation ---

# Полный прогон дифференциальной атаки
run_diff: differential
	@echo "--- 1. Generating DDT ---"
	./ddt_gen
	@echo "--- 2. Finding Optimal Trail ---"
	./trail_search
	@echo "--- 3. Generating Data (Adaptive) ---"
	./generator
	@echo "--- 4. Analyzing Data ---"
	./analysis
	@echo "--- 5. Running Attack ---"
	./attack

# Полный прогон линейной атаки
run_linear: linear
	@echo "--- Running Linear Attack Pipeline ---"
	./linear_search
	./generator_linear
	./attack_linear

# Очистка
clean:
	rm -f generator analysis attack linear_search generator_linear attack_linear ddt_gen trail_search
	rm -f pairs_data.txt diff_round_5_top.txt diff_round_5_by_dX.txt last_round_key_guess.txt
	rm -f linear_result_5_rounds.txt linear_data.txt linear_key_guess.txt
	rm -f trail_results.txt trail_debug.txt ddt_pretty.txt ddt_table.bin
	rm -f pairs_data_part_*.txt
	rm -f *.o
