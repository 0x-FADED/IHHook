// WindowsMessageHook.cpp - from RE2Framework

#include <spdlog/spdlog.h>
#include <TlHelp32.h>
#include "WindowsMessageHook.hpp"

ankerl::unordered_dense::segmented_set<HANDLE>* WindowsMessageHook::g_SuspendedThreads = nullptr;
WindowsMessageHook* WindowsMessageHook::g_windows_message_hook{nullptr};
std::recursive_mutex WindowsMessageHook::g_proc_mutex{};

LRESULT WINAPI WindowsMessageHook::window_proc(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param)
{
    std::lock_guard _{g_proc_mutex};

    if (g_windows_message_hook == nullptr)
    {
        return 0;
    }

    // Call our onMessage callback.
    auto& on_message = g_windows_message_hook->on_message;

    if (on_message)
    {
        // If it returns false we don't call the original window procedure.
        if (!on_message(wnd, message, w_param, l_param))
        {
            return DefWindowProc(wnd, message, w_param, l_param);
        }
    }

    // Call the original message procedure.
    return CallWindowProc(g_windows_message_hook->get_original(), wnd, message, w_param, l_param);
}

void WindowsMessageHook::SuspendAllThreadsButCurrent()
{
    if (g_SuspendedThreads != nullptr)
        return;

    g_SuspendedThreads = new ankerl::unordered_dense::segmented_set<HANDLE>();

    HANDLE s_Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (s_Snapshot == INVALID_HANDLE_VALUE)
        return;

    auto s_CurrentThread = GetCurrentThreadId();

    THREADENTRY32 s_ThreadEntry{};
    s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);

    if (Thread32First(s_Snapshot, &s_ThreadEntry))
    {
        do
        {
            if (s_ThreadEntry.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(s_ThreadEntry.th32OwnerProcessID) && s_ThreadEntry.th32ThreadID != s_CurrentThread && s_ThreadEntry.th32OwnerProcessID == GetCurrentProcessId())
            {
                HANDLE s_Thread = OpenThread(THREAD_ALL_ACCESS, false, s_ThreadEntry.th32ThreadID);

                if (s_Thread != nullptr)
                    g_SuspendedThreads->insert(s_Thread);
            }

            s_ThreadEntry.dwSize = sizeof(s_ThreadEntry);
        } while (Thread32Next(s_Snapshot, &s_ThreadEntry));
    }

    CloseHandle(s_Snapshot);

    for (auto* s_Thread : *g_SuspendedThreads)
        SuspendThread(s_Thread);
}

void WindowsMessageHook::ResumeSuspendedThreads()
{
    if (g_SuspendedThreads == nullptr)
        return;

    for (auto* s_Thread : *g_SuspendedThreads)
    {
        ResumeThread(s_Thread);
        CloseHandle(s_Thread);
    }

    delete g_SuspendedThreads;
    g_SuspendedThreads = nullptr;
}

WindowsMessageHook::WindowsMessageHook(HWND wnd)
    : m_wnd{wnd}
    , m_original_proc{nullptr}
{
    spdlog::info("Initializing WindowsMessageHook");

    SuspendAllThreadsButCurrent();

    g_windows_message_hook = this;

    // Save the original window procedure.
    m_original_proc = (WNDPROC) GetWindowLongPtr(m_wnd, GWLP_WNDPROC);

    // Set it to our "hook" procedure.
    SetWindowLongPtr(m_wnd, GWLP_WNDPROC, (LONG_PTR) &window_proc);

    ResumeSuspendedThreads();

    spdlog::info("Hooked Windows message handler");
}

WindowsMessageHook::~WindowsMessageHook()
{
    std::lock_guard _{g_proc_mutex};
    spdlog::info("Destroying WindowsMessageHook");

    SuspendAllThreadsButCurrent();
    remove();
    g_windows_message_hook = nullptr;
    ResumeSuspendedThreads();
}

bool WindowsMessageHook::remove()
{
    // Don't attempt to restore invalid original window procedures.
    if (m_original_proc == nullptr || m_wnd == nullptr)
    {
        return true;
    }

    // Restore the original window procedure.
    SetWindowLongPtr(m_wnd, GWLP_WNDPROC, (LONG_PTR) m_original_proc);

    // Invalidate this message hook.
    m_wnd = nullptr;
    m_original_proc = nullptr;

    return true;
}