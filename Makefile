# TODO location of jvmti.h and jni.h is Mac OS X-specific
# TODO only links against current headers (will need to check older ones too)
INCLUDE := /System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers
CFLAGS	:= -W -Wall -Wextra \
	-Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes \
	-dynamiclib -m64
# TODO might want a 32-bit version too

JNILIB 	:= libdtrace4j.jnilib
CSOURCE := main.c
CC 		:= clang

JSOURCE := ThreadTest.java MethodTest.java
JCLASS 	:= ThreadTest.class MethodTest.class
JAVAC  	:= javac
OUTPUT  := output


# astyle --style=kr main.c --indent=spaces=8 --indent-preprocessor
#        --min-conditional-indent=2 --pad-oper --unpad-paren
#        --align-pointer=name --add-brackets --convert-tabs


# "-agentlib" searches for lib"name".jnilib' on the classpath, including '.'
# this behavior is platform-dependent but works on Mac OS X
test: $(OUTPUT) all
	java -agentlib:dtrace4j=do_method_entry MethodTest > $(OUTPUT)/method_test.txt
	java -agentlib:dtrace4j=do_method_entry > $(OUTPUT)/usage_method_test.txt
	java -agentlib:dtrace4j ThreadTest threads > $(OUTPUT)/thread_test.txt
	java -agentlib:dtrace4j=do_method_entry ThreadTest > $(OUTPUT)/method_and_thread_tests.txt

$(OUTPUT):
	mkdir -p $(OUTPUT)

all: $(CSOURCE) $(JSOURCE) $(JNILIB) $(JCLASS)

$(JNILIB): $(CSOURCE)
	$(CC) $(CFLAGS) -isystem $(INCLUDE) -o $@ $(CSOURCE)

$(JCLASS): $(JSOURCE)
	$(JAVAC) $(JSOURCE)

clean:
	rm -f $(JNILIB)
	rm -f $(JCLASS)
	rm -rf $(OUTPUT)
