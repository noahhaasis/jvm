class Main {
  public static void main(String[] args) {
    fac();
  }

  public static int fac() {
    int n = five();
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
}
