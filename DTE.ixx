//
// Link with: atls.lib oleaut32.lib
//
// Also remember to call CoInitialize or CoInitializeEx
// ~~29KB binary

module;
#pragma warning(push)
#pragma warning(disable : 4668)
#include <windows.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4775 4310 4471 4365 4191 5204)
#include <atlbase.h>
// #import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("9.0") lcid("0") raw_interfaces_only named_guids
#pragma warning(pop)

export module deckard.DTE;

import std;
import deckard.win32;


const GUID GUID_NULL = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

namespace DTE
{

#pragma warning(push)
#pragma warning(disable : 4471 5204)

	struct __declspec(uuid("04a72314-32e9-48e2-9b87-a63603454f3e")) _DTE : IDispatch
	{
		virtual HRESULT get_Name(BSTR *)                   = 0;
		virtual HRESULT get_FileName(BSTR *)               = 0;
		virtual HRESULT get_Version(BSTR *)                = 0;
		virtual HRESULT get_CommandBars(IDispatch **)      = 0;
		virtual HRESULT get_Windows(struct Windows **)     = 0;
		virtual HRESULT get_Events(struct Events **)       = 0;
		virtual HRESULT get_AddIns(struct AddIns **)       = 0;
		virtual HRESULT get_MainWindow(struct Window **)   = 0;
		virtual HRESULT get_ActiveWindow(struct Window **) = 0;
		virtual HRESULT Quit()                             = 0;
		virtual HRESULT get_DisplayMode(enum vsDisplay *)  = 0;
		virtual HRESULT put_DisplayMode(enum vsDisplay)    = 0;

		virtual HRESULT get_Solution(struct _Solution **) = 0;

		virtual HRESULT get_Commands(struct Commands **)                         = 0;
		virtual HRESULT GetObject2(BSTR Name, IDispatch **)                      = 0;
		virtual HRESULT get_Properties(BSTR, BSTR, struct Properties **)         = 0;
		virtual HRESULT get_SelectedItems(struct SelectedItems **)               = 0;
		virtual HRESULT get_CommandLineArguments(BSTR *)                         = 0;
		virtual HRESULT OpenFile(BSTR ViewKind, BSTR, struct Window **)          = 0;
		virtual HRESULT get_IsOpenFile(BSTR, BSTR, VARIANT_BOOL *)               = 0;
		virtual HRESULT get_DTE(struct _DTE **)                                  = 0;
		virtual HRESULT get_LocaleID(long *)                                     = 0;
		virtual HRESULT get_WindowConfigurations(struct WindowConfigurations **) = 0;
		virtual HRESULT get_Documents(struct Documents **)                       = 0;

		virtual HRESULT get_ActiveDocument(struct _Document **) = 0;

		virtual HRESULT ExecuteCommand(BSTR, BSTR) = 0;

		virtual HRESULT get_Globals(struct Globals **)                        = 0;
		virtual HRESULT get_StatusBar(struct StatusBar **)                    = 0;
		virtual HRESULT get_FullName(BSTR *)                                  = 0;
		virtual HRESULT get_UserControl(VARIANT_BOOL *)                       = 0;
		virtual HRESULT put_UserControl(VARIANT_BOOL)                         = 0;
		virtual HRESULT get_ObjectExtenders(struct ObjectExtenders **)        = 0;
		virtual HRESULT get_Find(struct Find **)                              = 0;
		virtual HRESULT get_Mode(enum vsIDEMode *)                            = 0;
		virtual HRESULT LaunchWizard(BSTR, SAFEARRAY **, enum wizardResult *) = 0;
		virtual HRESULT get_ItemOperations(struct ItemOperations **)          = 0;
		virtual HRESULT get_UndoContext(struct UndoContext **)                = 0;
		virtual HRESULT get_Macros(struct Macros **)                          = 0;
		virtual HRESULT get_ActiveSolutionProjects(VARIANT *)                 = 0;
		virtual HRESULT get_MacrosIDE(struct _DTE **)                         = 0;
		virtual HRESULT get_RegistryRoot(BSTR *)                              = 0;
		virtual HRESULT get_Application(struct _DTE **pVal)                   = 0;
		virtual HRESULT get_ContextAttributes(struct ContextAttributes **)    = 0;
		virtual HRESULT get_SourceControl(struct SourceControl **)            = 0;
		virtual HRESULT get_SuppressUI(VARIANT_BOOL *)                        = 0;
		virtual HRESULT put_SuppressUI(VARIANT_BOOL)                          = 0;
		virtual HRESULT get_Debugger(struct Debugger **)                      = 0;
		virtual HRESULT SatelliteDllPath(BSTR, BSTR, BSTR *)                  = 0;
		virtual HRESULT get_Edition(BSTR *)                                   = 0;
	};

#pragma warning(pop)

	using Func_CLSIDFromProID  = HRESULT(LPCOLESTR, LPCLSID);
	using Func_GetActiveObject = HRESULT(REFCLSID, void *, IUnknown **);

	auto GetDTE() -> CComPtr<_DTE>
	{
		static auto CLSIDFromProgID = deckard::LoadDynamic<Func_CLSIDFromProID *>("ole32.dll", "CLSIDFromProgID");
		static auto GetActiveObject = deckard::LoadDynamic<Func_GetActiveObject *>("oleaut32.dll", "GetActiveObject");

		if (!CLSIDFromProgID || !GetActiveObject)
			return nullptr;

		CLSID clsid;

		CComPtr<_DTE> dte;
		HRESULT       result = CLSIDFromProgID(L"VisualStudio.DTE", &clsid);
		if (FAILED(result))
			return nullptr;

		CComPtr<IUnknown> punk;
		result = GetActiveObject(clsid, nullptr, (IUnknown **)&punk);
		if (FAILED(result))
			return nullptr;

		punk->QueryInterface(&dte);

		return dte ? dte : nullptr;
	}

	export bool ExecuteCommand(wchar_t const *action, const wchar_t *action_parameter)
	{
		auto DTE = GetDTE();
		if (!DTE)
			return false;

		CComBSTR command(action);
		CComBSTR param(action_parameter);
		auto     hr = DTE->ExecuteCommand(command, param);
		if (FAILED(hr))
			return false;

		return true;
	}

	export bool GotoLine(const wchar_t *filename, unsigned int line)
	{
		wchar_t quoted_filename[MAX_PATH]{0};
		if (const auto result = std::format_to_n(quoted_filename, std::ssize(quoted_filename), L"\"{}\"", filename); result.size <= 0)
			return false;

		if (ExecuteCommand(L"File.OpenFile", quoted_filename) == false)
			return false;

		wchar_t linetext[64]{0};
		if (const auto result = std::format_to_n(linetext, std::ssize(linetext), L"{}", line); result.size <= 0)
			return false;

		if (ExecuteCommand(L"Edit.Goto", linetext) == false)
			return false;

		return true;
	}

	export bool GotoLine(char const *filename, unsigned int line) { return GotoLine(piku::to_wide(filename).c_str(), line); }

} // namespace DTE
