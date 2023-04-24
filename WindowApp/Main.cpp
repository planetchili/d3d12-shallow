#include <Core/src/win/IWindow.h>
#include <Core/src/log/Log.h>
#include <Core/src/ioc/Container.h>
#include <Core/src/log/SeverityLevelPolicy.h>
#include <Core/src/win/Boot.h>
#include "App.h"

using namespace chil;
using namespace std::string_literals;

void Boot()
{
	log::Boot();
	ioc::Get().Register<log::ISeverityLevelPolicy>([] {
		return std::make_shared<log::SeverityLevelPolicy>(log::Level::Info);
	});

	win::Boot();
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	PWSTR pCmdLine,
	int nCmdShow)
{
	try {
		Boot();
		auto pWindow = ioc::Get().Resolve<win::IWindow>();
		return app::Run(*pWindow);
	}
	catch (const std::exception& e) {
		chilog.error(utl::ToWide(e.what())).no_line().no_trace();
		MessageBoxA(nullptr, e.what(), "Error", MB_ICONERROR | MB_SETFOREGROUND);
	}
	return -1;
}