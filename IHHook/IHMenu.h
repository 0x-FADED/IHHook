#pragma once
#include "SafeQueue.h"

namespace IHHook
{
namespace IHMenu
{
void AddMenuCommands();
void ProcessMessages();

void SetInitialText();
void DrawMenu(bool* p_open, bool openPrev);

void QueueMessageOut(const std::string& message);
void QueueMessageIn(const std::string& message);

extern SafeQueue<std::string> messagesOut;
extern SafeQueue<std::string> messagesIn;
} // namespace IHMenu
} // namespace IHHook