//
// Created by dell on 2023/2/9.
//
#include <sstream>

int covert2Int(const char* buffer, int i, int j) {
    char* numStr = new char[j-i+1];
    for(int k=i;k<j;k++) {
        numStr[k-i] = buffer[k];
    }
    std::stringstream ss;
    ss << std::hex << numStr;
    int res = 0;
    ss >> res;
    return res;
}

