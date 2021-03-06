#include "x86.h"
#include "Common.h"
#include "FuncCtx.h"
#include "VerbSequence.h"
#include "VerbMixins.h"
#include "Trinity.h"

typedef void(*JitProc)(X86Compiler&, FuncCtx&, VerbSequence&);

static JitRuntime s_runtime;

DLL_EXPORT void* CompileFunctionToNative(FunctionDescriptor* fdesc)
{
    print(L"In CompileFunctionToNative");

    asmjit::Error   eresult;
    CodeHolder      code;
    CodeInfo        ci = s_runtime.getCodeInfo();
    CallConv::Id    ccId = CallConv::kIdHost;
    void*           ret = nullptr;
    FuncSignature   fsig;
    TypeId::Id      retId;
    bool            wresize;
    uint8_t*        pargs;
    int32_t         nargs;
    CCFunc*         func;
    ErrHandler      handler;
    StringLogger    logger;

    if (eresult = code.init(ci)) return nullptr;
    code.setErrorHandler(&handler);
    code.setLogger(&logger);
    X86Compiler cc(&code);

    print(L"Code initialized");

    retId = Mixin::GetReturnId(fdesc);

    debug(retId);

    Mixin::GetArgument(fdesc, pargs, nargs);

    debug(nargs);

    wresize = Mixin::WouldResize(fdesc);

    print(L"Initializing func");

    fsig.init(ccId, retId, pargs, nargs);
    func = cc.addFunc(fsig);

    print(L"Jit dispatching");
    // 1st arg is always CellAccessor* cellAccessor
    FuncCtx fctx(cc, retId, wresize);
    VerbSequence seq(fdesc);

    Mixin::DoDispatch(cc, fctx, seq);

    print(L"Jit dispatched");
    if (eresult = fctx.finalize()) {
        debug(eresult);
        return nullptr;
    }
    print(L"Finalized");
    if (eresult = s_runtime.add(&ret, &code)) {
        debug(eresult);
        return nullptr;
    }
    print(L"added");

    debug(logger.getString());

    cc.resetLastError();

    fdesc->~FunctionDescriptor();
    free(fdesc);

    return ret;
}
