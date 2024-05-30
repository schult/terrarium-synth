#include "PersistentSettings.h"

#include <array>
#include <cstdint>
#include <type_traits>

namespace
{

uint32_t crc32(const uint8_t *data, size_t length)
{
    constexpr uint32_t poly = 0x82f63b78;
    uint32_t crc = ~0;
    while (length--)
    {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
        {
            crc = (crc >> 1) ^ (poly & (0 - (crc & 1)));
        }
    }
    return ~crc;
}

struct Slot
{
    static constexpr uint32_t empty = 0xFFFFFFFF;
    static constexpr uint32_t flag = 0x4AC0FFEE;

    uint32_t header;
    Settings settings;
    uint32_t check;

    uint32_t calculateCheck() const
    {
        const auto data = reinterpret_cast<const uint8_t*>(this);
        const auto size = sizeof(*this) - sizeof(check);
        return crc32(data, size);
    }
};

static_assert(std::is_trivially_copyable_v<Slot>);

constexpr size_t slot_count = 512;
constexpr size_t flash_size = slot_count * sizeof(Slot);
uint8_t DSY_QSPI_BSS alignas(Slot) flash[flash_size];
const Slot* slots = reinterpret_cast<Slot *>(flash);
size_t current_slot = slot_count;

} // namespace


Settings loadSettings()
{
    for (auto i = slot_count; i--;)
    {
        const auto& slot = slots[i];
        if (slot.header != Slot::flag) { continue; }
        if (slot.check != slot.calculateCheck()) { continue; }
        current_slot = i;
        return slot.settings;
    }
    return Settings();
}

void saveSettings(daisy::QSPIHandle& qspi, const Settings& settings)
{
    int retry = 3;
    while (retry--)
    {
        do {
            current_slot++;
        } while(current_slot < slot_count &&
            slots[current_slot].header != Slot::empty);

        if (current_slot >= slot_count)
        {
            const auto address = reinterpret_cast<uint32_t>(flash);
            const auto size = static_cast<uint32_t>(flash_size);
            const auto result = qspi.Erase(address, address+size);
            if (result != daisy::QSPIHandle::Result::OK)
            {
                continue;
            }
            current_slot = 0;
        }

        Slot slot{
            .header = Slot::flag,
            .settings = settings,
            .check = 0
        };
        slot.check = slot.calculateCheck();

        const auto address =
            reinterpret_cast<uint32_t>(slots + current_slot);
        const auto size = static_cast<uint32_t>(sizeof(slot));
        const auto data = reinterpret_cast<uint8_t*>(&slot);
        const auto result = qspi.Write(address, size, data);
        if (result == daisy::QSPIHandle::Result::OK)
        {
            break;
        }
    }
}
