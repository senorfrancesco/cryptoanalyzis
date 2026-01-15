CXX = g++
CXXFLAGS = -O3 -pthread -std=c++17 -Wall

# Targets
all: generator analysis attack linear_search

generator: generator_of_data.cpp zmak.h
	$(CXX) $(CXXFLAGS) generator_of_data.cpp -o generator

analysis: analysis_attack.cpp zmak.h
	$(CXX) $(CXXFLAGS) analysis_attack.cpp -o analysis

attack: attack_last_round.cpp zmak.h
	$(CXX) $(CXXFLAGS) attack_last_round.cpp -o attack

# --- Linear Cryptanalysis Tools ---

linear_search: linear_search.cpp zmak.h
	$(CXX) $(CXXFLAGS) linear_search.cpp -o linear_search

generator_linear: generator_linear.cpp zmak.h
	$(CXX) $(CXXFLAGS) generator_linear.cpp -o generator_linear

attack_linear: attack_linear.cpp zmak.h
	$(CXX) $(CXXFLAGS) attack_last_round.cpp -o attack # Old differential attack
	$(CXX) $(CXXFLAGS) attack_linear.cpp -o attack_linear

clean:
	rm -f generator analysis attack linear_search generator_linear attack_linear pairs_data.txt diff_round_5_top.txt diff_round_5_by_dX.txt last_round_key_guess.txt linear_result_5_rounds.txt linear_data.txt linear_key_guess.txt *.o

# Тестовый запуск (полный цикл)
run_test: all
	@echo "--- 1. Generating Data ---"
	./generator
	@echo "--- 2. Analyzing Data ---"
	./analysis
	@echo "--- 3. Running Attack ---"
	./attack
