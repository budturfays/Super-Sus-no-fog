#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>

class CheatEntry {
public:
    int ID;
    std::string Description;
    std::string RealAddress;
    std::string VariableType;
    std::string Address;
    std::vector<int> Offsets;
};

uintptr_t GetModuleBaseAddress(DWORD processID, const std::string& moduleName) {
    uintptr_t baseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &moduleEntry)) {
        do {
            if (std::string(moduleEntry.szModule, moduleEntry.szModule + wcslen(moduleEntry.szModule)) == moduleName) {
                baseAddress = (uintptr_t)moduleEntry.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &moduleEntry));
    }

    if (hSnapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(hSnapshot);
    }
    return baseAddress;
}

uintptr_t GetAddressWithOffsets(uintptr_t baseAddress, const std::vector<int>& offsets, HANDLE hProcess) {
    if (baseAddress == 0 || offsets.empty()) {
        return 0;
    }

    uintptr_t address = baseAddress;
    for (const auto& offset : offsets) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)address, &address, sizeof(address), nullptr)) {
            return 0;
        }
        address += offset;
    }
    return address;
}

int main() {
    CheatEntry cheatEntry;
    cheatEntry.RealAddress = "17512619BC8";
    cheatEntry.VariableType = "Float";
    cheatEntry.Address = "GameAssembly.dll+0364E118";
    cheatEntry.Offsets = { 0xA30, 0x38, 0x30, 0xC8, 0x170, 0x100, 0x88 };

    HWND hwnd = FindWindowA(nullptr, "Super Sus");
    if (!hwnd) {
        return 1;
    }

    DWORD processID;
    GetWindowThreadProcessId(hwnd, &processID);

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, processID);
    if (!hProcess) {
        return 1;
    }

    uintptr_t gameAssemblyBase = GetModuleBaseAddress(processID, "GameAssembly.dll");
    if (gameAssemblyBase == 0) {
        CloseHandle(hProcess);
        return 1;
    }

    uintptr_t baseAddress = gameAssemblyBase + 0x0364E118;
    uintptr_t targetAddress = GetAddressWithOffsets(baseAddress, cheatEntry.Offsets, hProcess);

    if (targetAddress == 0) {
        CloseHandle(hProcess);
        return 1;
    }

    // Print address and offsets once
    std::cout << "Target Address: 0x" << std::hex << targetAddress << std::dec << std::endl;
    std::cout << "Offsets: ";
    for (const auto& offset : cheatEntry.Offsets) {
        std::cout << "0x" << std::hex << offset << " " << std::dec;
    }
    std::cout << std::endl;

    float currentValue = 0.0f;
    bool isOn = false;

    while (true) {
        if (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) {
            isOn = !isOn;
            currentValue = isOn ? 3.0f : 0.0f;
            WriteProcessMemory(hProcess, (LPVOID)targetAddress, &currentValue, sizeof(currentValue), nullptr);
            std::cout << "Set value to " << currentValue << " at address 0x" << std::hex << targetAddress << std::dec << std::endl;
            Sleep(150); // Adjusted delay for responsiveness
        }
        Sleep(10); // Reduce CPU usage
    }

    CloseHandle(hProcess);
    return 0;
}
