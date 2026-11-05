//
// SPDX-FileCopyrightText: The LineageOS Project
// SPDX-License-Identifier: Apache-2.0
//

#include <dlfcn.h>
#include <log/log.h>

#include <utils/RefBase.h>

namespace {

// RefBase stores its ref-count block in the `mRefs` member, which sits right
// after the vtable pointer, i.e. at offset 8 on 64-bit. The Dolby C2 service
// keeps using Components after ~RefBase has run, so `this` is still non-null
// while `mRefs` has been cleared. Always check mRefs instead of faulting at
// [mRefs + 4].
void* refs_of(void* thisptr) {
    return *reinterpret_cast<void**>(reinterpret_cast<char*>(thisptr) + 8);
}

// Resolve the real symbol from libutils itself. RTLD_NEXT depends on the
// DT_NEEDED order and silently fails when the shim is loaded before libutils,
// which would break every incStrong/decStrong and cause exactly the
// use-after-free this shim is meant to work around.
void* real_symbol(const char* name) {
    static void* libutils = dlopen("libutils.so", RTLD_NOW | RTLD_NOLOAD);
    void* symbol = (libutils != nullptr) ? dlsym(libutils, name) : nullptr;
    if (symbol == nullptr) {
        symbol = dlsym(RTLD_NEXT, name);
    }
    return symbol;
}

// wp<> requires a usable weakref_type: its destructor does
//   if (m_ptr) m_refs->decWeak(this);
// so returning nullptr from createWeak() for a dead RefBase would fault in
// the caller instead. Hand out the weakref of a dummy RefBase that is dead
// (mStrong == 0) but kept alive by our own weak reference, and account for
// the weak reference the caller will drop. Because mStrong is 0, calling
// wp::promote() on such a wp returns nullptr instead of a dangling pointer.
class ShimRefBase : public android::RefBase {};

struct DummyWeakRef {
    android::RefBase::weakref_type* refs = nullptr;
};

DummyWeakRef& dummy_weakref() {
    static DummyWeakRef dummy = [] {
        DummyWeakRef result;

        typedef void (*IncDecFn)(void*, const void*);
        typedef void* (*CreateWeakFn)(void*, const void*);

        auto incStrong = reinterpret_cast<IncDecFn>(
                real_symbol("_ZNK7android7RefBase9incStrongEPKv"));
        auto decStrong = reinterpret_cast<IncDecFn>(
                real_symbol("_ZNK7android7RefBase9decStrongEPKv"));
        auto createWeak = reinterpret_cast<CreateWeakFn>(
                real_symbol("_ZNK7android7RefBase10createWeakEPKv"));

        if (incStrong != nullptr && decStrong != nullptr && createWeak != nullptr) {
            auto* object = new ShimRefBase();
            incStrong(object, nullptr);  // INITIAL_STRONG_VALUE -> 1
            result.refs = reinterpret_cast<android::RefBase::weakref_type*>(
                    createWeak(object, nullptr));  // weak reference, kept forever
            decStrong(object, nullptr);  // 1 -> 0: the dummy dies, refs survive

            // The dummy now has mStrong == 0 with a live weakref_impl, so
            // attemptIncStrong() takes the OBJECT_LIFETIME_STRONG path and
            // returns false. wp::promote() therefore yields nullptr instead
            // of a dangling strong pointer.
        }
        return result;
    }();
    return dummy;
}

}  // namespace

extern "C" void _ZNK7android7RefBase9incStrongEPKv(void* thisptr, const void* id) {
    if (!thisptr || !refs_of(thisptr)) {
        ALOGD("DolbyShim: incStrong skipped (this=%p, mRefs=%p)", thisptr,
              thisptr ? refs_of(thisptr) : nullptr);
        return;
    }
    typedef void (*RealFunc)(void*, const void*);
    static RealFunc real = nullptr;
    if (real == nullptr) {
        real = reinterpret_cast<RealFunc>(
                real_symbol("_ZNK7android7RefBase9incStrongEPKv"));
    }
    if (real) real(thisptr, id);
}

extern "C" void _ZNK7android7RefBase9decStrongEPKv(void* thisptr, const void* id) {
    if (!thisptr || !refs_of(thisptr)) {
        ALOGD("DolbyShim: decStrong skipped (this=%p, mRefs=%p)", thisptr,
              thisptr ? refs_of(thisptr) : nullptr);
        return;
    }
    typedef void (*RealFunc)(void*, const void*);
    static RealFunc real = nullptr;
    if (real == nullptr) {
        real = reinterpret_cast<RealFunc>(
                real_symbol("_ZNK7android7RefBase9decStrongEPKv"));
    }
    if (real) real(thisptr, id);
}

extern "C" void* _ZNK7android7RefBase10createWeakEPKv(void* thisptr, const void* id) {
    if (thisptr == nullptr) {
        return nullptr;
    }

    if (refs_of(thisptr) == nullptr) {
        DummyWeakRef& dummy = dummy_weakref();
        if (dummy.refs == nullptr) {
            ALOGE("DolbyShim: createWeak on dead RefBase %p, no dummy weakref", thisptr);
            return nullptr;
        }
        // Balanced by the wp<> destructor calling decWeak().
        dummy.refs->incWeak(id);
        ALOGD("DolbyShim: createWeak on dead RefBase %p -> dummy weakref %p", thisptr,
              dummy.refs);
        return dummy.refs;
    }

    typedef void* (*RealFunc)(void*, const void*);
    static RealFunc real = nullptr;
    if (real == nullptr) {
        real = reinterpret_cast<RealFunc>(
                real_symbol("_ZNK7android7RefBase10createWeakEPKv"));
    }
    if (real == nullptr) {
        ALOGE("DolbyShim: real createWeak not found");
        return nullptr;
    }
    return real(thisptr, id);
}
