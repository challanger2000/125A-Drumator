// 125A Drumator: standalone Win32 factory/controller editor geometry smoke.
// Runs only after the final installable VST3 bundle has been assembled.
// The Steinberg validator does not test this UI-description failure mode.
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "vst3/PluginIDs.h"
#include "core/Utf8Path.h"
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <system_error>

namespace {
[[noreturn]] void fail(const char* reason) {
    std::cerr << "125A Drumator editor smoke FAILED: " << reason << '\n';
    std::exit(1);
}
}
int main(int argc,char** argv) {
    if(argc!=2)fail("Expected one VST3 binary path");
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
    std::cout<<"125A Drumator editor geometry smoke PASS\n";
    return 0;
}
