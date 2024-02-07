# Shivam Singh
# shivam.27.iitd@gmail.com
# (+91)7753990027

CXX = g++
CXXFLAGS = -std=c++17
LIBS = -lboost_system -lboost_thread -pthread -lboost_chrono -lboost_iostreams

main: main.cpp
	@echo "Project by Shivam Singh"
	@echo "email: shivam.27.iitd@gmail.com"
	@echo "mobile: (+91)7753990027"
	$(CXX) $(CXXFLAGS) -o server main.cpp $(LIBS) -w

.PHONY: clean
clean:
	rm -f server