#pragma once

#include "Input/Input.h"

#ifdef FLING_LINUX

#include "LinuxKeycodes.h"

namespace Fling
{
	class LinuxInput : public Input
	{
	protected:

		virtual void InitImpl() override;
		virtual void ShutdownImpl() override;

		virtual void InitKeyMap() override;

		virtual void PollImpl() override;

		virtual bool IsKeyDownImpl(const std::string& t_KeyName) override;
		virtual bool IsKeyHelpImpl(const std::string& t_KeyName) override;
		virtual bool IsMouseButtonPressedImpl(const std::string& t_KeyName) override;

		virtual MousePos GetMousePosImpl() override;
	};
} // namespace Fling

#endif	// FLING_LINUX