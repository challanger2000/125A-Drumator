// 125A Drumator: standalone Win32 factory/controller editor geometry smoke.
// Runs only after the final installable VST3 bundle has been assembled.
// The Steinberg validator does not test this UI-description failure mode.
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "vst3/PluginIDs.h"
#include "core/Utf8Path.h"
#include "integration/ByteStream.h"
#include "integration/ResultLog.h"
#include <windows.h>
#include <objbase.h>
#include <filesystem>
#include <iostream>
#include <system_error>

namespace {
[[noreturn]] void fail(const char* reason) {
    drumator::qa::result(std::string("Packaged editor FAIL: ")+reason);
    std::cerr << "125A Drumator editor smoke FAILED: " << reason << '\n';
    std::exit(1);
}
struct NativeFrame final:Steinberg::IPlugFrame {
 bool accept=true;unsigned calls=0;
 Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,void** out) override {
   if(Steinberg::FUnknownPrivate::iidEqual(iid,Steinberg::IPlugFrame::iid)||
      Steinberg::FUnknownPrivate::iidEqual(iid,Steinberg::FUnknown::iid)){
     *out=static_cast<Steinberg::IPlugFrame*>(this);return Steinberg::kResultOk;
   }*out=nullptr;return Steinberg::kNoInterface;
 }
 Steinberg::uint32 PLUGIN_API addRef() override{return 1;}
 Steinberg::uint32 PLUGIN_API release() override{return 1;}
 Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view,Steinberg::ViewRect* size) override {
   ++calls;return accept?view->onSize(size):Steinberg::kResultFalse;
 }
};
}
int main(int argc,char** argv) {
    if(argc!=2)fail("Expected one VST3 binary path");
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    const auto modulePath=drumator::pathFromUtf8(argv[1]);
    auto* module=LoadLibraryW(modulePath.c_str());
    if(!module) {
        std::cerr<<"LoadLibraryW Windows error: "<<GetLastError()<<'\n';
        fail("Could not load packaged VST3 module");
    }
    using InitDllFn=bool (*)();
    auto* initDll=reinterpret_cast<InitDllFn>(
        GetProcAddress(module,"InitDll"));
    if(!initDll||!initDll())fail("InitDll did not initialize VST3");
    using FactoryFn=Steinberg::IPluginFactory* (*)();
    auto* makeFactory=reinterpret_cast<FactoryFn>(
        GetProcAddress(module,"GetPluginFactory"));
    if(!makeFactory)fail("No GetPluginFactory export");
    auto* factory=makeFactory();
    if(!factory)fail("No plugin factory");
    Steinberg::Vst::IEditController* controller=nullptr;
    if(factory->createInstance(
           drumator::kControllerCID.toTUID(),
           Steinberg::Vst::IEditController::iid,
           reinterpret_cast<void**>(&controller))!=Steinberg::kResultOk||
       !controller)fail("Unable to create Drumator controller");
    if(controller->initialize(nullptr)!=Steinberg::kResultOk)
        fail("Controller initialize failed");
    auto* editor=controller->createView(Steinberg::Vst::ViewType::kEditor);
    if(!editor)fail("Controller returned null editor");
    Steinberg::ViewRect rect{};
    if(editor->getSize(&rect)!=Steinberg::kResultOk)
        fail("Editor getSize failed");
    std::cout << "Actual editor size: "
              <<rect.right-rect.left<<" x "<<rect.bottom-rect.top<<"\n";
    if(rect.left!=0||rect.top!=0||rect.right!=1280||rect.bottom!=300)
        fail("GUI resource or template was not loaded as 1280 x 300");
    Steinberg::IPlugViewContentScaleSupport* scale=nullptr;
    if(editor->queryInterface(
         Steinberg::IPlugViewContentScaleSupport::iid,
         reinterpret_cast<void**>(&scale))!=Steinberg::kResultOk||!scale)
        fail("N10 editor lacks IPlugViewContentScaleSupport");
    if(scale->setContentScaleFactor(1.5)!=Steinberg::kResultTrue||
       editor->getSize(&rect)!=Steinberg::kResultOk||
       rect.getWidth()!=1920||rect.getHeight()!=450)
        fail("N10 preattach 150-percent host DPI did not resize getSize");
    if(scale->setContentScaleFactor(1.0)!=Steinberg::kResultTrue||
       editor->getSize(&rect)!=Steinberg::kResultOk||
       rect.getWidth()!=1280||rect.getHeight()!=300)
        fail("N10 preattach DPI reset did not restore editor dimensions");
    // Actual packaged DLL: attach a native frame, then exercise controller
    // restore and host refusal. This complements the source-instrumented QA.
    NativeFrame hostFrame;
    auto parent=CreateWindowExW(0,L"STATIC",L"Packaged Drumator QA",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        0,0,1280,850,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!parent||editor->setFrame(&hostFrame)!=Steinberg::kResultOk||
       editor->attached(parent,Steinberg::kPlatformTypeHWND)!=Steinberg::kResultOk)
        fail("Z02 packaged native attach failed");
    auto restoreZoom=[&](bool large){
      std::uint8_t bytes[]{0x44,0x5a,0x4d,0x31,static_cast<std::uint8_t>(large)};
      drumator::qa::ByteStream state(bytes,5);return controller->setState(&state);
    };
    for(bool large:{true,false}) {
      Steinberg::ViewRect before{},after{};editor->getSize(&before);
      hostFrame.accept=false;
      if(restoreZoom(large)==Steinberg::kResultOk||editor->getSize(&after)!=Steinberg::kResultOk||
         before.getWidth()!=after.getWidth()||before.getHeight()!=after.getHeight())
        fail("Z02 packaged refusal changed editor geometry");
      hostFrame.accept=true;
      if(restoreZoom(large)!=Steinberg::kResultOk||editor->getSize(&after)!=Steinberg::kResultOk||
         after.getWidth()!=(large?1920:1280))fail("Z02 packaged accepted zoom geometry");
    }
    if(editor->removed()!=Steinberg::kResultOk)fail("Z02 packaged native remove failed");
    editor->setFrame(nullptr);DestroyWindow(parent);
    drumator::qa::result("Z02 packaged DLL PASS: native HWND attach, controller zoom 100/150 and resize rejection");
    std::cout<<"Z02 packaged DLL native attach, 100/150 controller recall and IPlugFrame refusals PASS\n";
    scale->release();
    std::cout<<"N10 preattach 100/150 percent host DPI geometry PASS\\n";
    editor->release();
    controller->terminate();
    controller->release();
    factory->release();
    if(auto* exitDll=reinterpret_cast<InitDllFn>(
            GetProcAddress(module,"ExitDll"))) {
        if(!exitDll())fail("ExitDll failed");
    }
    FreeLibrary(module);
    CoUninitialize();
    std::cout<<"125A Drumator editor geometry smoke PASS\n";
    return 0;
}
