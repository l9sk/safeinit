//===-- AMDGPUAnnotateKernelFeaturesPass.cpp ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file This pass adds target attributes to functions which use intrinsics
/// which will impact calling convention lowering.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

#define DEBUG_TYPE "amdgpu-annotate-kernel-features"

using namespace llvm;

namespace {

class AMDGPUAnnotateKernelFeatures : public ModulePass {
private:
  static bool hasAddrSpaceCast(const Function &F);

  void addAttrToCallers(Function *Intrin, StringRef AttrName);
  bool addAttrsForIntrinsics(Module &M, ArrayRef<StringRef[2]>);

public:
  static char ID;

  AMDGPUAnnotateKernelFeatures() : ModulePass(ID) { }
  bool runOnModule(Module &M) override;
  const char *getPassName() const override {
    return "AMDGPU Annotate Kernel Features";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
    ModulePass::getAnalysisUsage(AU);
  }
};

}

char AMDGPUAnnotateKernelFeatures::ID = 0;

char &llvm::AMDGPUAnnotateKernelFeaturesID = AMDGPUAnnotateKernelFeatures::ID;

INITIALIZE_PASS(AMDGPUAnnotateKernelFeatures, DEBUG_TYPE,
                "Add AMDGPU function attributes", false, false)

static bool castRequiresQueuePtr(const AddrSpaceCastInst *ASC) {
  unsigned SrcAS = ASC->getSrcAddressSpace();

  // The queue ptr is only needed when casting to flat, not from it.
  return SrcAS == AMDGPUAS::LOCAL_ADDRESS || SrcAS == AMDGPUAS::PRIVATE_ADDRESS;
}

// Return true if an addrspacecast is used that requires the queue ptr.
bool AMDGPUAnnotateKernelFeatures::hasAddrSpaceCast(const Function &F) {
  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      if (const AddrSpaceCastInst *ASC = dyn_cast<AddrSpaceCastInst>(&I)) {
        if (castRequiresQueuePtr(ASC))
          return true;
      }
    }
  }

  return false;
}

void AMDGPUAnnotateKernelFeatures::addAttrToCallers(Function *Intrin,
                                                    StringRef AttrName) {
  SmallPtrSet<Function *, 4> SeenFuncs;

  for (User *U : Intrin->users()) {
    // CallInst is the only valid user for an intrinsic.
    CallInst *CI = cast<CallInst>(U);

    Function *CallingFunction = CI->getParent()->getParent();
    if (SeenFuncs.insert(CallingFunction).second)
      CallingFunction->addFnAttr(AttrName);
  }
}

bool AMDGPUAnnotateKernelFeatures::addAttrsForIntrinsics(
  Module &M,
  ArrayRef<StringRef[2]> IntrinsicToAttr) {
  bool Changed = false;

  for (const StringRef *Arr  : IntrinsicToAttr) {
    if (Function *Fn = M.getFunction(Arr[0])) {
      addAttrToCallers(Fn, Arr[1]);
      Changed = true;
    }
  }

  return Changed;
}

bool AMDGPUAnnotateKernelFeatures::runOnModule(Module &M) {
  Triple TT(M.getTargetTriple());

  static const StringRef IntrinsicToAttr[][2] = {
    // .x omitted
    { "llvm.amdgcn.workitem.id.y", "amdgpu-work-item-id-y" },
    { "llvm.amdgcn.workitem.id.z", "amdgpu-work-item-id-z" },

    { "llvm.amdgcn.workgroup.id.y", "amdgpu-work-group-id-y" },
    { "llvm.amdgcn.workgroup.id.z", "amdgpu-work-group-id-z" },

    { "llvm.r600.read.tgid.y", "amdgpu-work-group-id-y" },
    { "llvm.r600.read.tgid.z", "amdgpu-work-group-id-z" },

    // .x omitted
    { "llvm.r600.read.tidig.y", "amdgpu-work-item-id-y" },
    { "llvm.r600.read.tidig.z", "amdgpu-work-item-id-z" }
  };

  static const StringRef HSAIntrinsicToAttr[][2] = {
    { "llvm.amdgcn.dispatch.ptr", "amdgpu-dispatch-ptr" },
    { "llvm.amdgcn.queue.ptr", "amdgpu-queue-ptr" }
  };

  // TODO: We should not add the attributes if the known compile time workgroup
  // size is 1 for y/z.

  // TODO: Intrinsics that require queue ptr.

  // We do not need to note the x workitem or workgroup id because they are
  // always initialized.

  bool Changed = addAttrsForIntrinsics(M, IntrinsicToAttr);
  if (TT.getOS() == Triple::AMDHSA) {
    Changed |= addAttrsForIntrinsics(M, HSAIntrinsicToAttr);

    for (Function &F : M) {
      if (F.hasFnAttribute("amdgpu-queue-ptr"))
        continue;

      if (hasAddrSpaceCast(F))
        F.addFnAttr("amdgpu-queue-ptr");
    }
  }

  return Changed;
}

ModulePass *llvm::createAMDGPUAnnotateKernelFeaturesPass() {
  return new AMDGPUAnnotateKernelFeatures();
}
