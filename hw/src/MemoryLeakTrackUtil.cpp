/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/Log.h>

#include "MemoryLeakTrackUtil.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "DTV_LOG"

/*
 * The code here originally resided in MediaPlayerService.cpp and was
 * shamelessly copied over to support memory leak tracking from
 * multiple places.
 */
namespace android {

#if defined(__arm__)

extern "C" void get_malloc_leak_info(uint8_t** info, size_t* overallSize,
        size_t* infoSize, size_t* totalMemory, size_t* backtraceSize);

extern "C" void free_malloc_leak_info(uint8_t* info);

// Use the String-class below instead of String8 to allocate all memory
// beforehand and not reenter the heap while we are examining it...
struct MyString8 {
    static const size_t MAX_SIZE = 256 * 1024;

    MyString8()
        : mPtr((char *)malloc(MAX_SIZE)) {
        *mPtr = '\0';
    }

    ~MyString8() {
        free(mPtr);
    }

    void append(const char *s) {
        strncat(mPtr, s, MAX_SIZE - size() - 1);
    }

    const char *string() const {
        return mPtr;
    }

    size_t size() const {
        return strlen(mPtr);
    }

    void clear() {
        *mPtr = '\0';
    }

private:
    char *mPtr;

    MyString8(const MyString8 &);
    MyString8 &operator=(const MyString8 &);
};

void dumpMemoryAddresses(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    MyString8 result;

    typedef struct {
        size_t size;
        size_t dups;
        intptr_t * backtrace;
    } AllocEntry;

    uint8_t *info = NULL;
    size_t overallSize = 0;
    size_t infoSize = 0;
    size_t totalMemory = 0;
    size_t backtraceSize = 0;

    get_malloc_leak_info(&info, &overallSize, &infoSize, &totalMemory, &backtraceSize);
    ALOGI("dumpMemoryAddresses start %d",__LINE__);
    if (info) {
        ALOGI("dumpMemoryAddresses end%d",__LINE__);
        uint8_t *ptr = info;
        size_t count = overallSize / infoSize;

        snprintf(buffer, SIZE, " Allocation count %i\n", count);
        result.append(buffer);
        snprintf(buffer, SIZE, " Total memory %i\n", totalMemory);
        result.append(buffer);

        AllocEntry * entries = new AllocEntry[count];

        for (size_t i = 0; i < count; i++) {
            // Each entry should be size_t, size_t, intptr_t[backtraceSize]
            AllocEntry *e = &entries[i];

            e->size = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->dups = *reinterpret_cast<size_t *>(ptr);
            ptr += sizeof(size_t);

            e->backtrace = reinterpret_cast<intptr_t *>(ptr);
            ptr += sizeof(intptr_t) * backtraceSize;
        }

        // Now we need to sort the entries.  They come sorted by size but
        // not by stack trace which causes problems using diff.
        bool moved;
        do {
            moved = false;
            for (size_t i = 0; i < (count - 1); i++) {
                AllocEntry *e1 = &entries[i];
                AllocEntry *e2 = &entries[i+1];

                bool swap = e1->size < e2->size;
                if (e1->size == e2->size) {
                    for (size_t j = 0; j < backtraceSize; j++) {
                        if (e1->backtrace[j] == e2->backtrace[j]) {
                            continue;
                        }
                        swap = e1->backtrace[j] < e2->backtrace[j];
                        break;
                    }
                }
                if (swap) {
                    AllocEntry t = entries[i];
                    entries[i] = entries[i+1];
                    entries[i+1] = t;
                    moved = true;
                }
            }
        } while (moved);

        write(fd, result.string(), result.size());
        //ALOGI("1%s", result.string()  );
        result.clear();

        for (size_t i = 0; i < count; i++) {
            AllocEntry *e = &entries[i];

            snprintf(buffer, SIZE, "size %8i, dup %4i, ", e->size, e->dups);
            result.append(buffer);
            for (size_t ct = 0; (ct < backtraceSize) && e->backtrace[ct]; ct++) {
                if (ct) {
                    result.append(", ");
                }
                snprintf(buffer, SIZE, "0x%08x", e->backtrace[ct]);
                result.append(buffer);
            }
            result.append("\n");

            write(fd, result.string(), result.size());
            //ALOGI("2%s", result.string()  );
            result.clear();
        }

        delete[] entries;
        free_malloc_leak_info(info);
    }
}

#else
// Does nothing
//void dumpMemoryAddresses(int fd __unused) {}

#endif
    static pid_t myPid = 0;
    static void signal_handler(int sig)
    {
        char file_path_name[64];
        static int idx = 0;
        ALOGI("dumpMemoryAddresses");
        snprintf(file_path_name, 63, "%s/unfree%d.log", "/data/vendor/dtvkit/", idx++);
        FILE *file = fopen(file_path_name, "w");
        if (file == NULL)
        {
            ALOGI("Failed to open file");
            return;
        }
        dumpMemoryAddresses(fileno(file));
        fclose(file);
    }

    extern "C" void MemoryLeakTrackUtil() {
        myPid = getpid();
        ALOGI("PID(%d): register snapshot SIG: %d\n", myPid, SIGUSR1);
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signal_handler;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGUSR1, &sa, NULL);
    }
}  // namespace android
