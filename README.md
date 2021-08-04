# A toy Java Virtual Machine
## Build
`./build/build.sh`

## Test
`./bin/main test/Main.class`

## Small snippet of code that is supported

Main.java
```java
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
```

Fac.java
```java
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
```

Fib.java
```java
class Fib {
  public int fib(int i) {
    if (i == 0 || i == 1) {
      return 1;
    }
    return fib(i-1) + fib(i-2);
  }
}
```

Two.java
```java
class Two {
  public static int TWO = 2;
}
```

To run:
```bash
./build/build.sh
cd test
javac Main.java
../bin/main Main
```
