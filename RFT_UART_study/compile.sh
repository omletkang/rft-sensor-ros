
g++ -c -o main.o main.cpp
g++ -c -o RFT_UART.o RFT_UART.cpp

g++ -o app main.o RFT_UART.o

rm -r main.o RFT_UART.o

echo "Compile Completed"

