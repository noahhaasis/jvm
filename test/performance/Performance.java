class Performance {
  public static void main(String[] args) {
    for (int i = 0; i < 5_000_000; i++) {
      fac(10);
    }
  }

  public static int fac(int n) {
    if (n == 0) {
      return 1;
    }

    int res = 1;

    for (int i = 2; i <= n; i++) {
      res *= i;
    }

    return res;
  }
}
