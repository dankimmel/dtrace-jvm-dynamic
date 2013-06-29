#include <jvmti.h>
#include <stdlib.h>
#include <string.h> // memset

#include "libusdt/usdt.h"

#define UNUSED __attribute__ ((unused))
; // <-- fixes syntax highlighting

static usdt_provider_t *provider;
static usdt_probedef_t *probe;
static char *type0 = "int";
static char **types = &type0;
static int iii = 15;
static void *arg0 = &iii;
static void **args = &arg0;

void JNICALL handler_vminit(jvmtiEnv *ti, JNIEnv *jni, jthread thread);
void JNICALL handler_breakpoint(jvmtiEnv *ti, JNIEnv *jni, jthread thread, jmethodID method, jlocation location);

static void check_jvmti_error(jvmtiEnv *env UNUSED, jvmtiError err, char *msg)
{
    if (err != JVMTI_ERROR_NONE) {
        printf("JVMTI error (%d): %s\n", err, msg);
        fflush(stdout);
    }
    // TODO should exit program - not sure what to do here
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options UNUSED, void *reserved UNUSED)
{
    jvmtiEnv *ti;
    jvmtiEventCallbacks event_cbs;
    jvmtiCapabilities caps;
    jvmtiError err;

    printf("Loading module now.\n");

    (*jvm)->GetEnv(jvm, (void **) &ti, JVMTI_VERSION_1_0);

    /* Set up callbacks, capabilities, and enable breakpoint events. */

    (void) memset(&event_cbs, 0, sizeof(jvmtiEventCallbacks));
    event_cbs.VMInit = handler_vminit;
    event_cbs.Breakpoint = handler_breakpoint;

    check_jvmti_error(ti, err, "Unable to load callbacks");

    (void) memset(&caps, 0, sizeof(jvmtiCapabilities));
    caps.can_generate_breakpoint_events = 1;

    /*
     * Ennable event notifications.
     */
    if ((err = (*ti)->SetEventCallbacks(ti, &event_cbs,
            sizeof(jvmtiEventCallbacks)) != JVMTI_ERROR_NONE) != JVMTI_ERROR_NONE
        || (err = (*ti)->AddCapabilities(ti, &caps) != JVMTI_ERROR_NONE)
        || (err = (*ti)->SetEventNotificationMode(ti, JVMTI_ENABLE,
            JVMTI_EVENT_VM_INIT, NULL)) != JVMTI_ERROR_NONE
        || (err = (*ti)->SetEventNotificationMode(ti, JVMTI_ENABLE,
            JVMTI_EVENT_BREAKPOINT, NULL)) != JVMTI_ERROR_NONE) {
    }

    fflush(stdout);

    return JNI_OK;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm UNUSED)
{
    printf("Unloading module now.\n");
    usdt_probe_release(probe);
    usdt_provider_disable(provider);
    usdt_provider_free(provider);
}

void JNICALL handler_breakpoint(jvmtiEnv *ti UNUSED, JNIEnv *jni UNUSED, jthread thread UNUSED, jmethodID method UNUSED, jlocation location UNUSED)
{
    jvmtiError err;
    char *name, *signature, *generic;

    err = (*ti)->GetMethodName(ti, method, &name, &signature, &generic);
    if (err != JVMTI_ERROR_NONE) {
        printf("Unable to get method name information on breakpoint\n");
        goto out;
    }

    printf("Hit breakpoint on method:   %s   %s   %s\n", name, signature, generic);

    usdt_fire_probe(probe->probe, 1, args);

    (*ti)->Deallocate(ti, name);
    (*ti)->Deallocate(ti, signature);
    (*ti)->Deallocate(ti, generic);
out:
    fflush(stdout);
}

// TODO will still be called if I attach the agent after starting the JVM?
void JNICALL handler_vminit(jvmtiEnv *ti UNUSED, JNIEnv *jni UNUSED, jthread thread UNUSED)
{
    jclass klass;
    jmethodID mid;
    jvmtiError err;
    jlocation begin, end;

    printf("Running handler_vminit.\n");

    /* Get a class by name, then add a breakpoint to one of its methods by name. */

    klass = (*jni)->FindClass(jni, "java/lang/String");
    if (klass == NULL) {
        printf("Error: unable to find class\n");
    }
    (*jni)->ExceptionClear(jni);
    mid = (*jni)->GetStaticMethodID(jni, klass, "valueOf", "(I)Ljava/lang/String;");
    // TODO note that GetMethodID and GetStaticMethodID are different here...
    if (mid == NULL) {
        printf("Error: unable to find method\n");
    }
    (*jni)->ExceptionClear(jni);

    err = (*ti)->GetMethodLocation(ti, mid, &begin, &end);
    check_jvmti_error(ti, err, "Unable to find method location");
    (*jni)->ExceptionClear(jni);

    err = (*ti)->SetBreakpoint(ti, mid, (jlocation) 0); // TODO probably will not work... we'll see
    check_jvmti_error(ti, err, "Unable to set breakpoint");

    provider = usdt_create_provider("javafast", "java.lang.String"); // TODO get $pid and sprintf() it into this string, decide on module==class issue
    probe = usdt_create_probe("valueOf(char[],int,int)java.lang.String", "entry", 1, types);
    usdt_provider_add_probe(provider, probe);
    usdt_provider_enable(provider);

    fflush(stdout);
}
