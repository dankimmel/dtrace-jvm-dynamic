# dtrace-jvm-dynamic

Allowing efficient DTrace dynamic probes to run against JVM languages.


## Status

This library is not completed. It will not be useful or stable until significantly more work has been completed. Here is a list of milestones we must reach before we will consider the work to be complete and stable. (Descriptions of most technical terms and libraries are given in the "Design" section below.)

1. Create a Java agent which can set a breakpoint on a Java method entrypoint.
2. Make the agent receive notification that the breakpoint is reached and print something to the terminal.
3. After printing something, make the agent resume execution of the stopped thread.
4. Make the agent read arguments in from the command line that causes it to be loaded by the Java process.
5. Translate those arguments into breakpoints which should be set.
6. Link `libusdt` into the agent and make sure its functions are accessible inside the JVM.
7. Define a new provider `java$pid` and create static probes in that provider corresponding to the arguments passed into the agent. DTrace should be able to find these and set breakpoints on them, even though they aren't being called yet.
8. Make the breakpoints trigger their respective USDT probes and ensure DTrace is collecting data.
9. Test with some heavier workloads, see what goes wrong, fix it, repeat until it basically works.
10. Celebrate!

There are a few more steps which can be addressed after this minimum functionality has been achieved - those are detailed below in the "Open Questions" section.


## Purpose

Running DTrace against Java and other high-level languages is an unsatisfactory experience. Here is what it looks like today:

    $ dtrace -n '
        hotspot_jni$target:::method-entry
        /stringof(copyin(arg1, arg2)) == "java/io/BufferedInputStream" &&
         stringof(copyin(arg3, arg4)) == "read"/
        {
                @[jstack()] = count();
        }
    ' -c 'java Main'

Ideally, the probe's syntax and structure would more closely resemble probes which are available for C, which give access to method arguments, global variables, and such with C `struct`-like syntax.

Furthermore, the approach shown above causes every Java method call to generate a trap into DTrace to determine whether the method we care about was called. Before executing the trap, the Java program must also generate string representations of the class and method to pass into the kernel. These issues cause performance degradation to become so significant as to outweigh the benefits DTrace normally provides.

As it stands, you would be better-served using [Byteman](https://www.jboss.org/byteman) or [some other Java code injection framework](http://java.dzone.com/articles/practical-introduction-code) to monitor your code both because of convenience *and* performance.

However, DTrace promises something which other debugger tools can't - it's not tied to a particular language, and as a result it can be used to instrument almost everything in the kernel and native userland code with minimal performance impact, easily providing answers to both simple and difficult questions. In addition to being available everywhere, D scripts can debug multiple data sources at once, making it easy to correlate function calls across libraries, threads, processes, or the user-kernel boundary. As a result, DTrace is one of the best performance monitoring and debugging tools available, and in order to rise to prominence, it will need better integration with high level languages. Because of Java's popularity and the diverse ecosystem of languages which can run alongside it in the JVM, the JVM is an ideal initial target for DTrace integration.


## Design

The JVM provides a set of native code APIs collectively known as the [JVM Tool Interface](http://www.oracle.com/technetwork/articles/javase/jvmti-136367.html) which can be accessed by Java "agents." An agent is essentially just a shared library which can be loaded either at the beginning of the Java process or during the middle of execution, and Java debuggers and profilers are all agents of some kind or another. An agent is able to manipulate the state of the JVM directly using [a standard set of functions](http://docs.oracle.com/javase/1.5.0/docs/guide/jvmti/jvmti.html#FunctionIndex). Every function is not necessarily supported on every JVM implementation, but according to informal readings online, Oracle and OpenJVM implementations since Java 5 should support (almost?) all of them as a replacement for the old debugger APIs (known as the JVM Debugger Interface). They may not be available on IBM JVMs, although that has not yet been confirmed.

Typically, profilers and debuggers for Java are actually written inside a separate JVM in another process. To facilitate this, Java implementations with JVMTI support also include standard library APIs known as the JDI (Java Debugger Interface) for all the functions listed above and pass commands through the `jdwp` (Java Debugging Wire Protocol) agent. However, using the Java interfaces for these functions from a second process would complicate and slow down the second half of our project, which ties the Java debugger we're running in with DTrace.

The issue with probing dynamic languages is that DTrace only knows how to probe by writing breakpoint instructions over native code. However, there are no sections of native code which associated in a 1:1 relation with each Java method call, so currently DTrace just probes all method calls (I believe there is a single codepath for all of them which is written in native code). This single probe point is defined at compile time using the DTrace User Statically Defined Tracing (USDT) framework. This isn't terribly useful if you don't even know what Java method will exist until after the JVM can read them in, but the [`libusdt`](https://github.com/chrisa/libusdt) library allows one to dynamically generate USDT probing points which can be called from native code. As far as I know, up until this point this has only been used to define static probe points from within a high-level language, but we can use them for dynamic tracing as well by adding a bit of stitching.

Now, we can finally see the design from beginning to end. We plan to write a Java agent in native code which uses the JVMTI APIs. When it is loaded into a JVM it will be handed some arguments defining the probe points which DTrace would like to see. Because DTrace's debugging expressions cannot be changed while it's running, these will all be known before the probes start running (and hence we won't have to pass more information to the Java agent while the probes are running). When it is loaded, the Java agent will read these arguments and place JVM breakpoints on these locations. It will also dynamically generate a USDT probe point for each of them, and when a JVM breakpoint is hit, the corresponding USDT probe will be called. This will generate the trap with minimal overhead; a truly ideal situation from a performance standpoint.


## Open Questions

While we've more or less figured out how to avoid the performance penalty, other issues and questions remain.

1. How can we make Java objects accessible inside DTrace scripts? Is there a way to do this so that it can be ported to languages which are not statically typed in the future?
2. How can we list all probes without enabling all of them for a short period of time (which would create a brief but significant performance impact)?
3. Will we need special logic to handle multiple DTrace instances probing the same JVM simultaneously?
4. Which JVMs, OSes, and processor architectures can we support?
5. Once we have a working tool, how can we integrate with the `dtrace(1m)` command line tool so you don't have to manually load the agent before attempting to run DTrace?
