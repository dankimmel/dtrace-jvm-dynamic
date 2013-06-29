import java.lang.System;

public class MethodTest {
    public static void main(String[] args) throws InterruptedException {
        for (int i = 0; i < 50; i++) {
            Thread.sleep(1000);
            System.out.println(String.valueOf(i));
        }
    }
}
