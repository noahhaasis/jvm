class Main {
  public int three;

  public Main() {
    this.three = 3;
  }

  public static int MyField = 5;

  public static void main(String[] args) {
    five();
    fac(5);
    fib(5);
    two();

    Main main = new Main();
    main.getThree();
  }

  public static int two() {
    return Two.TWO;
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
    return MyField;
  }

  public int getThree() {
    return this.three;
  }

  public static int fib(int i) {
    if (i == 0 || i == 1) {
      return 1;
    }
    return fib(i-1) + fib(i-2);
  }
}
