#pragma once
#include <ole.h>
#include <minwindef.h>
#include <WinUser.h>
#include <wtypes.h>
#include <ShObjIdl.h>

#define OnFileButtonClicked 1
#define OnFolderButtonClicked 2
#define OnCopyButtonClicked 3
#define OnMoveButtonClicked 4


LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);

HWND fileButton;
HWND folderButton;

IFileDialog* pFileDialog = NULL;

WCHAR fileName[MAX_PATH];
OPENFILENAMEW ofnFile;

LPWSTR folderName;
HRESULT folderHR;
DWORD folderOptions;

LPWSTR sourcePath;
LPWSTR destinationPath;

HWND sourceText;
HWND destinationText;

HWND copyButton;
HWND moveButton;

bool isFileChosen = false;
bool isFolderChosen = false;

HWND hEdit;

int currentProccesses = 0;

void CreateButtons(HWND hWnd);
void ChangeButtonsSizes(HWND hWnd);
void SetOpenFilesParams(HWND hWnd);
bool GetFolder(HWND hwnd);

void ChangeSourceText();
void ChangeDestinationText();
void CheckCopyAndMoveButtons();

DWORD CopyFunction(LPVOID lpParam);
DWORD MoveFunction(LPVOID lpParam); 