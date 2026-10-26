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
#include <fstream>
#include <vector>
#include <array>
#include <cstdint>

namespace {
[[noreturn]] void fail(const char* reason) {
    drumator::qa::result(std::string("Packaged editor FAIL: ")+reason);
    std::cerr << "125A Drumator editor smoke FAILED: " << reason << '\n';
    std::exit(1);
}
struct NativeFrame final:Steinberg::IPlugFrame {
 bool accept=true;unsigned calls=0;
 Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID requestedIid,void** out) override {
   if(Steinberg::FUnknownPrivate::iidEqual(requestedIid,Steinberg::IPlugFrame::iid)||
      Steinberg::FUnknownPrivate::iidEqual(requestedIid,Steinberg::FUnknown::iid)){
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
    // Verify packaged GUI assets from the installable VST3, not source-tree paths.
    const auto resourceDir=modulePath.parent_path().parent_path()/L"Resources";
    for(const auto* required:{L"Metallisches DRUMATOR-Logo.png",L"125A_Drumator_Faceplate.png"}) {
      const auto file=resourceDir/required;
      std::ifstream in(file,std::ios::binary);
      std::array<unsigned char,24> png{};
      if(!in.read(reinterpret_cast<char*>(png.data()),static_cast<std::streamsize>(png.size()))||
         png[0]!=137||png[1]!=80||png[2]!=78||png[3]!=71||
         png[12]!='I'||png[13]!='H'||png[14]!='D'||png[15]!='R')
         fail("Mandatory logo/faceplate PNG absent or corrupted in packaged VST3");
      const auto be32=[&](unsigned at){return (std::uint32_t(png[at])<<24)|
         (std::uint32_t(png[at+1])<<16)|(std::uint32_t(png[at+2])<<8)|png[at+3];};
      if(be32(16)<100||be32(20)<50)
         fail("Mandatory packaged logo/faceplate PNG has invalid dimensions");
      std::cout<<"Packaged PNG resource verified: "<<file.filename().string()<<
                 " "<<be32(16)<<"x"<<be32(20)<<"\\n";
    }
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
    // Genuine HWND screen capture *after* final-bundle VSTGUI attach and paint.
    // This is the pixel output of the packaged plugin, never a mockup.
    // Grab the VSTGUI child HWND, never the blank STATIC host placeholder.
    HWND pluginWindow=nullptr;
    EnumChildWindows(parent,[](HWND candidate,LPARAM value)->BOOL {
      RECT area{};
      if(GetClientRect(candidate,&area)&&
         area.right-area.left>=1280&&area.bottom-area.top>=300) {
        *reinterpret_cast<HWND*>(value)=candidate;
      }
      return TRUE;
    },reinterpret_cast<LPARAM>(&pluginWindow));
    if(!pluginWindow)fail("No actual VSTGUI child HWND to screenshot");
    ShowWindow(pluginWindow,SW_SHOW);
    for(int pass=0;pass<16;++pass) {
      MSG message{};
      while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
        TranslateMessage(&message);DispatchMessageW(&message);
      }
      RedrawWindow(pluginWindow,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
      Sleep(10);
    }
    constexpr int width=1280,height=300;
    HDC windowDc=GetDC(pluginWindow);
    if(!windowDc)fail("Packaged screenshot could not get window DC");
    HDC memoryDc=CreateCompatibleDC(windowDc);
    HBITMAP bitmap=memoryDc?CreateCompatibleBitmap(windowDc,width,height):nullptr;
    if(!memoryDc||!bitmap)fail("Packaged screenshot could not allocate GDI bitmap");
    auto previous=SelectObject(memoryDc,bitmap);
    if(!BitBlt(memoryDc,0,0,width,height,windowDc,0,0,SRCCOPY|CAPTUREBLT))
      fail("Packaged native HWND screenshot BitBlt failed");
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width)*height*4);
    // GDI requires the bitmap to be deselected before GetDIBits.
    SelectObject(memoryDc,previous);
    if(GetDIBits(memoryDc,bitmap,0,height,pixels.data(),&info,DIB_RGB_COLORS)!=height)
      fail("Packaged native HWND screenshot readback failed");
    // Reject a white host-only screenshot and the text-only fallback.
    std::size_t darkCount=0, silverCount=0;
    for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
      const auto* p=&pixels[(static_cast<std::size_t>(y)*width+x)*4];
      const int blue=p[0],green=p[1],red=p[2];
      if(red+green+blue<270)++darkCount;
      if(y>=5&&y<75&&x>=475&&x<810&&red>=135&&green>=135&&blue>=135&&
         std::abs(red-green)<35&&std::abs(red-blue)<35)++silverCount;
    }
    if(darkCount<width*height/4||silverCount<500)
      fail("Screenshot lacks rendered VSTGUI and metallic silver product logo");
    std::filesystem::create_directories("build/gui-shots");
    const auto screenshot=std::filesystem::path("build/gui-shots/packaged-silver-drumator-100.bmp");
    std::ofstream bmp(screenshot,std::ios::binary);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;
    header.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
    header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
    bmp.write(reinterpret_cast<const char*>(&header),sizeof(header));
    bmp.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));
    bmp.write(reinterpret_cast<const char*>(pixels.data()),
              static_cast<std::streamsize>(pixels.size()));
    if(!bmp)fail("Packaged native screenshot write failed");
    bmp.close();DeleteObject(bitmap);
    DeleteDC(memoryDc);ReleaseDC(pluginWindow,windowDc);
    std::cout<<"Packaged VST3 native silver-logo screenshot: "<<screenshot.string()<<"\\n";
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
    drumator::qa::result("125A Drumator editor geometry smoke PASS: packaged native HWND and geometry checks completed");
    return 0;
}
