#include "cbfs.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <csignal>
//#include <Python.h>

#pragma region Variables

/* Variables that are globally accessible throughout */

//CBFS_Process::CBFSProcess* Process;
bool FileExists = true;
bool Interrupted;

std::vector<std::string> FilterNames;
std::vector<int> FilterIDs;

#pragma endregion


#pragma region Utility Functions

/* Utility functions that serve some purpose */

char* ExtractCharPtr(PyObject* service) {
    
    PyObject* repr_filt = PyObject_Repr(service);
    PyObject* str_filt = PyUnicode_AsEncodedString(repr_filt, "utf-8", "~E~");
    char* bytes_filt = PyBytes_AsString(str_filt);

    char* remover = bytes_filt + 1;
    remover[strlen(remover) - 1] = '\0';

    return remover;
}

const wchar_t* ConvertWideChar(char* input) {
    wchar_t* wString = new wchar_t[8192];
    MultiByteToWideChar(CP_ACP, 0, input, -1, wString, 4096);
    const wchar_t* return_char = wString;
    return return_char;
}

// Gets just the file name from a full path
std::string GetFileName(std::string input) {
    std::size_t found = input.find_last_of("\\");
    return input.substr(found + 1);
}

// Converts a PCW type into a string
std::string PCWToString(PCWSTR input) {
    int len = WideCharToMultiByte(CP_ACP, 0, input, wcslen(input), NULL, 0, NULL, NULL);
    char* buffer = new char[len + (size_t)1];
    WideCharToMultiByte(CP_ACP, 0, input, wcslen(input), buffer, len, NULL, NULL);
    buffer[len] = '\0';
    std::string outtie = buffer;

    return outtie;
}

// Checks if the required CBFS file exists in the necessary directory
PyObject* CheckFileExist(PyObject*, PyObject* directory)
{
    std::string filename = ExtractCharPtr(directory);
    filename += "cbfsprocess2017.sys";

    std::ifstream ifile;
    ifile.open(filename);
    if (ifile) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

// Adds processes that are in the lists to be monitored
void AddFilters(CBFS_Process::CBFSProcess* Process)
{
    for (std::vector<std::string>::iterator it = FilterNames.begin(); it != FilterNames.end(); ++it) {
        std::string tempy = *it;
        std::wstring stemp = std::wstring(tempy.begin(), tempy.end());
        Process->AddFilteredProcessByName(stemp.c_str(), true);
        std::cout << "Added process (name): " << tempy << std::endl;
    }

    for (std::vector<int>::iterator it = FilterIDs.begin(); it != FilterIDs.end(); ++it) {
        int tempy = *it;
        DWORD IDToPass = static_cast<DWORD>(tempy);
        Process->AddFilteredProcessById(IDToPass, true);
        std::cout << "Added process (ID): " << tempy << std::endl;

    }
}

// Adds the handlers that CBFS requires for Process Creation/Handling/Termination
void AddHandlers(CBFS_Process::CBFSProcess* Process)
{
    CBFS_Process::ProcessCreationEvent CreateEvent = &ProcCreEvent;
    Process->SetOnProcessCreation(CreateEvent);

    CBFS_Process::ProcessHandleOperationEvent HandleEvent = &ProcHandEvent;
    Process->SetOnProcessHandleOperation(HandleEvent);

    CBFS_Process::ProcessTerminationEvent TerminateEvent = &ProcTermEvent;
    Process->SetOnProcessTermination(TerminateEvent);
}

// Adds a process by name to a list
PyObject* AddName(PyObject*, PyObject* name)
{
    char* filter = ExtractCharPtr(name);

    FilterNames.push_back(filter);
    
    //return PyBool_FromLong(1);

    Py_RETURN_NONE;
}

// Adds a process by process ID to a list
PyObject* AddID(PyObject*, PyObject* id)
{
    long LongObj = PyLong_AsLong(id);
    int passed_id = (int)LongObj;

    FilterIDs.push_back(passed_id);

    Py_RETURN_NONE;
}

void SignalHandler(int signum) {
    Interrupted = true;
    std::cout << "Interrupt Signal received" << std::endl;
    exit(signum);
}

#pragma endregion


#pragma region Handlers

/* Required CBFS Process Handlers */

// When a Process is created
void ProcCreEvent(CBFS_Process::CBFSProcess* Sender,
    DWORD ProcessId,
    DWORD ParentProcessId,
    DWORD CreatingProcessId,
    DWORD CreatingThreadId,
    PCWSTR ProcessName,
    PCWSTR ImageFileName,
    BOOL FileOpenNameAvailable,
    PCWSTR CommandLine)
{
    std::cout << "Started: " << GetFileName(PCWToString(ProcessName)) << " === Process ID: " << ProcessId << std::endl;
}

// When a Process is handled
void ProcHandEvent(CBFS_Process::CBFSProcess* Sender,
    BOOL Duplication,
    DWORD ProcessId,
    DWORD OriginatorProcessId,
    DWORD OriginatorThreadId,
    ACCESS_MASK DesiredAccess,
    ACCESS_MASK OriginalDesiredAccess,
    DWORD SourceProcessId,
    DWORD TargetProcessId,
    ACCESS_MASK& AllowedAccess)
{
    if (DesiredAccess == 5121) {
        // Prevents the elevated user from being able to terminate a process
        std::cout << "Attempt to terminate Process: " << ProcessId << std::endl;
        AllowedAccess -= PROCESS_TERMINATE;
    }
}

// When a Process is terminated
void ProcTermEvent(CBFS_Process::CBFSProcess* Sender,
    DWORD ProcessId,
    PCWSTR ProcessName)
{
    std::cout << "Termed: " << GetFileName(PCWToString(ProcessName)) << " === Process ID: " << ProcessId << std::endl;
}

#pragma endregion


#pragma region Control Functions

/* Functions that deal directly with the CBFS library */

// Instantiates the CBFS controller and checks if necessary file is installed

PyObject* Controller(PyObject*, PyObject* key_param)
{
    std::cout << "Initialised CBFS Controller" << std::endl;

    Interrupted = false;
    signal(SIGINT, SignalHandler);

    CBFS_Process::CBFSProcess* Process = new CBFS_Process::CBFSProcess();

    LPCSTR newkey = ExtractCharPtr(key_param);
    Process->SetRegistrationKey(newkey);

    AddHandlers(Process);

    LPCSTR prod_name = "CBFSDriver";
    Process->Initialize(prod_name);

    Process->StartFilter();
    AddFilters(Process);

    while (!Interrupted) {}
    
    std::cout << "\nEnd loop" << std::endl;
    
    Py_RETURN_NONE;
}


#pragma endregion

#pragma region CPython Stuff

static PyMethodDef CBFSController_methods[] = {
    // The first property is the name exposed to Python, fast_tanh, the second is the C++
    // function name that contains the implementation.
    { "CheckFileExist", (PyCFunction)CheckFileExist, METH_O, nullptr },
    { "Controller", (PyCFunction)Controller, METH_O, nullptr },
    { "AddName", (PyCFunction)AddName, METH_O, nullptr },
    { "AddID", (PyCFunction)AddID, METH_O, nullptr },

    // Terminate the array with an object containing nulls.
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef CBFSController_module = {
    PyModuleDef_HEAD_INIT,
    "CBFSController",                        // Module name to use with Python import statements
    "Provides some functions, but faster",  // Module description
    0,
    CBFSController_methods                   // Structure that defines the methods of the module
};

PyMODINIT_FUNC PyInit_CBFSController() {
    return PyModule_Create(&CBFSController_module);
}

#pragma endregion