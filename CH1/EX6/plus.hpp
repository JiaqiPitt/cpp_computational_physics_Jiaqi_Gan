#ifndef PLUS_HPP
#define PLUS_HPP

bool parse_arguments(
    int argc,
    char* argv[],
    int& first,
    int& second
);

int add(int first, int second);

void print_result(int first, int second, int result);

#endif

