#include <jvmti.h>
#include <stdlib.h>
#include <string.h> // memset

#define UNUSED __attribute__ ((unused))
; // <-- fixes syntax highlighting

void JNICALL handler_vminit(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread);
void JNICALL handler_breakpoint(jvmtiEnv *jvmti_env, JNIEnv* jni_env, jthread thread, jmethodID method, jlocation location);

static void check_jvmti_error(jvmtiEnv *env UNUSED, jvmtiError err, char *msg) {
        if (err != JVMTI_ERROR_NONE) {
                printf("JVMTI error (%d): %s\n", err, msg);
                fflush(stdout);
        }
        // TODO should exit program - not sure what to do here
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options UNUSED, void *reserved UNUSED)
{
        jvmtiEnv *env;
        jvmtiEventCallbacks event_cbs;
        jvmtiCapabilities caps;
        jvmtiError err;

        printf("Loading module now.\n");

        (*jvm)->GetEnv(jvm, (void **) &env, JVMTI_VERSION_1_0);

        /* Set up callbacks, capabilities, and enable breakpoint events. */

        (void) memset(&event_cbs, 0, sizeof(jvmtiEventCallbacks));
        event_cbs.VMInit = handler_vminit;
        event_cbs.Breakpoint = handler_breakpoint;

        err = (*env)->SetEventCallbacks(env, &event_cbs, sizeof(jvmtiEventCallbacks));
        check_jvmti_error(env, err, "Unable to load callbacks");

        (void) memset(&caps, 0, sizeof(jvmtiCapabilities));
        caps.can_generate_breakpoint_events = 1;

        err = (*env)->AddCapabilities(env, &caps);
        check_jvmti_error(env, err, "Unable to add capabilities");

        err = (*env)->SetEventNotificationMode(env, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
        check_jvmti_error(env, err, "Unable to enable VMInit notifications");
        err = (*env)->SetEventNotificationMode(env, JVMTI_ENABLE, JVMTI_EVENT_BREAKPOINT, NULL);
        check_jvmti_error(env, err, "Unable to enable breakpoint event notifications");

        fflush(stdout);

        return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm UNUSED)
{
        printf("Unloading module now.\n");
        fflush(stdout);
}

void JNICALL handler_breakpoint(jvmtiEnv *jvmti_env UNUSED, JNIEnv* jni_env UNUSED, jthread thread UNUSED, jmethodID method UNUSED, jlocation location UNUSED)
{
        printf("hit a breakpoint!\n");
        fflush(stdout);
}

// TODO will still be called if I attach the agent after starting the JVM?
void JNICALL handler_vminit(jvmtiEnv *jvmti_env UNUSED, JNIEnv *jni_env UNUSED, jthread thread UNUSED)
{
        jclass klass;
        jmethodID mid;
        jvmtiError err;
        jlocation begin, end;

        printf("Running handler_vminit.\n");
        fflush(stdout);

        /* Get a class by name, then add a breakpoint to one of its methods by name. */

        klass = (*jni_env)->FindClass(jni_env, "java/lang/String");
        if (klass == NULL) {
                printf("Error: unable to find class\n");
        }
        (*jni_env)->ExceptionClear(jni_env);
        mid = (*jni_env)->GetStaticMethodID(jni_env, klass, "valueOf", "(I)Ljava/lang/String;");
        // TODO note that GetMethodID and GetStaticMethodID are different here...
        if (mid == NULL) {
                printf("Error: unable to find method\n");
        }
        (*jni_env)->ExceptionClear(jni_env);

        err = (*jvmti_env)->GetMethodLocation(jvmti_env, mid, &begin, &end);
        check_jvmti_error(jvmti_env, err, "Unable to find method location");
        (*jni_env)->ExceptionClear(jni_env);

        err = (*jvmti_env)->SetBreakpoint(jvmti_env, mid, (jlocation) 0); // TODO probably will not work... we'll see
        check_jvmti_error(jvmti_env, err, "Unable to set breakpoint");

        fflush(stdout);
}
