class Main {
  public int three;

  public Main() {
    this.three = 3;
  }

  public static int MyField = 5;

  public static void main(String[] args) {
    five();
    (new Fac(5)).compute();
    (new Fib()).fib(5);
    two();

    Main main = new Main();
    main.getThree();
  }

  public static int two() {
    return Two.TWO;
  }

  public static int five() {
    return MyField;
  }

  public int getThree() {
    return this.three;
  }
}
