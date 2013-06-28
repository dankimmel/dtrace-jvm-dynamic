import java.lang.System;
import java.lang.Thread;

public class ThreadTest extends Thread {

  private static Object lock = new Object();

  public static void main(String[] args) {
    System.out.println("J: In Java");
    ThreadTest me = new ThreadTest();
    me.go();
  }

  private void go() {
    for (int i = 0; i < 10; i++) {
      Thread t = new ThreadTest();
      t.setDaemon(true);
      t.start();
    }

    for (int i = 0; i < 100; i++) {
      synchronized (lock) {
        lock.notify();
      }
      try {
        Thread.sleep(10 + (long)(Math.random() * 10d));
      } catch (InterruptedException ie) {
      }
    }
  }

  public void run() {
    while (true) {
      synchronized (lock) {
        try {
          System.out.println("J: Thread " + Thread.currentThread().getName() + " waiting.");
          lock.wait();
          System.out.println("J: Thread " + Thread.currentThread().getName() + " awakened.");
        } catch (InterruptedException ie) {
        }
      }

      try {
        Thread.sleep(100);
      } catch (InterruptedException ie) {
      }
    }
  }
}
