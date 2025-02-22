#pragma once

#include "qt/mmioviewer.hxx"
#include <emulator_types.hxx>
#include <log.hxx>
#include <memory>
#include <qt/mmioviewer.hxx>

enum EmulatorTool : size_t {
    ET_Debugger,
    ET_Tracelogger,
    ET_MmioViewer,

    EmulatorToolsSize
};

struct EmulatorToolFactory
{
    static QWidget* CreateTool(EmulatorTool tool, bool& open, hydra::EmuType emu_type,
                               hydra::Emulator* emulator)
    {
        switch (tool)
        {
            case ET_Debugger:
            {
                switch (emu_type)
                {
                    // case hydra::EmuType::N64:
                    // {
                    //     auto ret = new N64Debugger(open, nullptr);
                    //     ret->SetEmulator(dynamic_cast<hydra::N64::N64_TKPWrapper*>(emulator));
                    //     return ret;
                    // }
                    default:
                    {
                        Logger::Warn("EmulatorToolFactory::CreateTool tried to create debugger for "
                                     "unsupported emulator type ({})",
                                     (int)tool, (int)emu_type);
                        return nullptr;
                    }
                }
            }
            case ET_Tracelogger:
            {
                return nullptr;
            }
            case ET_MmioViewer:
            {
                return new MmioViewer(open, emulator, emu_type, nullptr);
            }
            default:
            {
                Logger::Warn("EmulatorToolFactory::CreateTool tried to create unknown tool ({})",
                             (int)tool);
                return nullptr;
            }
        }
        return nullptr;
    }
};
