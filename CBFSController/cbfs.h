#pragma once
#include "pch.h"
#include <winnt.h>
#include <string>
#include "CBFSProcess.h"
#include "Python.h"

#define DLLEXPORT extern "C" __declspec(dllexport)

/* Utility Functions */
std::string GetFileName(std::string);
std::string PCWToString(PCWSTR);
void AddFilters(CBFS_Process::CBFSProcess* Process);
void AddHandlers(CBFS_Process::CBFSProcess*);
void SignalHandler(int);

/* Python Functions */
PyObject* CheckFileExist(PyObject*, PyObject*);
char* ExtractCharPtr(PyObject*);
PyObject* AddName(PyObject*, PyObject*);
PyObject* AddID(PyObject*, PyObject*);
PyObject* Controller(PyObject*, PyObject*);

/* Handlers */
void ProcCreEvent(CBFS_Process::CBFSProcess*, DWORD, DWORD, DWORD, DWORD, PCWSTR, PCWSTR, BOOL, PCWSTR);
void ProcHandEvent(CBFS_Process::CBFSProcess*, BOOL, DWORD, DWORD, DWORD, ACCESS_MASK, ACCESS_MASK, DWORD, DWORD, ACCESS_MASK&);
void ProcTermEvent(CBFS_Process::CBFSProcess*, DWORD, PCWSTR);

/* Control Functions */
//DLLEXPORT void controller(const char*, const wchar_t*, const char*, const wchar_t*);
//DLLEXPORT void begin(const char*);