#include "simulator.cpp"
#include "instruction.cpp"

int main(){
    Instruction a("Kmeans", 16, 65536, 1048576);
    a.generate_instruction("example.txt"); //generate instructions
    Simulator b("example.txt", (long long)pow(2, 32)); //simulate
    // b.simulate_hta((int)pow(2, 16), 4, 0);
    b.simulate_line(1, 1);
    // b.simulate_kwta(4096, 4, 0);
    b.print_res("result_1.txt");
    return 0;
}