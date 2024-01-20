#include "generator.hpp"
#include "generator.cpp"
#include <string.h>

int get_num(char* a){
    int size = strlen(a);
    int num = 0;
    for(int i = 0; i < size; i++){
        num = num * 10 + a[i] - '0';
    }
    return num;
}

using namespace std;
int main(int argc, char* argv[]){
    string target;
    string file;
    int depth;

    if (argc < 4){
        cout << "Too few input parameters" << endl;
        return 1;
    }else{
        target = argv[1];
        cout << "The target allocator type is: " << target<< endl;
        file = argv[2];
        cout << "The generated file is: " << file << endl;
        depth = get_num(argv[3]);
        // if(depth > 65536 || depth <= 64){
        //     cout << "Invalid input depth." << endl;
        //     return 1;
        // }
        cout << "The depth of the allocator is " << depth << endl;
    }
    Generator a(target, file, depth);
    a.generate_allocator();
    return 0;
}