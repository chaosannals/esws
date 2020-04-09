#include <Windows.h>
#include <cstring>
#include <string>
#include "logging.h"

TCHAR SERVICE_NAME[] = TEXT("Elastic Search Windows Service");
DWORD ThreadId;
SERVICE_STATUS SelfStatus;
SERVICE_STATUS_HANDLE SelfStatusHandle;
TCHAR MODULE_FILE_NAME[MAX_PATH];
CHAR MODULE_DIR[MAX_PATH];

BOOL IsInstalledService() {
	BOOL result = FALSE;
	SC_HANDLE handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (handle == NULL) {
		LogWriteLn("Open scmanager failed.");
		return result;
	}

	SC_HANDLE service = OpenService(handle, SERVICE_NAME, SERVICE_QUERY_CONFIG);
	if (service != NULL) {
		result = TRUE;
		CloseServiceHandle(service);
	}
	CloseServiceHandle(handle);

	return result;
}

BOOL InstallService() {
	if (IsInstalledService()) {
		return TRUE;
	}
	SC_HANDLE handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (handle == NULL) {
		LogWriteLn("Open scmanager failed.");
		return FALSE;
	}
	SC_HANDLE service = CreateService(
		handle, SERVICE_NAME, SERVICE_NAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
		SERVICE_AUTO_START,
		SERVICE_ERROR_NORMAL,
		MODULE_FILE_NAME, NULL, NULL,
		TEXT(""),
		NULL, NULL
	);
	CloseServiceHandle(handle);
	if (service == NULL) {
		LogWriteLn("create service failed.");
		return FALSE;
	}
	CloseServiceHandle(service);
	return TRUE;
}

BOOL UninstallService() {
	if (!IsInstalledService()) {
		return TRUE;
	}
	SC_HANDLE handle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (handle == NULL) {
		LogWriteLn("Open scmanager failed.");
		return FALSE;
	}
	SC_HANDLE service = OpenService(handle, SERVICE_NAME, SERVICE_STOP|DELETE);

	if (service == NULL) {
		CloseServiceHandle(handle);
		LogWriteLn("open servcie failed.(stop delete)");
		return FALSE;
	}

	SERVICE_STATUS status;
	ControlService(service, SERVICE_CONTROL_STOP, &status);
	BOOL result = DeleteService(service);

	if (!result) {
		LogWriteLn("delete service failed.");
	}

	CloseServiceHandle(handle);
	CloseServiceHandle(service);
	return result;
}

void WINAPI ServiceCtrl(DWORD opcode) {
	LogWriteLn("service ctrl:", opcode);
	switch (opcode) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		SelfStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(SelfStatusHandle, &SelfStatus);
		break;
	case SERVICE_CONTROL_PAUSE:
		break;
	case SERVICE_CONTROL_CONTINUE:
		break;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		LogWriteLn("unknown opcode.");
		break;
	}
}

void WINAPI ServiceMain(int argc, char* param[]) {
	LogWriteLn("service main");
	SelfStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;;
	SelfStatus.dwCurrentState = SERVICE_START_PENDING;
	SelfStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	SelfStatus.dwWin32ExitCode = S_OK;
	SelfStatus.dwServiceSpecificExitCode = 0;
	SelfStatus.dwCheckPoint = 0;
	SelfStatus.dwWaitHint = 0;

	SelfStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrl);
	if (SelfStatusHandle == NULL) {
		LogWriteLn("register servcie ctrl failed.");
		return;
	}

	SelfStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(SelfStatusHandle, &SelfStatus);

	while (SelfStatus.dwCurrentState == SERVICE_RUNNING) {
		LogWriteLn("tick.");
		Sleep(2000);
	}

	LogWriteLn("service stoped");
	SelfStatus.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(SelfStatusHandle, &SelfStatus);
}

INT WINAPI WinMain(HINSTANCE self, HINSTANCE prev, LPSTR param, INT visible) {
	GetModuleFileName(NULL, MODULE_FILE_NAME, MAX_PATH);
	std::wstring mfn(MODULE_FILE_NAME);
	size_t end = mfn.rfind('\\');
	WideCharToMultiByte(CP_ACP, 0, MODULE_FILE_NAME, end, MODULE_DIR, MAX_PATH, NULL, NULL);
	ThreadId = GetCurrentThreadId();
	LogWriteLn("service win main:", ThreadId);
	SERVICE_TABLE_ENTRY ste[] = {
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};
	if (std::strcmp(param, "/install") == 0) {
		LogWriteLn("install...");
		InstallService();
	}
	else if (std::strcmp(param, "/uninstall") == 0) {
		LogWriteLn("uninstall...");
		UninstallService();
	}
	else {
		LogWriteLn("service dispatch");
		if (!StartServiceCtrlDispatcher(ste)) {
			LogWriteLn("service dispatcher failed.");
		}
	}
	return 0;
}
