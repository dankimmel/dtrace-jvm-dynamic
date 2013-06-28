#include <jvmti.h>
#include <stdlib.h>

#define UNUSED __attribute__ ((unused))

JavaVM *jvm;

jthread threads[10] = {0};
long delays[10] = {0};

void JNICALL VMInit(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread);
void JNICALL MethodEntry(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method);
void JNICALL MonitorWait(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jobject object, jlong timeout);
void JNICALL MonitorWaited(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jobject object, jboolean timed_out);

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *vm UNUSED, char *options, void *reserved UNUSED)
{
        (void) reserved;

        char do_method_entry = 0;

        jvmtiEnv *jvmti_env;
        jvmtiError err;
        jvmtiEventCallbacks callbacks;
        jvmtiCapabilities capabilities;

        printf("c: In OnLoad - option: %s\n", options);
        fflush(stdout);

        if(options != NULL) {
                do_method_entry = 1;
        }

        jvm = vm;
        (*jvm)->GetEnv(jvm, (void **)&jvmti_env, JVMTI_VERSION_1_0);

        callbacks.VMInit = VMInit;
        callbacks.MethodEntry = MethodEntry;
        callbacks.MonitorWait = MonitorWait;
        callbacks.MonitorWaited = MonitorWaited;

        err = (*jvmti_env)->SetEventCallbacks(jvmti_env, &callbacks, sizeof(jvmtiEventCallbacks));
        if(err != JVMTI_ERROR_NONE) {
                printf("c: SetCallbacks failed: %d\n", err);
                fflush(stdout);
        }

        err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, NULL);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: SetEventNotificationMode(VM_INIT) failed: %d\n", err);
                fflush(stdout);
        }

        capabilities.can_generate_method_entry_events = 1;
        capabilities.can_generate_monitor_events = 1;

        err = (*jvmti_env)->AddCapabilities(jvmti_env, &capabilities);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: AddCapabilities failed: %d\n", err);
                fflush(stdout);
        }

        if(do_method_entry) {
                err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, NULL);
                if(err != JVMTI_ERROR_NONE) {
                        printf("c: SetEventNotificationMode(METHOD_ENTRY) failed: %d\n", err);
                        fflush(stdout);
                }
        }

        err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAIT, NULL);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: SetEventNotificationMode(MONITOR_WAIT) failed: %d\n", err);
                fflush(stdout);
        }

        err = (*jvmti_env)->SetEventNotificationMode(jvmti_env, JVMTI_ENABLE, JVMTI_EVENT_MONITOR_WAITED, NULL);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: SetEventNotificationMode(MONITOR_WAITED) failed: %d\n", err);
                fflush(stdout);
        }

        return 0;
}

JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm UNUSED)
{
        printf("c: In OnUnLoad\n");
        fflush(stdout);
}

void JNICALL VMInit(jvmtiEnv *jvmti_env UNUSED, JNIEnv *jni_env UNUSED, jthread thread UNUSED)
{
        printf("c: In VMInit!\n");
        fflush(stdout);
}

void JNICALL MethodEntry(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jmethodID method)
{
        jvmtiError err;
        jclass clazz;
        char *name;
        char *signature;
        char *generic;
        jvmtiThreadInfo info;

        err = (*jvmti_env)->GetThreadInfo(jvmti_env, thread, &info);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: GetThreadInfo failed: %d\n", err);
        } else {
                printf("c: Thread %s", info.name);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) info.name);
                (*jni_env)->DeleteLocalRef(jni_env, info.thread_group);
                (*jni_env)->DeleteLocalRef(jni_env, info.context_class_loader);
        }

        err = (*jvmti_env)->GetMethodDeclaringClass(jvmti_env, method, &clazz);
        if(err != JVMTI_ERROR_NONE) {
                printf("\nc: GetMethodDeclaringClass failed: %d\n", err);
        }

        err = (*jvmti_env)->GetClassSignature(jvmti_env, clazz, &signature, &generic);
        if(err != JVMTI_ERROR_NONE) {
                printf("\nc: GetClassSignature failed: %d\n", err);
        } else {
                printf(" called Class %s/%s", signature, generic);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) signature);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) generic);
        }

        (*jni_env)->DeleteLocalRef(jni_env, clazz);

        err = (*jvmti_env)->GetMethodName(jvmti_env, method, &name, &signature, &generic);
        if(err != JVMTI_ERROR_NONE) {
                printf("\nc: GetMethodName failed: %d\n", err);
        } else {
                printf(" Method %s%s%s", name, signature, generic);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) name);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) signature);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) generic);
        }

        printf("\n");
        fflush(stdout);
}

void JNICALL MonitorWait(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jobject object, jlong timeout)
{
        int i;
        jvmtiError err;
        jlong time;
        jvmtiThreadInfo info;

        err = (*jvmti_env)->GetTime(jvmti_env, &time);
        for(i = 0; i < 10; i++) {
                if(threads[i] == 0) {
                        threads[i] = thread;
                        delays[i] = time;
                        break;
                }

                if(threads[i] == thread) {
                        delays[i] = time;
                        break;
                }
        }

        err = (*jvmti_env)->GetThreadInfo(jvmti_env, thread, &info);
        if(err != JVMTI_ERROR_NONE) {
                printf("c: GetThreadInfo failed: %d\n", err);
                fflush(stdout);
        } else {
                printf("c: Thread %s waiting: %d, %d, %d\n", info.name, thread, object, timeout);
                fflush(stdout);
                (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) info.name);
                (*jni_env)->DeleteLocalRef(jni_env, info.thread_group);
                (*jni_env)->DeleteLocalRef(jni_env, info.context_class_loader);
        }
}

void JNICALL MonitorWaited(jvmtiEnv *jvmti_env, JNIEnv *jni_env, jthread thread, jobject object UNUSED, jboolean timed_out UNUSED)
{
        char found = 0;
        int i;
        jvmtiError err;
        jvmtiThreadInfo info;
        jlong time;
        long delta;

        err = (*jvmti_env)->GetTime(jvmti_env, &time);
        for(i = 0; i < 10; i++) {
                if(threads[i] == thread) {
                        found++;
                        delta = time - delays[i];
                        delays[i] = 0;
                        break;
                }

        }
        if(!found) {
                printf("c: Orphaned monitor: %d\n", thread);
        } else {
                err = (*jvmti_env)->GetThreadInfo(jvmti_env, thread, &info);
                if(err != JVMTI_ERROR_NONE) {
                        printf("c: GetThreadInfo failed: %d\n", err);
                } else {
                        printf("c: Thread %s waited %ld (%0.2f)\n", info.name, delta, ((double) delta / 1e6));
                        (*jvmti_env)->Deallocate(jvmti_env, (unsigned char *) info.name);
                        (*jni_env)->DeleteLocalRef(jni_env, info.thread_group);
                        (*jni_env)->DeleteLocalRef(jni_env, info.context_class_loader);
                }
                fflush(stdout);
        }
}

