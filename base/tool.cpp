#include "tool.h"

/*
Module:  Toolhelp.h
Notices : Copyright(c) 2008 Jeffrey Richter & Christophe Nasarre
*/

CToolhelp::CToolhelp(DWORD dwFlags, DWORD dwProcessID) {
    m_hSnapshot = INVALID_HANDLE_VALUE;
    CreateSnapshot(dwFlags, dwProcessID);
}

CToolhelp::~CToolhelp() {
    if (m_hSnapshot != INVALID_HANDLE_VALUE)
        CloseHandle(m_hSnapshot);
}

BOOL CToolhelp::CreateSnapshot(DWORD dwFlags, DWORD dwProcessID) {
    if (m_hSnapshot != INVALID_HANDLE_VALUE)
        CloseHandle(m_hSnapshot);

    if (dwFlags == 0) {
        m_hSnapshot = INVALID_HANDLE_VALUE;
    }
    else {
        m_hSnapshot = CreateToolhelp32Snapshot(dwFlags, dwProcessID);
    }
    return(m_hSnapshot != INVALID_HANDLE_VALUE);
}

BOOL CToolhelp::ProcessFirst(PPROCESSENTRY32 ppe) const {
    BOOL fOk = Process32First(m_hSnapshot, ppe);
    if (fOk && (ppe->th32ProcessID == 0))
        fOk = ProcessNext(ppe); // Remove the "[System Process]" (PID = 0)
    return(fOk);
}

BOOL CToolhelp::ProcessNext(PPROCESSENTRY32 ppe) const {
    BOOL fOk = Process32Next(m_hSnapshot, ppe);
    if (fOk && (ppe->th32ProcessID == 0))
        fOk = ProcessNext(ppe); // Remove the "[System Process]" (PID = 0)
    return(fOk);
}

BOOL CToolhelp::ProcessFind(DWORD dwProcessId, PPROCESSENTRY32 ppe) const {
    BOOL fFound = FALSE;
    for (BOOL fOk = ProcessFirst(ppe); fOk; fOk = ProcessNext(ppe)) {
        fFound = (ppe->th32ProcessID == dwProcessId);
        if (fFound) break;
    }
    return(fFound);
}

BOOL CToolhelp::ThreadFirst(PTHREADENTRY32 pte) const {
    return(Thread32First(m_hSnapshot, pte));
}

BOOL CToolhelp::ThreadNext(PTHREADENTRY32 pte) const {
    return(Thread32Next(m_hSnapshot, pte));
}