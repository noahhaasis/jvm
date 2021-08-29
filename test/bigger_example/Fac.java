class Fac {
  private int n;

  public Fac(int n) {
    this.n = n;
  }

  public int compute() {
    if (n == 0) return 0;

    int res = 1;
    for (int i = 2; i <= n; i++) {
      res *= i;
    } 
    return res;
  }
}
