int abs(int i) {
    return i < 0 ? -i : i;
}
int max(int a, int b) {
    return a > b ? a : b;
}
int min(int a, int b) {
    return a < b ? a : b;
}
int pow(int base, int exp) {
    int result = 1;
    while (exp--) {
        result *= base;
    }
    return result;
}
int gcd(int a, int b) {
    while (b) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}
int lcm(int a, int b) {
    return a * b / gcd(a, b);
}

