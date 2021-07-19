class Main {
  public static int MyField = -12;

  public static void main(String[] args) {
    five();
    fac(5);
    // fib(5);
  }

  public static int fac(int n) {
    if (n == 0) return 0;

    int res = 1;
    for (int i = 2; i <= n; i++) {
      res *= i;
    } 
    return res;
  }

  public static int five() {
    return 5;
  }

  public static int fib(int i) {
    if (i == 0 || i == 1) {
      return 1;
    }
    return fib(i-1) + fib(i-2);
  }
}
