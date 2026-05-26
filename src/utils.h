// ============================================================================
// utils.h - Memory patching utilities
//
// These helper functions let us modify the game's code in memory at runtime.
// This is how NVSE plugins change game behavior without modifying the .exe.
//
// HOW IT WORKS:
// The game's executable is loaded into memory. Each instruction lives at a
// specific memory address. We can overwrite those bytes to change behavior.
// For example, we can replace a "call CreateDevice(fullscreen)" with
// "call CreateDevice(windowed)" by changing a few bytes.
// ============================================================================
#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace Utils
{
    // ========================================================================
    // SafeWrite - Write arbitrary bytes to a memory address
    //
    // We need VirtualProtect because the game's code section is read-only.
    // We temporarily make it writable, write our bytes, then restore protection.
    // ========================================================================
    inline void SafeWriteBuf(uintptr_t addr, const void* data, size_t len)
    {
        DWORD oldProtect = 0;
        VirtualProtect(reinterpret_cast<void*>(addr), len, PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(reinterpret_cast<void*>(addr), data, len);
        VirtualProtect(reinterpret_cast<void*>(addr), len, oldProtect, &oldProtect);
    }

    // Write a single 32-bit value
    inline void SafeWrite32(uintptr_t addr, uint32_t value)
    {
        SafeWriteBuf(addr, &value, sizeof(value));
    }

    // Write a single 16-bit value
    inline void SafeWrite16(uintptr_t addr, uint16_t value)
    {
        SafeWriteBuf(addr, &value, sizeof(value));
    }

    // Write a single byte
    inline void SafeWrite8(uintptr_t addr, uint8_t value)
    {
        SafeWriteBuf(addr, &value, sizeof(value));
    }

    // ========================================================================
    // WriteRelJump - Write a relative JMP instruction (5 bytes: 0xE9 + offset)
    //
    // This is used to "detour" a function - when the game tries to execute
    // the code at 'src', it jumps to our function at 'dst' instead.
    // ========================================================================
    inline void WriteRelJump(uintptr_t src, uintptr_t dst)
    {
        // A relative jump: the offset is calculated from the END of the
        // 5-byte instruction (src + 5), not from the start
        uint8_t buf[5];
        buf[0] = 0xE9;  // JMP rel32 opcode
        *reinterpret_cast<int32_t*>(&buf[1]) = static_cast<int32_t>(dst - src - 5);
        SafeWriteBuf(src, buf, 5);
    }

    // ========================================================================
    // WriteRelCall - Write a relative CALL instruction (5 bytes: 0xE8 + offset)
    //
    // Similar to jump, but pushes return address on stack so the original
    // code continues after our function returns.
    // ========================================================================
    inline void WriteRelCall(uintptr_t src, uintptr_t dst)
    {
        uint8_t buf[5];
        buf[0] = 0xE8;  // CALL rel32 opcode
        *reinterpret_cast<int32_t*>(&buf[1]) = static_cast<int32_t>(dst - src - 5);
        SafeWriteBuf(src, buf, 5);
    }

    // ========================================================================
    // PatchNop - Fill an address range with NOP (0x90) instructions
    //
    // "NOP" means "no operation" - the CPU does nothing and moves on.
    // We use this to effectively delete game code we don't want to run.
    // ========================================================================
    inline void PatchNop(uintptr_t addr, size_t len)
    {
        DWORD oldProtect = 0;
        VirtualProtect(reinterpret_cast<void*>(addr), len, PAGE_EXECUTE_READWRITE, &oldProtect);
        memset(reinterpret_cast<void*>(addr), 0x90, len);
        VirtualProtect(reinterpret_cast<void*>(addr), len, oldProtect, &oldProtect);
    }
}
