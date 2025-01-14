#pragma once

#include "core/n64_impl.hxx"
#include <chrono>
#include <emulator.hxx>
#include <map>

class MmioViewer;

namespace hydra::N64
{
    class N64_TKPWrapper : public Emulator
    {
        TKP_EMULATOR(N64_TKPWrapper);

    public:

    private:
        N64 n64_impl_;
        static bool ipl_loaded_;

        int GetWidth() override
        {
            return n64_impl_.GetWidth();
        }

        int GetHeight() override
        {
            return n64_impl_.GetHeight();
        }

        void HandleMouseMove(int32_t, int32_t) override;

        std::map<uint32_t, uint32_t> key_mappings_;

        friend class ::N64Debugger;
        friend class ::MmioViewer;
    };
} // namespace hydra::N64
