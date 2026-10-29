
int fact(int n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main() {
    print(fact(5));          /* 120 */
    print(gcd(48, 18));      /* 6 */
    int sum = 0;
    for (int i = 1; i <= 10; i = i + 1) {
        if (i % 2 == 0) sum = sum + i;
    }
    print(sum);              /* 30 */
    print(2 + 3 * 4 - -1);   /* 15 */
    return 0;
}
