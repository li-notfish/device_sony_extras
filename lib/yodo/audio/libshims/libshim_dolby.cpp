//
// SPDX-FileCopyrightText: The LineageOS Project
// SPDX-License-Identifier: Apache-2.0
//

#include <log/log.h>
#include <dlfcn.h>

// RefBase stores its ref-count block in the `mRefs` member, which sits right
// after the vtable pointer, i.e. at offset 8 on 64-bit. The Dolby C2 service
// dereferences a Component that has already been destroyed (~RefBase clears
// mRefs to nullptr), so the refcount pointer is null while `this` is not.
// Guard on mRefs as well, otherwise the real incStrong/decStrong still faults
// at [mRefs + 4].
static inline void* refs_of(void* thisptr) {
    return *reinterpret_cast<void**>(reinterpret_cast<char*>(thisptr) + 8);
}

extern "C" void _ZNK7android7RefBase9incStrongEPKv(void* thisptr, const void* id) {
    if (!thisptr || !refs_of(thisptr)) {
        ALOGE("DolbyShim: incStrong skipped (this=%p, mRefs=%p)", thisptr,
              thisptr ? refs_of(thisptr) : nullptr);
        return;
    }
    typedef void (*RealFunc)(void*, const void*);
    static RealFunc real = (RealFunc)dlsym(RTLD_NEXT, "_ZNK7android7RefBase9incStrongEPKv");
    if (real) real(thisptr, id);
}

extern "C" void _ZNK7android7RefBase9decStrongEPKv(void* thisptr, const void* id) {
    if (!thisptr || !refs_of(thisptr)) {
        ALOGE("DolbyShim: decStrong skipped (this=%p, mRefs=%p)", thisptr,
              thisptr ? refs_of(thisptr) : nullptr);
        return;
    }
    typedef void (*RealFunc)(void*, const void*);
    static RealFunc real = (RealFunc)dlsym(RTLD_NEXT, "_ZNK7android7RefBase9decStrongEPKv");
    if (real) real(thisptr, id);
}
