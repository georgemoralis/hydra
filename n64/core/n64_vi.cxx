#include <compatibility.hxx>
#include <fmt/format.h>
#include <log.hxx>
#include <n64/core/n64_addresses.hxx>
#include <n64/core/n64_types.hxx>
#include <n64/core/n64_vi.hxx>

namespace hydra::N64
{
    void Vi::Reset()
    {
        vi_v_intr_ = 0x100;
    }

    bool Vi::Redraw()
    {
        auto new_width = vi_h_end_ - vi_h_start_;
        auto new_height = (vi_v_end_ - vi_v_start_) / 2;
        new_width *= vi_x_scale_ ? vi_x_scale_ : 512;
        new_height *= vi_y_scale_ ? vi_y_scale_ : 512;
        if (new_width == 0 || new_height == 0 || !memory_ptr_ || pixel_mode_ == 0)
        {
            if (!blacked_out_)
            {
                std::fill(framebuffer_.begin(), framebuffer_.end(), 0);
                blacked_out_ = true;
            }
            return true;
        }
        blacked_out_ = false;
        new_width >>= 10;
        new_height >>= 10;
        width_ = new_width;
        height_ = new_height;
        size_t new_size = width_ * height_ * 4;
        if (framebuffer_.size() != new_size)
        {
            framebuffer_.resize(new_size);
        }
        switch (pixel_mode_)
        {
            case 0b11:
            {
                for (int y = 0; y < height_; y++)
                {
                    for (int x = 0; x < width_; x++)
                    {
                        uint32_t color =
                            (reinterpret_cast<uint32_t*>(memory_ptr_))[(y * vi_width_) + x];
                        set_pixel(x, y, color);
                    }
                }
                break;
            }
            case 0b10:
            {
                for (int y = 0; y < height_; y++)
                {
                    for (int x = 0; x < width_; x++)
                    {
                        uint16_t color_temp =
                            (reinterpret_cast<uint16_t*>(memory_ptr_))[(y * vi_width_) + x];
                        uint8_t r = (color_temp >> 11) & 0x1F;
                        uint8_t g = (color_temp >> 6) & 0x1F;
                        uint8_t b = (color_temp >> 1) & 0x1F;
                        r = (r << 3) | (r >> 2);
                        g = (g << 3) | (g >> 2);
                        b = (b << 3) | (b >> 2);
                        uint32_t color = 0xffu << 24 | b << 16 | g << 8 | r;
                        set_pixel(x, y, color);
                    }
                }
                break;
            }
            default:
                break;
        }
        framebuffer_ptr_ = framebuffer_.data();
        return true;
    }

    uint32_t Vi::ReadWord(uint32_t addr)
    {
        switch (addr)
        {
            case VI_CTRL:
            {
                return vi_ctrl_;
            }
            case VI_ORIGIN:
            {
                return vi_origin_;
            }
            case VI_WIDTH:
            {
                return vi_width_;
            }
            case VI_V_INTR:
            {
                return vi_v_intr_;
            }
            case VI_V_CURRENT:
            {
                return vi_v_current_;
            }
            case VI_BURST:
            {
                return vi_burst_;
            }
            case VI_V_SYNC:
            {
                return vi_v_sync_;
            }
            case VI_H_SYNC:
            {
                return vi_h_sync_;
            }
            case VI_H_SYNC_LEAP:
            {
                return vi_h_sync_leap_;
            }
            case VI_H_VIDEO:
            {
                return vi_h_end_ | (vi_h_start_ << 16);
            }
            case VI_V_VIDEO:
            {
                return vi_v_end_ | (vi_v_start_ << 16);
            }
            case VI_V_BURST:
            {
                return vi_v_burst_;
            }
            case VI_X_SCALE:
            {
                return vi_x_scale_;
            }
            case VI_Y_SCALE:
            {
                return vi_y_scale_;
            }
            case VI_TEST_ADDR:
            {
                return vi_test_addr_;
            }
            case VI_STAGED_DATA:
            {
                return vi_staged_data_;
            }
            default:
            {
                Logger::Warn("VI: Unhandled read from {:08x}", addr);
                return 0;
            }
        }
    }

    void Vi::WriteWord(uint32_t addr, uint32_t data)
    {
        switch (addr)
        {
            case VI_CTRL:
            {
                auto format = data & 0b11;
                pixel_mode_ = format;
                if ((data >> 6) & 0b1)
                {
                    Logger::WarnOnce("Interlacing enabled");
                }
                vi_ctrl_ = data;
                break;
            }
            case VI_ORIGIN:
            {
                data &= 0x00FFFFFF;
                memory_ptr_ = &rdram_ptr_[data];
                vis_counter_ += 1;
                break;
            }
            case VI_WIDTH:
            {
                vi_width_ = data;
                break;
            }
            case VI_V_CURRENT:
            {
                mi_interrupt_->VI = false;
                break;
            }
            case VI_H_SYNC:
            case VI_H_SYNC_LEAP:
            case VI_V_BURST:
            case VI_TEST_ADDR:
            case VI_STAGED_DATA:
            case VI_BURST:
            {
                break;
            }
            case VI_V_INTR:
            {
                vi_v_intr_ = data & 0x3ff;
                break;
            }
            case VI_V_SYNC:
            {
                num_halflines_ = data >> 1;
                cycles_per_halfline_ = (93750000 / 60) / num_halflines_;
                break;
            }
            case VI_H_VIDEO:
            {
                vi_h_end_ = data & 0b11'1111'1111;
                vi_h_start_ = (data >> 16) & 0b11'1111'1111;
                break;
            }
            case VI_V_VIDEO:
            {
                vi_v_end_ = data & 0b11'1111'1111;
                vi_v_start_ = (data >> 16) & 0b11'1111'1111;
                break;
            }
            case VI_X_SCALE:
            {
                vi_x_scale_ = data & 0b1111'1111'1111;
                break;
            }
            case VI_Y_SCALE:
            {
                vi_y_scale_ = data & 0b1111'1111'1111;
                break;
            }
        }
    }

    void Vi::set_pixel(int x, int y, uint32_t color)
    {
        memcpy(&framebuffer_[(x + y * width_) * 4], &color, 4);
    }
} // namespace hydra::N64